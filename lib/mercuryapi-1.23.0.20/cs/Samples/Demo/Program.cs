/*
 * Copyright (c) 2008 ThingMagic, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using ThingMagic;

namespace SampleConsoleApplication
{
    class Program
    {
        delegate void CommandFunc(Reader rdr, ArgParser pargs);
        class Command
        {
            /// <summary>
            /// Create a command, including handler function and documentation
            /// </summary>
            /// <param name="handler">Function implementing command</param>
            /// <param name="usage">Space-separated list of arguments, including command name; e.g., "readmembytes bank addr len [filt]"</param>
            /// <param name="docLines">Descriptive documentation.  Arbitrary number of args may be provided, with each becoming a separate line in the final doc.  First line should be a one-line summary.</param>
            public Command(CommandFunc handler, string usage, params string[] docLines) : this("", handler, null, null)
            {
                string[] argv = Split(usage);
                Name = argv[0];
                Args = new string[argv.Length-1];
                Array.Copy(argv, 1, Args, 0, Args.Length);
                Doc = JoinLines(docLines);
            }
            /// <summary>
            /// Create a command, including handler function and documentation
            /// </summary>
            /// <param name="name">Name used to invoke command</param>
            /// <param name="handler">Function implementing command</param>
            public Command(string name, CommandFunc handler) : this(name, handler, null, "") { }
            /// <summary>
            /// Create a command, including handler function and documentation
            /// </summary>
            /// <param name="handler">Function implementing command</param>
            /// <param name="args">List of arguments, not including command name; e.g., "new string[] { "readmembytes", "bank", "addr", len [filt]"</param>
            /// <param name="doc">Descriptive documentation.</param>
            public Command(string name, CommandFunc handler, string[] args, string doc)
            {
                Handler = handler;
                Name = name;
                Args = (null != args) ? args : new string[] { };
                Doc = doc;
            }
            internal CommandFunc Handler;
            internal string Name;
            internal string[] Args;
            internal string Doc;
            public string Summary { get { return Split(Doc, '\n')[0]; } }
            public string Usage { get { return String.Format("{0} {1}", Name, String.Join(" ", Args)); } }
        }

        static void Main(string[] args)
        {
            try
            {
                ArgParser pargs = new ArgParser(args);
                Reader rdr = Reader.Create(pargs.readerURI);

                EventHandler<TransportListenerEventArgs> tl = null;
                if (pargs.keywordArgs.ContainsKey("log"))
                {
                    String logname = (String)pargs.keywordArgs["log"][0];
                    System.IO.TextWriter logwriter;
                    if ("-" == logname)
                    {
                        logwriter = System.Console.Out;
                    }
                    else
                    {
                        logwriter = new System.IO.StreamWriter(logname);
                    }
                    tl = delegate(Object sender, TransportListenerEventArgs e)
                    {
                        string format = "eapi";
                        if (sender is RqlReader)
                        {
                            format = "rql";
                        }
#if !WindowsCE
                        else if (sender is LlrpReader)
                        {
                            format = "llrp";
                        }
#endif
                        string msg = null;
                        switch (format)
                        {
                        case "rql":
                            msg = Encoding.ASCII.GetString(e.Data, 0, e.Data.Length);
                            msg = "\"" + msg.Replace("\r","\\r").Replace("\n","\\n") + "\"";
                            break;
                        case "llrp":
                            msg = Encoding.ASCII.GetString(e.Data, 0, e.Data.Length);
                            break;
                        default:
                            msg = BytesToString(e.Data, "", " ");
                            break;
                        }
                        logwriter.WriteLine(String.Format(
                            "{0}: {1} (timeout={2:D}ms)",
                            e.Tx ? "TX" : "RX",
                            msg,
                            e.Timeout
                            ));
                        logwriter.Flush();
                    };
                }

                rdr.Transport += tl;
                try
                {
                    rdr.Connect();
                    if (Reader.Region.UNSPEC.Equals(rdr.ParamGet("/reader/region/id")))
                    {
                        rdr.ParamSet("/reader/region/id", Reader.Region.NA);
                    }
                }
                catch (FAULT_BL_INVALID_IMAGE_CRC_Exception)
                {
                    // Ignore INVALID_IMAGE here so we can reach FirmwareLoad -- will be checked later in SendInitCommands, anyway
                }
                try
                {
                    if (("shell" == pargs.command.ToLower()) ||
                        ("script" == pargs.command.ToLower()))
                    {
                        System.IO.StreamReader inStream = null;
                        if ("script" == pargs.command.ToLower())
                        {
                            if (pargs.commandArgs.Length < 1) { throw new UsageException("script command must be followed by a filename"); }
                            string filename = pargs.commandArgs[0];
                            inStream = new System.IO.StreamReader(filename);
                        }
                        while (true)
                        {
                            Console.Write("\r\n>>> ");
                            Console.Out.Flush();

                        NextLine:
                            string line = (null != inStream) ? inStream.ReadLine() : Console.ReadLine();
                            if (null == line) break;

                            if (0 == line.Length) { goto NextLine; }
                            if (line.StartsWith("#")) goto NextLine;

                            if (pargs.keywordArgs.ContainsKey("echo") &&
                                ("on" == ((string)pargs.keywordArgs["echo"][0])))
                            {
                                Console.WriteLine(line);
                                Console.Out.Flush();
                            }

                            if ("exit" == line.ToLower()) break;

                            string[] lineargs = line.Split(null);
                            string[] argv = new string[1 + lineargs.Length];
                            argv[0] = "shell";
                            lineargs.CopyTo(argv, 1);

                            ArgParser pargv = new ArgParser(argv);
                            RunCommand(rdr, pargv);
                        }
                    }
                    else
                    {
                        RunCommand(rdr, pargs);
                    }
                }
                finally
                {
                    rdr.Destroy();
                }
            }
            catch (UsageException ex)
            {
                Console.WriteLine(ex.Message);
            }
            catch (Exception ex)
            {
                // Show exception to user
                // I thought this would happen by default, but Reader.Create is failing silently on an ArgumentException --Harry 2009Feb19
                Console.WriteLine(ex.ToString());
            }
        }

        static Dictionary<string, Command> CommandDispatchTable
        {
            get
            {
                if (null == _cdt)
                {
                    _cdt = new Dictionary<string, Command>();
                    AddCommand(Echo);
                    AddCommand(Help);
                    AddCommand(Script);
                    AddCommand(Shell);

                    AddCommand(DemoRead);
                    AddCommand(GetParam);
                    AddCommand(GetGPIO);
                    AddCommand(Kill);
                    AddCommand(LoadFirmware);
                    AddCommand(Lock);
                    AddCommand(ListParams);
                    AddCommand(Read);
                    AddCommand(ReadAsync);
                    AddCommand(ReadSync);
                    AddCommand(MultiRead);
                    AddCommand(MultiReadAsync);
                    AddCommand(MultiReadSync);
                    AddCommand(MakeReadLevel3Command("l3read.bycount", "count-based fetch", ReadLevel3Type.BYCOUNT));
                    AddCommand(MakeReadLevel3Command("l3read.bycount496", "count-based, 496-bit fetch", ReadLevel3Type.BYCOUNT496));
                    AddCommand(MakeReadLevel3Command("l3read.byindex", "index-based fetch", ReadLevel3Type.BYINDEX));
                    AddCommand(MakeReadLevel3Command("l3read.byindex496", "index-based, 496-bit fetch", ReadLevel3Type.BYINDEX496));
                    AddCommand(MakeReadLevel3Command("l3read.withmetadata", "full metadata fetch", ReadLevel3Type.WITHMETADATA));
                    AddCommand(Raw);
                    AddCommand(ReadMemBytes);
                    AddCommand(ReadMemWords);
                    AddCommand(SetGPIO);
                    AddCommand(SetParam);
                    AddCommand(SetAccessPassword);
                    AddCommand(SetKillPassword);
                    AddCommand(WriteEpc);
                    AddCommand(WriteMemBytes);
                    AddCommand(WriteMemWords);

                    AddCommand(TestCommand);
                    AddCommand(TestGen2TagDataConstructorsCommand);
                    AddCommand(TestMatchesCommand);
                    AddCommand(TestReadTagProtocolsCommand);
                    AddCommand(TestSync);
                    AddCommand(TestStrFilterReadPlans);
                    AddCommand(GpioDirection);
                    AddCommand(TagMetaData);
                    AddCommand(BlockPermaLockCmds);
                    AddCommand(BlockWriteCmds);
                    AddCommand(SavedConfig);
                    AddCommand(TestRegCmds);

                }
                return _cdt;
            }
        }

        private static Command MakeReadLevel3Command(string name, string description, ReadLevel3Type readtype)
        {
            return new Command(
                delegate(Reader rdr, ArgParser pargs) { ReadLevel3Helper(rdr, pargs, readtype); },
                name + " [timeout]",
                "Search for tags, using Level 3 commands with " + description,
                "timeout -- Number of milliseconds to search.  Defaults to a reasonable average number.",
                "",
                name,
                name + " 3000");
        }

        private static Dictionary<string, Command> _cdt;

        static void AddCommand(string name, CommandFunc handler)
        {
            CommandDispatchTable.Add(name, new Command(name, handler));
        }
        static void AddCommand(Command Cmd)
        {
            CommandDispatchTable.Add(Cmd.Name, Cmd);
        }

        static void RunCommand(Reader rdr, ArgParser pargs)
        {
            try
            {
                try
                {
                    CommandFunc Cmdfunc = CommandDispatchTable[pargs.command].Handler;
                    if (null == Cmdfunc)
                    {
                        throw new UsageException("Command \"" + pargs.command + "\" not yet implemented");
                    }
                    Cmdfunc(rdr, pargs);
                }
                catch (KeyNotFoundException)
                {
                    throw new UsageException("Unknown command: \"" + pargs.command + "\"");
                }
            }
            catch (Exception ex)
            {
                // TODO: Mark errors more obviously, but not yet,
                // because all the test scripts will have to be edited.
                //Console.WriteLine("Error: " + ex.Message);
                Console.WriteLine(ex.Message);

                if (pargs.keywordArgs.ContainsKey("stackdump") &&
                    ("on" == ((string)pargs.keywordArgs["stackdump"][0])))
                {
                    if (!(ex is UsageException))
                    {
                        Console.WriteLine();
                        Console.WriteLine(ex.ToString());
                    }
                }
            }
        }

        class ArgParser
        {
            public string readerURI;
            public string command;
            public string[] commandArgs;
            public Dictionary<string, List<string>> keywordArgs = new Dictionary<string, List<string>>();

            public ArgParser(string[] fullArgs)
            {
                try
                {
                    List<string> args = FilterKeywordArguments(fullArgs, ref keywordArgs);
                    this.readerURI = args[0];
                    this.command = args[1];
                    args.RemoveRange(0, 2);
                    this.commandArgs = args.ToArray();

                    if (0 < this.commandArgs.Length)
                    {
                        string lastArg = this.commandArgs[this.commandArgs.Length - 1];
                        if (lastArg.EndsWith("\r"))
                        {
                            this.commandArgs[this.commandArgs.Length - 1] = lastArg.Substring(0, lastArg.Length - 1);
                        }
                    }
                }
                catch (ArgumentOutOfRangeException)
                {
                    throw new UsageException("Not enough arguments provided in \"" + String.Join(" ", fullArgs) + "\"");
                }
            }
            private List<string> FilterKeywordArguments(string[] args, ref Dictionary<string, List<string>> keywordArgs)
            {
                List<string> argsList = new List<string>();
                for (int i = 0; i < args.Length; i++)
                {
                    if (false == args[i].StartsWith("--"))
                    {
                        argsList.Add(args[i]);
                    }
                    else
                    {
                        string key = args[i].Substring("--".Length);
                        try
                        {
                            string value = args[i + 1];
                            if (false == keywordArgs.ContainsKey(key))
                            {
                                keywordArgs[key] = new List<string>();
                            }
                            keywordArgs[key].Add(value);
                        }
                        catch (IndexOutOfRangeException)
                        {
                            throw new UsageException(String.Format("Missing argument after args[{0:d}] \"{1}\"", i, args[i]));
                        }
                        i++;
                    }
                }
                return argsList;
            }
        }

        class UsageException : Exception
        {
            public UsageException(string message)
                : base(String.Join("\r\n", new string[] {
                    message,
                    "",
                    "Usage: " + Basename(GetProgramName()) + " [--echo on|off] [--log -|filename] readeruri command [commandargs]",
                    "For detailed help: " + Basename(GetProgramName()) + " readeruri help",
                    "Sample readeruris:",
                    "  fake:",
                    "  eapi:///COM1",
                    "  tmr:///COM1",
                    "  rql://astra-21003e",
                    "  rql://m4-100505",
                    "  tmr://m4-100505",
            })) { }
        }
        private static string GetProgramName()
        {
            return System.Reflection.Assembly.GetExecutingAssembly().GetName().CodeBase;
        }
        private static string Basename(string path)
        {
            return System.IO.Path.GetFileName(path);
        }
        private static string JoinLines(params string[] lines)
        {
            return String.Join("\n", lines);
        }
        /// <summary>
        /// Parse string to boolean value
        /// </summary>
        /// <param name="boolString">String representing a boolean value</param>
        /// <returns>False if input is "False", "Low", or "0".  True if input is "True", "High", or "1".  Case-insensitive.</returns>
        private static bool ParseBool(string boolString)
        {
            switch (boolString.ToLower())
            {
                case "true":
                case "high":
                case "1":
                    return true;
                case "false":
                case "low":
                case "0":
                    return false;
                default:
                    throw new FormatException();
            }
        }
        /// <summary>
        /// Easier-to-use form of String.Split.  Defaults to split on space.
        /// </summary>
        static string[] Split(string str)
        {
            return str.Split(new char[] { ' ' });
        }
        /// <summary>
        /// Easier-to-use form of String.Split
        /// </summary>
        /// <param name="separator">[Single] string to use as separator</param>
        static string[] Split(string str, char separator)
        {
            return str.Split(new char[] { separator });
        }

        private static Command Echo = new Command(EchoFunc, "echo [word...]",
            "Print message to output");
        private static void EchoFunc(Reader rdr, ArgParser pargs)
        {
            Console.WriteLine(String.Join(" ", pargs.commandArgs));
        }
        private static Command Help = new Command(HelpFunc, "help [command]",
            "Print program usage instructions",
            "With no arguments, prints a list of available commands.",
            "With a command name, prints detailed help for that command."
        );
        private static void HelpFunc(Reader rdr, ArgParser pargs)
        {
            string[] args = pargs.commandArgs;
            if (0 == args.Length)
            {
                foreach (Command Cmd in CommandDispatchTable.Values)
                {
                    Console.WriteLine(String.Format("{0} -- {1}", Cmd.Name, Cmd.Summary));
                }
                Console.WriteLine();
                Console.WriteLine("For more detailed documentation, type \"help <commandname>\" (e.g., \"help help\")");
            }
            else
            {
                string commandName = args[0];
                Command Cmd = CommandDispatchTable[commandName];
                Console.WriteLine(Cmd.Usage);
                Console.WriteLine(Cmd.Doc);
            }
        }
        private static Command Shell = new Command(null, "shell",
            "Start interactive shell",
            "Enter commands through console (one command per line) instead of as command line args.");
        private static Command Script = new Command(null, "script file",
            "Read commands from file",
            "Enter commands into file (one command per line) instead of as command line args.",
            "Recommend \"--echo on\" option to show what commands are being run.");

        private static Command DemoRead = new Command(DemoReadFunc, "demoread", "Demonstrate sync and async read methods");
        private static void DemoReadFunc(Reader rdr, ArgParser pargs)
        {
            while (true)
            {
                //Console.WriteLine("Press Enter to start reading...");
                //Console.In.Read();

                while (true)
                {
                    int millis = 5000;
                    Console.WriteLine("Reading for " + millis + " milliseconds");
                    TagReadData[] reads = rdr.Read(millis);
                    Console.WriteLine("Read " + reads.Length + " tags");
                    PrintTagReads(reads);

                    Console.WriteLine("----");

                    Console.WriteLine("\r\nReading until Enter pressed...");
                    rdr.TagRead += PrintTagReadHandler;
                    // (Optional) Subscribe to reader exception notifications
                    rdr.ReadException += ReadExceptionHandler;
                    Console.WriteLine("Starting...");
                    rdr.StartReading();
                    Console.WriteLine("Started");

                    DateTime start = DateTime.Now;
                    DateTime lastUpdate = DateTime.MinValue;
                    //Console.In.Read();
                    Console.WriteLine("Stopping...");
                    rdr.StopReading();
                    rdr.TagRead -= PrintTagReadHandler;
                    rdr.ReadException -= ReadExceptionHandler;
                    Console.WriteLine("Stopped");
                }
            }
        }
        private static Command LoadFirmware = new Command(LoadFirmwareFunc, "loadfw filename",
            "Reload reader firmware",
            "filename -- Name of firmware file to load",
            "",
            "loadfw M5eApp_1.0.37.85-20081001-0412.sim");
        private static void LoadFirmwareFunc(Reader rdr, ArgParser pargs)
        {
            // loadfw fwfilename
            // loadfw M5eApp_1.0.37.85-20081001-0412.sim
            string filename = pargs.commandArgs[0];
            System.IO.FileStream fs = new System.IO.FileStream(filename, System.IO.FileMode.Open, System.IO.FileAccess.Read);
            Console.WriteLine("Please wait: Loading firmware \"" + filename + "\"");
            rdr.FirmwareLoad(fs);
            Console.WriteLine("Firmware loaded.");
        }
        private static Command GetGPIO = new Command(GetGPIOFunc, "get-gpio",
            "Read input pins",
            "Returns list of [pin,state] pairs.",
            "State is True if pin is high, False if pin is low",
            "",
            "get-gpio");
        private static void GetGPIOFunc(Reader rdr, ArgParser pargs)
        {
            Console.WriteLine(FormatValue(rdr.GpiGet()));
        }
        private static Command SetGPIO = new Command(SetGPIOFunc, "set-gpio [[pin,value],...]",
            "Write output pins",
            "pin -- Pin number",
            "value -- Pin state: True or 1 to set pin high, False or 0 to set pin low",
            "",
            "set-gpio [[1,1],[2,0]]");
        private static void SetGPIOFunc(Reader rdr, ArgParser pargs)
        {
            string[] args = pargs.commandArgs;
            ArrayList pinvallist = (ArrayList)ParseValue(args[0]);
            GpioPin[] gps = ArrayListToGpioPinArray(pinvallist);
            rdr.GpoSet(gps);
        }

        private static Command ReadAsync = new Command(ReadAsyncFunc, "read-async",
            "Demonstrate asynchronous read functionality");
        private static void ReadAsyncFunc(Reader rdr, ArgParser pargs)
        {
            string[] args = pargs.commandArgs;
            int timeout = 3000;
            if (0 < args.Length)
            {
                timeout = Int32.Parse(args[0], System.Globalization.NumberStyles.Any);
            }
            rdr.TagRead += PrintTagReadHandler;
            rdr.ReadException += delegate(Object sender, ReaderExceptionEventArgs e)
            {
                Console.WriteLine("Asynchronous Read Exception:");
                Console.WriteLine(e.ReaderException.ToString());
            };
            rdr.ParamSet("/reader/read/plan", MakeMultiReadPlan());
            rdr.StartReading();
            Thread.Sleep(timeout);
            rdr.StopReading();
        }
        private static Command MultiReadAsync = new Command(MultiReadAsyncFunc, "multiread-async",
            "Demonstrate asynchronous multiread functionality");
        private static void MultiReadAsyncFunc(Reader rdr, ArgParser pargs)
        {
            Object oldValue = rdr.ParamGet("/reader/read/plan");
            rdr.ParamSet("/reader/read/plan", MakeMultiReadPlan());
            ReadAsyncFunc(rdr, pargs);
            rdr.ParamSet("/reader/read/plan", oldValue);
        }

        private static Command Read = new Command(ReadSyncFunc, "read [timeout]",
            "Search for tags",
            "timeout -- Number of milliseconds to search.  Defaults to a reasonable average number.",
            "",
            "read",
            "read 3000");
        private static Command ReadSync = new Command(ReadSyncFunc, "read-sync [timeout]",
            "Search for tags",
            "timeout -- Number of milliseconds to search.  Defaults to a reasonable average number.",
            "",
            "read-sync",
            "read-sync 3000");
        private static void ReadSyncFunc(Reader rdr, ArgParser pargs)
        {
            string[] args = pargs.commandArgs;
            int timeout = 500;
            if (0 < args.Length)
            {
                timeout = Int32.Parse(args[0], System.Globalization.NumberStyles.Any);
            }
            TagReadData[] reads = rdr.Read(timeout);
            PrintTagReads(reads);
        }
        private static Command MultiRead = new Command(MultiReadSyncFunc, "multiread [timeout]",
            "Search for tags on multiple plans",
            "timeout -- Number of milliseconds to search.  Defaults to a reasonable average number.",
            "",
            "multiread",
            "multiread 3000");
        private static Command MultiReadSync = new Command(MultiReadSyncFunc, "multiread-sync [timeout]",
            "Search for tags on multiple plans",
            "timeout -- Number of milliseconds to search.  Defaults to a reasonable average number.",
            "",
            "multiread-sync",
            "multiread-sync 3000");
        private static void MultiReadSyncFunc(Reader rdr, ArgParser pargs)
        {
            Object oldValue = rdr.ParamGet("/reader/read/plan");
            rdr.ParamSet("/reader/read/plan", MakeMultiReadPlan());
            ReadSyncFunc(rdr, pargs);
            rdr.ParamSet("/reader/read/plan", oldValue);
        }
        private static MultiReadPlan MakeMultiReadPlan()
        {
            List<ReadPlan> plans = new List<ReadPlan>();
            plans.Add(new SimpleReadPlan(null, TagProtocol.GEN2, null, 100));
            plans.Add(new SimpleReadPlan(null, TagProtocol.ISO180006B, null, 200));
            //plans.Add(new SimpleReadPlan(null, TagProtocol.IPX256, null, 200));
            MultiReadPlan mrp = new MultiReadPlan(plans);
            return mrp;
        }
        private enum ReadLevel3Type { BYCOUNT, BYCOUNT496, BYINDEX, BYINDEX496, WITHMETADATA };
        private delegate void Level3GetTagFunc();
        private static void ReadLevel3Helper(Reader rdr, ArgParser pargs, ReadLevel3Type type)
        {
            SerialReader sr = (SerialReader)rdr;
            string[] args = pargs.commandArgs;
            int timeout = 500;
            if (0 < args.Length)
            {
                timeout = Int32.Parse(args[0], System.Globalization.NumberStyles.Any);
            }

            Level3GetTagFunc gettags = null;
            switch (type)
            {
                default:
                    throw new NotSupportedException("Don't yet support ReadLevel3Type." + type.ToString());
                case ReadLevel3Type.WITHMETADATA:
                case ReadLevel3Type.BYCOUNT:
                case ReadLevel3Type.BYCOUNT496:
                    gettags = delegate()
                    {
                        bool epc496 = type == ReadLevel3Type.BYCOUNT496;
                        sr.CmdSetReaderConfiguration(SerialReader.Configuration.EXTENDED_EPC, epc496);
                        UInt16 maxrecspermsg = (UInt16)(240 / (epc496 ? 68 : 18));

                        while (true)
                        {
                            UInt16 bufcount = sr.CmdGetTagsRemaining()[0];
                            if (bufcount <= 0) { break; }
                            bufcount = Math.Min((UInt16)bufcount, maxrecspermsg);

                            if (ReadLevel3Type.WITHMETADATA == type)
                            {
                                TagReadData[] reads = sr.CmdGetTagBuffer(SerialReader.TagMetadataFlag.ALL, false, sr.CmdGetProtocol());
                                PrintTagReads(reads);
                            }
                            else
                            {
                                TagData[] tags = sr.CmdGetTagBuffer(bufcount, epc496, sr.CmdGetProtocol());
                                PrintTagDatas(tags);
                            }
                        }
                    };
                    break;
                case ReadLevel3Type.BYINDEX:
                case ReadLevel3Type.BYINDEX496:
                    gettags = delegate()
                    {
                        bool epc496 = type == ReadLevel3Type.BYINDEX496;
                        sr.CmdSetReaderConfiguration(SerialReader.Configuration.EXTENDED_EPC, epc496);
                        UInt16 maxrecspermsg = (UInt16)(240 / (epc496 ? 68 : 18));

                        UInt16 bufcount = sr.CmdGetTagsRemaining()[0];
                        if (bufcount <= 0) { return; }

                        UInt16 start = 0;
                        UInt16 end = bufcount;
                        while (start < end)
                        {
                            UInt16 subend = end;
                            UInt16 subcount = (UInt16)(subend - start);
                            subcount = Math.Min((UInt16)subcount, maxrecspermsg);
                            subend = (UInt16)(start + subcount);

                            TagData[] tags = sr.CmdGetTagBuffer(start, subend, epc496, sr.CmdGetProtocol());
                            PrintTagDatas(tags);
                            start += (UInt16)tags.Length;
                        }
                    };
                    break;
            }

            sr.CmdClearTagBuffer();
            sr.CmdReadTagMultiple((ushort)timeout, SerialReader.AntennaSelection.CONFIGURED_LIST, null, TagProtocol.NONE);
            gettags();
        }
        private static Command Raw = new Command(RawFunc, "cmdraw timeout opcode [args...]",
            "Send raw SerialReader message",
            "timeout -- milliseconds to wait for response",
            "opcode -- M5e opcode",
            "args -- byte arguments of M5e command",
            "",
            "cmdraw 03");
        private static void RawFunc(Reader rdr, ArgParser pargs)
        {
            if (false == (rdr is SerialReader))
            {
                throw new ArgumentException("CmdRaw only works on SerialReaders.  You provided a "+rdr.GetType().ToString());
            }
            SerialReader sr = (SerialReader)rdr;

            string[] args = pargs.commandArgs;
            int i=0;
            int timeout = (int)ParseValue(args[i++]);
            if (i >= args.Length)
            {
                throw new ArgumentException("Not enough args to cmdraw.  Did you remember to provide a timeout?");
            }
            List<byte> byteList = new List<byte>();
            while (i<args.Length)
            {
                byteList.AddRange(ByteFormat.FromHex(args[i++]));
            }
            byte[] response = sr.CmdRaw(timeout, byteList.ToArray());
            Console.WriteLine(ByteFormat.ToHex(response));
        }
        private static Command ReadMemBytes = new Command(ReadMemBytesFunc, "readmembytes bank addr len [target]",
            "Read tag memory bytewise",
            "bank -- Memory bank",
            "addr -- Byte address within memory bank",
            "len -- Number of bytes to read",
            "target -- Tag whose memory to read",
            "",
            "readmembytes 1 4 12",
            "readmembytes 1 4 12 EPC:E2003411B802011083257015");
        private static void ReadMemBytesFunc(Reader rdr, ArgParser pargs)
        {
            string[] args = pargs.commandArgs;
            int i = 0;
            int bank = (int)ParseValue(args[i++]);
            int addr = (int)ParseValue(args[i++]);
            int byteCount = (int)ParseValue(args[i++]);
            TagFilter filter = null;
            if (i < args.Length)
            {
                filter = (TagFilter)ParseValue(args[i++]);
            }
            Console.WriteLine(FormatValue(rdr.ReadTagMemBytes(filter, bank, addr, byteCount), true));
        }
        private static Command ReadMemWords = new Command(ReadMemWordsFunc, "readmemwords bank addr len [target]",
            "Read tag memory wordwise",
            "bank -- Memory bank",
            "addr -- Word address within memory bank",
            "len -- Number of Words to read",
            "target -- Tag whose memory to read",
            "",
            "readmemwords 1 2 6",
            "readmemwords 1 2 6 EPC:E2003411B802011083257015");
        private static void ReadMemWordsFunc(Reader rdr, ArgParser pargs)
        {
            string[] args = pargs.commandArgs;
            int i = 0;
            int bank = (int)ParseValue(args[i++]);
            int addr = (int)ParseValue(args[i++]);
            int wordCount = (int)ParseValue(args[i++]);
            TagFilter filter = null;
            if (i < args.Length)
            {
                filter = (TagFilter)ParseValue(args[i++]);
            }
            Console.WriteLine(FormatValue(rdr.ReadTagMemWords(filter, bank, addr, wordCount), true));
        }
        private static Command SetAccessPassword = new Command(SetAccessPasswordFunc, "setaccpw password [target]",
            "Set Gen2 tag's access password",
            "password -- 32-bit access password",
            "target -- Tag to modify.  If none given, uses first available tag",
            "",
             "setaccpw 0x12345678",
             "setaccpw 0x12345678 EPC:044D00000000040007010062");
        private static void SetAccessPasswordFunc(Reader rdr, ArgParser pargs)
        {
            WritePassword(rdr, pargs, 2);
        }
        private static Command SetKillPassword = new Command(SetKillPasswordFunc, "setkillpw password [target]",
            "Set Gen2 tag's kill password",
            "password -- 32-bit kill password",
            "target -- Tag to modify.  If none given, uses first available tag",
            "",
             "setkillpw 0x12345678",
             "setkillpw 0x12345678 EPC:044D00000000040007010062");
        private static void SetKillPasswordFunc(Reader rdr, ArgParser pargs)
        {
            // setkillpw 0x12345678
            // setkillpw 0x12345678 EPC:044D00000000040007010062
            WritePassword(rdr, pargs, 0);
        }
        delegate void WritePasswordFunc(Reader rdr, TagFilter target, Gen2.Password password);
        private static void WritePassword(Reader rdr, ArgParser pargs, int wordAddress)
        {
            string[] args = pargs.commandArgs;
            int i = 0;
            UInt32 password = (UInt32)(int)ParseValue(args[i++]);
            TagFilter target = null;
            if (i < args.Length)
            {
                target = (TagFilter)ParseValue(args[i++]);
            }

            ushort[] data = new ushort[] {
                (ushort)((password >> 16) & 0xFFFF),
                (ushort)((password >>  0) & 0xFFFF)};
            rdr.WriteTagMemWords(target, (int)(Gen2.Bank.RESERVED), wordAddress, data);
        }

        private static Command Kill = new Command(KillFunc, "kill password [target]",
            "Kill tag",
            "password -- Kill password",
            "target -- Tag to kill",
            "",
            "kill 0x12345678",
            "kill 0x12345678 EPC:044D00000000040007010062");
        private static void KillFunc(Reader rdr, ArgParser pargs)
        {
            // kill password [target]
            string[] args = pargs.commandArgs;
            int i = 0;
            uint pwval = Convert.ToUInt32(ParseValue(args[i++]));
            Gen2.Password password = new Gen2.Password(pwval);
            TagFilter filt = (i < args.Length) ? (TagFilter)ParseValue(args[i++]) : null;
            rdr.KillTag(filt, password);
        }
        private static Command Lock = new Command(LockFunc, "lock action [target]",
            "Lock (or unlock) tag memory",
            "action -- Lock action to apply to tag",
            "target -- Tag to act on",
            "",
            "lock Gen2.LockAction:EPC_LOCK,EPC_PERMALOCK,USER_UNLOCK 0x12345678",
            "lock Gen2.LockAction:EPC_LOCK,EPC_PERMALOCK,USER_UNLOCK 0x12345678 EPC:044D00000000040007010062");
        private static void LockFunc(Reader rdr, ArgParser pargs)
        {
            string[] args = pargs.commandArgs;
            int i = 0;
            TagLockAction action = (TagLockAction)ParseValue(args[i++]);
            uint pwval = Convert.ToUInt32(ParseValue(args[i++]));
            Gen2.Password password = new Gen2.Password(pwval);
            rdr.ParamSet("/reader/gen2/accessPassword",password);
            TagFilter target = (i >= args.Length) ? null : (TagFilter)ParseValue(args[i++]);
            rdr.LockTag(target, action);
        }
        private static Command WriteEpc = new Command(WriteEpcFunc, "writetag epc [target]",
            "Write tag EPC",
            "epc -- EPC to write to tag",
            "target -- Tag to write",
            "",
            "writetag EPC:0123456789ABCDEF01234567",
            "writetag EPC:4B4F6C095977225FC81B6A0F EPC:0123456789ABCDEF01234567");
        private static void WriteEpcFunc(Reader rdr, ArgParser pargs)
        {
            string[] args = pargs.commandArgs;
            int i = 0;
            TagData epc = (TagData)ParseValue(args[i++]);
            TagFilter filter = null;
            if (i < args.Length)
            {
                filter = (TagFilter)ParseValue(args[i++]);
            }
            rdr.WriteTag(filter, epc);
        }
        private static Command WriteMemBytes = new Command(WriteMemBytesFunc, "writemembytes bank addr data [target]",
            "Write tag memory bytewise",
            "bank -- Memory bank",
            "addr -- Byte address within memory bank",
            "data -- Bytes to write",
            "target -- Tag whose memory to write",
            "",
            "writemembytes 0 0 bytes:12345678",
            "writemembytes 0 0 bytes:12345678 EPC:044D00000000040007010062");
        private static void WriteMemBytesFunc(Reader rdr, ArgParser pargs)
        {
            string[] args = pargs.commandArgs;
            int i = 0;
            int bank = (int)ParseValue(args[i++]);
            int addr = (int)ParseValue(args[i++]);
            byte[] data = (byte[])ParseValue(args[i++]);
            TagFilter filter = null;
            if (i < args.Length)
            {
                filter = (TagFilter)ParseValue(args[i++]);
            }
            rdr.WriteTagMemBytes(filter, bank, addr, data);
        }
        private static Command WriteMemWords = new Command(WriteMemWordsFunc, "writememwords bank addr data [target]",
            "Write tag memory wordwise",
            "bank -- Memory bank",
            "addr -- Word address within memory bank",
            "data -- Words to write",
            "target -- Tag whose memory to write",
            "",
            "writememwords 0 0 words:12345678",
            "writememwords 0 0 words:12345678 EPC:044D00000000040007010062");
        private static void WriteMemWordsFunc(Reader rdr, ArgParser pargs)
        {
            string[] args = pargs.commandArgs;
            int i = 0;
            int bank = (int)ParseValue(args[i++]);
            int addr = (int)ParseValue(args[i++]);
            ushort[] data = (ushort[])ParseValue(args[i++]);
            TagFilter filter = null;
            if (i < args.Length)
            {
                filter = (TagFilter)ParseValue(args[i++]);
            }
            rdr.WriteTagMemWords(filter, bank, addr, data);
        }

        private static Command GetParam = new Command(GetParamFunc, "get [name...]", JoinLines(
            "Read reader parameter",
            "With no arguments, gets all reader parameters",
            "With one or more arguments, gets each named parameter",
            "",
            "get",
            "get /reader/version/software",
            "get /reader/version/software /reader/version/supportedProtocols"
        ));
        private static void GetParamFunc(Reader rdr, ArgParser pargs)
        {
            string[] args = pargs.commandArgs;
            string[] names;
            if (0 < args.Length)
            {
                names = args;
            }
            else
            {
                names = rdr.ParamList();
            }
            foreach (string name in names)
            {
                ShowParameter(name, rdr);
            }
        }
        private static Command ListParams = new Command(ListParamsFunc, "list",
            "List available reader parameters (names which can be used with get and set commands)");
        private static void ListParamsFunc(Reader rdr, ArgParser pargs)
        {
            Console.WriteLine(String.Join("\r\n", (string[])rdr.ParamList()));
        }
        private static Command SetParam = new Command(SetParamFunc, "set name value",
            "Set reader parameter",
            "name -- Name of reader parameter",
            "value -- Value to assign to reader parameter",
            "",
            "set /reader/gen2/session 1",
            "set /reader/read/plan [1,2]",
            "set /reader/read/plan Auto/reader/read/plan:");
        private static void SetParamFunc(Reader rdr, ArgParser pargs)
        {
            string[] args = pargs.commandArgs;
            string name = args[0];
            Object value = ParseValue(name.ToLower(), args[1]);
            rdr.ParamSet(name, value);
            ShowParameter(name, rdr);
        }

        private static Command TestCommand = new Command(TestFunc, "test",
            "Run internal tests");
        private static void TestFunc(Reader rdr, ArgParser pargs)
        {
            TestLockActionClass();
            TestParseTagData(rdr);
        }
        private static void TestLockActionClass()
        {
            foreach (string name in new string[] {
                "KILL_LOCK",
                "KILL_UNLOCK",
                "KILL_PERMALOCK",
                "KILL_PERMAUNLOCK",
                "ACCESS_LOCK",
                "ACCESS_UNLOCK",
                "ACCESS_PERMALOCK",
                "ACCESS_PERMAUNLOCK",
                "EPC_LOCK",
                "EPC_UNLOCK",
                "EPC_PERMALOCK",
                "EPC_PERMAUNLOCK",
                "TID_LOCK",
                "TID_UNLOCK",
                "TID_PERMALOCK",
                "TID_PERMAUNLOCK",
                "USER_LOCK",
                "USER_UNLOCK",
                "USER_PERMALOCK",
                "USER_PERMAUNLOCK",
                "kill_lock,access_unlock",
                "kill_lock,access_unlock,epc_permalock,tid_permaunlock,User_loCK",
            })
            {
                Gen2.LockAction action = Gen2.LockAction.Parse(name);
                string tostring = action.ToString();
                Console.WriteLine(name + " -> " + tostring);
                if (name.ToLower() != tostring.ToLower())
                {
                    Console.WriteLine("ERROR: Names don't match");
                }
            }
        }
        private static void TestParseTagData(Reader rdr)
        {
            try
            {
                if (rdr is SerialReader)
                {
                    SerialReader sr = (SerialReader)rdr;

                    Console.WriteLine("Read Tag Multiple");
                    foreach (TagReadData trd in sr.Read(500))
                    {
                        Console.WriteLine(trd.ToString());
                    }

                    Console.WriteLine("Read Tag Single");
                    sr.CmdSetTxRxPorts(2, 2);
                    TagReadData read = sr.CmdReadTagSingle(500, SerialReader.TagMetadataFlag.ALL, null, TagProtocol.NONE);
                    Console.WriteLine(read.ToString());

                    Console.WriteLine("Read Tag Data");
                    read = sr.CmdGen2ReadTagData(500, SerialReader.TagMetadataFlag.ALL, Gen2.Bank.EPC, 0, 4, 0, null);
                    Console.Write(read.ToString());
                    Console.Write(" Data:" + ByteFormat.ToHex(read.Data));
                    Console.WriteLine();

                    Console.WriteLine("Read Multiple with Read Data");
                    sr.CmdClearTagBuffer();
                    sr.CmdReadTagAndDataReadMultiple(500, SerialReader.AntennaSelection.CONFIGURED_ANTENNA, null, 0, Gen2.Bank.EPC, 0, 2);
                    TagReadData[] reads = sr.CmdGetTagBuffer(SerialReader.TagMetadataFlag.ALL | SerialReader.TagMetadataFlag.DATA, false, TagProtocol.GEN2);
                    foreach (TagReadData r in reads)
                    {
                        Console.Write(r.ToString());
                        Console.Write(" Data:" + ByteFormat.ToHex(r.Data));
                        Console.WriteLine();
                    }
                }
            }
            catch (FAULT_NO_TAGS_FOUND_Exception)
            {
                Console.WriteLine("No Tag Found in the selected category.");
            }
        }

        private static Command TestSync = new Command(TestSyncFunc, "testsync",
            "Test code synchronization to prevent threads from sending overlapped commands");
        private static void TestSyncFunc(Reader rdr, ArgParser pargs)
        {
            // USB Reader, M5e Dev Kit: LED on = GPO false
            bool on = false;
            bool off = !on;

            rdr.TagRead += delegate(Object sender, TagReadDataEventArgs e)
            {
                PrintTagRead(e.TagReadData);
                rdr.GpoSet(new GpioPin[] { 
                    new GpioPin(2, on), 
                });
                Console.WriteLine("Wasting time:");
                for (int i = 3; 0 < i; i--)
                {
                    Console.Write(" {0:D}", i);
                    Thread.Sleep(300);
                }
                Console.WriteLine();
            };
            rdr.ReadException += delegate(Object sender, ReaderExceptionEventArgs e)
            {
                Console.WriteLine("Asynchronous Read Exception:");
                Console.WriteLine(e.ReaderException.ToString());
            };

            rdr.GpoSet(new GpioPin[] { 
                    new GpioPin(1, off), 
                    new GpioPin(2, off),
                });
            rdr.StartReading();
            DateTime startTime = DateTime.Now;
            DateTime stopTime = startTime + new TimeSpan(0, 0, 5);
            while (DateTime.Now < stopTime)
            {
                rdr.GpoSet(new GpioPin[] { 
                    new GpioPin(1, off), 
                    new GpioPin(2, off),
                });
                Thread.Sleep(10);
            }
            rdr.StopReading();
            rdr.GpoSet(new GpioPin[] { 
                    new GpioPin(1, on), 
                    new GpioPin(2, on),
                });
        }

        private static Command TestGen2TagDataConstructorsCommand = new Command(TestGen2TagDataConstructorsFunc, "testgen2construct",
    "Run internal Gen2 constructor tests");
        private static void TestGen2TagDataConstructorsFunc(Reader rdr, ArgParser pargs)
        {
            string pcstring = "3000";
            string epcstring = "0x044D00000000040005010064";
            string crcstring = "0xCEDA";
            Gen2.TagData td = MakeGen2TagData(pcstring, epcstring, crcstring);
            Console.WriteLine(td);
        }

        private static Command TestMatchesCommand = new Command(TestMatchesFunc,
            "testmatches",
            "Test TagFilter.matches implementations");
        private static void TestMatchesFunc(Reader rdr, ArgParser pargs)
        {
            List<Gen2.TagData> list = new List<Gen2.TagData>();
            list.Add(MakeGen2TagData("3000", "0x044D00000000040005010064", "CEDA"));
            list.Add(MakeGen2TagData("3000", "044D00000000040007010070", "7107"));
            list.Add(MakeGen2TagData("3000", "044D0000000004000701006E", "82F8"));
            list.Add(MakeGen2TagData("3000", "044D0000000004000401006B", "4981"));
            list.Add(MakeGen2TagData("3000", "044D0000000004000E01006C", "51CD"));
            list.Add(MakeGen2TagData("3000", "044D0000000004000601006D", "C42F"));
            list.Add(MakeGen2TagData("3000", "3028354D8202000000001A6B", "1119"));
            list.Add(MakeGen2TagData("3000", "050000000000000000003E61", "5911"));
            list.Add(MakeGen2TagData("3000", "C8399E480126DC040101006B", "E90C"));
            list.Add(MakeGen2TagData("3000", "2B3A27B0A182AF350101005C", "3548"));
            list.Add(MakeGen2TagData("3000", "88C10D7556FFCAB801010085", "87E8"));
            list.Add(MakeGen2TagData("3000", "050000000000000000003E62", "6972"));
            list.Add(MakeGen2TagData("3000", "D54F973EB39959DC0101007B", "A9D7"));
            list.Add(MakeGen2TagData("3000", "017DD52B43277B3B0101006A", "5E15"));
            list.Add(MakeGen2TagData("3000", "050000000000000000003E63", "7953"));
            list.Add(MakeGen2TagData("3000", "B4759AAAE63B8777010100DC", "1D17"));
            list.Add(MakeGen2TagData("3000", "48C4A2C790E52858020100A3", "D4FB"));
            list.Add(MakeGen2TagData("3000", "A726B70BA4B0566501010055", "3C5D"));
            list.Add(MakeGen2TagData("3000", "EA37F41943C9897901010098", "D187"));
            list.Add(MakeGen2TagData("3000", "A969352F11405A8001010080", "B69B"));
            list.Add(MakeGen2TagData("3000", "12BF6BAB28F2122801010079", "7201"));
            list.Add(MakeGen2TagData("3000", "F0581C70A5E07DF7010100B3", "1B3C"));
            list.Add(MakeGen2TagData("3000", "FE911EEF7B009548010100C2", "059C"));
            list.Add(MakeGen2TagData("3000", "7CDAAD120A073CE90101007A", "64FB"));
            list.Add(MakeGen2TagData("3000", "F1AA7950D9DCA7D6010100AF", "5DEA"));
            list.Add(MakeGen2TagData("3000", "7062721836BE7E9C010100C8", "CBBE"));
            list.Add(MakeGen2TagData("3000", "7F38863B16B4DE5501010050", "27CB"));
            list.Add(MakeGen2TagData("3000", "2D6FB358C2ED07960201004F", "F803"));
            list.Add(MakeGen2TagData("3000", "CC3797853CA8DB8102010058", "F117"));
            list.Add(MakeGen2TagData("3000", "7AF713E5D9ECD647020100B2", "BD94"));
            list.Add(MakeGen2TagData("3000", "069B3ED79043CBCE0101009A", "B18F"));
            list.Add(MakeGen2TagData("3000", "300833B2DDD9014035050000", "42E7"));
            list.Add(MakeGen2TagData("3000", "6EECABF073C4FD980101007C", "C2AE"));
            list.Add(MakeGen2TagData("3000", "349D9427086A289002010056", "C57B"));
            list.Add(MakeGen2TagData("3000", "D4E8CFB7D2D51C7F010100AB", "1250"));
            list.Add(MakeGen2TagData("3000", "69AE2A765487268601010099", "F8B9"));

            TestFilter("No Filter", null, list);
            Gen2.Select startsWithC = new Gen2.Select(false, Gen2.Bank.EPC, 32, 4, new byte[] { 0xC0 });
            TestFilter("EPCs starting with C", startsWithC, list);
            Gen2.Select endsWith100xx = new Gen2.Select(false, Gen2.Bank.EPC, 32 + 96 - (5 * 4), 12, new byte[] { 0x10, 0x00 });
            TestFilter("EPCs ending in 100xx", endsWith100xx, list);
            Gen2.Select doesntEndWith100xx = new Gen2.Select(true, Gen2.Bank.EPC, 32 + 96 - (5 * 4), 12, new byte[] { 0x10, 0x00 });
            TestFilter("EPCs NOT ending in 100xx", doesntEndWith100xx, list);

            MultiFilter startsWithCAndEndsWith100xx = new MultiFilter(new TagFilter[] {
                startsWithC,
                endsWith100xx,
            });
            TestFilter("EPCs starting with C AND ending in 100xx", startsWithCAndEndsWith100xx, list);

            TagData specificTag = new TagData("A726B70BA4B0566501010055");
            TestFilter("Matches A726B70BA4B0566501010055", specificTag, list);
        }
        private static Gen2.TagData MakeGen2TagData(string pcstring, string epcstring, string crcstring)
        {
            byte[] pcbytes = ByteFormat.FromHex(pcstring);
            byte[] epcbytes = ByteFormat.FromHex(epcstring);
            byte[] crcbytes = ByteFormat.FromHex(crcstring);
            Gen2.TagData td = new Gen2.TagData(epcbytes, crcbytes, pcbytes);
            return td;
        }
        private static void TestFilter(string name, TagFilter filter, ICollection tags)
        {
            Console.WriteLine(name);
            foreach (Gen2.TagData td in tags)
            {
                if ((null == filter) || (filter.Matches(td)))
                {
                    Console.WriteLine(td);
                    //String.Format("PC:{0} EPC:{1} CRC:{2}",
                    //    ByteFormat.ToHex(td.PcBytes),
                    //    ByteFormat.ToHex(td.EpcBytes),
                    //    ByteFormat.ToHex(td.CrcBytes)
                    //    ));
                }
            }
            Console.WriteLine();
        }

        private static Command TestReadTagProtocolsCommand = new Command(TestReadTagProtocolsFunc, "testreadtagprotocols",
            "Are we actually assigning protocols to read tags?");
        private static void TestReadTagProtocolsFunc(Reader rdr, ArgParser pargs)
        {
            TagReadData[] reads = rdr.Read(250);
            if (0 == reads.Length)
            {
                Console.WriteLine("Can't read any tags.");
                return;
            }
            foreach (TagReadData read in reads)
            {
                TagProtocol proto = read.Tag.Protocol;
                if (TagProtocol.NONE != proto)
                {
                    Console.WriteLine("Got a non-NONE TagProtocol");
                    return;
                }
            }
            Console.WriteLine("All TagProtocols are NONE.  Are you parsing the tag read data correctly?");
        }

        private static Command TestStrFilterReadPlans = new Command(TestStrFilterReadPlansFunc,
            "teststrfilterreadplans",
            "Is ToString implemented for all filters and read plans?");
        private static void TestStrFilterReadPlansFunc(Reader rdr, ArgParser pargs)
        {
            Gen2.Select g2s = new Gen2.Select(false, Gen2.Bank.EPC, 16, 16, new byte[] { 0x12, 0x34 });
            Console.WriteLine(g2s);
            Gen2.Select notg2s = new Gen2.Select(true, Gen2.Bank.EPC, 16, 16, new byte[] { 0x12, 0x34 });
            Console.WriteLine(notg2s);
            Iso180006b.Select i6bs = new Iso180006b.Select(false, Iso180006b.SelectOp.EQUALS, 0, 0xC0, new byte[] { 0x12, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
            Console.WriteLine(i6bs);
            Iso180006b.Select noti6bs = new Iso180006b.Select(true, Iso180006b.SelectOp.EQUALS, 0, 0xC0, new byte[] { 0x12, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
            Console.WriteLine(noti6bs);
            TagData td = new TagData("1234567890ABCDEF");
            Console.WriteLine(td);
            MultiFilter mf = new MultiFilter(new TagFilter[] { g2s, i6bs, td });
            Console.WriteLine(mf);

            SimpleReadPlan srp1 = new SimpleReadPlan(null, TagProtocol.GEN2, g2s, 1000);
            Console.WriteLine(srp1);
            SimpleReadPlan srp2 = new SimpleReadPlan(new int[] { 1, 2 }, TagProtocol.ISO180006B, i6bs, 1000);
            Console.WriteLine(srp2);
            MultiReadPlan mrp = new MultiReadPlan(new ReadPlan[] { srp1, srp2 });
            Console.WriteLine(mrp);
        }

        private static Command TestRegCmds = new Command(TestRegCmdsFunc, "TestRegCmds", "Hard-coded test of regulatory commands");
        private static void TestRegCmdsFunc(Reader rdr, ArgParser pargs)
        {
            if (rdr is SerialReader)
            {
                ((SerialReader)rdr).CmdTestSetFrequency(915250);
                Console.WriteLine("Set frequency succeeded");

                ((SerialReader)rdr).CmdTestSendPrbs(1000);
                Console.WriteLine("SendPrbs succeeded ");

                ((SerialReader)rdr).CmdTestSendCw(true);
                Console.WriteLine("SendCw succeeded when on");

                ((SerialReader)rdr).CmdTestSendCw(false);
                Console.WriteLine("SendCw succeeded when off");
            }
            else
            {
                throw new Exception("Reader is not Serial Reader");
            }
        }

        private static Command SavedConfig = new Command(SavedConfigFunc, "TestSavedConfig", "Hard-coded test of user profile configurations");
        private static void SavedConfigFunc(Reader rdr, ArgParser pargs)
        {
            ((SerialReader)rdr).CmdSetProtocol(TagProtocol.GEN2);
            ((SerialReader)rdr).CmdSetUserProfile(SerialReader.UserConfigOperation.SAVE, SerialReader.UserConfigCategory.ALL, SerialReader.UserConfigType.CUSTOM_CONFIGURATION); //save all the configurations
            Console.WriteLine("User profile set option:save all configuration");

            ((SerialReader)rdr).CmdSetUserProfile(SerialReader.UserConfigOperation.RESTORE, SerialReader.UserConfigCategory.ALL, SerialReader.UserConfigType.CUSTOM_CONFIGURATION);//restore all the configurations
            Console.WriteLine("User profile set option:restore all configuration");

            ((SerialReader)rdr).CmdSetUserProfile(SerialReader.UserConfigOperation.VERIFY, SerialReader.UserConfigCategory.ALL, SerialReader.UserConfigType.CUSTOM_CONFIGURATION);//verify all the configurations
            Console.WriteLine("User profile set option:verify all configuration");

            /**********  Testing cmdGetUserProfile function ***********/

            byte[] data1 = new byte[] { 0x67 };
            byte[] response1;
            response1 = ((SerialReader)rdr).cmdGetUserProfile(data1);
            Console.WriteLine("Get user profile success option:Region");
            foreach (byte i in response1)
            {
                Console.Write(" {0:X}", i);
            }
            Console.WriteLine();

            byte[] data2 = new byte[] { 0x63 };
            byte[] response2;
            response2 = ((SerialReader)rdr).cmdGetUserProfile(data2);
            Console.WriteLine("Get user profile success option:Protocol");
            foreach (byte i in response2)
            {
                Console.Write(" {0:X2}", i);
            }
            Console.WriteLine();

            byte[] data3 = new byte[] { 0x06 };
            byte[] response3;
            response3 = ((SerialReader)rdr).cmdGetUserProfile(data3);
            Console.WriteLine("Get user profile success option:Baudrate");
            foreach (byte i in response3)
            {
                Console.Write(" {0:X2}", i);
            }
            Console.WriteLine();

            ((SerialReader)rdr).CmdSetUserProfile(SerialReader.UserConfigOperation.CLEAR, SerialReader.UserConfigCategory.ALL, SerialReader.UserConfigType.CUSTOM_CONFIGURATION);//reset all the configurations
            Console.WriteLine("User profile set option:reset all configuration");
            ((SerialReader)rdr).CmdSetProtocol(TagProtocol.GEN2);
            ((SerialReader)rdr).CmdSetUserProfile(SerialReader.UserConfigOperation.SAVE, SerialReader.UserConfigCategory.ALL, SerialReader.UserConfigType.CUSTOM_CONFIGURATION); //save all the configurations
            Console.WriteLine("User profile set option:save all configuration");
            ((SerialReader)rdr).CmdSetUserProfile(SerialReader.UserConfigOperation.RESTORE, SerialReader.UserConfigCategory.ALL, SerialReader.UserConfigType.FIRMWARE_DEFAULT); //restore firmware default configuration parameters
            Console.WriteLine("User profile set option:restore firmware default configuration parameters");
        }

        private static Command BlockWriteCmds = new Command(BlockWriteCmdsFunc, "TestBlockWrite", "Hard-coded test for Block Write");
        private static void BlockWriteCmdsFunc(Reader rdr, ArgParser pargs)
        {
            ushort[] data = new ushort[] { 0x0102, 0x0304 };
            ((SerialReader)rdr).CmdSetProtocol(TagProtocol.GEN2);
            ((SerialReader)rdr).CmdSetReaderConfiguration(SerialReader.Configuration.EXTENDED_EPC, true);
            Gen2.BlockWrite tagOp = new Gen2.BlockWrite(Gen2.Bank.USER, 0x00, data);
            ((SerialReader)rdr).ExecuteTagOp(tagOp, null);
            Console.WriteLine("BlockWrite succeeded.");

        }

        private static Command BlockPermaLockCmds = new Command(BlockPermaLockCmdsFunc, "TestBlockPermaLock", "Hard-coded test for Block PermaLock");
        private static void BlockPermaLockCmdsFunc(Reader rdr, ArgParser pargs)
        {
            ((SerialReader)rdr).CmdSetProtocol(TagProtocol.GEN2);
            ((SerialReader)rdr).CmdSetReaderConfiguration(SerialReader.Configuration.EXTENDED_EPC, true);
            Gen2.BlockPermaLock tagOp = new Gen2.BlockPermaLock(0x00, Gen2.Bank.USER, 0x00, 0x01, new ushort[] { 0x0000 });
            ((SerialReader)rdr).ExecuteTagOp(tagOp, null);
            Console.WriteLine("BlockPermaLock succeeded.");

        }

        private static Command TagMetaData = new Command(TagMetaDataFunc, "TestTagMetaData", "printing phase and gpio state of tag metadata");
        private static void TagMetaDataFunc(Reader rdr, ArgParser pargs)
        {
            TagReadData[] data;
            ((SerialReader)rdr).ParamSet("/reader/enableStreaming", false);
            data = ((SerialReader)rdr).Read(1000);
            foreach (TagReadData i in data)
            {
                Console.Write("Phase:{0}", i.Phase);
                Console.WriteLine("  GPIO State:{0:X}", i.GPIO);
            }
        }

        private static Command GpioDirection = new Command(GpioDirectionFunc, "TestGpioDirection", "verifying gpio directionality");
        private static void GpioDirectionFunc(Reader rdr, ArgParser pargs)
        {
            int[] input = new int[] { 1, 2 };
            int[] output = new int[] { 3, 4 };

            int[] inputList;
            int[] outputList;

            ((SerialReader)rdr).ParamSet("/reader/gpio/inputList", input);
            Console.WriteLine("Input list set.");

            ((SerialReader)rdr).ParamSet("/reader/gpio/outputList", output);
            Console.WriteLine("Output list set.");

            inputList = (int[])((SerialReader)rdr).ParamGet("/reader/gpio/inputList");

            foreach (int i in inputList)
            {
                Console.WriteLine("input list={0}", i);
            }

            outputList = (int[])((SerialReader)rdr).ParamGet("/reader/gpio/outputList");

            foreach (int i in outputList)
            {
                Console.WriteLine("output list={0}", i);
            }

        }




        /// <summary>
        /// Format arbitrary objects into strings
        /// </summary>
        /// <param name="val">Object</param>
        /// <returns>Human-readable string</returns>
        private static string FormatValue(Object val)
        {
            return FormatValue(val, false);
        }
        /// <summary>
        /// Format arbitrary objects into strings
        /// </summary>
        /// <param name="val">Object</param>
        /// <param name="useHex">If true, format integers in hex, else use decimal</param>
        /// <returns>Human-readable string</returns>
        private static string FormatValue(Object val, bool useHex)
        {
            if (null == val)
            {
                return "null";
            }
            else if (val.GetType().IsArray)
            {
                Array arr = (Array)val;
                string[] valstrings = new string[arr.Length];
                for (int i = 0; i < arr.Length; i++)
                {
                    valstrings[i] = FormatValue(arr.GetValue(i), useHex);
                }

                StringBuilder sb = new StringBuilder();
                sb.Append("[");
                sb.Append(String.Join(",", valstrings));
                sb.Append("]");
                return sb.ToString();
            }
            else if (val is byte)
            {
                return ((byte)val).ToString(useHex ? "X2" : "D");
            }
            else if ((val is ushort) || (val is UInt16))
            {
                return ((ushort)val).ToString(useHex ? "X4" : "D");
            }
            else
            {
                return val.ToString();
            }
        }
        /// <summary>
        /// Parse string representing a parameter value.
        /// </summary>
        /// <param name="name">Name of parameter</param>
        /// <param name="valstr">String to be parsed into a parameter value</param>
        private static Object ParseValue(string name, string valstr)
        {
            Object value = ParseValue(valstr);
            switch (name.ToLower())
            {
                case "/reader/antenna/portswitchgpos":
                case "/reader/region/hoptable":
                    value = ((ArrayList)value).ToArray(typeof(int));
                    break;
                case "/reader/gpio/inputlist":
                    value = ((ArrayList)value).ToArray(typeof(int));
                    break;
                case "/reader/gpio/outputlist":
                    value = ((ArrayList)value).ToArray(typeof(int));
                    break;
                case "/reader/antenna/settlingtimelist":
                case "/reader/antenna/txrxmap":
                case "/reader/radio/portreadpowerlist":
                case "/reader/radio/portwritepowerlist":
                    value = ArrayListToInt2Array((ArrayList)value);
                    break;
                case "/reader/region/lbt/enable":
                case "/reader/antenna/checkport":
                case "/reader/tagreaddata/recordhighestrssi":
                case "/reader/tagreaddata/uniquebyantenna":
                case "/reader/tagreaddata/uniquebydata":
                case "/reader/tagreaddata/reportrssiindbm":
                    value = ParseBool(valstr);
                    break;
                case "/reader/read/plan":
                    // Special Case: If value is list of integers, automatically
                    // interpret as SimpleReadPlan with list of antennas.
                    if (value is ArrayList)
                    {
                        int[] antList = (int[])((ArrayList)value).ToArray(typeof(int));
                        SimpleReadPlan srp = new SimpleReadPlan();
                        srp.Antennas = antList;
                        value = srp;
                    }
                    break;
                case "/reader/region/id":
                    value = Enum.Parse(typeof(Reader.Region), (string)value, true);
                    break;
                case "/reader/powermode":
                    value = Enum.Parse(typeof(Reader.PowerMode), (string)value, true);
                    break;
                case "/reader/tagop/protocol":
                    if (value is string)
                    {
                        value = Enum.Parse(typeof(TagProtocol), (string)value, true);
                    }
                    break;
                case "/reader/gen2/accesspassword":
                    value = new Gen2.Password((UInt32)(int)value);
                    break;
                case "/reader/gen2/session":
                    if (value is int)
                    {
                        switch ((int)value)
                        {
                            case 0: return Gen2.Session.S0;
                            case 1: return Gen2.Session.S1;
                            case 2: return Gen2.Session.S2;
                            case 3: return Gen2.Session.S3;
                        }
                    }
                    break;
                case "/reader/gen2/tagencoding":
                    value = Enum.Parse(typeof(Gen2.TagEncoding), (string)value, true);
                    break;
                case "/reader/iso180006b/blf":
                    if (value is string)
                    {
                        value = Enum.Parse(typeof(Iso180006b.LinkFrequency), (string)value, true);
                    }
                    break;
                default:
                    break;
            }
            return value;
        }
        /// <summary>
        /// Parse arbitrary string-encoded value; e.g.,
        ///   "123" -> (int)123
        ///   "[1,2]" -> new int[] {1,2}
        /// </summary>
        /// <param name="str"></param>
        private static Object ParseValue(string str)
        {
            // null
            if ("null" == str.ToLower())
            {
                return null;
            }

            // Array
            if (str.StartsWith("[") && str.EndsWith("]"))
            {
                try
                {
                    ArrayList list = new ArrayList();

                    // Array of arrays
                    if ('[' == str[1])
                    {
                        int open = 1;
                        while (-1 != open)
                        {
                            int close = str.IndexOf(']', open);
                            list.Add(ParseValue(str.Substring(open, (close - open + 1))));
                            open = str.IndexOf('[', close + 1);
                        }
                    }
                    // Array of scalars
                    else
                    {
                        foreach (string eltstr in str.Substring(1, str.Length - 2).Split(new char[] { ',' }))
                        {
                            if (0 < eltstr.Length) { list.Add(ParseValue(eltstr)); }
                        }
                    }

                    return list;
                }
                catch (Exception) { }
            }

            // Hex Integer
            if (str.ToLower().StartsWith("0x"))
            {
                try { return int.Parse(str.Substring(2), System.Globalization.NumberStyles.HexNumber); }
                catch (Exception) { }
            }

            // Integer (Decimal)
            try { return int.Parse(str); }
            catch (Exception) { }

            // Byte Array: "byte[]{1234567890ABCDEF}"
            {
                string inner = Unwrap(str, "byte[]{", "}");
                if (null != inner)
                {
                    return ByteFormat.FromHex(inner);
                }
            }

            // Byte String: "bytes:1234567890ABCDEF"
            {
                string inner = Unwrap(str, "bytes:", "");
                if (null != inner) { return ByteFormat.FromHex(inner); }
            }
            // Word String: "words:1234567890ABCDEF"
            {
                string inner = Unwrap(str, "words:", "");
                if (null != inner)
                {
                    byte[] bytes = ByteFormat.FromHex(inner);
                    ushort[] words = new ushort[bytes.Length / 2];
                    for (int i = 0; i < words.Length; i++)
                    {
                        words[i] = bytes[2 * i];
                        words[i] <<= 8;
                        words[i] |= bytes[2 * i + 1];
                    }
                    return words;
                }
            }

            // EPC (for TagFilter): "EPC:1234567890ABCDEF)"
            {
                string inner = Unwrap(str, "EPC:", "");
                if (null != inner) { return new TagData(ByteFormat.FromHex(inner)); }
            }

            // Gen2.LockAction:EPC_LOCK
            // Gen2.LockAction:EPC_UNLOCK
            // Gen2.LockAction:EPC_PERMALOCK
            // Gen2.LockAction:EPC_PERMAUNLOCK
            // Gen2.LockAction:EPC_LOCK,ACCESS_LOCK,KILL_LOCK
            {
                string inner = Unwrap(str, "Gen2.LockAction:", "");
                if (null != inner)
                {
                    return Gen2.LockAction.Parse(inner);
                }
            }

            // Iso180006b.LockAction:16
            {
                string inner = Unwrap(str, "Iso180006b.LockAction:", "");
                if (null != inner)
                {
                    byte x = (byte)(int)ParseValue(inner);
                    return new Iso180006b.LockAction(x);
                }
            }

            // SimpleReadPlan:
            // SimpleReadPlan:[[1,2],GEN2]
            // SimpleReadPlan:[[1,2],GEN2,Gen2.Select:[???],1000]
            // SimpleReadPlan:[1,2]
            // TODO: Antenna list alone is no longer supported by the SRP constructors.  Remove from test scripts and this program.
            // TODO: Handle all combinations of protocolList, antennaList
            // TODO: Should CLI provide access to all individual fields; e.g., through some sort of keyword args interface?  e.g., SimpleReadPlan:antennaList=[1,2],weight=2000.  In the API, you would do this by creating a default list, then modifying it, but we don't have that sophistication in the CLI.
            {
                string inner = Unwrap(str, "SimpleReadPlan:", "");
                if (null != inner)
                {
                    if (0 == inner.Length)
                    {
                        return new SimpleReadPlan();
                    }
                    else
                    {
                        ArrayList arglist = (ArrayList)ParseValue(inner);
                        if (arglist[0] is TagProtocol)
                        {
                            return new SimpleReadPlan((int[])(((ArrayList)arglist[1]).ToArray(typeof(int))),
                                                      (TagProtocol)arglist[0]);
                        }
                        else if (arglist[1] is TagProtocol)
                        {
                            return new SimpleReadPlan((int[])(((ArrayList)arglist[0]).ToArray(typeof(int))),
                                                      (TagProtocol)arglist[1]);
                        }
                        else
                        {
                            return new SimpleReadPlan((int[])(((ArrayList)arglist[1]).ToArray(typeof(int))),
                                                      (TagProtocol)arglist[1],
                                                      (TagFilter)arglist[2],
                                                      (int)arglist[3]);
                        }
                    }
                }
            }

            // "TagProtocol:GEN2"
            {
                string inner = Unwrap(str, "TagProtocol:", "");
                if (null != inner)
                {
                    return Enum.Parse(typeof(TagProtocol), inner, true);
                }
            }

            // "Gen2Target:A"
            {
                string inner = Unwrap(str, "Gen2Target:", "");
                if (null != inner)
                {
                    return Enum.Parse(typeof(ThingMagic.Gen2.Target), inner, true);
                }
            }

            // "Gen2Q:DynamicQ"
            {
                string inner = Unwrap(str, "Gen2Q:DynamicQ", "");
                if (null != inner)
                {
                    return new Gen2.DynamicQ();
                }
            }
            // "Gen2Q:StaticQ:1"
            {
                string inner = Unwrap(str, "Gen2Q:StaticQ:", "");
                if (null != inner)
                {
                    return new Gen2.StaticQ(byte.Parse(inner));
                }
            }

            // PowerMode:FULL
            {
                string inner = Unwrap(str, "PowerMode:", "");
                if (null != inner)
                {
                    return Enum.Parse(typeof(SerialReader.PowerMode), inner, true);
                }
            }
            // UserMode:PORTAL
            {
                string inner = Unwrap(str, "UserMode:", "");
                if (null != inner)
                {
                    return Enum.Parse(typeof(SerialReader.UserMode), inner, true);
                }
            }

            return str;
        }
        /// <summary>
        /// Convert ArrayList to array for ArrayList containing ArrayLists of ints.
        /// </summary>
        /// <param name="list">ArrayList</param>
        /// <returns>Array version of input list</returns>
        private static int[][] ArrayListToInt2Array(ArrayList list)
        {
            List<int[]> buildlist = new List<int[]>();
            foreach (ArrayList innerlist in list)
            {
                buildlist.Add((int[])innerlist.ToArray(typeof(int)));
            }
            return buildlist.ToArray();
        }
        private static GpioPin[] ArrayListToGpioPinArray(ArrayList list)
        {
            List<GpioPin> gps = new List<GpioPin>();
            foreach (ArrayList innerlist in list)
            {
                int[] pinval = (int[])innerlist.ToArray(typeof(int));
                int id = pinval[0];
                bool high = (0 != pinval[1]);
                GpioPin gp = new GpioPin(id, high);
                gps.Add(gp);
            }
            return gps.ToArray();
        }
        private static string HexDec(int value)
        {
            return String.Format("0x{0} ({1})", ((int)value).ToString("X"), FormatValue(value));
        }
        private static string Unwrap(string str, string prefix, string suffix)
        {
            return Unwrap(str, prefix, suffix, true);
        }
        private static string Unwrap(string str, string prefix, string suffix, bool ignorecase)
        {
            if (ignorecase)
            {
                str = str.ToLower();
                prefix = prefix.ToLower();
                suffix = suffix.ToLower();
            }
            if (str.StartsWith(prefix) && str.EndsWith(suffix))
            {
                return str.Substring(prefix.Length, str.Length - prefix.Length - suffix.Length);
            }
            else
            {
                return null;
            }
        }

        private static void ShowParameter(string name, Reader rdr)
        {
            Object value = rdr.ParamGet(name);
            // Don't print name unless parameter fetch succeeds;
            // "not found" exception already contains param name.
            Console.Write(name + ": ");
            string repr;
            // Per-parameter conversions
            switch (name)
            {
                default:
                    repr = FormatValue(value);
                    break;
            }
            Console.WriteLine(repr);
        }

        static void PrintTagReadHandler(Object sender, TagReadDataEventArgs e)
        {
            PrintTagRead(e.TagReadData);
        }
        static void PrintTagRead(TagReadData read)
        {
            List<string> strl = new List<string>();
            strl.Add(read.ToString());
            strl.Add("Protocol:" + read.Tag.Protocol.ToString());
            strl.Add("CRC:" + ByteFormat.ToHex(read.Tag.CrcBytes));
            if (read.Tag is Gen2.TagData)
            {
                Gen2.TagData g2td = (Gen2.TagData)(read.Tag);
                strl.Add("PC:" + ByteFormat.ToHex(g2td.PcBytes));
            }
            Console.WriteLine(String.Join(" ", strl.ToArray()));
        }
        static void PrintTagReads(TagReadData[] reads)
        {
            foreach (TagReadData read in reads)
            {
                PrintTagRead(read);
            }
        }
        static void PrintTagDatas(TagData[] reads)
        {
            foreach (TagData read in reads)
            {
                Console.WriteLine(read.ToString());
            }
        }

        private static string BytesToString(byte[] bytes)
        {
            return BytesToString(bytes, "0x", "");
        }
        private static string BytesToString(byte[] bytes, String prefix, String separator)
        {
            List<string> bytestrings = new List<string>();
            foreach (byte b in bytes)
            {
                bytestrings.Add(b.ToString("X2"));
            }
            return prefix + String.Join(separator, bytestrings.ToArray());
        }

        static void ReadExceptionHandler(Object sender, ReaderExceptionEventArgs e)
        {
            Console.WriteLine("Reader threw an exception: " + e.ReaderException.Message);
            Console.WriteLine(e.ReaderException.ToString());
        }
    }
}

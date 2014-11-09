using System;
using System.Collections.Generic;
using System.Text;
// for Thread.Sleep
using System.Threading;

// Reference the API
using ThingMagic;

namespace ReadCustomTransport
{
    /// <summary>
    /// Sample programme that uses custom transport scheme.
    /// It shows the usage of tcp serial transport scheme.
    /// </summary>
    class ReadCustomTransport
    {
        static void Main(string[] args)
        {
             // Program setup
            if (1 != args.Length)
            {
                Console.WriteLine(String.Join("\r\n", new string[] {
                    "Please provide reader URL, such as:",
                    "customschemename://readerIP:portname",
                }));
                Environment.Exit(1);
            }

            try
            {
                // Add the custom transport scheme before calling Create().
                // This can be done by using C# API helper function SetSerialTransport().
                // It accepts two arguments. scheme and serial transport factory function.
                // scheme: the custom transport scheme name. For demonstration using scheme as "tcp".
                // Factory function:custom serial transport factory function
                Reader.SetSerialTransport("tcp", SerialTransportTCP.CreateSerialReader);

                // Create Reader object, connecting to physical device.
                // Wrap reader in a "using" block to get automatic
                // reader shutdown (using IDisposable interface).
                using (Reader r = Reader.Create(args[0]))
                {
                    //Uncomment this line to add default transport listener.
                    //r.Transport += r.SimpleTransportListener;

                    r.Connect();
                    if (Reader.Region.UNSPEC == (Reader.Region)r.ParamGet("/reader/region/id"))
                    {
                        Reader.Region[] supportedRegions = (Reader.Region[])r.ParamGet("/reader/region/supportedRegions");
                        if (supportedRegions.Length < 1)
                        {
                            throw new FAULT_INVALID_REGION_Exception();
                        }
                        else
                        {
                            r.ParamSet("/reader/region/id", supportedRegions[0]);
                        }
                    }
                    //Sync Read
                    Console.WriteLine("Doing a sync read for 1sec duration");
                    TagReadData[] tagReads;
                    // Read tags
                    tagReads = r.Read(1000);
                    // Print tag reads
                    foreach (TagReadData tr in tagReads)
                        Console.WriteLine(tr.ToString());

                    // Async Read
                    Console.WriteLine("Doing an async read for 2sec duration");
                    // Create and add tag listener
                    r.TagRead += delegate(Object sender, TagReadDataEventArgs e)
                    {
                        Console.WriteLine("Background read: " + e.TagReadData);
                    };
                    // Create and add read exception listener
                    r.ReadException += new EventHandler<ReaderExceptionEventArgs>(r_ReadException);

                    r.StartReading();

                    Console.WriteLine("\r\n<Do other work here>\r\n");
                    Thread.Sleep(1000);
                    Console.WriteLine("\r\n<Do other work here>\r\n");
                    Thread.Sleep(1000);

                    r.StopReading();
                }
            }
            catch (ReaderException re)
            {
                Console.WriteLine("Error: " + re.Message);
                Console.Out.Flush();
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error: " + ex.Message);
            }
        }
        static void r_ReadException(object sender, ReaderExceptionEventArgs e)
        {
            Console.WriteLine("Error: " + e.ReaderException.Message);
        }
    }
}

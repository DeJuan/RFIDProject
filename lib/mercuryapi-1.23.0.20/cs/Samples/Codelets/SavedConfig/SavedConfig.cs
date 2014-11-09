using System;
using System.Collections.Generic;
using System.Text;

// Reference the API
using ThingMagic;

namespace SavedConfig
{
    class SavedConfig
    {
        static void Main(string[] args)
        {
            // Program setup
            if (1 != args.Length)
            {
                Console.WriteLine(String.Join("\r\n", new string[] {
                    "Please provide reader URL, such as:",
                    "tmr:///com4",
                    "tmr://my-reader.example.com",
                }));
                Environment.Exit(1);
            }

            try
            {
                // Create Reader object, connecting to physical device.
                // Wrap reader in a "using" block to get automaticq
                // reader shutdown (using IDisposable interface).
                using (Reader r = Reader.Create(args[0]))
                {
                    //Uncomment this line to add default transport listener.
                    //r.Transport += r.SimpleTransportListener;

                    r.Connect();
                    string model = (string)r.ParamGet("/reader/version/model");
                    if ("M6e".Equals(model)
                        || "M6e PRC".Equals(model)
                        || "M6e Micro".Equals(model))
                    {
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

                        ((SerialReader)r).CmdSetProtocol(TagProtocol.ISO180006B);

                        r.ParamSet("/reader/userConfig", new SerialReader.UserConfigOp(SerialReader.UserConfigOperation.SAVE));
                        Console.WriteLine("User profile set option:save all configuration");

                        r.ParamSet("/reader/userConfig", new SerialReader.UserConfigOp(SerialReader.UserConfigOperation.RESTORE));
                        Console.WriteLine("User profile set option:restore all configuration");

                        r.ParamSet("/reader/userConfig", new SerialReader.UserConfigOp(SerialReader.UserConfigOperation.VERIFY));
                        Console.WriteLine("User profile set option:verify all configuration");

                        /**********  Testing cmdGetUserProfile function ***********/


                        object region = r.ParamGet("/reader/region/id");
                        Console.WriteLine("Get user profile success option:Region");
                        Console.WriteLine(region.ToString());


                        object proto = r.ParamGet("/reader/tagop/protocol");
                        Console.WriteLine("Get user profile success option:Protocol");
                        Console.WriteLine(proto.ToString());

                        Console.WriteLine("Get user profile success option:Baudrate");
                        Console.WriteLine(r.ParamGet("/reader/baudRate").ToString());

                        //reset all the configurations
                        r.ParamSet("/reader/userConfig", new SerialReader.UserConfigOp(SerialReader.UserConfigOperation.CLEAR));
                        Console.WriteLine("User profile set option:reset all configuration");

                    }
                    else
                    {
                        Console.WriteLine("Error: This codelet works only on M6e variants");
                    }
                }
            }
            catch (ReaderException re)
            {
                Console.WriteLine("Error: " + re.Message);
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error: " + ex.Message);
            }
        }
    }
}

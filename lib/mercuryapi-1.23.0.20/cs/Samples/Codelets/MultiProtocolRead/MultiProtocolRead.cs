using System;
using System.Collections.Generic;
using System.Text;

// Reference the API
using ThingMagic;

namespace MultiProtocolRead
{
    class Program
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
                    List<ReadPlan> readPlans = new List<ReadPlan>();
                    TagProtocol[] protocolList = (TagProtocol[])r.ParamGet("/reader/version/supportedProtocols");
                    foreach (TagProtocol protocol in protocolList)
                    {
                        readPlans.Add(new SimpleReadPlan(null, protocol, null, null, 10));
                    }
                    MultiReadPlan testMultiReadPlan = new MultiReadPlan(readPlans);
                    r.ParamSet("/reader/read/plan", testMultiReadPlan);
                    TagReadData[] tagRead = r.Read(1000);
                    foreach (TagReadData tr in tagRead)
                        Console.WriteLine(String.Format("{0} {1}",
                            tr.Tag.Protocol, tr.ToString()));
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

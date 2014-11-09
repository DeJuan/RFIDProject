using System;
using System.Collections.Generic;
using System.Text;

// Reference the API
using ThingMagic;

namespace AntennaList
{
    /// <summary>
    /// Sample program that shows how to create a simplereadplan which uses a list of 
    /// antennas provided by the user, sets simple read plan and reads tags for a fixed 
    /// period of time (500ms) and prints the tags found.
    /// </summary>
    class AntennaList
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

                    // Create a list of antennas
                    int[] antennaList = new int[] { 1 };

                    // Create a simplereadplan which uses the antenna list created above
                    SimpleReadPlan plan = new SimpleReadPlan(antennaList, TagProtocol.GEN2);

                    // Set the created readplan
                    r.ParamSet("/reader/read/plan", plan);

                    TagReadData[] tagReads;
                    // Read tags
                    tagReads = r.Read(500);
                    // Print tag reads
                    foreach (TagReadData tr in tagReads)
                        Console.WriteLine(tr.ToString());
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

using System;
using System.Collections.Generic;
using System.Text;

// Reference the API
using ThingMagic;

namespace ReadStopTrigger
{
    /// <summary>
    /// Sample program that shows how to create a stoptrigger readplan to reads tags for a n number of tags
    /// and prints the tags found.
    /// </summary>
    class ReadStopTrigger
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

                    // Set the q value
                    r.ParamSet("/reader/gen2/q", new Gen2.StaticQ(1));
                    
                    // Set the number of tags to read
                    StopOnTagCount sotc = new StopOnTagCount();
                    sotc.N = 1;

                    // Prepare multireadplan. Comment the below code if only single readplan is needed;
                    //StopTriggerReadPlan str1 = new StopTriggerReadPlan(sotc, new int[] { 1 }, 
                    //TagProtocol.GEN2, null, new Gen2.ReadData(Gen2.Bank.TID, 0, 0), 1000);
                    //StopTriggerReadPlan str2 = new StopTriggerReadPlan(sotc, new int[] { 2 }, 
                    //TagProtocol.GEN2, null, new Gen2.ReadData(Gen2.Bank.EPC, 0, 0), 1000);
                    //List<ReadPlan> plan = new List<ReadPlan>();
                    //plan.Add(str1);
                    //plan.Add(str2);
                    //MultiReadPlan readplan = new MultiReadPlan(plan);
                    
                    // Prepare single read plan. Comment the below code if multireadplan is needed
                    StopTriggerReadPlan readplan = new StopTriggerReadPlan(sotc, new int[] { 1 }, 
                        TagProtocol.GEN2, null, new Gen2.ReadData(Gen2.Bank.RESERVED, 0, 0), 1000);
                    
                    // Set readplan
                    r.ParamSet("/reader/read/plan", readplan);
                    
                    TagReadData [] tagReads;
                    // Read tags
                    tagReads = r.Read(1000);
                    // Print tag reads
                    foreach (TagReadData tr in tagReads)
                    {
                        Console.WriteLine(tr.ToString() + ", Protocol: " + tr.Tag.Protocol.ToString());
                        Console.WriteLine("Data: " + ByteFormat.ToHex(tr.Data));
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

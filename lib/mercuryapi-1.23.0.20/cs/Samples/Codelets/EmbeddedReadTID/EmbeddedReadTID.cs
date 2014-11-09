using System;
using System.Collections.Generic;
using System.Text;

// Reference the API
using ThingMagic;

namespace Read
{
    /// <summary>
    /// Sample program that reads tags for a fixed period of time (500ms)
    /// and prints the tags found.
    /// </summary>
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
                    TagReadData[] tagReads;

                    // Read Plan
                    byte length;
                    string model = (string)r.ParamGet("/reader/version/model");
                    if ("M6e".Equals(model)
                        || "M6e PRC".Equals(model)
                        || "M6e Micro".Equals(model)
                        || "Mercury6".Equals(model)
                        || "Astra-EX".Equals(model))
                    {
                        // Specifying the readLength = 0 will return full TID for any tag read in case of M6e varients, M6 and Astra-EX reader.
                        length = 0;
                    }
                    else
                    {
                        length = 2;
                    }
                    TagOp op = new Gen2.ReadData(Gen2.Bank.TID, 0, length);
                    SimpleReadPlan plan = new SimpleReadPlan(null, TagProtocol.GEN2, null, op, 1000);
                    r.ParamSet("/reader/read/plan", plan);

                    // Read tags
                    tagReads = r.Read(500);
                    // Print tag reads
                    foreach (TagReadData tr in tagReads)
                    {
                        Console.WriteLine(tr.ToString());
                        if (0 < tr.Data.Length)
                        {
                            Console.WriteLine("  Data:" + ByteFormat.ToHex(tr.Data, "", " "));
                        } 
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

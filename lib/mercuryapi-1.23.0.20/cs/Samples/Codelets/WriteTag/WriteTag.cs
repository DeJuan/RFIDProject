using System;
using System.Collections.Generic;
using System.Text;

// Reference the API
using ThingMagic;

namespace WriteTag
{
    /// <summary>
    /// Sample program that writes an EPC to a tag
    /// </summary>
    class WriteTag
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

                    Gen2.TagData epc = new Gen2.TagData(new byte[] {
                        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB,
                        0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67,
                    });
                    Gen2.WriteTag tagop = new Gen2.WriteTag(epc);
                    r.ExecuteTagOp(tagop, null);
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

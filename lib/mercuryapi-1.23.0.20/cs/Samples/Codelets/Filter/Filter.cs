using System;
using System.Collections.Generic;
using System.Text;
using ThingMagic;
using System.Threading;

// Sample program that demonstrates different types and uses of TagFilter objects.

namespace Filter
{
    class Program
    {
        static void Main(string[] args)
        {
            // Program setup
            if (args.Length != 1)
            {
                Console.WriteLine("Please provide reader URL, such as:\n"
                                 + "tmr:///com4\n"
                                 + "tmr://my-reader.example.com\n");
                Environment.Exit(1);
            }

            // Create Reader object, connecting to physical device
            try
            {
                Reader r;
                TagReadData[] tagReads, filteredTagReads;
                TagFilter filter;

                r = Reader.Create(args[0]);

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

                // In the current system, sequences of Gen2 operations require Session 0,
                // since each operation resingulates the tag.  In other sessions,
                // the tag will still be "asleep" from the preceding singulation.
                Gen2.Session oldSession = (Gen2.Session)r.ParamGet("/reader/gen2/session");
                Gen2.Session newSession = Gen2.Session.S0;
                Console.WriteLine("Changing to Session "+newSession+" (from Session "+oldSession+")");
                r.ParamSet("/reader/gen2/session", newSession);
                Console.WriteLine();

                try
                {
                    Console.WriteLine("Unfiltered Read:");
                    // Read the tags in the field
                    tagReads = r.Read(500);
                    foreach (TagReadData tr in tagReads)
                        Console.WriteLine(tr.ToString());
                    Console.WriteLine();

                    if (0 == tagReads.Length)
                    {
                        Console.WriteLine("No tags found.");
                    }
                    else
                    {
                        // A TagData object may be used as a filter, for example to
                        // perform a tag data operation on a particular tag.
                        Console.WriteLine("Filtered Tagop:");
                        // Read kill password of tag found in previous operation
                        filter = tagReads[0].Tag;
                        Console.WriteLine("Read kill password of tag {0}", filter);
                        Gen2.ReadData tagop = new Gen2.ReadData(Gen2.Bank.RESERVED, 0, 2);
                        try
                        {
                            ushort[] data = (ushort[])r.ExecuteTagOp(tagop, filter);
                            foreach (ushort word in data)
                            {
                                Console.Write("{0:X4}", word);
                            }
                            Console.WriteLine();
                        }
                        catch (ReaderException ex)
                        {
                            Console.WriteLine("Can't read tag: {0}", ex.Message);
                        }
                        Console.WriteLine();


                        // Filter objects that apply to multiple tags are most useful in
                        // narrowing the set of tags that will be read. This is
                        // performed by setting a read plan that contains a filter.

                        // A TagData with a short EPC will filter for tags whose EPC
                        // starts with the same sequence.
                        filter = new TagData(tagReads[0].Tag.EpcString.Substring(0, 4));
                        Console.WriteLine("EPCs that begin with {0}:", filter);
                        r.ParamSet("/reader/read/plan",
                            new SimpleReadPlan(null, TagProtocol.GEN2, filter, 1000));
                        filteredTagReads = r.Read(500);
                        foreach (TagReadData tr in filteredTagReads)
                            Console.WriteLine(tr.ToString());
                        Console.WriteLine();

                        // A filter can also be an full Gen2 Select operation.  For
                        // example, this filter matches all Gen2 tags where bits 8-19 of
                        // the TID are 0x003 (that is, tags manufactured by Alien
                        // Technology).
                        Console.WriteLine("Tags with Alien Technology TID");
                        filter = new Gen2.Select(false, Gen2.Bank.TID, 8, 12, new byte[] { 0, 0x30 });
                        r.ParamSet("/reader/read/plan", new SimpleReadPlan(null, TagProtocol.GEN2, filter, 1000));
                        filteredTagReads = r.Read(500);
                        foreach (TagReadData tr in filteredTagReads)
                            Console.WriteLine(tr.ToString());
                        Console.WriteLine();

                        // Gen2 Select may also be inverted, to give all non-matching tags
                        Console.WriteLine("Tags without Alien Technology TID");
                        filter = new Gen2.Select(true, Gen2.Bank.TID, 8, 12, new byte[] { 0, 0x30 });
                        r.ParamSet("/reader/read/plan", new SimpleReadPlan(null, TagProtocol.GEN2, filter, 1000));
                        filteredTagReads = r.Read(500);
                        foreach (TagReadData tr in filteredTagReads)
                            Console.WriteLine(tr.ToString());
                        Console.WriteLine();

                        // Filters can also be used to match tags that have already been
                        // read. This form can only match on the EPC, as that's the only
                        // data from the tag's memory that is contained in a TagData
                        // object.
                        // Note that this filter has invert=true. This filter will match
                        // tags whose bits do not match the selection mask.
                        // Also note the offset - the EPC code starts at bit 32 of the
                        // EPC memory bank, after the StoredCRC and StoredPC.
                        filter = new Gen2.Select(true, Gen2.Bank.EPC, 32, 2, new byte[] { (byte)0xC0 });
                        Console.WriteLine("EPCs with first 2 bits equal to zero (post-filtered):");
                        foreach (TagReadData tr in tagReads) // unfiltered tag reads from the first example
                            if (filter.Matches(tr.Tag))
                                Console.WriteLine(tr.ToString());
                        Console.WriteLine();
                    }
                }
                finally
                {
                    // Restore original settings
                    Console.WriteLine("Restoring Session " + oldSession);
                    r.ParamSet("/reader/gen2/session", oldSession);
                }

                // Shut down reader
                r.Destroy();
            }
            catch (ReaderException re)
            {
                Console.WriteLine("Error: " + re.ToString());
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error: " + ex.Message);
            }
        }
    }
}

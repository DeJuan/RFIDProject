using System;
using System.Collections.Generic;
using System.Text;

// Reference the API
using ThingMagic;

namespace LockTag
{
    /// <summary>
    /// Sample program that sets an access password on a tag and
    /// locks its EPC.
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

                    TagReadData[] tagReads;
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
                    Console.WriteLine("Changing to Session " + newSession + " (from Session " + oldSession + ")");
                    r.ParamSet("/reader/gen2/session", newSession);

                    try
                    {
                        // Find a tag to work on
                        tagReads = r.Read(1000);
                        if (0 == tagReads.Length)
                        {
                            Console.WriteLine("No tags found to work on");
                            return;
                        }

                        TagData t = tagReads[0].Tag;

                        // Lock the tag
                        r.ExecuteTagOp(new Gen2.Lock(0, new Gen2.LockAction(Gen2.LockAction.EPC_LOCK)), t);
                        Console.WriteLine("Locked EPC of tag " + t);

                        // Unlock the tag
                        r.ExecuteTagOp(new Gen2.Lock(0, new Gen2.LockAction(Gen2.LockAction.EPC_UNLOCK)), t);
                        Console.WriteLine("Unlocked EPC of tag " + t);
                    }
                    finally
                    {
                        // Restore original settings
                        Console.WriteLine("Restoring Session " + oldSession);
                        r.ParamSet("/reader/gen2/session", oldSession);
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

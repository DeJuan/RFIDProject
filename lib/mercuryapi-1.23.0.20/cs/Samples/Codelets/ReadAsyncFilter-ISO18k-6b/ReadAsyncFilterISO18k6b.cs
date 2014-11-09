using System;
using System.Collections.Generic;
using System.Text;
// for Thread.Sleep
using System.Threading;

// Reference the API
using ThingMagic;

namespace ReadAsyncFilterISO18k6b
{
    /// <summary>
    /// Sample program that reads all standard and non-standard ISO18k-6b in the background and prints the
    /// tags found.
    /// </summary>
    class ReadAsyncFilterISO18k6b
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
                    
                    //Set delimiter to 1
                    r.ParamSet("/reader/iso180006b/delimiter", Iso180006b.Delimiter.DELIMITER1);

                    // Read Plan
                    Iso180006b.Select filter = new Iso180006b.Select(false, Iso180006b.SelectOp.NOTEQUALS, 0, 0xff, new byte[] { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
                    SimpleReadPlan plan = new SimpleReadPlan(null, TagProtocol.ISO180006B, filter, null, 1000);
                    r.ParamSet("/reader/read/plan", plan);

                    // Create and add tag listener
                    r.TagRead += delegate(Object sender, TagReadDataEventArgs e)
                     {
                         Console.WriteLine("Background read: " + e.TagReadData);
                     };

                    // Create and add read exception listener
                    r.ReadException += new EventHandler<ReaderExceptionEventArgs>(r_ReadException);
                    // Search for tags in the background
                    r.StartReading();

                    Console.WriteLine("\r\n<Do other work here>\r\n");
                    Thread.Sleep(500);
                    Console.WriteLine("\r\n<Do other work here>\r\n");
                    Thread.Sleep(500);

                    r.StopReading();
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

        static void r_ReadException(object sender, ReaderExceptionEventArgs e)
        {
            Console.WriteLine("Error: " + e.ReaderException.Message);
    }
    }
}

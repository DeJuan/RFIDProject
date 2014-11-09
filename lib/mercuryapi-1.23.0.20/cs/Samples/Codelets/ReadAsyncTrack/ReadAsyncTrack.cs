using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
// for Thread.Sleep
using System.Threading;

// Reference the API
using ThingMagic;

namespace ReadAsyncTrack
{
    /// <summary>
    /// Sample program that reads tags in the background and track tags
    /// that have been seen; only print the tags that have not been seen
    /// before.
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
                    // Create and add tag listener
                    PrintNewListener rl = new PrintNewListener();
                    r.TagRead += rl.TagRead;

                    // Create and add read exception listener
                    r.ReadException += new EventHandler<ReaderExceptionEventArgs>(r_ReadException);
                    // Search for tags in the background
                    r.StartReading();
                    Thread.Sleep(10000);
                    r.StopReading();

                    r.TagRead -= rl.TagRead;
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

    class PrintNewListener
    {
        Hashtable SeenTags = new Hashtable();

        public void TagRead(Object sender, TagReadDataEventArgs e)
        {
            lock (SeenTags.SyncRoot)
            {
                try
                {
                    TagData t = e.TagReadData.Tag;
                    string epc = t.EpcString;
                    if (!SeenTags.ContainsKey(epc))
                    {
                        Console.WriteLine("New tag: " + t.ToString());
                        SeenTags.Add(epc, null);
                    }
                }
                catch (ArgumentException ex)
                {
                    Console.WriteLine("EPC already added");
                }
            }
        }
    }
}

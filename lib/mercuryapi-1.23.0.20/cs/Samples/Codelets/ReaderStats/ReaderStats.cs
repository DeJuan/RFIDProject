using System;
using System.Collections.Generic;
using System.Text;

// Reference the API
using ThingMagic;
using System.Threading;

namespace ReaderStats
{
    /// <summary>
    /// Sample program that supports reader stats functionality
    /// </summary>
    class ReaderStats
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
                        r.ParamSet("/reader/region/id", Reader.Region.NA);
                    }

                    // Request all reader stats
                    r.ParamSet("/reader/stats/enable", Reader.Stat.StatsFlag.ALL);
                    Console.WriteLine("Get requested reader stats : " + r.ParamGet("/reader/stats/enable").ToString());

                    TagReadData[] tagReads;
                    Reader.Stat.Values objRdrStats = null;

                    #region Perform sync read

                    for (int iteration = 1; iteration <= 4; iteration++)
                    {
                        Console.WriteLine("Iteration: " + iteration);
                        Console.WriteLine("Performing the search operation for 1 sec");
                        tagReads = null;
                        objRdrStats = null;
                        // Read tags
                        tagReads = r.Read(1000);
                        Console.WriteLine("Search is completed. Get the reader stats");
                        objRdrStats = (Reader.Stat.Values)r.ParamGet("/reader/stats");
                        Console.WriteLine(objRdrStats.ToString());
                        Console.WriteLine();
                        Int16[][] objAntennaReturnLoss = (Int16[][])r.ParamGet("/reader/antenna/returnLoss");
                        Console.WriteLine("Antenna Return Loss");
                        foreach (short[] antennaLoss in objAntennaReturnLoss)
                        {
                            Console.WriteLine(" Antenna {0:D} | {1:D}", antennaLoss[0], antennaLoss[1]);
                        }
                        Console.WriteLine();
                    }

                    #endregion Perform sync read

                    #region Perform async read

                    Console.WriteLine();
                    Console.WriteLine("Performing async read for 1 sec");
                    
                    #region Create and add listeners

                    // Create and add tag listener
                    r.TagRead += delegate(Object sender, TagReadDataEventArgs e)
                    {
                        Console.WriteLine("Background read: " + e.TagReadData);
                    };

                    // Create and add read exception listener
                    r.ReadException += r_ReadException;

                    // Add reader stats listener
                    r.StatsListener += r_StatsListener;

                    #endregion Create and add listeners

                    // Search for tags in the background
                    r.StartReading();

                    Console.WriteLine("\r\n<Do other work here>\r\n");
                    Thread.Sleep(500);
                    Console.WriteLine("\r\n<Do other work here>\r\n");
                    Thread.Sleep(500);

                    r.StopReading();

                    #endregion Perform async read
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

        static void r_StatsListener(object sender, StatsReportEventArgs e)
        {
            Console.WriteLine(e.StatsReport.ToString());
            Console.WriteLine();
        }

        static void r_ReadException(object sender, ReaderExceptionEventArgs e)
        {
            Console.WriteLine("Error: " + e.ReaderException.Message);
        }
    }
}
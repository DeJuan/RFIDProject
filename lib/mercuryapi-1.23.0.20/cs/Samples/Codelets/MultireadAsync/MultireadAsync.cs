using System;
using System.Collections.Generic;
using System.Text;
// for Thread.Sleep
using System.Threading;

// Reference the API
using ThingMagic;
namespace MultireadAsync
{
    class MultireadAsync
    {
        static void Main(string[] args)
        {
            // Program setup
            if (1 > args.Length)
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
                Reader[] r = new Reader[args.Length];

                for (int i = 0; i < args.Length; i++)
                {
                    r[i] = Reader.Create(args[i]);

                    //Uncomment this line to add default transport listener.
                    //r[i].Transport += r[i].SimpleTransportListener;
                    Console.WriteLine("Created Reader {0},{1}",i,(string)r[i].ParamGet("/reader/uri"));
                    r[i].Connect();

                    if (Reader.Region.UNSPEC == (Reader.Region)r[i].ParamGet("/reader/region/id"))
                    {
                        Reader.Region[] supportedRegions = (Reader.Region[])r[i].ParamGet("/reader/region/supportedRegions");
                        if (supportedRegions.Length < 1)
                        {
                            throw new FAULT_INVALID_REGION_Exception();
                        }
                        else
                        {
                            r[i].ParamSet("/reader/region/id", supportedRegions[0]);
                        }
                    }
                    // Create and add tag listener
                    r[i].TagRead += PrintTagreads;
                    // Create and add read exception listener
                    r[i].ReadException += new EventHandler<ReaderExceptionEventArgs>(r_ReadException);
                    // Search for tags in the background
                    r[i].StartReading();
                }
                
                Console.WriteLine("\r\n<Do other work here>\r\n");
                Thread.Sleep(5000);
                Console.WriteLine("\r\n<Do other work here>\r\n");
                Thread.Sleep(5000);

                for (int i = 0; i < args.Length; i++)
                {
                    r[i].StopReading();
                    r[i].Destroy();
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
            Reader r = (Reader)sender;
            Console.WriteLine("Exception reader uri {0}",(string)r.ParamGet("/reader/uri"));
            Console.WriteLine("Error: " + e.ReaderException.Message);
        }
        static void PrintTagreads(Object sender, TagReadDataEventArgs e)
        {
            Reader r = (Reader)sender;
            Console.WriteLine("Background read:" + (string)r.ParamGet("/reader/uri") + e.TagReadData);
        }
    }
}

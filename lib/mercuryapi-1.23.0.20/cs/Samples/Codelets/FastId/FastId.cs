using System;
using System.Collections.Generic;
using System.Text;

// Reference the API
using ThingMagic;

namespace FastId
{
    /// <summary>
    ///  Codelet to test the Monza4and5 FastID feature.
    /// </summary>
    class FastId
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
                    // Uncomment this line to add default transport listener.
                    // r.Transport += r.SimpleTransportListener;

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
                    TagFilter filter;
                    byte[] mask = new byte[4];
                    Gen2.Impinj.Monza4.QTPayload payLoad;
                    Gen2.Impinj.Monza4.QTControlByte controlByte;
                    Gen2.Impinj.Monza4.QTReadWrite readWrite;
                    uint accesspassword = 0;

                    r.ParamSet("/reader/tagop/antenna", 1);

                    Gen2.Session session = Gen2.Session.S0;
                    r.ParamSet("/reader/gen2/session", session);

                    SimpleReadPlan readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, null, null, 1000);
                    r.ParamSet("/reader/read/plan", readPlan);

                    // Reading tags with a Monza 4 public EPC in response
                    Console.WriteLine("Reading tags with a Monza 4 public EPC in response");
                    tagReads = r.Read(1000);
                    foreach (TagReadData tagData in tagReads)
                    {
                         Console.WriteLine("Monza4 tag epc: " +tagData.EpcString);
                    }
                    Console.WriteLine();

                    // Initialize the payload and the controlByte of Monza4
                    payLoad = new Gen2.Impinj.Monza4.QTPayload();
                    controlByte = new Gen2.Impinj.Monza4.QTControlByte();

                    Console.WriteLine("Changing to private Mode ");
                    // Executing Monza4 QT Write Set Private tagop
                    payLoad.QTMEM = false;
                    payLoad.QTSR = false;
                    controlByte.QTReadWrite = true;
                    controlByte.Persistence = true;

                    readWrite = new Gen2.Impinj.Monza4.QTReadWrite(accesspassword, payLoad, controlByte);
                    r.ExecuteTagOp(readWrite, null);
                    Console.WriteLine();

                    // Setting the session to S2
                    session = Gen2.Session.S2;
                    r.ParamSet("/reader/gen2/session", session);

                    // Enable filter
                    mask[0] = (byte) 0x20;
                    mask[1] = (byte) 0x01;
                    mask[2] = (byte) 0xB0;
                    mask[3] = (byte) 0x00;
                    filter = new Gen2.Select(true, Gen2.Bank.TID, 0x04, 0x18, mask);

                    Console.WriteLine("Reading tags private Mode with session s2 ");
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, null, 1000);
                    r.ParamSet("/reader/read/plan", readPlan);
                    // Reading tags with a Monza 4 FastID with TID in response
                    Console.WriteLine("Reading tags with a Monza 4 FastID with TID in response");
                    tagReads = r.Read(1000);
                    foreach (TagReadData tagData in tagReads)
                    {
                         Console.WriteLine("Monza4 tag epc: " +tagData.EpcString);
                    }
                    Console.WriteLine();
                    
                    Console.WriteLine("Setting the session to S0");

                    // Setting the session to S0
                    session = Gen2.Session.S0;
                    r.ParamSet("/reader/gen2/session", session);

                    mask[0] = (byte) 0xE2;
                    mask[1] = (byte) 0x80;
                    mask[2] = (byte) 0x11;
                    mask[3] = (byte) 0x05;
                    filter = new Gen2.Select(false, Gen2.Bank.TID, 0x00, 0x20, mask);
                    Console.WriteLine("Reading tags private Mode with session s0 ");
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, null, 1000);
                    r.ParamSet("/reader/read/plan", readPlan);
                    // Reading tags with a Monza 4 FastID with NO TID in response
                    Console.WriteLine("Reading tags with a Monza 4 FastID with NO TID in response");
                    tagReads = r.Read(1000);
                    foreach (TagReadData tagData in tagReads)
                    {
                         Console.WriteLine("Monza4 tag epc: " +tagData.EpcString);
                    }
                    Console.WriteLine();

                    Console.WriteLine("Converting to public mode");
                    // Executing  Monza4 QT Write Set Public tagop
                    payLoad.QTMEM = true;
                    payLoad.QTSR = false;
                    controlByte.QTReadWrite = true;
                    controlByte.Persistence = true;

                    readWrite = new Gen2.Impinj.Monza4.QTReadWrite(accesspassword, payLoad, controlByte);
                    r.ExecuteTagOp(readWrite, null);
                    Console.WriteLine();

                    // Enable filter
                    mask[0] = (byte) 0x20;
                    mask[1] = (byte) 0x01;
                    mask[2] = (byte) 0xB0;
                    mask[3] = (byte) 0x00;
                    filter = new Gen2.Select(true, Gen2.Bank.TID, 0x04, 0x18, mask);
                    Console.WriteLine("Reading tags public Mode with session s0 ");
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, null, 1000);
                    r.ParamSet("/reader/read/plan", readPlan);
                    // Reading tags with a Monza 4 FastID with TID in response
                    tagReads = r.Read(1000);
                    foreach (TagReadData tagData in tagReads)
                    {
                         Console.WriteLine("Monza4 tag epc: " +tagData.EpcString);
                    }
                    Console.WriteLine();
                   
                    Console.WriteLine("Reset the Read protect on ");
                    // Reset the Read protect on
                    payLoad.QTMEM = false;
                    payLoad.QTSR = false;
                    controlByte.QTReadWrite = false;
                    controlByte.Persistence = false;

                    readWrite = new Gen2.Impinj.Monza4.QTReadWrite(accesspassword, payLoad, controlByte);
                    r.ExecuteTagOp(readWrite, null);
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

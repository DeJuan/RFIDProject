using System;
using System.Collections.Generic;
using System.Text;

// Reference the API
using ThingMagic;

namespace DenatranIAVCustomTagOperations
{
    /// <summary>
    /// Sample program that performs the Gen2 Denatran IAV custom tag operations    
    /// </summary>
    class DenatranIAVCustomTagOperations
    {
        #region Fields
        
        //Payload
        public byte[] payload = new byte [] { 0x80 };
        //IAVDenatran secure tag operation - ActivateSecureMode
        Gen2.Denatran.IAV.ActivateSecureMode tagOpActivateSecureMode = null;
        //IAVDenatran secure tag operation - AuthenticateOBU
        Gen2.Denatran.IAV.AuthenticateOBU tagOpAuthenticateOBU = null;
        //IAVDenatran secure tag operation - ActivateSiniavMode
        Gen2.Denatran.IAV.ActivateSiniavMode tagOpActivateSiniavMode = null;
        //IAVDenatran secure tag operation - OBUAuthFullPass1
        Gen2.Denatran.IAV.OBUAuthFullPass1 tagOpOBUAuthFullPass1 = null;
        //IAVDenatran secure tag operation - OBUAuthFullPass2
        Gen2.Denatran.IAV.OBUAuthFullPass2 tagOpOBUAuthFullPass2 = null;
        //IAVDenatran secure tag operation - OBUAuthID
        Gen2.Denatran.IAV.OBUAuthID tagOpOBUAuthID = null;
        //IAVDenatran secure tag operation - OBUReadFromMemMap
        Gen2.Denatran.IAV.OBUReadFromMemMap tagOpOBUReadFromMemMap = null;
        //IAVDenatran secure tag operation - OBUWriteToMemMap
        Gen2.Denatran.IAV.OBUWriteToMemMap tagOpOBUWriteToMemMap = null;
        //IAVDenatran secure tag operation - OBUAuthFullPass
        Gen2.Denatran.IAV.OBUAuthFullPass tagOpOBUAuthFullPass = null;
        //IAVDenatran secure tag operation - GetTokenId
        Gen2.Denatran.IAV.GetTokenId tagOpGetTokenId = null;
        //IAVDenatran secure tag operation - ReadSec
        Gen2.Denatran.IAV.ReadSec tagOpReadSec = null;
        //IAVDenatran secure tag operation - WriteSec
        Gen2.Denatran.IAV.WriteSec tagOpWriteSec = null;

        Reader reader;

        #endregion

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

                    DenatranIAVCustomTagOperations denatranIavCustomTagOp = new DenatranIAVCustomTagOperations();

                    denatranIavCustomTagOp.reader = r;

                    #region Initialsettings
                    //Initial settings
                    //Set BLF 320KHZ
                    Console.WriteLine(String.Format("Get BLF : {0}", r.ParamGet("/reader/gen2/blf")));
                    Console.WriteLine("Set BLF to 320KHZ");
                    r.ParamSet("/reader/gen2/blf", Gen2.LinkFrequency.LINK320KHZ);
                    Console.WriteLine(String.Format("Get BLF : {0}", r.ParamGet("/reader/gen2/blf")));
                    Console.WriteLine();

                    //Set session
                    Console.WriteLine(String.Format("Get session : {0}", r.ParamGet("/reader/gen2/session")));
                    Console.WriteLine("Set session");
                    r.ParamSet("/reader/gen2/session", Gen2.Session.S0);
                    Console.WriteLine(String.Format("Get session : {0}", r.ParamGet("/reader/gen2/session")));
                    Console.WriteLine();

                    //Set target
                    Console.WriteLine(String.Format("Get target : {0}", r.ParamGet("/reader/gen2/target")));
                    Console.WriteLine("Set target");
                    r.ParamSet("/reader/gen2/target", Gen2.Target.AB);
                    Console.WriteLine(String.Format("Get target : {0}", r.ParamGet("/reader/gen2/target")));
                    Console.WriteLine();

                    //Set tari
                    Console.WriteLine(String.Format("Get tari : {0}", r.ParamGet("/reader/gen2/tari")));
                    Console.WriteLine("Set tari");
                    r.ParamSet("/reader/gen2/tari", Gen2.Tari.TARI_6_25US);
                    Console.WriteLine(String.Format("Get tari : {0}", r.ParamGet("/reader/gen2/tari")));
                    Console.WriteLine();

                    //Set tagencoding
                    Console.WriteLine(String.Format("Get tagencoding : {0}", r.ParamGet("/reader/gen2/tagEncoding")));
                    Console.WriteLine("Set tagencoding");
                    r.ParamSet("/reader/gen2/tagEncoding", Gen2.TagEncoding.FM0);
                    Console.WriteLine(String.Format("Get tagencoding : {0}", r.ParamGet("/reader/gen2/tagEncoding")));
                    Console.WriteLine();

                    //Set Q
                    Console.WriteLine(String.Format("Get Q : {0}", r.ParamGet("/reader/gen2/q")));
                    Console.WriteLine("Set Q");
                    Gen2.StaticQ staticQ = new Gen2.StaticQ(0);
                    r.ParamSet("/reader/gen2/q", staticQ);
                    Console.WriteLine(String.Format("Get Q : {0}", r.ParamGet("/reader/gen2/q")));
                    Console.WriteLine();
                    #endregion Initialsettings

                    TagReadData[] tagReads;
                    // Read
                    tagReads = r.Read(500);

                    if (tagReads.Length == 0)
                    {
                        Console.WriteLine("Error : No tags found");
                    }
                    else
                    {
                        #region Filter initialization
                        //Gen2Select filter
                        Gen2.Select selectfilter = new Gen2.Select(false, Gen2.Bank.EPC, 32,
                            (ushort)(tagReads[0].Epc.Length * 8), ByteFormat.FromHex(tagReads[0].EpcString));
                        //TagData filter
                        TagFilter tagdataFilter = new TagData(ByteFormat.FromHex(tagReads[0].EpcString));

                        #endregion Filter initialization

                        #region Tag Operation Initialization

                        byte payload = 0x80;
                        ushort readptr = 0xFFFF;
                        ushort wordAddress = 0xFFFF;
                        ushort word = 0xFFFF;
                        //Read ptr for readsec tagop
                        ushort readSecReadPtr = 0x0000;
                        //Set the tag Identification and writeCredentials
                        Gen2.DenatranIAVWriteCredential writeCredential = new Gen2.DenatranIAVWriteCredential
                            ( new byte [] {0x80, 0x10, 0x00, 0x12, 0x34, 0xAD, 0xBD, 0xC0} , 
                            new byte [] {0xDE,0xAD,0xBE,0xEF,0xDE,0xAD,0xBE,0xEF,0xDE,0xAD,0xBE,0xEF,0xDE,0xAD,0xBE,0xEF});
                        //Set the data and writeCredentials for WriteSec
                        Gen2.DenatranIAVWriteSecCredential writeSecCredential = new Gen2.DenatranIAVWriteSecCredential
                            (new byte[] { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 },
                            new byte[] { 0x35, 0x49, 0x87, 0xbd, 0xb2, 0xab, 0xd2, 0x7c, 0x2e, 0x34, 0x78, 0x8b, 0xf2, 0xf7, 0x0b, 0xa2 });
                        //IAVDenatran secure tag operation - ActivateSecureMode
                        denatranIavCustomTagOp.tagOpActivateSecureMode = new Gen2.Denatran.IAV.ActivateSecureMode(payload);
                        //IAVDenatran secure tag operation - AuthenticateOBU
                        denatranIavCustomTagOp.tagOpAuthenticateOBU = new Gen2.Denatran.IAV.AuthenticateOBU(payload);
                        //IAVDenatran secure tag operation - ActivateSiniavMode
                        denatranIavCustomTagOp.tagOpActivateSiniavMode = new Gen2.Denatran.IAV.ActivateSiniavMode(0x81, new byte[] { 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef });
                        //IAVDenatran secure tag operation - OBUAuthFullPass1
                        denatranIavCustomTagOp.tagOpOBUAuthFullPass1 = new Gen2.Denatran.IAV.OBUAuthFullPass1(payload);
                        //IAVDenatran secure tag operation - OBUAuthFullPass2
                        denatranIavCustomTagOp.tagOpOBUAuthFullPass2 = new Gen2.Denatran.IAV.OBUAuthFullPass2(payload);
                        //IAVDenatran secure tag operation - OBUAuthID
                        denatranIavCustomTagOp.tagOpOBUAuthID = new Gen2.Denatran.IAV.OBUAuthID(payload);
                        //IAVDenatran secure tag operation - OBUReadFromMemMap
                        denatranIavCustomTagOp.tagOpOBUReadFromMemMap = new Gen2.Denatran.IAV.OBUReadFromMemMap
                            (payload, readptr);
                        //IAVDenatran secure tag operation - OBUWriteToMemMap
                        denatranIavCustomTagOp.tagOpOBUWriteToMemMap = new Gen2.Denatran.IAV.OBUWriteToMemMap
                            (payload, wordAddress, word, writeCredential);
                        //IAVDenatran secure tag operation - OBUAuthFullPass
                        denatranIavCustomTagOp.tagOpOBUAuthFullPass = new Gen2.Denatran.IAV.OBUAuthFullPass(payload);
                        //IAVDenatran secure tag operation - GetTokenId
                        denatranIavCustomTagOp.tagOpGetTokenId = new Gen2.Denatran.IAV.GetTokenId();
                        //IAVDenatran secure tag operation - ReadSec
                        denatranIavCustomTagOp.tagOpReadSec = new Gen2.Denatran.IAV.ReadSec
                            (payload, readSecReadPtr);
                        //IAVDenatran secure tag operation - WriteSec
                        denatranIavCustomTagOp.tagOpWriteSec = new Gen2.Denatran.IAV.WriteSec
                            (payload, writeSecCredential);
                        #endregion Tag Operation Initialization

                        #region Standalonetagoperations

                        //Standalone tagop
                        Console.WriteLine("Standalone tagop without filter : ");
                        Console.WriteLine();
                        denatranIavCustomTagOp.ExecuteTagOpFilter(null);

                        Console.WriteLine("Standalone tagop with tagdata filter : ");
                        Console.WriteLine();
                        denatranIavCustomTagOp.ExecuteTagOpFilter(tagdataFilter);

                        Console.WriteLine("Standalone tagop with gen2Select filter : ");
                        Console.WriteLine();
                        denatranIavCustomTagOp.ExecuteTagOpFilter(selectfilter);

                        #endregion Standalonetagoperations

                        #region Embeddedtagoperations

                        //Embedded tagop
                        Console.WriteLine("Embedded tagop without filter and fastsearch : ");
                        Console.WriteLine();
                        denatranIavCustomTagOp.EmbeddedTagOpFilter(null, false);

                        Console.WriteLine("Embedded tagop with tagdata filter and fastsearch enabled : ");
                        Console.WriteLine();
                        denatranIavCustomTagOp.EmbeddedTagOpFilter(tagdataFilter, true);

                        Console.WriteLine("Embedded tagop with gen2select filter and fast search enabled : ");
                        Console.WriteLine();
                        denatranIavCustomTagOp.EmbeddedTagOpFilter(selectfilter, true);

                        #endregion Embeddedtagoperations
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

        /// <summary>
        /// Execute standalone tag operations
        /// </summary>
        /// <param name="filter"></param>
        private void ExecuteTagOpFilter(TagFilter filter)
        {
            try
            {
                //ActivateSecureMode tag operation
                byte[] activateSecureModedata = (byte[])reader.ExecuteTagOp(tagOpActivateSecureMode, filter);
                Console.WriteLine("Activate secure mode response data : {0}", ByteFormat.ToHex(activateSecureModedata));
                Console.WriteLine("Data length : " + activateSecureModedata.Length);
                Console.WriteLine();

                //AuthenticateOBU tag operation
                byte[] authenticateOBUdata = (byte[])reader.ExecuteTagOp(tagOpAuthenticateOBU, filter);
                Console.WriteLine("AuthenticateOBU response data : {0}", ByteFormat.ToHex(authenticateOBUdata));
                Console.WriteLine("Data length : " + authenticateOBUdata.Length);
                Console.WriteLine();

                //ActivateSiniavMode tag operation
                byte[] activateSiniavModedata = (byte[])reader.ExecuteTagOp(tagOpActivateSiniavMode, filter);
                Console.WriteLine("Activate siniav mode response data : {0}", ByteFormat.ToHex(activateSiniavModedata));
                Console.WriteLine("Data length : " + activateSiniavModedata.Length);
                Console.WriteLine();

                //OBUAuthFullPass1 tag operation
                byte[] OBUAuthFullPass1data = (byte[])reader.ExecuteTagOp(tagOpOBUAuthFullPass1, filter);
                Console.WriteLine("OBUAuthFullPass1 response data : {0}", ByteFormat.ToHex(OBUAuthFullPass1data));
                Console.WriteLine("Data length : " + OBUAuthFullPass1data.Length);
                Console.WriteLine();

                //OBUAuthFullPass2 tag operation
                byte[] OBUAuthFullPass2data = (byte[])reader.ExecuteTagOp(tagOpOBUAuthFullPass2, filter);
                Console.WriteLine("OBUAuthFullPass2 response data : {0}", ByteFormat.ToHex(OBUAuthFullPass2data));
                Console.WriteLine("Data length : " + OBUAuthFullPass2data.Length);
                Console.WriteLine();

                //OBUAuthID tag operation
                byte[] OBUAuthIDdata = (byte[])reader.ExecuteTagOp(tagOpOBUAuthID, filter);
                Console.WriteLine("OBUAuthID response data : {0}", ByteFormat.ToHex(OBUAuthIDdata));
                Console.WriteLine("Data length : " + OBUAuthIDdata.Length);
                Console.WriteLine();

                //OBUReadFromMemMap tag operation
                byte[] OBUReadFromMemMapdata = (byte[])reader.ExecuteTagOp(tagOpOBUReadFromMemMap, filter);
                Console.WriteLine("OBUReadFromMemMap response data : {0}", ByteFormat.ToHex(OBUReadFromMemMapdata));
                Console.WriteLine("Data length : " + OBUReadFromMemMapdata.Length);
                Console.WriteLine();

                //OBUWriteToMemMap tag operation
                byte[] OBUWriteToMemMapdata = (byte[])reader.ExecuteTagOp(tagOpOBUWriteToMemMap, filter);
                Console.WriteLine("OBUWriteToMemMap response data : {0}", ByteFormat.ToHex(OBUWriteToMemMapdata));
                Console.WriteLine("Data length : " + OBUWriteToMemMapdata.Length);
                Console.WriteLine();

                //OBUAuthFullPass tag operation
                byte[] OBUAuthFullPassdata = (byte[])reader.ExecuteTagOp(tagOpOBUAuthFullPass, filter);
                Console.WriteLine("OBUAuthFullPass response data : {0}", ByteFormat.ToHex(OBUAuthFullPassdata));
                Console.WriteLine("Data length : " + OBUAuthFullPassdata.Length);
                Console.WriteLine();

                //GetTokenId tag operation
                byte[] GetTokenIddata = (byte[])reader.ExecuteTagOp(tagOpGetTokenId, filter);
                Console.WriteLine("GetTokenId response data : {0}", ByteFormat.ToHex(GetTokenIddata));
                Console.WriteLine("Data length : " + GetTokenIddata.Length);
                Console.WriteLine();

                //ReadSec tag operation
                byte[] ReadSec = (byte[])reader.ExecuteTagOp(tagOpReadSec, filter);
                Console.WriteLine("ReadSec response data : {0}", ByteFormat.ToHex(ReadSec));
                Console.WriteLine("Data length : " + ReadSec.Length);
                Console.WriteLine();

                //WriteSec tag operation
                byte[] WriteSec = (byte[])reader.ExecuteTagOp(tagOpWriteSec, filter);
                Console.WriteLine("WriteSec response data : {0}", ByteFormat.ToHex(WriteSec));
                Console.WriteLine("Data length : " + ReadSec.Length);
                Console.WriteLine();

                activateSecureModedata = null;
                authenticateOBUdata = null;
                activateSiniavModedata = null;
                OBUAuthFullPass1data = null;
                OBUAuthFullPass2data = null;
                OBUAuthIDdata = null;
                OBUReadFromMemMapdata = null;
                OBUWriteToMemMapdata = null;
                OBUAuthFullPassdata = null;
                GetTokenIddata = null;
                ReadSec = null;
                WriteSec = null;
            }
            catch (ReaderException re)
            {
                throw re;
            }
            catch (Exception ex)
            {
                throw ex;
            }
        }

        /// <summary>
        /// Execute embedded tag operations
        /// </summary>
        /// <param name="filter"> filter </param>
        /// <param name="isFastSearch"> fast search enable</param>
        private void EmbeddedTagOpFilter(TagFilter filter, bool isFastSearch)
        {
            TagReadData[] tagReads = null;
            SimpleReadPlan readPlan = null;
            try
            {
                //ActivateSecureMode tag operation
                if (!isFastSearch)
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpActivateSecureMode, 10);
                }
                else
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpActivateSecureMode, isFastSearch, 10);
                    Console.WriteLine("Fast search enable : " + isFastSearch.ToString());
                }
                reader.ParamSet("/reader/read/plan", readPlan);
                Console.WriteLine("Read Plan : " + readPlan.ToString());
                tagReads = reader.Read(1000);
                Console.WriteLine("Activate secure mode tag operation : ");
                foreach (TagReadData tr in tagReads)
                {
                    Console.WriteLine("Data : {0}", ByteFormat.ToHex(tr.Data));
                    Console.WriteLine("Data length : " + tr.Data.Length);
                }
                Console.WriteLine();

                //AuthenticateOBU tag operation
                tagReads = null;
                readPlan = null;
                if (!isFastSearch)
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpAuthenticateOBU, 10);
                }
                else
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpAuthenticateOBU, isFastSearch, 10);
                    Console.WriteLine("Fast search enable : " + isFastSearch.ToString());
                }
                reader.ParamSet("/reader/read/plan", readPlan);
                Console.WriteLine("Read Plan : " + readPlan.ToString());
                tagReads = reader.Read(1000);
                Console.WriteLine("AuthenticateOBU tag operation : ");
                foreach (TagReadData tr in tagReads)
                {
                    Console.WriteLine("Data : {0}", ByteFormat.ToHex(tr.Data));
                    Console.WriteLine("Data length : " + tr.Data.Length);
                }
                Console.WriteLine();

                //ActivateSiniavMode tag operation
                tagReads = null;
                readPlan = null;
                if (!isFastSearch)
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpActivateSiniavMode, 10);
                }
                else
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpActivateSiniavMode, isFastSearch, 10);
                    Console.WriteLine("Fast search enable : " + isFastSearch.ToString());
                }
                reader.ParamSet("/reader/read/plan", readPlan);
                Console.WriteLine("Read Plan : " + readPlan.ToString());
                tagReads = reader.Read(1000);
                Console.WriteLine("Activate siniav mode tag operation : ");
                foreach (TagReadData tr in tagReads)
                {
                    Console.WriteLine("Data : {0}", ByteFormat.ToHex(tr.Data));
                    Console.WriteLine("Data length : " + tr.Data.Length);
                }
                Console.WriteLine();

                //OBUAuthFullPass1 tag operation
                tagReads = null;
                readPlan = null;
                if (!isFastSearch)
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpOBUAuthFullPass1, 10);
                }
                else
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpOBUAuthFullPass1, isFastSearch, 10);
                    Console.WriteLine("Fast search enable : " + isFastSearch.ToString());
                }
                reader.ParamSet("/reader/read/plan", readPlan);
                Console.WriteLine("Read Plan : " + readPlan.ToString());
                tagReads = reader.Read(1000);
                Console.WriteLine("OBUAuthFullPass1 tag operation : ");
                foreach (TagReadData tr in tagReads)
                {
                    Console.WriteLine("Data : {0}", ByteFormat.ToHex(tr.Data));
                    Console.WriteLine("Data length : " + tr.Data.Length);
                }
                Console.WriteLine();

                //OBUAuthFullPass2 tag operation
                tagReads = null;
                readPlan = null;
                if (!isFastSearch)
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpOBUAuthFullPass2, 10);
                }
                else
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpOBUAuthFullPass2, isFastSearch, 10);
                    Console.WriteLine("Fast search enable : " + isFastSearch.ToString());
                }
                reader.ParamSet("/reader/read/plan", readPlan);
                Console.WriteLine("Read Plan : " + readPlan.ToString());
                tagReads = reader.Read(1000);
                Console.WriteLine("OBUAuthFullPass2 tag operation : ");
                foreach (TagReadData tr in tagReads)
                {
                    Console.WriteLine("Data : {0}", ByteFormat.ToHex(tr.Data));
                    Console.WriteLine("Data length : " + tr.Data.Length);
                }
                Console.WriteLine();

                //OBUAuthID tag operation
                tagReads = null;
                readPlan = null;
                if (!isFastSearch)
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpOBUAuthID, 10);
                }
                else
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpOBUAuthID, isFastSearch, 10);
                    Console.WriteLine("Fast search enable : " + isFastSearch.ToString());
                }
                reader.ParamSet("/reader/read/plan", readPlan);
                Console.WriteLine("Read Plan : " + readPlan.ToString());
                tagReads = reader.Read(1000);
                Console.WriteLine("OBUAuthID tag operation : ");
                foreach (TagReadData tr in tagReads)
                {
                    Console.WriteLine("Data : {0}", ByteFormat.ToHex(tr.Data));
                    Console.WriteLine("Data length : " + tr.Data.Length);
                }
                Console.WriteLine();

                //OBUReadFromMemMap tag operation
                tagReads = null;
                readPlan = null;
                if (!isFastSearch)
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpOBUReadFromMemMap, 10);
                }
                else
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpOBUReadFromMemMap, isFastSearch, 10);
                    Console.WriteLine("Fast search enable : " + isFastSearch.ToString());
                }
                reader.ParamSet("/reader/read/plan", readPlan);
                Console.WriteLine("Read Plan : " + readPlan.ToString());
                tagReads = reader.Read(1000);
                Console.WriteLine("OBUReadFromMemMap tag operation : ");
                foreach (TagReadData tr in tagReads)
                {
                    Console.WriteLine("Data : {0}", ByteFormat.ToHex(tr.Data));
                    Console.WriteLine("Data length : " + tr.Data.Length);
                }
                Console.WriteLine();

                //OBUWriteToMemMap tag operation
                tagReads = null;
                readPlan = null;
                if (!isFastSearch)
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpOBUWriteToMemMap, 10);
                }
                else
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpOBUWriteToMemMap, isFastSearch, 10);
                    Console.WriteLine("Fast search enable : " + isFastSearch.ToString());
                }
                reader.ParamSet("/reader/read/plan", readPlan);
                Console.WriteLine("Read Plan : " + readPlan.ToString());
                tagReads = reader.Read(1000);
                Console.WriteLine("OBUWriteToMemMap tag operation : ");
                foreach (TagReadData tr in tagReads)
                {
                    Console.WriteLine("Data : {0}", ByteFormat.ToHex(tr.Data));
                    Console.WriteLine("Data length : " + tr.Data.Length);
                }
                Console.WriteLine();

                //OBUAuthFullPass tag operation
                tagReads = null;
                readPlan = null;
                if (!isFastSearch)
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpOBUAuthFullPass, 10);
                }
                else
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpOBUAuthFullPass, isFastSearch, 10);
                    Console.WriteLine("Fast search enable : " + isFastSearch.ToString());
                }
                reader.ParamSet("/reader/read/plan", readPlan);
                Console.WriteLine("Read Plan : " + readPlan.ToString());
                tagReads = reader.Read(1000);
                Console.WriteLine("OBUAuthFullPass tag operation : ");
                foreach (TagReadData tr in tagReads)
                {
                    Console.WriteLine("Data : {0}", ByteFormat.ToHex(tr.Data));
                    Console.WriteLine("Data length : " + tr.Data.Length);
                }
                Console.WriteLine();

                //GetTokenId tag operation
                tagReads = null;
                readPlan = null;
                if (!isFastSearch)
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpGetTokenId, 10);
                }
                else
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpGetTokenId, isFastSearch, 10);
                    Console.WriteLine("Fast search enable : " + isFastSearch.ToString());
                }
                reader.ParamSet("/reader/read/plan", readPlan);
                Console.WriteLine("Read Plan : " + readPlan.ToString());
                tagReads = reader.Read(1000);
                Console.WriteLine("GetTokenId tag operation : ");
                foreach (TagReadData tr in tagReads)
                {
                    Console.WriteLine("Data : {0}", ByteFormat.ToHex(tr.Data));
                    Console.WriteLine("Data length : " + tr.Data.Length);
                }
                Console.WriteLine();

                //ReadSec tag operation
                tagReads = null;
                readPlan = null;
                if (!isFastSearch)
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpReadSec, 10);
                }
                else
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpReadSec, isFastSearch, 10);
                    Console.WriteLine("Fast search enable : " + isFastSearch.ToString());
                }
                reader.ParamSet("/reader/read/plan", readPlan);
                Console.WriteLine("Read Plan : " + readPlan.ToString());
                tagReads = reader.Read(1000);
                Console.WriteLine("ReadSec tag operation : ");
                foreach (TagReadData tr in tagReads)
                {
                    Console.WriteLine("Data : {0}", ByteFormat.ToHex(tr.Data));
                    Console.WriteLine("Data length : " + tr.Data.Length);
                }
                Console.WriteLine();

                //WriteSec tag operation
                tagReads = null;
                readPlan = null;
                if (!isFastSearch)
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpWriteSec, 10);
                }
                else
                {
                    readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, tagOpWriteSec, isFastSearch, 10);
                    Console.WriteLine("Fast search enable : " + isFastSearch.ToString());
                }
                reader.ParamSet("/reader/read/plan", readPlan);
                Console.WriteLine("Read Plan : " + readPlan.ToString());
                tagReads = reader.Read(1000);
                Console.WriteLine("WriteSec tag operation : ");
                foreach (TagReadData tr in tagReads)
                {
                    Console.WriteLine("Data : {0}", ByteFormat.ToHex(tr.Data));
                    Console.WriteLine("Data length : " + tr.Data.Length);
                }
                Console.WriteLine();
            }
            catch (ReaderException re)
            {
                throw re;
            }
            catch (Exception ex)
            {

                throw ex;
            }
        }
    }
}

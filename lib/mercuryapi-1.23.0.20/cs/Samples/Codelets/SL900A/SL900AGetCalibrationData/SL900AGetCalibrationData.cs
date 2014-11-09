using System;
using System.Collections.Generic;
using System.Text;
using ThingMagic;

namespace SL900AProjectGetCalibrationData
{
    class SL900AGetCalibrationValueTest
    {
        Reader reader;
        static void Main(string[] args)
        {
            Console.WriteLine("Test the SL900A Get Calibration Value function in the Mercury API");
            if (args.Length != 1)
            {
                //If the user did not supply the reader uri command arg, display this message
                Console.WriteLine("Must provide reader uri");
                Console.WriteLine("Sl900AGetCalibrationValueTest.exe [reader]");
            }
            else
            {
                //Create the test object
                SL900AGetCalibrationValueTest test = new SL900AGetCalibrationValueTest();
                //Pass the reader uri to the object
                test.run(args[0]);
            }
        }
        private void run(String reader_uri)
        {
            try
            {
                String PARAM_STR_REGION = "/reader/region/id";
                String PARAM_STR_SESSION = "/reader/gen2/session";
                String PARAM_STR_READPLAN = "/reader/read/plan";

                Console.WriteLine(String.Format("Connecting to {0}", reader_uri));
                //Create the reader
                reader = Reader.Create(reader_uri);

                try
                {
                    //Uncomment this line to add default transport listener.
                    //reader.Transport += reader.SimpleTransportListener;

                    //Connect to the reader
                    reader.Connect();

                    //Set the region to NA
                    if (Reader.Region.UNSPEC == (Reader.Region)reader.ParamGet(PARAM_STR_REGION))
                    {
                        Reader.Region[] supportedRegions = (Reader.Region[])reader.ParamGet("/reader/region/supportedRegions");
                        if (supportedRegions.Length < 1)
                        {
                            throw new FAULT_INVALID_REGION_Exception();
                        }
                        else
                        {
                            reader.ParamSet(PARAM_STR_REGION, supportedRegions[0]);
                        }
                    }
                    //Set the session to session 0
                    reader.ParamSet(PARAM_STR_SESSION, Gen2.Session.S0);

                    //Get the region
                    Reader.Region region = (Reader.Region)reader.ParamGet(PARAM_STR_REGION);
                    Console.WriteLine("The current region is " + region);

                    //Get the session
                    Gen2.Session session = (Gen2.Session)reader.ParamGet(PARAM_STR_SESSION);
                    Console.WriteLine("The current session is " + session);

                    //Get the read plan
                    ReadPlan rp = (ReadPlan)reader.ParamGet(PARAM_STR_READPLAN);
                    Console.WriteLine("The current Read Plan is: " + rp);

                    //Create the Get Calibration Data tag operation
                    Gen2.IDS.SL900A.GetCalibrationData tagOp = new Gen2.IDS.SL900A.GetCalibrationData();

                    //Use the Get Calibration Data (and SFE Parameters) tag op
                    Gen2.IDS.SL900A.CalSfe calSfe = (Gen2.IDS.SL900A.CalSfe)reader.ExecuteTagOp(tagOp, null);

                    //Display the Calibration (and SFE Parameters) Data
                    Console.WriteLine(calSfe);

                    //Display the specific Calibration data gnd_switch
                    Console.WriteLine("gnd_switch: " + calSfe.Cal.GndSwitch);

                    //Display the specific SFE Parameter Verify Sensor ID
                    Console.WriteLine("Verify Sensor ID: " + calSfe.Sfe.VerifySensorID);
                }
                finally
                {
                    //Disconnect from the reader
                    reader.Destroy();
                }
            }
            catch (Exception e) {
                Console.WriteLine("Error: " + e.Message);
            }

        }
    }
}

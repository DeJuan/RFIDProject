using System;
using System.Collections.Generic;
using System.Text;
using ThingMagic;

namespace SL900A
{
    class SL900ASetCalibrationDataTest
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
                SL900ASetCalibrationDataTest test = new SL900ASetCalibrationDataTest();
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
                    Gen2.IDS.SL900A.GetCalibrationData getCal_tagop = new Gen2.IDS.SL900A.GetCalibrationData();

                    //Use the Get Calibration Data (and SFE Parameters) tag op
                    Gen2.IDS.SL900A.CalSfe calSfe = (Gen2.IDS.SL900A.CalSfe)reader.ExecuteTagOp(getCal_tagop, null);

                    //Save the current Cal to restore it to the tag after the test
                    Gen2.IDS.SL900A.CalibrationData restore_cal = (Gen2.IDS.SL900A.CalibrationData)calSfe.Cal;

                    //Display the Calibration (and SFE Parameters) Data
                    Console.WriteLine("Detected Calibration: " + calSfe);

                    //Set the Calibration Data to 0x0123456789ABCD (56 bits)
                    byte[] test_cal_byte_array = new byte[7] { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD };

                    Gen2.IDS.SL900A.CalibrationData test_cal = new Gen2.IDS.SL900A.CalibrationData(test_cal_byte_array, 0);

                    //Execute the Set Calibration Data command with test_cal to change its value 
                    reader.ExecuteTagOp(new Gen2.IDS.SL900A.SetCalibrationData(test_cal), null);

                    //Use Get Calibration Data to retrieve the new Calibration (and SFE Parameters) from the tag
                    Gen2.IDS.SL900A.CalSfe verification_calSfe = (Gen2.IDS.SL900A.CalSfe)reader.ExecuteTagOp(getCal_tagop, null);

                    //Get the Cal data from the CalSfe data
                    Gen2.IDS.SL900A.CalibrationData verification_cal = (Gen2.IDS.SL900A.CalibrationData)verification_calSfe.Cal;

                    //Print the verificationCal
                    Console.WriteLine("Verification Cal: " + verification_cal.ToString());

                    //Ensure that the Calibration Data we set matches the current Calibration Data
                    Console.WriteLine("Set Calibration Data Succeeded? " + test_cal.ToString().Equals(verification_cal.ToString()));

                    //Restore the starting Calibration Data
                    reader.ExecuteTagOp(new Gen2.IDS.SL900A.SetCalibrationData(restore_cal), null);

                    //Get CalSfe of the restored tag
                    Gen2.IDS.SL900A.CalSfe restored_calSfe = (Gen2.IDS.SL900A.CalSfe)reader.ExecuteTagOp(getCal_tagop, null);

                    //Make sure that CalSfe is now the same as it was before the test
                    Console.WriteLine("Restore Calibration Data Succeeded? " + calSfe.ToString().Equals(restored_calSfe.ToString()));
                }
                finally
                {
                    //Disconnect from the reader
                    reader.Destroy();
                }
            }
            catch (Exception e)
            {
                Console.WriteLine("Error: " + e.Message);
            }
        }
    }
}

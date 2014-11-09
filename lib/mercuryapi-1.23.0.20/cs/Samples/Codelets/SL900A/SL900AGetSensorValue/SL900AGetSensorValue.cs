using System;
using System.Collections.Generic;
using System.Text;
using ThingMagic;

namespace SL900A
{
    class SL900AGetSensorValueTest
    {
        private Reader reader;

        static void Main(string[] args)
        {
            Console.WriteLine("Test the SL900A Get Sensor Value function in the Mercury API");
            if (args.Length != 2)
            {
                //If the user did not supply the reader uri command arg, display this message
                Console.WriteLine("Must provide reader uri and sensor type (TEMP, EXT1, EXT2, BATT");
                Console.WriteLine("Sl900ATest.exe [reader] [sensor]");
            }
            else {
                //Create an object
                SL900AGetSensorValueTest test = new SL900AGetSensorValueTest();
                //Pass the reader uri to the object
                test.run(args[0], args[1].ToUpper());
            }
            
        }
        private void run(String reader_uri, String mode) 
        {
            //Create the reader
            reader = Reader.Create(reader_uri);

            try
            {
                //Uncomment this line to add default transport listener.
                //reader.Transport += reader.SimpleTransportListener;

                //Connect to the reader
                reader.Connect();

                //Set up the reader configuration
                setupReaderConfiguration();

                ////Read a tag to ensure that the tag can be seen
                //TagReadData[] trd = this.reader.Read(100);
                //displayTags(trd);

                Gen2.IDS.SL900A.GetSensorValue tagop = null;
                if (mode.Equals("TEMP"))
                {
                    //Create a tag op to retrieve the TEMP sensor value
                    tagop = new Gen2.IDS.SL900A.GetSensorValue(Gen2.IDS.SL900A.Sensor.TEMP);
                }
                else if (mode.Equals("EXT1"))
                {
                    //Create a tag op to retrieve the EXT1 sensor value
                    tagop = new Gen2.IDS.SL900A.GetSensorValue(Gen2.IDS.SL900A.Sensor.EXT1);
                }
                else if (mode.Equals("EXT2"))
                {
                    //Create a tag op to retrieve the EXT2 sensor value
                    tagop = new Gen2.IDS.SL900A.GetSensorValue(Gen2.IDS.SL900A.Sensor.EXT2);
                }
                else if (mode.Equals("BATTV"))
                {
                    //Create a tag op to retrieve the BATTV sensor value
                    tagop = new Gen2.IDS.SL900A.GetSensorValue(Gen2.IDS.SL900A.Sensor.BATTV);
                }
                else
                {
                    //Print that an invalid input was detected
                    Console.WriteLine(String.Format("{0} is not a valid sensor", mode));
                    //Exit the program
                    Environment.Exit(1);
                }

                //Perform an SL900A Get Sensor Value TEMP
                Gen2.IDS.SL900A.SensorReading sensorReading = performSensorReading(tagop);

                //Print the raw sensor value info
                Console.WriteLine(String.Format("ADError:{0} Value:{1} RangeLimit:{2} Raw: {3}", sensorReading.ADError, sensorReading.Value, sensorReading.RangeLimit, sensorReading.Raw));
                //Print the converted sensor value
                if (mode.Equals("TEMP"))
                {
                    Console.WriteLine(String.Format("Temp: {0} C", getCelsiusTemp(sensorReading)));
                }
                else if (mode.Equals("EXT1") || mode.Equals("EXT2"))
                {
                    Console.WriteLine(String.Format("Voltage: {0} V", getVoltage(sensorReading)));
                }
            }
            catch (Exception ex) 
            {
                Console.WriteLine("Error: " + ex.Message);
            }
            finally
            {
                //Destroy reader
                this.reader.Destroy();
            }
            
        }
        private void setupReaderConfiguration() {
            String PARAM_STR_REGION = "/reader/region/id";
            String PARAM_STR_SESSION = "/reader/gen2/session";
            String PARAM_STR_READPLAN = "/reader/read/plan";

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
            Console.WriteLine("Read plan: " + rp);
        }
        private void displayTags(TagReadData[] trd) 
        {
            foreach (TagReadData tag in trd) 
            {
                //Print antenna and epc information for each tag found
                Console.WriteLine(String.Format("Ant:{0} EPC:{1}", tag.Antenna, tag.Epc));
            }
        }
        private Gen2.IDS.SL900A.SensorReading performSensorReading(Gen2.IDS.SL900A.GetSensorValue tagop) 
        {
            //Execute the tagop
            Gen2.IDS.SL900A.SensorReading sensorReading = (Gen2.IDS.SL900A.SensorReading)reader.ExecuteTagOp(tagop, null);
            //Return the sensor reading
            return sensorReading;
        }
        private double getCelsiusTemp(Gen2.IDS.SL900A.SensorReading sensorReading) 
        { 
            //Get the code value
            ushort value = sensorReading.Value;
            //Convert the code to a Temp (Using default config function)
            double temp = ((double)value)*0.18-89.3;
            //Return the temp as a double
            return temp;
        }
        private double getVoltage(Gen2.IDS.SL900A.SensorReading sensorReading)
        {
            //Get the code value
            ushort value = sensorReading.Value;
            //Convert the code to a Voltage (V) (Using default config function)
            double voltage = ((double)value) * .310 / 1024 + .310;
            //Return the voltage as a double
            return voltage;
        }
    }
}

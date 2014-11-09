using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Windows;
using System.Windows.Controls;

namespace ThingMagic.URA2.BL
{
    class LoadSaveConfiguration
    {
        /// <summary>
        /// List of load configuration parameters
        /// </summary>
        private Dictionary<string, string> properties;
        
        // Accepted load save configurations settings
        private Dictionary<string, string> ConfigurationsMapping = new Dictionary<string, string>();

        public Dictionary<string, string> Properties
        {
            get { return properties; }
            set { properties = value; }
        }
        
        /// <summary>
        /// Check for valid config parameter. Throws error if command not found
        /// </summary>
        private void checkForValidCommand()
        {
            foreach (KeyValuePair<string, string> item in ConfigurationsMapping)
            {
                // Check if any configuration parameter is missing
                if(!(Properties.ContainsKey(item.Key)))
                {
                    if (item.Key == "/application/performanceTuning/staticQValue")
                    {
                        if (Properties.ContainsKey("/reader/gen2/q").Equals("StaticQ"))
                        {
                            throw new Exception("Configuration setting is missing: [" + item.Key + "]");
                        }
                    }
                    else
                    {
                        throw new Exception("Configuration setting is missing: [" + item.Key + "]");
                    }
                }

                // Validation for below listed parameters
                // Don't throw exception immediately when the values are empty for below parameters
                if (((item.Key == "/application/displayOption/tagResultColumn/displayEmbeddedReadDataAs") ||
                    (item.Key == "/application/displayOption/tagResultColumn/displayEPCAs") ||
                    (item.Key == "/application/displayOption/tagResult/bigNumSelection") ||
                    (item.Key == "/application/displayOption/tagResultColumn/timeStampFormat") ||
                    (item.Key == "/application/displayOption/tagResultColumnSelection") ||
                    (item.Key == "/reader/baudRate") ||
                    (item.Key == "/reader/gen2/BLF") ||
                    (item.Key == "/reader/gen2/tari") ||
                    (item.Key == "/reader/gen2/tagEncoding") ||
                    (item.Key == "/reader/gen2/session") ||
                    (item.Key == "/reader/gen2/target") ||
                    (item.Key == "/reader/gen2/q") ||
                    (item.Key == "/application/performanceTuning/staticQValue")))
                {
                    // Validation for "/application/performanceTuning/configureGen2SettingsType" 
                    // parameter.
                    // Don't do anything as these parameters can accept empty string only if
                    // "/application/performanceTuning/configureGen2SettingsType" is set to Auto.
                    if (Properties.ContainsKey("/application/performanceTuning/configureGen2SettingsType"))
                    {
                        if (((item.Key == "/reader/gen2/BLF") ||
                            //(item.Key == "/reader/gen2/tari") ||
                            (item.Key == "/reader/gen2/tagEncoding") ||
                            (item.Key == "/reader/gen2/session") ||
                            (item.Key == "/reader/gen2/target") ||
                            (item.Key == "/reader/gen2/q") ||
                            (item.Key == "/application/performanceTuning/staticQValue")) &&
                            (Properties["/application/performanceTuning/configureGen2SettingsType"].Equals("Manual")))
                        {
                            // Validation for "/application/performanceTuning/staticQValue" parameter.
                            // If q paramter is set to static q then throw an exception if staticqvalue is 
                            // empty. StaticQvalue cannnot be empty
                            if (item.Key == "/application/performanceTuning/staticQValue")
                            {
                                if ((Properties["/reader/gen2/q"] == "StaticQ"))
                                {
                                    if (Properties["/application/performanceTuning/staticQValue"] == "")
                                    {
                                        // static q cannot be empty
                                        throw new Exception("Configuration setting: [" + item.Key + "] cannot be empty. When "
                                    + "[/reader/gen2/q] parameter is set "
                                    + "to StaticQ. Please enter a value for this setting and reload the configuration.");
                                    }
                                }
                            }
                            else if (Properties[item.Key] == "")
                            {
                                // Throw an exception when any of the gen2 paramters are empty and only
                                // if configure gen2 settings type is set to manual.
                                throw new Exception("Configuration setting: [" + item.Key + "] cannot be empty. When "
                                    + "[/application/performanceTuning/configureGen2SettingsType] parameter is set "
                                    + "to Manual. Please enter a value for this setting and reload the configuration.");
                            }
                        }
                    }
                }
                else
                {
                    // Validate the below parameters, if either of these two parameters are 
                    // empty, throw an exception because both paramters depend on each other.
                    if ((item.Key == "/application/connect/readerType") ||
                        (item.Key == "/application/connect/readerURI"))
                    {
                        Window mainWindow = App.Current.MainWindow;
                        Label lblConnectionStatus = (Label)mainWindow.FindName("lblshowStatus");
                        if (lblConnectionStatus.Content.ToString().Equals("Connected"))
                        {
                            // Don't throw exception if the reader is already connected and the readertype, readerURI are empty.
                        }
                        else
                        {
                            if (Properties[item.Key] == "")
                            {
                                throw new Exception("Configuration setting: [" + item.Key + "] cannot be empty. Please enter a value"
                                    +" for this setting and reload the configuration.");
                            }
                        }
                    }
                    // Validation for "/reader/region/id" parameter
                    else if ((item.Key == "/reader/region/id"))
                    {
                        if (Properties["/application/connect/readerType"].Equals("SerialReader"))
                        {
                            if (Properties[item.Key] == "")
                            {
                                // Region cannot be empty for serial reader. Hence throw an exception
                                throw new Exception("Configuration setting: [" + item.Key + "] cannot be empty for SerialReader."
                                    + " Please enter a value for this setting and reload the configuration.");
                            }
                        }
                        else
                        {
                            // Don't do anything if reader type is fixedreader
                        }
                    }
                    // Validation for "/application/readwriteOption/Antennas" parameter
                    else if (item.Key == "/application/readwriteOption/Antennas")
                    {
                        // Antenna can be only empty if antenna detection is set to true or else throw the exception
                        if (Properties["/reader/antenna/checkPort"].ToLower().Equals("true"))
                        {
                            // Don't do anything if /reader/antenna/checkPort parameter is set to true
                        }
                        else if (Properties["/reader/antenna/checkPort"].ToLower().Equals("false"))
                        {
                            if (Properties["/application/readwriteOption/Antennas"] == "")
                            {
                                // Antennas cannot be empty if antenna detection is set to false. Hence throw an exception
                                throw new Exception("Configuration setting: [" + item.Key + "] cannot be empty if antenna detection"
                                    + "[/reader/antenna/checkPort] is set other then true."
                                    + " Please enter a value for this setting and reload the configuration.");
                            }
                        }
                        else
                        {
                            // Antennas cannot be empty if antenna detection is set to false. Hence throw an exception
                            throw new Exception("Configuration setting: [" + item.Key + "] cannot be empty if antenna detection"
                                + "[/reader/antenna/checkPort] is set other then true."
                                + " Please enter a value for this setting and reload the configuration.");
                        }
                    }
                    // For rest of the configuration paramters throw an exception if 
                    // values for the parameters are empty.
                    else if (Properties[item.Key] == "")
                    {
                        throw new Exception("Configuration setting: [" + item.Key + "] cannot be empty. Please enter a value"
                            + " for this setting  and reload the configuration.");
                    }
                }
            }
        }
        
        /// <summary>
        /// Load configuration parameters from the specified file and check if the command is valid or no.
        /// </summary>
        /// <param name="Filename">Filename which contains configuration paramters</param>
        public void LoadConfigurations(string Filename)
        {
            if (!(ConfigurationsMapping.Count > 0))
            {
                CreateLoadSaveConfigMapping();
            }
            Properties = GetProperties(Filename);
            checkForValidCommand();
        }

        /// <summary>
        /// Save the configuration paramters from the list into the file specified by the user
        /// </summary>
        /// <param name="Filename">File where configuration parameters need to be saved</param>
        /// <param name="SavedParams">List of configuration paramters to be saved</param>
        public void SaveConfigurations(string Filename, Dictionary<string, string>SavedParams)
        {
            if (!(ConfigurationsMapping.Count > 0))
            {
                // Create configuration parameters mapping to provide comments in the config file
                CreateLoadSaveConfigMapping();
            }

            StreamWriter writer = new StreamWriter(Filename);
            writer.AutoFlush = true;
            //writer.WriteLine();
            //writer.WriteLine("##############################################################################################################################");
            //writer.WriteLine("##                                                                                                                          ##");
            //writer.WriteLine("##                                                                                                                          ##");
            //writer.WriteLine("##                       Configuration settings file to setup universal reader assistant.                                   ##");
            //writer.WriteLine("##                                                                                                                          ##");
            //writer.WriteLine("##                                                                                                                          ##");
            //writer.WriteLine("##############################################################################################################################");

            foreach (KeyValuePair<string, string> item in SavedParams)
            {
                // Check for the specified config command in the mapping and provide the relavant comment for it
                //if (ConfigurationsMapping.ContainsKey(item.Key))
                //{
                //    writer.WriteLine();
                //    writer.WriteLine("//");
                //    writer.WriteLine(ConfigurationsMapping[item.Key]);
                //    writer.WriteLine("//");
                //}
                writer.WriteLine(item.Key +"="+item.Value);
                //writer.WriteLine();
            }
            writer.Close();
        }

        /// <summary>
        /// Read the properties from TestConfiguration.properties file
        /// </summary>
        /// <param name="path"></param>
        /// <returns></returns>
        public Dictionary<string, string> GetProperties(string path)
        {
            Dictionary<string, string> Properties = new Dictionary<string, string>();
            foreach (string line in File.ReadAllLines(path))
            {
                if ((!string.IsNullOrEmpty(line)) &&
                    (!line.StartsWith(";")) &&
                    (!line.StartsWith("#")) &&
                    (!line.StartsWith("'")) &&
                    (!line.StartsWith("//")) &&
                    (!line.StartsWith("*")) &&
                    (line.Contains('=')))
                {
                    int index = line.IndexOf('=');
                    string keyConfig = line.Substring(0, index).Trim();
                    string valueConfig = line.Substring(index + 1).Trim();

                    if ((valueConfig.StartsWith("\"") && valueConfig.EndsWith("\"")) ||
                        (valueConfig.StartsWith("'") && valueConfig.EndsWith("'")))
                    {
                        valueConfig = valueConfig.Substring(1, valueConfig.Length - 2);
                    }
                    Properties.Add(keyConfig, valueConfig);
                }
                
            }
            return Properties;
        }

        /// <summary>
        /// Create a list of configuration parameters with relavant comments for it. This list can be used cor validating the command 
        /// and providing the comments for the specified command
        /// </summary>
        private void CreateLoadSaveConfigMapping()
        {
            ConfigurationsMapping.Add("/application/connect/readerType", "// The type of reader. SerialReader or "
                +"FixedReader. For ex: /application/connect/readerType=SerialReader ");
            
            ConfigurationsMapping.Add("/application/connect/readerURI","// Connect string: In case of serial reader, it is the comport to which the " +
                "reader is connected (e.g., COM3)." + Environment.NewLine + "// In case of fixed reader, it is the IP address or"+
                "hostname of the fixed reader." + Environment.NewLine +
                "// For ex: /application/connect/readerURI=COM3, /application/connect/readerURI=172.16.16.103 and /application/connect/readerURI=m6-2111d3");
            
            ConfigurationsMapping.Add("/reader/baudRate","// Set baudrate for serial readers. Leave this empty if you don't want"
                +" to set any baudrate. If fixed readers URA will skip this setting."+
                Environment.NewLine + "// For ex: /reader/baudRate=9600, /reader/baudRate=115200 ");
            
            ConfigurationsMapping.Add("/reader/region/id", "// Set the region. If fixed readers URA will skip this setting."
                + Environment.NewLine + "// For ex: /reader/region/id=NA(Default), /reader/region/id=EU ");
            
            ConfigurationsMapping.Add("/application/connect/enableTransportLogging", "// Enable or disable transport logging. true - Enable false - Disable");

            ConfigurationsMapping.Add("/application/readwriteOption/ReadBehaviour","// Perform read based on the read behavior "
                +" defined by \n the settings in the Read/Write"+
                " Options sidebar. ReadOnce - "+ Environment.NewLine + "// Perform sync read with the specified [timeout] ms. \n ReadContinuously"+
                " - Perform async read with the specified " + Environment.NewLine + "// [RF On and Off time] ms. " + Environment.NewLine + "// For ex:"+
                " /application/readwriteOption/ReadBehaviour=ReadOnce, /application/readwriteOption/ReadBehaviour=ReadContinuously");

            ConfigurationsMapping.Add("/reader/read/asyncOnTime", "// RF on time. If read behaviour is ReadOnce, URA skips this setting."
                +Environment.NewLine + "// For ex: /reader/read/asyncOnTime=1000(Default), /reader/read/asyncOnTime=250");

            ConfigurationsMapping.Add("/reader/read/asyncOffTime", "// RF off time. If read behaviour is ReadOnce, URA skips this setting."
                + Environment.NewLine + "// For ex: /reader/read/asyncOffTime=0(Default), /reader/read/asyncOffTime=50");

            ConfigurationsMapping.Add("/application/readwriteOption/readOnceTimeout", "// Read for specified time. If read behaviour is "
                +"ReadContinuously, URA skips this setting."+ Environment.NewLine + "// For ex: /application/readwriteOption/readOnceTimeout=500(default),"
                +" /application/readwriteOption/readOnceTimeout=1000 ");

            ConfigurationsMapping.Add("/application/readwriteOption/enableFastSearch", "// Enable or disable fast search. \n true - Enable false - Disable");

            ConfigurationsMapping.Add("/application/readwriteOption/switchingMethod", "// Antenna switching method. \n Equal - Assigns equal "
                +"time to each antenna. Dynamic - Dynamically switches through antennas." + Environment.NewLine + "// Goes to next antenna when "
                +" no more tags found with current." + Environment.NewLine + "// For ex: /application/readwriteOption/switchingMethod=Equal(Default),"
                +" /application/readwriteOption/switchingMethod=Dynamic ");

            ConfigurationsMapping.Add("/application/readwriteOption/Protocols", "// Set protocols Gen2, ISO18000-6B, IPX64, IPX256. \n "
                + Environment.NewLine + "// For ex: /application/readwriteOption/Protocols=Gen2(Default), "
                +"/application/readwriteOption/Protocols=Gen2,IPX64,IPX256 ");

            ConfigurationsMapping.Add("/application/readwriteOption/Antennas", "// Set antenna. For ex: /application/readwriteOption/Antennas=1, "
                +" /application/readwriteOption/Antennas=1,2,3 ");

            ConfigurationsMapping.Add("/reader/antenna/checkPort", "// Enable or disable antenna detection. true - Enable false - Disable");

            ConfigurationsMapping.Add("/application/readwriteOption/enableEmbeddedReadData", "// Add an embedded ReadData TagOp to Inventory."
                +" true - Enable false - Disable");

            ConfigurationsMapping.Add("/application/readwriteOption/enableEmbeddedReadData/MemBank", "// Select the Memory Bank to read from."
                +" Options available are TID, Reserved, EPC, User. "
                + Environment.NewLine + "// For ex: /application/readwriteOption/enableEmbeddedReadData/MemBank=TID, "
                +"/application/readwriteOption/enableEmbeddedReadData/MemBank=Reserved ");

            ConfigurationsMapping.Add("/application/readwriteOption/enableEmbeddedReadData/StartAddress", "// Starting Word Address to read "
                +"from in HEX or Decimal. Prefix HEX number with 0x"
                + Environment.NewLine + "// For ex: /application/readwriteOption/enableEmbeddedReadData/StartAddress=0, "
                +"/application/readwriteOption/enableEmbeddedReadData/StartAddress=0x12 ");

            ConfigurationsMapping.Add("/application/readwriteOption/enableEmbeddedReadData/NoOfWordsToRead", "// Number of Words to read in "
                +"HEX or Decimal. 0 start address and 0 length = Full"
                + Environment.NewLine + "// memory bank. Prefix HEX number with 0x."+ Environment.NewLine + "// For ex: "
                +" /application/readwriteOption/enableEmbeddedReadData/NoOfWordsToRead=0, "
                +" /application/readwriteOption/enableEmbeddedReadData/NoOfWordsToRead=0x12 ");

            ConfigurationsMapping.Add("/reader/tagReadData/uniqueByData", "// Makes Data read a unique characteristic of a Tag Results Entry."
                + Environment.NewLine + "// true - Enable false - Disable");

            ConfigurationsMapping.Add("/application/readwriteOption/enableEmbeddedReadData/uniqueByData/ShowFailedDataRead", "// Show Failed "
                +" Data Reads. true - Enable false - Disable");

            ConfigurationsMapping.Add("/application/readwriteOption/applyFilter", "// Add a Select Filter to the Query. Limits response to tags matching " +
                "Filter settings." + Environment.NewLine + "// true - Enable false - Disable ");

            ConfigurationsMapping.Add("/application/readwriteOption/applyFilter/FilterMemBank", "// Select the Memory Bank to Filter on EPC ID, TID, User. "
                + Environment.NewLine + "// For ex: /application/readwriteOption/applyFilter/FilterMemBank=TID, "
                +"/application/readwriteOption/applyFilter/FilterMemBank=EPC ID ");

            ConfigurationsMapping.Add("/application/readwriteOption/applyFilter/FilterStartAddress", "// Starting BIT Address to apply Filter "
                +"from in HEX or Decimal. Prefix HEX number with 0x"
                + Environment.NewLine + "// For ex: /application/readwriteOption/applyFilter/FilterStartAddress=0, "
                +" /application/readwriteOption/applyFilter/FilterStartAddress=0x12");

            ConfigurationsMapping.Add("/application/readwriteOption/applyFilter/FilterData", "// Data, in HEX, to Filter on. Tags with matching "
                +"data will respond. Prefix HEX number with 0x"
                + Environment.NewLine + "// For ex: /application/readwriteOption/applyFilter/FilterData=DEADBEEF, "
                +"/application/readwriteOption/applyFilter/FilterData=0x1234");

            ConfigurationsMapping.Add("/application/readwriteOption/applyFilter/InvertFilter", "// Causes tags NOT matching the filter "
                +"to respond. true - Enable false - Disable");

            ConfigurationsMapping.Add("/reader/radio/readPower", "// Set read power in cdBm. For ex: /reader/radio/readPower=1000, "
                +"/reader/radio/readPower=3000, /reader/radio/readPower=-1000");

            ConfigurationsMapping.Add("/application/performanceTuning/Enable", "// Enable or disable performance tuning parameters"
                + ". true - Enable false - Disable");

            ConfigurationsMapping.Add("/application/performanceTuning/configureGen2SettingsType", "// Configure gen2 setting manually "
                +" or automatic. Manually - Manual, Automatic - Auto."
                + Environment.NewLine + "// For ex: /application/performanceTuning/configureGen2SettingsType=Auto");

            ConfigurationsMapping.Add("/reader/gen2/BLF", "// Set BLF LINK250KHZ, LINK640KHZ. Leave this empty if "
                + " [/application/performanceTuning/configureGen2SettingsType] parameter is set to Auto." 
                + Environment.NewLine + "// For ex: /reader/gen2/BLF=LINK250KHZ ");

            ConfigurationsMapping.Add("/reader/gen2/tari", "// Set tari TARI_6_25US, TARI_12_5US, TARI_25US. Leave this empty if "
                + " [/application/performanceTuning/configureGen2SettingsType] parameter is set to Auto."
                + Environment.NewLine + "// For ex: /reader/gen2/tari=TARI_12_5US");

            ConfigurationsMapping.Add("/reader/gen2/tagEncoding", "// Set tag encoding FM0, M2, M4, M8. Leave this empty if "
                + "[/application/performanceTuning/configureGen2SettingsType] parameter is set to Auto."
                + Environment.NewLine + "// For ex: /reader/gen2/tagEncoding=FM0 ");

            ConfigurationsMapping.Add("/reader/gen2/session", "// Set session S0, S1, S2, S3. Leave this empty if "
                + "[/application/performanceTuning/configureGen2SettingsType] parameter is set " 
                + Environment.NewLine + "// to Auto. For ex: /reader/gen2/session=S0");

            ConfigurationsMapping.Add("/reader/gen2/target", "// Set target A, B, AB, BA. Leave this empty if "
                + "[/application/performanceTuning/configureGen2SettingsType] parameter is set to Auto."
                + Environment.NewLine + "// For ex: /reader/gen2/target=A");

            ConfigurationsMapping.Add("/reader/gen2/q", "// Set Q to DynamicQ or StaticQ. Leave this empty if "
                + "[/application/performanceTuning/configureGen2SettingsType] parameter is set to Auto."
                + Environment.NewLine + "// For ex: /reader/gen2/q=DynamicQ");

            ConfigurationsMapping.Add("/application/performanceTuning/staticQValue", "// Set static q value within 0 to 15. Leave this empty if "
                + "[/application/performanceTuning/configureGen2SettingsType] parameter is set to Auto."
                + Environment.NewLine + "// URA skips if Q is Dynamic. "
                + Environment.NewLine + "// For ex: /application/performanceTuning/staticQValue=1");

            ConfigurationsMapping.Add("/application/performanceTuning/automaticallyAdjustAsPopulationChanges", "// Automatically adjust "
                +"as population changes. true - Enable false - Disable ");

            ConfigurationsMapping.Add("/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField", "// Optimize for estimated "
                +" number of tags in field."+ Environment.NewLine + "// true - Enable false - Disable");

            ConfigurationsMapping.Add("/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField/tagsInTheField", "// Estimated "
                +" number of tags in field. URA skips this setting if "+
                "[/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField]" + Environment.NewLine + "// setting is set to false "
                + Environment.NewLine + "// For ex: /application/performanceTuning/optimizeForEstimatedNumberOfTagsInField/tagsInTheField=100");

            ConfigurationsMapping.Add("/application/performanceTuning/readDistancevsReadRate", "// Read Distance vs. Read Rate. \n 0 - Maximize "
                +" tag read distance 2 - Maximize tag read rate 1 - Default nothing is set. "
                + Environment.NewLine + "// For ex: /application/performanceTuning/readDistancevsReadRate=0 ");

            ConfigurationsMapping.Add("/application/performanceTuning/selectBestChoiceForPopulationSize", "// Select best choice for population size." +
                " true - Enable false - Disable ");

            ConfigurationsMapping.Add("/application/performanceTuning/customizeTagResponseRate", "// Customize tag response rate from Tags respond."
                +" true - Enable false - Disable");

            ConfigurationsMapping.Add("/application/performanceTuning/customizeTagResponseRate/TagResponseRate", "// Tag response rate from "
                +" Tags respond. URA skips this setting if "+
                "[/application/performanceTuning/customizeTagResponseRate] setting " + Environment.NewLine + "// is set to false " +
                "less often to Tags respond more often i.e 0 to 4." + Environment.NewLine + "// "
                +"For ex: /application/performanceTuning/customizeTagResponseRate/TagResponseRate=0,"
                + Environment.NewLine + "// /application/performanceTuning/customizeTagResponseRate/TagResponseRate=2");

            ConfigurationsMapping.Add("/application/displayOption/fontSize", "// Increase Tag Results font size. For ex: "
                +"/application/displayOption/fontSize=14(Default), /application/displayOption/fontSize=18");

            ConfigurationsMapping.Add("/application/displayOption/enableTagAging", "// Enable or disable tag aging.  \n true - Enable false - Disable ");

            ConfigurationsMapping.Add("/application/displayOption/refreshRate", "// Refresh [TagResults] for every refresh rate interval "
                +" set. For ex: /application/displayOption/refreshRate=100(Default)");

            ConfigurationsMapping.Add("/application/displayOption/tagResultColumnSelection", "// Columns to be displayed on Tag Results. Leave this blank");

            ConfigurationsMapping.Add("/application/displayOption/tagResultColumnSelection/enableAntenna", "// Antenna column to be "
                +" displayed on tag results.  \n true - Enable false - Disable ");
            ConfigurationsMapping.Add("/application/displayOption/tagResultColumnSelection/enableFrequency", "// Frequency column to "
                +" be displayed on tag results.  \n true - Enable false - Disable ");
            ConfigurationsMapping.Add("/application/displayOption/tagResultColumnSelection/enablePhase", "// Phase column to be displayed "
                +" on tag results.  \n true - Enable false - Disable ");
            ConfigurationsMapping.Add("/application/displayOption/tagResultColumnSelection/enableProtocol", "// Protocol column to be "
                +" displayed on tag results.  \n true - Enable false - Disable ");

            ConfigurationsMapping.Add("/application/displayOption/tagResultColumn/timeStampFormat", "// Select to change the Timestamp "
                +"format. Leave this empty if you don't want to change"
                + " the Timestamp format." + Environment.NewLine + "// Options available are Select, DD/MM/YYY HH:MM:Sec.MillSec," +
                " MM/DD/YYY HH:MM:Sec.MillSec," + Environment.NewLine + "// YYY/DD/MM HH:MM:Sec.MillSec, HH:MM:Sec.MillSec."
                +" For ex: /application/displayOption/tagResultColumn/timeStampFormat=YYY/DD/MM HH:MM:Sec.MillSec");

            ConfigurationsMapping.Add("/application/displayOption/tagResult/bigNumSelection", "// Select to get counts in big num format. Leave blank"
                + " if you don't want to get counts in big num format." + Environment.NewLine + "// Options available are Select, "
                +"Remove Big Num, Unique Tag Count" +
                "Total Tag Count, Summary of Tag Result." + Environment.NewLine + "// Maintain the same space as the options has."
                +" For ex: /application/displayOption/tagResult/bigNumSelection="+
                "Total Tag Count is valid and"+ Environment.NewLine + "// /application/displayOption/tagResult/bigNumSelection=TotalTagCount "
                +" is invalid " + Environment.NewLine + "// For ex: /application/displayOption/tagResult/bigNumSelection=Summary of Tag Result");

            ConfigurationsMapping.Add("/application/displayOption/tagResultColumn/displayEPCAs", "// Select to display epc in different format. "
                +"Options available are Select, ASCII, Hex, ReverseBase36. Leave this empty if you don't want to"+Environment.NewLine + "// "
                +"display epc in different format. " +
                Environment.NewLine + "// For ex: /application/displayOption/tagResultColumn/displayEPCAs=ASCII");

            ConfigurationsMapping.Add("/application/displayOption/tagResultColumn/displayEmbeddedReadDataAs", "// Select to display embedded read data in "
                + "different format. Leave this empty if you don't want to" + Environment.NewLine + "// display embedded read data in different format " +
                " Select, ASCII, Hex." + Environment.NewLine + "// For ex: /application/displayOption/tagResultColumn/displayEmbeddedReadDataAs=ASCII");
        }
    }
}
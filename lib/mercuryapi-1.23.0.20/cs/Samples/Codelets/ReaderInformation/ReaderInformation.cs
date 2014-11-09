using System;
using System.Collections.Generic;
using System.Text;

// Reference the API
using ThingMagic;

namespace ReaderInformation
{
    /// <summary>
    /// Sample program to get reader information from the connected reader
    /// </summary>
    class ReaderInformation
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

                    // Create reader information obj
                    ReaderInformation readInfo = new ReaderInformation();
                    Console.WriteLine("Reader information of connected reader");

                    // Hardware info
                    readInfo.Get("Hardware", "/reader/version/hardware", r);

                    // Serial info
                    readInfo.Get("Serial", "/reader/version/serial", r);

                    // Model info
                    readInfo.Get("Model", "/reader/version/model", r);

                    // Software info
                    readInfo.Get("Software", "/reader/version/software", r);

                    // Reader uri info
                    readInfo.Get("Reader URI", "/reader/uri", r);

                    // Product id info
                    readInfo.Get("Product ID", "/reader/version/productID", r);

                    // Product group id info
                    readInfo.Get("Product Group ID", "/reader/version/productGroupID", r);

                    // Product group info
                    readInfo.Get("Product Group", "/reader/version/productGroup", r);

                    // Reader description info
                    readInfo.Get("Reader Description", "/reader/description", r);
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
        /// Get the data for the specified parameter from the connected reader
        /// </summary>
        /// <param name="paramString">Parameter descritpion</param>
        /// <param name="parameter">Parameter to get</param>
        /// <param name="rdrObj">Reader object</param>        
        public void Get(string paramString, string parameter, Reader rdrObj)
        {
            try
            {
                // Get data for the requested parameter from the connected reader
                Console.WriteLine();
                Console.WriteLine(paramString + ": " + rdrObj.ParamGet(parameter).ToString());
            }
            catch (Exception ex)
            {
                if ((ex is FeatureNotSupportedException) || (ex is ArgumentException))
                {
                    Console.WriteLine(paramString + ": " + parameter + " - Unsupported");
                }
                else
                {
                    throw;
                }
            }
        }
    }
}

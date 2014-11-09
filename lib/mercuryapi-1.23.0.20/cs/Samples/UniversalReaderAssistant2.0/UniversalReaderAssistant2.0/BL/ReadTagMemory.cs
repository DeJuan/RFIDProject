using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ThingMagic.URA2.BL
{
    public class ReadTagMemory
    {
        // Model
        string modelReader = string.Empty;
        Reader objectReader = null;

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="readObj">reader object</param>
        /// <param name="model">reader or module model</param>
        public ReadTagMemory(Reader readObj, string model)
        {
            objectReader = readObj;
            modelReader = model;
        }
        
        /// <summary>
        /// Read tag memory when tag has more then 64 words or 128 bytes. And module doesn't supports zero address and zero length.
        /// Reads the tag memory with 0 address and length of words as 64. If memory overrun received. Initiates reading of tag 
        /// word by word with start address as length of alread read data and length of words to read as 0. Returns the data read from 
        /// tag memory.
        /// </summary>
        /// <param name="bank">Gen2 mem bank</param>
        /// <param name="filter">filter</param>
        /// <param name="data">Data read from tag memory</param>
        private void ReadLargeTagMemoryWithoutZeroLengthSupport(Gen2.Bank bank, TagFilter filter, ref ushort[] data)
        {
            lock (new Object())
            {
                data = null;
                // Data from tag memory
                List<ushort> dataTemp = new List<ushort>();
                // Number of words to read
                int words = 64;
                // Start address
                uint startaddress = 0;
                TagOp op;
                bool isAllDatareceived = true;
                // Read till all the data from the tag memory is read
                while (isAllDatareceived)
                {
                    try
                    {
                        op = new Gen2.ReadData(bank, startaddress, Convert.ToByte(words));
                        dataTemp.AddRange(((ushort[])objectReader.ExecuteTagOp(op, filter)));
                        // Increment the start address to 64 and keep length of wrods to read as 64 for
                        // all the iterations
                        startaddress += 64;
                    }
                    catch (Exception ex)
                    {
                        try
                        {
                            if (ex is FAULT_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC_Exception || (-1 != ex.Message.IndexOf("Non-specific reader error")) || (-1 != ex.Message.IndexOf("General Tag Error")) || (-1 != ex.Message.IndexOf("Tag data access failed")))
                            {
                                // If the memory to read requested is more then the available memory in the tag, then initiate 
                                // reading the tag memory word by word with start address as length of already read data if exists 
                                // and lenght of words to read is 1
                                ushort[] wordData = null;
                                if (dataTemp.Count > 0)
                                {
                                    // If some data is already read then include this data also so that 
                                    // reading of memory doesn't continue from zero and the list sholud 
                                    // not contain same data twice
                                    wordData = dataTemp.ToArray();
                                }
                                ReadTagMemoryWordByWord(bank, filter, ref wordData);
                                dataTemp.AddRange(wordData);
                                // Come out of main while loop. And read the already read data
                                isAllDatareceived = false;
                            }
                            else
                            {
                                throw;
                            }
                        }
                        catch (Exception exception)
                        {
                            // If more then once the below exceptions are recieved then come out of the loop.
                            if (exception is FAULT_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC_Exception || (-1 != exception.Message.IndexOf("Non-specific reader error")) || (-1 != exception.Message.IndexOf("General Tag Error")) || (-1 != exception.Message.IndexOf("Tag data access failed")))
                            {
                                if (dataTemp.Count > 0)
                                {
                                    // Just skip the exception and move on. So as not to lose the already read data.
                                    isAllDatareceived = false;
                                }
                                else
                                {
                                    // throw the exception if the data received is null for the first iteration itself
                                    throw;
                                }
                            }
                            else
                            {
                                throw;
                            }
                        }
                    }
                }
                data = dataTemp.ToArray();
            }
        }

        /// <summary>
        /// Read tag memory which has more then 64 words or 128 bytes. Returns the data read from tag memory. 
        /// Stops reading the tag memory if FAULT_PROTOCOL_BIT_DECODING_FAILED_Exception exception is received
        /// more then once. If the data read from tag memory is less then 64 words then initiates reading tag
        /// memory word by word
        /// </summary>
        /// <param name="bank"> Gen2 mem bank</param>
        /// <param name="filter">filter</param>
        /// <param name="data">Mem data obtained from the tag</param>
        private void ReadLargeTagMemory(Gen2.Bank bank, TagFilter filter, ref ushort[] data)
        {
            lock (new Object())
            {
                List<ushort> dataTemp = new List<ushort>();
                if (data != null)
                {
                    dataTemp.AddRange(data);
                }
                int words = 0;
                uint startaddress = 64;
                TagOp op;
                bool isAllDatareceived = true;
                while (isAllDatareceived)
                {
                    try
                    {
                        op = new Gen2.ReadData(bank, startaddress, Convert.ToByte(words));
                        ushort[] tempDataReceived = (ushort[])objectReader.ExecuteTagOp(op, filter);
                        dataTemp.AddRange(tempDataReceived);
                        startaddress += 64;
                        if (tempDataReceived.Length < 64)
                        {
                            isAllDatareceived = false;
                            // If data received is less then 64 words, perform word by word with start address as length
                            // of the data and numer of words to read is 1 
                            ReadTagMemoryWordByWord(bank, filter, ref data);
                        }
                    }
                    catch (Exception ex)
                    {
                        if (ex is FAULT_PROTOCOL_BIT_DECODING_FAILED_Exception)
                        {
                            try
                            {
                                // If read is success then continue the while loop with incremented start address and length as 0 
                                words = 64;
                                op = new Gen2.ReadData(bank, startaddress, Convert.ToByte(words));
                                dataTemp.AddRange(((ushort[])objectReader.ExecuteTagOp(op, filter)));
                                startaddress += 64;
                                words = 0;
                            }
                            catch (Exception et)
                            {
                                // If consecutively we get FAULT_PROTOCOL_BIT_DECODING_FAILED_Exception exception blindly come out of the loop to avoid
                                // dead lock
                                if (et is FAULT_PROTOCOL_BIT_DECODING_FAILED_Exception)
                                {
                                    isAllDatareceived = false;
                                }
                                else
                                {
                                    throw;
                                }
                            }
                        }
                        else
                        {
                            throw;
                        }
                    }
                }
                data = dataTemp.ToArray();
            }
        }

        /// <summary>
        /// Read tag memory word by word. If FAULT_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC_Exception 
        /// or Non-specific reader error or General Tag Error or Tag data access failed exception
        /// is recieved. Memory reading is stopped and the already read data is returned
        /// </summary>
        /// <param name="bank">Gen2 memory bank</param>
        /// <param name="filter">Select filter</param>
        /// <param name="data">Read tag memory data is returned</param>
        private void ReadTagMemoryWordByWord(Gen2.Bank bank, TagFilter filter, ref ushort[] data)
        {
            lock (new Object())
            {
                List<ushort> dataTemp = new List<ushort>();
                uint startaddress = 0;
                if (data != null)
                {
                    // if the data varibale already contains the data, then add that data 
                    // to data temp and chnage the start address to total data length
                    dataTemp.AddRange(data);
                    startaddress = (uint)data.Length;
                }
                int words = 1;
                TagOp op;
                bool isAllDatareceived = true;
                while (isAllDatareceived)
                {
                    try
                    {
                        // Read tag memory word by word
                        op = new Gen2.ReadData(bank, startaddress, Convert.ToByte(words));
                        dataTemp.AddRange(((ushort[])objectReader.ExecuteTagOp(op, filter)));
                        startaddress += 1;
                    }
                    catch (Exception exception)
                    {
                        if (exception is FAULT_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC_Exception || (-1 != exception.Message.IndexOf("Non-specific reader error")) || (-1 != exception.Message.IndexOf("General Tag Error")) || (-1 != exception.Message.IndexOf("Tag data access failed")))
                        {
                            if (dataTemp.Count > 0)
                            {
                                // Just skip the exception and move on. So as not to lose the already read data.
                                isAllDatareceived = false;
                            }
                            else
                            {
                                // throw the exception if the data received is null for the first iteration itself
                                throw;
                            }
                        }
                        else
                        {
                            throw;
                        }
                    }
                }
                data = dataTemp.ToArray();
            }
        }

        /// <summary>
        /// Read tag memory for readers or modules other the M5e variants
        /// </summary>
        /// <param name="bank">Gen2 memory bank</param>
        /// <param name="filter">filter</param>
        /// <param name="MemData">Data read from tag memory</param>
        private void ReadMemoryM6eVariants(Gen2.Bank bank, TagFilter filter, ref ushort[] MemData)
        {
            try
            {
                // Read tag memory with zero as start address and lenght of words to read as zero
                TagOp op = new Gen2.ReadData(bank, 0, 0);
                MemData = (ushort[])objectReader.ExecuteTagOp(op, filter);

                if (MemData.Length < 64)
                {
                    // If data read is less then 64 words, then perform read word by word to make sure 
                    // all the data is read from the tag memory
                    ReadTagMemoryWordByWord(bank, filter, ref MemData);
                }
                else
                {
                    // If data read is more then 64 words then perform read with start address as 64 words
                    // and length as zero
                    ReadLargeTagMemory(bank, filter, ref MemData);
                }
            }
            catch (Exception ex)
            {
                if ((ex is FAULT_PROTOCOL_BIT_DECODING_FAILED_Exception) || ((ex is FAULT_NO_TAGS_FOUND_Exception)))
                {
                    // Perform read when the tag has mem more then 128 bytes and doesn't support 
                    // Zero start address and number of words to read as zero
                    ReadLargeTagMemoryWithoutZeroLengthSupport(bank, filter, ref MemData);
                }
                else
                {
                    throw;
                }
            }
        }

        /// <summary>
        /// Read specified tag memory bank 
        /// </summary>
        /// <param name="bank">Gen2 bank</param>
        /// <param name="filter">filter</param>
        /// <param name="MemData">Data read from the tag memory</param>
        public void ReadTagMemoryData(Gen2.Bank bank, TagFilter filter, ref ushort[] MemData)
        {

            if (modelReader.Equals("M5e") || modelReader.Equals("M5e EU") || modelReader.Equals("M5e Compact") || modelReader.Equals("Astra"))
            {
                // Read tag memory word by word for M5e variants
                ReadTagMemoryWordByWord(bank, filter, ref MemData);
            }
            else
            {
                // Read tag memory for m6e variants
                ReadMemoryM6eVariants(bank, filter, ref MemData);
            }
        }
    }
}

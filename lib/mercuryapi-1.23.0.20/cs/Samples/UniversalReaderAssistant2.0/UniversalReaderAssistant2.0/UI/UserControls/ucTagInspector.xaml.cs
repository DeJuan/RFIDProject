using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using ThingMagic;
using ThingMagic.URA2.BL;

namespace ThingMagic.URA2
{
    /// <summary>
    /// Interaction logic for ucTagInspectorxaml.xaml
    /// </summary>
    public partial class ucTagInspector : UserControl
    {
        Reader objReader;
        uint startAddress = 0;        
        int antenna = 0;
        Gen2.Bank selectMemBank;

        // Constant XPC_1 offset 
        const int XPC1OFFSET = 42;

        // Constant XPC_2 offset
        const int XPC2OFFSET = 44;

        string model = string.Empty;

        public ucTagInspector()
        {
            InitializeComponent();
        }

        public void LoadTagInspector(Reader reader,string readerModel)
        {
            objReader = reader;
            model = readerModel;
        }

        public void LoadTagInspector(Reader reader, uint address, Gen2.Bank selectedBank,TagReadRecord selectedTagRed,string readerModel)
        {
            objReader = reader;
            startAddress = address;
            model = readerModel;
            rbFirstTagIns.IsEnabled = true;
            rbSelectedTagIns.IsChecked = true;
            rbSelectedTagIns.IsEnabled = true;
            rbEPCAscii.IsEnabled = true;
            rbEPCBase36.IsEnabled = true;
            btnRead.Content = "Refresh";
            antenna = selectedTagRed.Antenna;
            selectMemBank = selectedBank;
            //txtEPCData.Text = selectedTagRed.EPC;
            string[] stringData = selectedTagRed.Data.Split(' ');
            txtEpc.Text = selectedTagRed.EPC;
            currentEPC = txtEpc.Text;
            txtData.Text = string.Join("", stringData);
            Window mainWindow = App.Current.MainWindow;
            ucTagResults tagResults = (ucTagResults)mainWindow.FindName("TagResults");
            switch (selectedBank)
            {
                case Gen2.Bank.EPC:
                    if (tagResults.txtSelectedCell.Text == "Data")
                    {
                        lblSelectFilter.Content = "Showing tag: EPC data at decimal address " + address.ToString() + "  = " + txtData.Text;
                    }
                    else
                    {
                        lblSelectFilter.Content = "Showing tag: EPC ID = " + selectedTagRed.EPC;
                    }
                    break;
                case Gen2.Bank.TID:
                    if (tagResults.txtSelectedCell.Text == "Data")
                    {
                        lblSelectFilter.Content = "Showing tag: TID data at decimal address " + address.ToString() + " = " + txtData.Text;
                    }
                    else
                    {
                        lblSelectFilter.Content = "Showing tag: EPC ID = " + selectedTagRed.EPC;
                    }                    
                    break;
                case Gen2.Bank.USER:
                    if (tagResults.txtSelectedCell.Text == "Data")
                    {
                        lblSelectFilter.Content = "Showing tag: User data at decimal address " + address.ToString() + " = " + txtData.Text;
                    }
                    else
                    {
                        lblSelectFilter.Content = "Showing tag: EPC ID = " + selectedTagRed.EPC;
                    }                    
                    break;
            }
            PopulateData();
        }

        string currentEPC = string.Empty;

        private void btnRead_Click(object sender, RoutedEventArgs e)
        {
            Mouse.SetCursor(Cursors.Wait);
            TagReadData[] tagReads = null;
            try
            {
                rbEPCAscii.IsEnabled = true;
                rbEPCBase36.IsEnabled = true;
                if (btnRead.Content.Equals("Read"))
                {
                    SimpleReadPlan srp = new SimpleReadPlan(((null != GetSelectedAntennaList()) ? (new int[] { GetSelectedAntennaList()[0] }) : null), TagProtocol.GEN2, null, 0);
                    objReader.ParamSet("/reader/read/plan", srp);
                    tagReads = objReader.Read(500);
                    if ((null != tagReads) && (tagReads.Length > 0))
                    {
                        currentEPC = tagReads[0].EpcString;
                        if ((bool)rbEPCAscii.IsChecked)
                        {
                            txtEpc.Text = Utilities.HexStringToAsciiString(tagReads[0].EpcString);
                        }
                        else if ((bool)rbEPCBase36.IsChecked)
                        {
                            txtEpc.Text = Utilities.ConvertHexToBase36(tagReads[0].EpcString);
                        }
                        else
                        {
                            txtEpc.Text = tagReads[0].EpcString;
                        }
                        lblSelectFilter.Content = "Showing tag: EPC ID = " + tagReads[0].EpcString;
                        if (tagReads.Length > 1)
                        {
                            lblTagInspectorError.Content = "Warning: More than one tag responded";
                            lblTagInspectorError.Visibility = System.Windows.Visibility.Visible;
                        }
                        else
                        {
                            lblTagInspectorError.Visibility = System.Windows.Visibility.Collapsed;
                        }
                    }
                    else
                    {
                        txtEpc.Text = "";
                        currentEPC = string.Empty;
                        MessageBox.Show("No tags found", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                    }
                }
                else
                {
                    rbSelectedTagIns.IsChecked = true;
                }

                PopulateData();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
            finally
            {
                Mouse.SetCursor(Cursors.Arrow);
            }
        }


        string userData = string.Empty;
        string unUsedEpcData = string.Empty;
        string unadditionalMemData = string.Empty;
        private void PopulateData()
        {
            try
            {
                Mouse.SetCursor(Cursors.Wait);

                // Create the object to read tag memory
                ReadTagMemory readTagMem = new ReadTagMemory(objReader, model);

                if ((bool)rbFirstTagIns.IsChecked)
                {
                    antenna = GetSelectedAntennaList()[0];
                }

                objReader.ParamSet("/reader/tagop/antenna", antenna);
                TagFilter searchSelect = null;

                if ((bool)rbSelectedTagIns.IsChecked)
                {
                    if (lblSelectFilter.Content.ToString().Contains("EPC ID"))
                    {
                        searchSelect = new TagData(currentEPC);
                    }
                    else
                    {
                        int dataLength = 0;
                        byte[] SearchSelectData = ByteFormat.FromHex(txtData.Text);
                        if (null != SearchSelectData)
                        {
                            dataLength = SearchSelectData.Length;
                        }

                        searchSelect = new Gen2.Select(false, selectMemBank, Convert.ToUInt16(startAddress * 16), Convert.ToUInt16(dataLength * 8), SearchSelectData);
                    }
                }
                else
                {
                    searchSelect = new TagData(currentEPC);
                }

                #region ReadReservedMemory
                
                //Read Reserved memory bank data
                ushort[] reservedData = null;

                txtKillPassword.Text = "";
                txtAcessPassword.Text = "";
                txtReservedMemUnusedValue.Text = "";
                // Hide additional memory textboxes
                lblAdditionalReservedMem.Visibility = System.Windows.Visibility.Collapsed;
                txtReservedMemUnusedValue.Visibility = System.Windows.Visibility.Collapsed;
                lblAdditionalReservedMemAdd.Visibility = System.Windows.Visibility.Collapsed;

                try
                {
                    readTagMem.ReadTagMemoryData(Gen2.Bank.RESERVED, searchSelect, ref reservedData);
                    // Parse the response to get access pwd, kill pwd and if additional memory exists
                    ParseReservedMemData(reservedData);
                }
                catch (Exception ex)
                {
                    // Hide additional memory textboxes
                    lblAdditionalReservedMem.Visibility = System.Windows.Visibility.Collapsed;
                    txtReservedMemUnusedValue.Visibility = System.Windows.Visibility.Collapsed;
                    lblAdditionalReservedMemAdd.Visibility = System.Windows.Visibility.Collapsed;
                    txtReservedMemUnusedValue.Text = "";
                    // If either of the memory is locked get in or else throw the exception.
                    if ((ex is FAULT_PROTOCOL_BIT_DECODING_FAILED_Exception) || (ex is FAULT_GEN2_PROTOCOL_MEMORY_LOCKED_Exception))
                    {
                        try
                        {
                            ReadReservedMemData(Gen2.Bank.RESERVED, searchSelect);
                        }
                        catch (Exception e)
                        {
                        txtKillPassword.Text = e.Message;
                        txtAcessPassword.Text = e.Message;
                        }
                    }
                    else if (ex is FAULT_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC_Exception)
                    {
                        txtKillPassword.Text = "Read Error";
                        txtAcessPassword.Text = "Read Error";
                    }
                    else
                    {
                        txtKillPassword.Text = ex.Message;
                        txtAcessPassword.Text = ex.Message;
                    }
                }
                #endregion ReadReservedMemory

                #region ReadEPCMemory
                //Read EPC bank data
                ushort[] epcData = null;

                txtPC.Text = "";
                txtCRC.Text = "";
                txtEPCData.Text = "";
                txtEPCValue.Text = "";
                txtEPCUnused.Text = "";
                txtEPCUnusedValue.Text = "";
                txtadditionalMemValue.Text = "";

                // Hide additional memory
                spUnused.Visibility = System.Windows.Visibility.Collapsed;
                spXPC.Visibility = System.Windows.Visibility.Collapsed;
                spXPC2.Visibility = System.Windows.Visibility.Collapsed;
                spAddMemory.Visibility = System.Windows.Visibility.Collapsed;

                try
                {
                    readTagMem.ReadTagMemoryData(Gen2.Bank.EPC, searchSelect, ref epcData);
                    ParseEPCMemData(epcData, searchSelect);
                }
                catch (Exception ex)
                {
                    txtEPCData.Text = ex.Message;
                    rbEPCAscii.IsEnabled = false;
                    rbEPCBase36.IsEnabled = false;
                }
                #endregion ReadEPCMemory

                #region ReadTIDMemory
                //Read TID bank data
                ushort[] tidData = null;

                txtClsID.Text = "";
                txtVendorID.Text = "";
                txtVendorValue.Text = "";
                txtModelID.Text = "";
                txtModeldIDValue.Text = "";
                txtUniqueIDValue.Text = "";

                try
                {
                    readTagMem.ReadTagMemoryData(Gen2.Bank.TID, searchSelect, ref tidData);
                    ParseTIDMemData(tidData);
                }
                catch (Exception ex)
                {
                    if (ex is FAULT_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC_Exception)
                    {
                        txtUniqueIDValue.Text = "Read Error";
                    }
                    else
                    {
                        txtUniqueIDValue.Text = ex.Message;
                    }
                }
                #endregion ReadTIDMemory

                #region ReadUserMemory
                //Read USER bank data
                ushort[] userMemData = null;

                txtUserDataValue.Text = "";
                txtUserMemData.Text = "";

                try
                {
                    readTagMem.ReadTagMemoryData(Gen2.Bank.USER, searchSelect, ref userMemData);
                    ParseUserMemData(userMemData);
                }
                catch (Exception ex)
                {
                    if (ex is FAULT_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC_Exception)
                    {
                        txtUserMemData.Text = "Read Error";
                    }
                    else
                    {
                        txtUserMemData.Text = ex.Message;
                    }
                }
                #endregion ReadUserMemory
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
            finally
            {
                Mouse.SetCursor(Cursors.Arrow);
            }
        }

        /// <summary>
        /// Parse reserved memory data and populate the reserved mem textboxes
        /// </summary>
        /// <param name="reservedData">accepts read reserved memory data</param>
        private void ParseReservedMemData(ushort[] reservedData)
        {            
            string reservedMemData = string.Empty;
            if (null != reservedData)
                reservedMemData = ByteFormat.ToHex(ByteConv.ConvertFromUshortArray(reservedData), "", " ");

            if (reservedMemData.Length > 0)
            {
                if (reservedMemData.Length > 11)
                {
                    // Extract kill pwd
                    txtKillPassword.Text = reservedMemData.Substring(0, 11).TrimStart(' ');
                    string tempData = reservedMemData.Substring(12).TrimStart(' ');
                    // Check if reserved memory has additional data
                    if (tempData.Length > 11)
                    {
                        // Extract access pwd
                        txtAcessPassword.Text = reservedMemData.Substring(12, 11).TrimStart(' ');
                        // Extract additional reserved memory
                        txtReservedMemUnusedValue.Text = reservedMemData.Substring(24).TrimStart(' ');

                        // Visible additional memory textboxes
                        lblAdditionalReservedMem.Visibility = System.Windows.Visibility.Visible;
                        txtReservedMemUnusedValue.Visibility = System.Windows.Visibility.Visible;
                        lblAdditionalReservedMemAdd.Visibility = System.Windows.Visibility.Visible;
                    }
                    else
                    {
                        // Extract access pwd
                        txtAcessPassword.Text = reservedMemData.Substring(12).TrimStart(' ');

                        // Hide additional memory textboxes
                        lblAdditionalReservedMem.Visibility = System.Windows.Visibility.Collapsed;
                        txtReservedMemUnusedValue.Visibility = System.Windows.Visibility.Collapsed;
                        lblAdditionalReservedMemAdd.Visibility = System.Windows.Visibility.Collapsed;
                    }
                }
                else
                {
                    txtKillPassword.Text = reservedMemData.Substring(0, reservedData.Length).TrimStart(' ');
                }
            }
        }

        /// <summary>
        /// Parse epc memory data and populate the epc mem textboxes
        /// </summary>
        /// <param name="epcData">accepts read epc memory data</param>
        private void ParseEPCMemData(ushort[] epcData, TagFilter filter)
        {
            unUsedEpcData = string.Empty;
            byte[] epcBankData = null;
            if (null != epcData)
            {
                epcBankData = ByteConv.ConvertFromUshortArray(epcData);
                int readOffset = 0;
                byte[] epc, crc, pc, unusedEpc = null, additionalMemData = null;
                int lengthCounter = 2;
                crc = SubArray(epcBankData, ref readOffset, lengthCounter);
                pc = SubArray(epcBankData, ref readOffset, lengthCounter);
                lengthCounter += 2;

                // Extract the epc length from pc word
                int epclength = Convert.ToInt32(((pc[0] & 0xf8) >> 3)) * 2;

                epc = SubArray(epcBankData, ref readOffset, epclength);

                List<byte> xpc = new List<byte>();

                /* Add support for XPC bits
                    * XPC_W1 is present, when the 6th most significant bit of PC word is set
                    */
                if ((pc[0] & 0x02) == 0x02)
                {
                    /* When this bit is set, the XPC_W1 word will follow the PC word
                        * Our TMR_Gen2_TagData::pc has enough space, so copying to the same.
                        */
                    try
                    {
                        ushort[] xpcW1 = (ushort[])objReader.ExecuteTagOp(new Gen2.ReadData(Gen2.Bank.EPC, 0x21, 1), filter);
                        spXPC.Visibility = System.Windows.Visibility.Visible;
                        lblXPC1MemAddress.Content = "33";
                        xpc.AddRange(ByteConv.ConvertFromUshortArray(xpcW1));
                        lengthCounter += 2;
                        txtXPC1.Text = ByteFormat.ToHex(ByteConv.ConvertFromUshortArray(xpcW1), "", " ");
                        lblXPC1.Content = "XPC";
                    }
                    catch (Exception ex)
                    {
                        spXPC.Visibility = System.Windows.Visibility.Visible;
                        txtXPC1.Text = ex.Message;
                        lblXPC1.Content = "XPC";
                    }
                        /* If the most siginificant bit of XPC_W1 is set, then there exists
                        * XPC_W2. A total of 6  (PC + XPC_W1 + XPC_W2 bytes)
                        */
                    if ((xpc[0] & 0x80) == 0x80)
                    {
                        try
                        {
                            ushort[] xpcW2 = (ushort[])objReader.ExecuteTagOp(new Gen2.ReadData(Gen2.Bank.EPC, 0x22, 1), filter);
                            spXPC2.Visibility = System.Windows.Visibility.Visible;
                            lblXPC2MemAddress.Content = "34";
                            lengthCounter += 2;
                            txtXPC2.Text = ByteFormat.ToHex(ByteConv.ConvertFromUshortArray(xpcW2), "", " ");
                            // Change the name of XPC to XPC1
                            lblXPC1.Content = "XPC1";
                        }
                        catch (Exception ex)
                        {
                            spXPC2.Visibility = System.Windows.Visibility.Visible;
                            txtXPC2.Text = ex.Message;
                            // Change the name of XPC to XPC1
                            lblXPC1.Content = "XPC1";
                        }
                    }
                }
                // Read extended epc memory
                if (epcBankData.Length > (lengthCounter + epclength))
                {
                    lblExtdEPCMemAddress.Content = Convert.ToString(readOffset/2);
                    bool isExtendedEPCMemover = true;
                    uint startExtdEPCMemAddress = (uint)readOffset/2;
                    List<ushort> data = new List<ushort>();
                    try
                    {
                        while (isExtendedEPCMemover)
                        {
                            // Make sure reading of memory word by word doesn't override XPC1 data
                            if (startExtdEPCMemAddress < 33)
                            {
                                data.AddRange((ushort[])objReader.ExecuteTagOp(new Gen2.ReadData(Gen2.Bank.EPC, startExtdEPCMemAddress, 1), filter));
                                startExtdEPCMemAddress += 1;
                            }
                            else
                            {
                                // Read of memory should not exceed XPC bytes
                                isExtendedEPCMemover = false;
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        // If more then once the below exceptions are recieved then come out of the loop.
                        if (ex is FAULT_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC_Exception || (-1 != ex.Message.IndexOf("Non-specific reader error")) || (-1 != ex.Message.IndexOf("General Tag Error")) || (-1 != ex.Message.IndexOf("Tag data access failed")))
                        {
                            if (data.Count > 0)
                            {
                                // Just skip the exception and move on. So as not to lose the already read data.
                                isExtendedEPCMemover = false;
                            }
                        }
                    }

                    if (data.Count > 0)
                    {
                        unusedEpc = ByteConv.ConvertFromUshortArray(data.ToArray());
                    }
                }

                // Read additional memory
                if (epcBankData.Length > (lengthCounter + epclength))
                {
                    lblAddMemAddress.Content = "35";
                    bool isAdditionalMemover = true;
                    uint startAdditionalMemAddress = 0x23;
                    List<ushort> dataAdditionalMem = new List<ushort>();
                    try
                    {
                        while (isAdditionalMemover)
                        {
                            dataAdditionalMem.AddRange((ushort[])objReader.ExecuteTagOp(new Gen2.ReadData(Gen2.Bank.EPC, startAdditionalMemAddress, 1), filter));
                            startAdditionalMemAddress += 1;
                        }
                    }
                    catch (Exception ex)
                    {
                        // If more then once the below exceptions are recieved then come out of the loop.
                        if (ex is FAULT_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC_Exception || (-1 != ex.Message.IndexOf("Non-specific reader error")) || (-1 != ex.Message.IndexOf("General Tag Error")) || (-1 != ex.Message.IndexOf("Tag data access failed")))
                        {
                            if (dataAdditionalMem.Count > 0)
                            {
                                // Just skip the exception and move on. So as not to lose the already read data.
                                isAdditionalMemover = false;
                            }
                        }
                    }
                    if (dataAdditionalMem.Count > 0)
                    {
                        additionalMemData = ByteConv.ConvertFromUshortArray(dataAdditionalMem.ToArray());
                    }                    
                }

                if (txtXPC1.Text != "")
                {
                    spXPC.Visibility = System.Windows.Visibility.Visible;
                }
                if (txtXPC2.Text != "")
                {
                    spXPC2.Visibility = System.Windows.Visibility.Visible;
                }

                txtCRC.Text = ByteFormat.ToHex(crc, "", " ");
                txtPC.Text = ByteFormat.ToHex(pc, "", " ");
                if (epc.Length == epclength)
                {
                    txtEPCData.Text = ByteFormat.ToHex(epc, "", " ");
                }
                else
                {
                    txtEPCData.Text = currentEPC;
                }
                if (null != unusedEpc)
                {
                    txtEPCUnused.Text = ByteFormat.ToHex(unusedEpc, "", " ");
                    unUsedEpcData = ByteFormat.ToHex(unusedEpc, "", "");
                    // Visible additional memory
                    spUnused.Visibility = System.Windows.Visibility.Visible;
                }

                if (null != additionalMemData)
                {
                    txtAdditionalMem.Text = ByteFormat.ToHex(additionalMemData, "", " ");
                    unadditionalMemData = ByteFormat.ToHex(additionalMemData, "", "");
                    // Visible additional memory
                    spAddMemory.Visibility = System.Windows.Visibility.Visible;
                }

                if ((bool)rbEPCAscii.IsChecked)
                {
                    txtEPCValue.Text = Utilities.HexStringToAsciiString(currentEPC);
                    txtEPCUnusedValue.Text = Utilities.HexStringToAsciiString(unUsedEpcData);
                    txtadditionalMemValue.Text = Utilities.HexStringToAsciiString(unadditionalMemData);
                }
                else if ((bool)rbEPCBase36.IsChecked)
                {
                    txtEPCValue.Text = Utilities.ConvertHexToBase36(currentEPC);
                    txtEPCUnusedValue.Text = Utilities.ConvertHexToBase36(unUsedEpcData);
                    txtadditionalMemValue.Text = Utilities.ConvertHexToBase36(unadditionalMemData);
                }

                #region 0 length read

                //if (model.Equals("M5e") || model.Equals("M5e EU") || model.Equals("M5e Compact"))
                //{
                //    ReadData(Gen2.Bank.EPC, searchSelect, out epcData);
                //}
                //else
                //{
                //    op = new Gen2.ReadData(Gen2.Bank.EPC, 0, 0);
                //    epcData = (ushort[])objReader.ExecuteTagOp(op, searchSelect);
                //}

                //if(null!= epcData)
                //    epcBankData = ByteFormat.ToHex(ByteConv.ConvertFromUshortArray(epcData), "", " ");

                //if (epcBankData.Length > 0)
                //{
                //    int epcLen = txtEpc.Text.Length;
                //    txtCRC.Text = epcBankData.Substring(0, 5).TrimStart(' ');
                //    txtPC.Text = epcBankData.Substring(6, 5).TrimStart(' ');
                //    int epcstringLength = epcLen+((epcLen/2)-1);
                //    txtEPCData.Text = epcBankData.Substring(11, epcstringLength).TrimStart(' ');

                //    //string epcDataString = ByteFormat.ToHex(ByteConv.ConvertFromUshortArray(epcData), "", "");
                //    txtEPCUnused.Text = epcBankData.Substring(11 + epcstringLength).TrimStart(' '); //String.Join(" ", (epcDataString.Substring(8 + epcLen)).ToArray());
                #endregion

            }
        }

        /// <summary>
        /// Parse tid memory data and populate the tid mem textboxes
        /// </summary>
        /// <param name="tidData">accepts read tid memory data</param>
        private void ParseTIDMemData(ushort[] tidData)
        {
            string tidBankData = string.Empty;
            if (null != tidData)
                tidBankData = ByteFormat.ToHex(ByteConv.ConvertFromUshortArray(tidData), "", " ");

            if (tidBankData.Length > 0)
            {
                txtClsID.Text = tidBankData.Substring(0, 2).TrimStart(' ');
                txtVendorID.Text = tidBankData.Substring(3, 4).Replace(" ", string.Empty);
                string tagModel = string.Empty;
                txtVendorValue.Text = GetVendor(txtVendorID.Text, tidBankData.Substring(7, 4).Replace(" ", string.Empty), out tagModel);
                txtModelID.Text = tidBankData.Substring(7, 4).Replace(" ", string.Empty);
                txtModeldIDValue.Text = tagModel;
                if (tidBankData.Length >= 12)
                    txtUniqueIDValue.Text = tidBankData.Substring(12).TrimStart(' ');
            }
        }

        /// <summary>
        /// Parse user memory data and populate the user mem textboxes
        /// </summary>
        /// <param name="userData">accepts read user memory data</param>
        private void ParseUserMemData(ushort[] userData)
        {
            string userMemData = string.Empty;
            if (null != userData)
            {
                userMemData = ByteFormat.ToHex(ByteConv.ConvertFromUshortArray(userData), "", "");
                txtUserDataValue.Text = ReplaceSpecialCharInAsciiData(Utilities.HexStringToAsciiString(userMemData).ToCharArray());
                txtUserMemData.Text = ByteFormat.ToHex(ByteConv.ConvertFromUshortArray(userData), "", " ");
            }
        }

        #region SubArray
        /// <summary>
        /// Extract subarray
        /// </summary>
        /// <param name="src">Source array</param>
        /// <param name="offset">Start index in source array</param>
        /// <param name="length">Number of source elements to extract</param>
        /// <returns>New array containing specified slice of source array</returns>
        private static byte[] SubArray(byte[] src, int offset, int length)
        {
            return SubArray(src, ref offset, length);
        }

        /// <summary>
        /// Extract subarray, automatically incrementing source offset
        /// </summary>
        /// <param name="src">Source array</param>
        /// <param name="offset">Start index in source array.  Automatically increments value by copied length.</param>
        /// <param name="length">Number of source elements to extract</param>
        /// <returns>New array containing specified slice of source array</returns>
        private static byte[] SubArray(byte[] src, ref int offset, int length)
        {
            byte[] dst = new byte[length];
            try
            {                
                Array.Copy(src, offset, dst, 0, length);
                offset += length;                
            }
            catch
            { 
            }
            return dst;
        }

        #endregion

        private void ReadReservedMemData(Gen2.Bank bank, TagFilter filter)
        {            
            ushort [] reservedData;
            TagOp op;
            try
            {
                try
                {
                    // Read kill password
                    op = new Gen2.ReadData(Gen2.Bank.RESERVED, 0, 2);
                   reservedData = (ushort[])objReader.ExecuteTagOp(op, filter);
                   if (null != reservedData)
                   {
                       txtKillPassword.Text = ByteFormat.ToHex(ByteConv.ConvertFromUshortArray(reservedData), "", " ");
                   }
                   else
                   {
                       txtKillPassword.Text = "";
                   }
                }
                catch (Exception ex)
                {
                    if (ex is FAULT_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC_Exception)
                    {
                        txtKillPassword.Text = "Read Error";                        
                    }
                    else
                    {
                        txtKillPassword.Text = ex.Message;
                    }
                }

                try
                {
                    // Read access password
                    reservedData = null;
                    op = new Gen2.ReadData(Gen2.Bank.RESERVED, 2, 2);
                    reservedData = (ushort[])objReader.ExecuteTagOp(op, filter);
                    if (null != reservedData)
                    {
                        txtAcessPassword.Text = ByteFormat.ToHex(ByteConv.ConvertFromUshortArray(reservedData), "", " ");
                    }
                    else
                    {
                        txtAcessPassword.Text = "";
                    }
                }
                catch (Exception ex)
                {
                    if (ex is FAULT_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC_Exception)
                    {
                        txtAcessPassword.Text = "Read Error";
                    }
                    else
                    {                     
                        txtAcessPassword.Text = ex.Message;
                    }
                }

                // Read additional memory password
                try
                {
                    reservedData = null;
                    if (model.Equals("M5e") || model.Equals("M5e EU") || model.Equals("M5e Compact") || model.Equals("Astra"))
                    {                        
                        ReadAdditionalReservedMemDataM5eVariants(Gen2.Bank.RESERVED, 4, filter, out reservedData);
                    }
                    else
                    {
                        op = new Gen2.ReadData(Gen2.Bank.RESERVED, 4, 0);
                        reservedData = (ushort[])objReader.ExecuteTagOp(op, filter);
                    }

                    if (null != reservedData)
                    {
                        txtReservedMemUnusedValue.Text = ByteFormat.ToHex(ByteConv.ConvertFromUshortArray(reservedData), "", " ");
                        // Visible additional memory textboxes
                        lblAdditionalReservedMem.Visibility = System.Windows.Visibility.Visible;
                        txtReservedMemUnusedValue.Visibility = System.Windows.Visibility.Visible;
                        lblAdditionalReservedMemAdd.Visibility = System.Windows.Visibility.Visible;
                    }
                    else
                    {
                        txtReservedMemUnusedValue.Text = "";
                    }
                }
                catch
                { 
                    // catch the exception and move on. Only some tags has aditional memory 
                    txtReservedMemUnusedValue.Text = "";
                    // Hide additional memory textboxes
                    lblAdditionalReservedMem.Visibility = System.Windows.Visibility.Collapsed;
                    txtReservedMemUnusedValue.Visibility = System.Windows.Visibility.Collapsed;
                    lblAdditionalReservedMemAdd.Visibility = System.Windows.Visibility.Collapsed;
                }
            }
            catch (Exception)
            {
                throw;
            }            
        }
     
        /// <summary>
        /// Read additional reserved memory for m5e variants
        /// </summary>
        /// <param name="bank"></param>
        /// <param name="startAddress"></param>
        /// <param name="filter"></param>
        /// <param name="data"></param>
        private void ReadAdditionalReservedMemDataM5eVariants(Gen2.Bank bank, uint startAddress, TagFilter filter, out ushort[] data)
        {
            data = null;
            int words = 1;
            TagOp op;
            while (true)
            {
                try
                {
                    op = new Gen2.ReadData(bank, startAddress, Convert.ToByte(words));
                    data = (ushort[])objReader.ExecuteTagOp(op, filter);
                    words++;
                }
                catch (Exception)
                {
                    throw;
                }
            }
        }

        /// <summary>
        /// Get selected antenna list
        /// </summary>
        /// <returns></returns>
        private List<int> GetSelectedAntennaList()
        {
            Window mainWindow = App.Current.MainWindow;
            CheckBox Ant1CheckBox = (CheckBox)mainWindow.FindName("Ant1CheckBox");
            CheckBox Ant2CheckBox = (CheckBox)mainWindow.FindName("Ant2CheckBox");
            CheckBox Ant3CheckBox = (CheckBox)mainWindow.FindName("Ant3CheckBox");
            CheckBox Ant4CheckBox = (CheckBox)mainWindow.FindName("Ant4CheckBox");
            CheckBox[] antennaBoxes = { Ant1CheckBox, Ant2CheckBox, Ant3CheckBox, Ant4CheckBox };
            List<int> ant = new List<int>();

            for (int antIdx = 0; antIdx < antennaBoxes.Length; antIdx++)
            {
                CheckBox antBox = antennaBoxes[antIdx];

                if ((bool)antBox.IsChecked)
                {
                    int antNum = antIdx + 1;
                    ant.Add(antNum);
                }
            }
            if (ant.Count > 0)
                return ant;
            else
                return null;
        }

        private string GetVendor(string vendorId,string tagId, out string model)
        {
            string vendor = string.Empty;
            model = string.Empty;

            switch (vendorId)
            {
                case Utilities.Impinj: vendor = "Impinj";
                    switch (tagId) 
                    {
                        case Utilities.ImpinjOld: model = "Old"; break;
                        case Utilities.ImpinjAnchor: model = "Anchor"; break;
                        case Utilities.ImpinjMonaco: model = "Monaco"; break;
                        case Utilities.ImpinjMonza: model = "Monza"; break;
                        case Utilities.ImpinjMonza2: model = "Monza2"; break;
                        case Utilities.ImpinjMonza3: model = "Monza3"; break;
                        case Utilities.ImpinjMonza4D: model = "Monza 4D"; break;
                        case Utilities.ImpinjMonza4E: model = "Monza 4E"; break;
                        case Utilities.ImpinjMonza4U: model = "Monza 4U"; break;
                        default: break;
                    }
                        break;
                case Utilities.ImpinjXTID: vendor = "Impinj";
                    switch (tagId)
                    {
                        case Utilities.ImpinjMonza4D: model = "Monza 4D"; break;
                        case Utilities.ImpinjMonza4E: model = "Monza 4E"; break;
                        case Utilities.ImpinjMonza4U: model = "Monza 4U"; break;
                        case Utilities.ImpinjMonza4QT: model = "Monza 4QT"; break;
                        case Utilities.ImpinjMonza5:model = "Monza 5"; break;
                        default: break;
                    }
                        break;
                case Utilities.TI: vendor = "TI"; break;
                case Utilities.Alien: vendor = "Alien";
                    switch (tagId)
                    {
                        case Utilities.AlienHiggs2: model = "Higgs 2"; break;
                        case Utilities.AlienHiggs3: model = "Higgs 3"; break;
                        case Utilities.AlienHiggs4: model = "Higgs 4"; break;
                        default: break;
                    }
                    break;
                case Utilities.Phillips: vendor = "Phillips"; break;
                case Utilities.NXP: vendor = "NXP"; 
                     switch (tagId)
                    {
                        case Utilities.NXPG2IL: model = "G2iL"; break;
                        case Utilities.NXPG2ILPLUS: model = "G2iL +"; break;
                        case Utilities.NXPG2IM: model = "G2iM"; break;
                        case Utilities.NXPG2IMPLUS: model = "G2IM +"; break;
                        case Utilities.NXPOld: model = "Old"; break;
                        case Utilities.NXPXL: model = "XL"; break;
                        case Utilities.NXPXM: model = "XM"; break;
                        case Utilities.NXPI2C: model = "I2C"; break;
                        default: break;
                    }
                    break;
                case Utilities.NXPXTID: vendor = "NXPXTID";
                    switch (tagId)
                    {
                        case Utilities.NXPUCODE7: model = "UCODE7"; break;
                        case Utilities.NXPI2C4011: model = "I2C 4011"; break;
                        case Utilities.NXPI2C4021: model = "I2C 4011"; break;
                        default: break;
                    }
                    break;
                case Utilities.STMicro: vendor = "STMicro"; break;
                case Utilities.PowerID: vendor = "Em Micro";
                    switch (tagId)
                    {
                        case Utilities.PowerIDEmMicroWithTamper: model = "With tamper alaram"; break;
                        case Utilities.PowerIDEmMicroWithOutTamper: model = "With out tamper alaram"; break;
                        default: break;
                    }
                    break;
                case Utilities.EM4325: vendor = "EM4325"; break;
                case Utilities.Hitchi: vendor = "Hitchi"; break;
                case Utilities.Quanray: vendor = "Quanray"; break;
                case Utilities.IDS: vendor = "IDS";
                    switch (tagId)
                    {
                        case Utilities.IDSSL900A: model = "SL900A"; break;
                        default: break;
                    }
                    break;
                case Utilities.Tego: vendor = "Tego";
                    switch (tagId)
                    {
                        case Utilities.Tego4K: model = "Tego4K"; break;
                        case Utilities.Tego8K: model = "Tego8K"; break;
                        case Utilities.Tego24K: model = "Tego24K"; break;
                        default: break;
                    }
                    break;
            }
            return vendor;
        }

        private void rbEPCAscii_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                if (!(txtEpc.Text.Equals("")))
                {
                    txtEPCValue.Text = Utilities.HexStringToAsciiString(currentEPC);
                    txtEPCUnusedValue.Text = Utilities.HexStringToAsciiString(unUsedEpcData);
                    txtadditionalMemValue.Text = Utilities.HexStringToAsciiString(unadditionalMemData);
                }
            }
        }

        private void rbEPCBase36_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                if (!(txtEpc.Text.Equals("")))
                {
                    txtEPCValue.Text = Utilities.ConvertHexToBase36(currentEPC);
                    txtEPCUnusedValue.Text = Utilities.ConvertHexToBase36(unUsedEpcData);
                    txtadditionalMemValue.Text = Utilities.ConvertHexToBase36(unadditionalMemData);
                }
            }
        }

        private void rbUserAscii_Checked(object sender, RoutedEventArgs e)
        {
            try
            {
                if (null != objReader)
                {
                    if (!(txtUserDataValue.Text.Equals(string.Empty)))
                    {
                        txtUserDataValue.Text = Utilities.HexStringToAsciiString(userData);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void rbUserBase36_Checked(object sender, RoutedEventArgs e)
        {
            try
            {
                if (null != objReader)
                {
                    if (!(txtUserDataValue.Text.Equals(string.Empty)))
                    {
                        txtUserDataValue.Text = Utilities.ConvertHexToBase36(userData);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void txtUserMemData_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            //Don't accept any characters in the textbox
            e.Handled = true;
        }

        private void CommandBinding_CanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = false;
            e.Handled = true;
        }

        private void txtEPCData_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if ((e.Key == Key.Back) || (e.Key == Key.Delete))
            {
                e.Handled = true;
            }
        }

        private void rbFirstTagIns_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                ResetTagInspectorTab();
            }
        }

        /// <summary>
        /// Replace all the special characters in the ascii string with .[DOT]
        /// </summary>
        /// <param name="asciiData"></param>
        /// <returns></returns>
        private string ReplaceSpecialCharInAsciiData(char[] asciiData)
        {
            string replacedData = string.Empty;
            foreach (char character in asciiData)
            {
                if (char.IsControl(character))
                {
                    replacedData += ".";
                }
                else
                {
                    replacedData += character;
                }
            }
            return replacedData;
        }

        /// <summary>
        /// Reset taginspector tab to default values
        /// </summary>
        public void ResetTagInspectorTab()
        {
            if(null != objReader)
            {
                //set the default values for TagInspector tab
                lblSelectFilter.Content = "Showing tag:";
                txtAcessPassword.Text = "";
                txtKillPassword.Text = "";
                txtEpc.Text = "";
                txtCRC.Text = "";
                txtPC.Text = "";
                lblTagInspectorError.Content = "";
                lblTagInspectorError.Visibility = System.Windows.Visibility.Collapsed;
                txtEPCData.Text = "";
                txtEPCValue.Text = "";

                // Hide XPC bytes textbox
                spXPC.Visibility = System.Windows.Visibility.Collapsed;
                spXPC2.Visibility = System.Windows.Visibility.Collapsed;
                lblXPC1.Content = "XPC";
                txtXPC1.Text = "";
                txtXPC2.Text = "";

                //Hide additional memory
                spAddMemory.Visibility = System.Windows.Visibility.Collapsed;
                txtAdditionalMem.Text = "";
                txtadditionalMemValue.Text = "";

                // Hide reserved additional memory textboxes
                lblAdditionalReservedMemAdd.Visibility = System.Windows.Visibility.Collapsed;
                lblAdditionalReservedMem.Visibility = System.Windows.Visibility.Collapsed;
                txtReservedMemUnusedValue.Visibility = System.Windows.Visibility.Collapsed;
                txtReservedMemUnusedValue.Text = "";

                // Hide epc additional memory textboxes
                spUnused.Visibility = System.Windows.Visibility.Collapsed;
                txtEPCUnused.Text = "";
                txtEPCUnusedValue.Text = "";

                txtClsID.Text = "";
                txtVendorID.Text = "";
                txtVendorValue.Text = "";
                txtModelID.Text = "";
                txtModeldIDValue.Text = "";
                txtUniqueIDValue.Text = "";

                txtUserMemData.Text = "";
                txtUserDataValue.Text = "";
                btnRead.Content = "Read";
                rbSelectedTagIns.IsEnabled = false;
                rbFirstTagIns.IsChecked = true;
                rbFirstTagIns.IsEnabled = true;
                rbEPCAscii.IsChecked = true;
                rbEPCAscii.IsEnabled = true;
                rbEPCBase36.IsEnabled = true;
            }
        }
    }
}

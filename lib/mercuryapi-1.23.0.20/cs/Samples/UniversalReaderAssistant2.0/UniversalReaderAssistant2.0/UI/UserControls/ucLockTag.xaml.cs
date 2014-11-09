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
using System.Collections.ObjectModel;
using System.Globalization;
using Microsoft.Win32;
using System.IO;

namespace ThingMagic.URA2
{
    /// <summary>
    /// Interaction logic for ucLockTag.xaml
    /// </summary>
    public partial class ucLockTag : UserControl
    {
        Reader objReader;
        uint accessPwddStartAddress = 0;
        int antenna = 0;
        int dataLength = 0;
        string model = string.Empty;
        Gen2.Bank selectMemBank;
        TagReadRecord selectedTagReadRecord;
        ObservableCollection<string> selectedLockActions = new ObservableCollection<string>();
        public ObservableCollection<string> statusLockActions = new ObservableCollection<string>();

        public ucLockTag()
        {
            InitializeComponent();
            lstviewSelectedLockActions.ItemsSource = selectedLockActions;
            lstviewstatusSelectedLockActions.ItemsSource = statusLockActions;
        }

        /// <summary>
        /// Load reserved memory to get access password of the tag
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="readerModel"></param>
        public void LoadReservedMemory(Reader reader, string readerModel)
        {
            objReader = reader;
            model = readerModel;
        }

        /// <summary>
        /// Load reserved memory to get access password of the tag
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="address"></param>
        /// <param name="selectedBank"></param>
        /// <param name="selectedTagRed"></param>
        /// <param name="readerModel"></param>
        public void LoadReservedMemory(Reader reader, uint address, Gen2.Bank selectedBank, TagReadRecord selectedTagRed, string readerModel)
        {
            objReader = reader;
            accessPwddStartAddress = address;
            model = readerModel;
            spLockTag.IsEnabled = true;
            rbFirstTagLockTagTb.IsEnabled = true;
            rbSelectedTagLockTagTb.IsChecked = true;
            rbSelectedTagLockTagTb.IsEnabled = true;

            btnRead.Content = "Refresh";
            selectedTagReadRecord = selectedTagRed;
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
            PopulateUserData();
        }

        /// <summary>
        /// Populate access password of the tag
        /// </summary>
        private void PopulateUserData()
        {
            try
            {
                // Clear the previous error
                ResetLockActionCheckBoxes();
                grpbxLockTgAccessPwd.IsEnabled = true;

                Mouse.SetCursor(Cursors.Wait);
                if ((bool)rbFirstTagLockTagTb.IsChecked)
                {
                    antenna = GetSelectedAntennaList()[0];
                }

                objReader.ParamSet("/reader/tagop/antenna", antenna);
                TagFilter searchSelect = null;

                if ((bool)rbSelectedTagLockTagTb.IsChecked)
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

                        searchSelect = new Gen2.Select(false, selectMemBank, Convert.ToUInt16(accessPwddStartAddress * 16), Convert.ToUInt16(dataLength * 8), SearchSelectData);
                    }
                }
                else
                {
                    searchSelect = new TagData(currentEPC);
                }

                //Read Reserved memory bank data
                TagOp op;
                ushort[] reservedData = null;
                txtbxAccesspaasword.Text = "";
                try
                {
                    string reservedBankData = string.Empty;
                    if (model.Equals("M5e") || model.Equals("M5e EU") || model.Equals("M5e Compact") || model.Equals("Astra"))
                    {
                        ReadData(Gen2.Bank.RESERVED, searchSelect, out reservedData);
                    }
                    else
                    {
                        op = new Gen2.ReadData(Gen2.Bank.RESERVED, 2, 0);
                        reservedData = (ushort[])objReader.ExecuteTagOp(op, searchSelect);
                    }

                    if (null != reservedData)
                        reservedBankData = ByteFormat.ToHex(ByteConv.ConvertFromUshortArray(reservedData), "", " ");
                    
                    txtbxAccesspaasword.Text = reservedBankData.Trim(' ');
                    grpbxLockTgApplyLocks.IsEnabled = true;
                    grpbxLockTgLockActns.IsEnabled = true;
                    btnWriteAccessPwd.IsEnabled = true;
                }
                catch (Exception ex)
                {
                    // Show the error in access password section if failed
                    txtblkErrorAccessPassword.Visibility = System.Windows.Visibility.Visible;
                    grpbxLockTgApplyLocks.IsEnabled = false;
                    grpbxLockTgLockActns.IsEnabled = false;
                    btnWriteAccessPwd.IsEnabled = false;
                    if (ex is FAULT_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC_Exception)
                    {
                        txtblkErrorAccessPassword.Text = "Error: Read Error";
                    }
                    else if (ex is FAULT_GEN2_PROTOCOL_MEMORY_LOCKED_Exception)
                    {
                        txtblkErrorAccessPassword.Text = "Error: Could not read access password. Access Password may be locked. You must enter the correct Access Password to proceed with locking"; 
                    }
                    else
                    {
                        txtblkErrorAccessPassword.Text = "Error: "+ex.Message;
                    }
                }
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
        /// Read reserved memory of the tag
        /// </summary>
        /// <param name="bank"></param>
        /// <param name="filter"></param>
        /// <param name="data"></param>
        private void ReadData(Gen2.Bank bank, TagFilter filter, out ushort[] data)
        {
            data = null;
            int words = 1;
            TagOp op;
            while (true)
            {
                try
                {
                    op = new Gen2.ReadData(bank, 2, Convert.ToByte(words));
                    data = (ushort[])objReader.ExecuteTagOp(op, filter);
                    words++;
                }
                catch (Exception ex)
                {
                    if (ex is FAULT_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC_Exception || (-1 != ex.Message.IndexOf("Non-specific reader error")) || (-1 != ex.Message.IndexOf("Tag data access failed")))
                    {
                        if (null != data)
                        {
                            // Just skip the exception and move on. So as not to lose the already read data.
                            break;
                        }
                        else
                        {
                            // throw the exception if the data received is null for the first iteration itself
                            throw;
                        }
                    }
                    else
                    {
                        throw ex;
                    }
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

        string currentEPC = string.Empty;

        /// <summary>
        /// Perform read to get the access password
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnRead_Click(object sender, RoutedEventArgs e)
        {
            Mouse.SetCursor(Cursors.Wait);
            TagReadData[] tagReads = null;
            try
            {
                if (btnRead.Content.Equals("Read"))
                {
                    SimpleReadPlan srp = new SimpleReadPlan(((null != GetSelectedAntennaList()) ? (new int[] { GetSelectedAntennaList()[0] }) : null), TagProtocol.GEN2, null, 0);
                    objReader.ParamSet("/reader/read/plan", srp);
                    tagReads = objReader.Read(500);
                    if ((null != tagReads) && (tagReads.Length > 0))
                    {
                        currentEPC = tagReads[0].EpcString;
                        txtEpc.Text = tagReads[0].EpcString;
                        lblSelectFilter.Content = "Showing tag: EPC ID = " + tagReads[0].EpcString;
                        if (tagReads.Length > 1)
                        {
                            lblLockTagError.Content = "Warning: More than one tag responded";
                            lblLockTagError.Visibility = System.Windows.Visibility.Visible;
                        }
                        else
                        {
                            lblLockTagError.Visibility = System.Windows.Visibility.Collapsed;
                        }
                    }
                    else
                    {
                        txtEpc.Text = "";
                        currentEPC = string.Empty;
                        MessageBox.Show("No tags found", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                        return;
                    }
                }
                else
                {
                    rbSelectedTagLockTagTb.IsChecked = true;
                }
                //Display user bank data
                PopulateUserData();
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
        /// Based on the checked state add or remove the Gen2.LockAction.USER_UNLOCK item from lock actions selected
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkbxUnLockUserMem_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                if ((bool)chkbxUnLockUserMem.IsChecked)
                {
                    if ((bool)chkbxPermaLockUserMem.IsChecked)
                    {
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.USER_PERMAUNLOCK.ToString());
                    }
                    else
                    {
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.USER_UNLOCK.ToString());
                    }

                    // uncheck the rest of the lock action checkboxes of the user memory
                    chkbxLockUserMem.IsChecked = false;
                    chkbxLockUserMem.IsEnabled = false;
                }
                else
                {
                    if ((bool)chkbxPermaLockUserMem.IsChecked)
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.USER_PERMAUNLOCK.ToString());
                    }
                    else
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.USER_UNLOCK.ToString());
                    }
                    chkbxLockUserMem.IsChecked = false;
                    chkbxLockUserMem.IsEnabled = true;
                }
            }
        }

        /// <summary>
        /// Based on the checked state add or remove the Gen2.LockAction.USER_LOCK item from lock actions selected
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkbxLockUserMem_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                if ((bool)chkbxLockUserMem.IsChecked)
                {
                    if ((bool)chkbxPermaLockUserMem.IsChecked)
                    {
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.USER_PERMALOCK.ToString());
                    }
                    else
                    {
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.USER_LOCK.ToString());
                    }                                       

                    // uncheck the rest of the lock action checkboxes of the user memory
                    chkbxUnLockUserMem.IsChecked = false;
                    chkbxUnLockUserMem.IsEnabled = false;                    
                }
                else
                {
                    if ((bool)chkbxPermaLockUserMem.IsChecked)
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.USER_PERMALOCK.ToString());
                    }
                    else
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.USER_LOCK.ToString());
                    }
                    chkbxUnLockUserMem.IsChecked = false;
                    chkbxUnLockUserMem.IsEnabled = true;
                }
            }
        }

        /// <summary>
        /// Based on the checked state add or remove the Gen2.LockAction.USER_PERMALOCK item from lock actions selected
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkbxPermaLockUserMem_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                if ((bool)chkbxPermaLockUserMem.IsChecked)
                {
                    if ((bool)chkbxLockUserMem.IsChecked)
                    {
                        if (selectedLockActions.Contains("Gen2.LockAction." + Gen2.LockAction.USER_LOCK.ToString()))
                        {
                            selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.USER_LOCK.ToString());
                        }
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.USER_PERMALOCK.ToString());
                    }
                    if ((bool)chkbxUnLockUserMem.IsChecked)
                    {
                        if (selectedLockActions.Contains("Gen2.LockAction." + Gen2.LockAction.USER_UNLOCK.ToString()))
                        {
                            selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.USER_UNLOCK.ToString());
                        }
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.USER_PERMAUNLOCK.ToString());
                    }
                }
                else
                {
                    if ((bool)chkbxLockUserMem.IsChecked)
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.USER_PERMALOCK.ToString());
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.USER_LOCK.ToString());
                    }
                    if ((bool)chkbxUnLockUserMem.IsChecked)
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.USER_PERMAUNLOCK.ToString());
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.USER_UNLOCK.ToString());
                    }                    
                }
            }
        }

        /// <summary>
        /// Based on the checked state add or remove the Gen2.LockAction.EPC_UNLOCK item from lock actions selected
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkbxUnLockEPCMem_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                if ((bool)chkbxUnLockEPCMem.IsChecked)
                {
                    if ((bool)chkbxPermaLockEPCMem.IsChecked)
                    {
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.EPC_PERMAUNLOCK.ToString());
                    }
                    else
                    {
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.EPC_UNLOCK.ToString());
                    }

                    // uncheck the rest of the lock action checkboxes of the user memory
                    chkbxLockEPCMem.IsChecked = false;
                    chkbxLockEPCMem.IsEnabled = false;
                }
                else
                {
                    if ((bool)chkbxPermaLockEPCMem.IsChecked)
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.EPC_PERMAUNLOCK.ToString());
                    }
                    else
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.EPC_UNLOCK.ToString());
                    }
                    chkbxLockEPCMem.IsChecked = false;
                    chkbxLockEPCMem.IsEnabled = true;
                }
            }            
        }

        /// <summary>
        /// Based on the checked state add or remove the Gen2.LockAction.EPC_LOCK item from lock actions selected
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkbxLockEPCMem_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                if ((bool)chkbxLockEPCMem.IsChecked)
                {
                    if ((bool)chkbxPermaLockEPCMem.IsChecked)
                    {
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.EPC_PERMALOCK.ToString());
                    }
                    else
                    {
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.EPC_LOCK.ToString());
                    }                                       

                    // uncheck the rest of the lock action checkboxes of the epc memory
                    chkbxUnLockEPCMem.IsChecked = false;
                    chkbxUnLockEPCMem.IsEnabled = false;                    
                }
                else
                {
                    if ((bool)chkbxPermaLockEPCMem.IsChecked)
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.EPC_PERMALOCK.ToString());
                    }
                    else
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.EPC_LOCK.ToString());
                    }
                    chkbxUnLockEPCMem.IsChecked = false;
                    chkbxUnLockEPCMem.IsEnabled = true;
                }
            }
        }

        /// <summary>
        /// Based on the checked state add or remove the Gen2.LockAction.EPC_PERMALOCK item from lock actions selected
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkbxPermaLockEPCMem_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                if ((bool)chkbxPermaLockEPCMem.IsChecked)
                {
                    if ((bool)chkbxLockEPCMem.IsChecked)
                    {
                        if (selectedLockActions.Contains("Gen2.LockAction." + Gen2.LockAction.EPC_LOCK.ToString()))
                        {
                            selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.EPC_LOCK.ToString());
                        }
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.EPC_PERMALOCK.ToString());
                    }
                    if ((bool)chkbxUnLockEPCMem.IsChecked)
                    {
                        if (selectedLockActions.Contains("Gen2.LockAction." + Gen2.LockAction.EPC_UNLOCK.ToString()))
                        {
                            selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.EPC_UNLOCK.ToString());
                        }
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.EPC_PERMAUNLOCK.ToString());
                    }
                }
                else
                {
                    if ((bool)chkbxLockEPCMem.IsChecked)
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.EPC_PERMALOCK.ToString());
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.EPC_LOCK.ToString());
                    }
                    if ((bool)chkbxUnLockEPCMem.IsChecked)
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.EPC_PERMAUNLOCK.ToString());
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.EPC_UNLOCK.ToString());
                    }
                }
            }
        }

        /// <summary>
        /// Based on the checked state add or remove the Gen2.LockAction.TID_UNLOCK item from lock actions selected
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkbxUnLockTIDMem_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                if ((bool)chkbxUnLockTIDMem.IsChecked)
                {
                    if ((bool)chkbxPermaLockTIDMem.IsChecked)
                    {
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.TID_PERMAUNLOCK.ToString());
                    }
                    else
                    {
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.TID_UNLOCK.ToString());
                    }

                    // uncheck the rest of the lock action checkboxes of the tid memory
                    chkbxLockTIDMem.IsChecked = false;
                    chkbxLockTIDMem.IsEnabled = false;
                }
                else
                {
                    if ((bool)chkbxPermaLockTIDMem.IsChecked)
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.TID_PERMAUNLOCK.ToString());
                    }
                    else
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.TID_UNLOCK.ToString());
                    }
                    chkbxLockTIDMem.IsChecked = false;
                    chkbxLockTIDMem.IsEnabled = true;
                }
            }
        }

        /// <summary>
        /// Based on the checked state add or remove the Gen2.LockAction.TID_LOCK item from lock actions selected
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkbxLockTIDMem_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                if ((bool)chkbxLockTIDMem.IsChecked)
                {
                    if ((bool)chkbxPermaLockTIDMem.IsChecked)
                    {
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.TID_PERMALOCK.ToString());
                    }
                    else
                    {
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.TID_LOCK.ToString());
                    }

                    // uncheck the rest of the lock action checkboxes of the tid memory
                    chkbxUnLockTIDMem.IsChecked = false;
                    chkbxUnLockTIDMem.IsEnabled = false;
                }
                else
                {
                    if ((bool)chkbxPermaLockTIDMem.IsChecked)
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.TID_PERMALOCK.ToString());
                    }
                    else
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.TID_LOCK.ToString());
                    }
                    chkbxUnLockTIDMem.IsChecked = false;
                    chkbxUnLockTIDMem.IsEnabled = true;
                }
            }
        }

        /// <summary>
        ///  Based on the checked state add or remove the Gen2.LockAction.TID_PERMALOCK item from lock actions selected
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkbxPermaLockTIDMem_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                if ((bool)chkbxPermaLockTIDMem.IsChecked)
                {
                    if ((bool)chkbxLockTIDMem.IsChecked)
                    {
                        if (selectedLockActions.Contains("Gen2.LockAction." + Gen2.LockAction.TID_LOCK.ToString()))
                        {
                            selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.TID_LOCK.ToString());
                        }
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.TID_PERMALOCK.ToString());
                    }
                    if ((bool)chkbxUnLockTIDMem.IsChecked)
                    {
                        if (selectedLockActions.Contains("Gen2.LockAction." + Gen2.LockAction.TID_UNLOCK.ToString()))
                        {
                            selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.TID_UNLOCK.ToString());
                        }
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.TID_PERMAUNLOCK.ToString());
                    }
                }
                else
                {
                    if ((bool)chkbxLockTIDMem.IsChecked)
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.TID_PERMALOCK.ToString());
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.TID_LOCK.ToString());
                    }
                    if ((bool)chkbxUnLockTIDMem.IsChecked)
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.TID_PERMAUNLOCK.ToString());
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.TID_UNLOCK.ToString());
                    }
                }
            }
        }

        /// <summary>
        /// Based on the checked state add or remove the Gen2.LockAction.ACCESS_UNLOCK item from lock actions selected
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkbxUnLocAccessPwd_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                if ((bool)chkbxUnLocAccessPwd.IsChecked)
                {
                    if ((bool)chkbxPermaLockAccessPwd.IsChecked)
                    {
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.ACCESS_PERMAUNLOCK.ToString());
                    }
                    else
                    {
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.ACCESS_UNLOCK.ToString());
                    }

                    // uncheck the rest of the lock action checkboxes of the tid memory
                    chkbxLockAccessPwd.IsChecked = false;
                    chkbxLockAccessPwd.IsEnabled = false;
                }
                else
                {
                    if ((bool)chkbxPermaLockAccessPwd.IsChecked)
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.ACCESS_PERMAUNLOCK.ToString());
                    }
                    else
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.ACCESS_UNLOCK.ToString());
                    }
                    chkbxLockAccessPwd.IsChecked = false;
                    chkbxLockAccessPwd.IsEnabled = true;
                }
            }
        }

        /// <summary>
        /// Based on the checked state add or remove the Gen2.LockAction.ACCESS_LOCK item from lock actions selected
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkbxLockAccessPwd_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                if ((bool)chkbxLockAccessPwd.IsChecked)
                {
                    if ((bool)chkbxPermaLockAccessPwd.IsChecked)
                    {
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.ACCESS_PERMALOCK.ToString());
                    }
                    else
                    {
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.ACCESS_LOCK.ToString());
                    }

                    // uncheck the rest of the lock action checkboxes of the tid memory
                    chkbxUnLocAccessPwd.IsChecked = false;
                    chkbxUnLocAccessPwd.IsEnabled = false;
                }
                else
                {
                    if ((bool)chkbxPermaLockAccessPwd.IsChecked)
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.ACCESS_PERMALOCK.ToString());
                    }
                    else
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.ACCESS_LOCK.ToString());
                    }
                    chkbxUnLocAccessPwd.IsChecked = false;
                    chkbxUnLocAccessPwd.IsEnabled = true;
                }
            }
        }

        /// <summary>
        /// Based on the checked state add or remove the Gen2.LockAction.ACCESS_PERMALOCK item from lock actions selected
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkbxPermaLockAccessPwd_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                if ((bool)chkbxPermaLockAccessPwd.IsChecked)
                {
                    if ((bool)chkbxLockAccessPwd.IsChecked)
                    {
                        if (selectedLockActions.Contains("Gen2.LockAction." + Gen2.LockAction.ACCESS_LOCK.ToString()))
                        {
                            selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.ACCESS_LOCK.ToString());
                        }
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.ACCESS_PERMALOCK.ToString());
                    }
                    if ((bool)chkbxUnLocAccessPwd.IsChecked)
                    {
                        if (selectedLockActions.Contains("Gen2.LockAction." + Gen2.LockAction.ACCESS_UNLOCK.ToString()))
                        {
                            selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.ACCESS_UNLOCK.ToString());
                        }
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.ACCESS_PERMAUNLOCK.ToString());
                    }
                }
                else
                {
                    if ((bool)chkbxLockAccessPwd.IsChecked)
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.ACCESS_PERMALOCK.ToString());
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.ACCESS_LOCK.ToString());
                    }
                    if ((bool)chkbxUnLocAccessPwd.IsChecked)
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.ACCESS_PERMAUNLOCK.ToString());
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.ACCESS_UNLOCK.ToString());
                    }
                }
            }
        }

        /// <summary>
        ///  Based on the checked state add or remove the Gen2.LockAction.KILL_UNLOCK item from lock actions selected
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkbxUnLockKillPwd_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                if ((bool)chkbxUnLockKillPwd.IsChecked)
                {
                    if ((bool)chkbxPermaLockKillPwd.IsChecked)
                    {
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.KILL_PERMAUNLOCK.ToString());
                    }
                    else
                    {
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.KILL_UNLOCK.ToString());
                    }

                    // uncheck the rest of the lock action checkboxes of the kill pwd memory
                    chkbxLockKillPwd.IsChecked = false;
                    chkbxLockKillPwd.IsEnabled = false;
                }
                else
                {
                    if ((bool)chkbxPermaLockKillPwd.IsChecked)
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.KILL_PERMAUNLOCK.ToString());
                    }
                    else
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.KILL_UNLOCK.ToString());
                    }
                    chkbxLockKillPwd.IsChecked = false;
                    chkbxLockKillPwd.IsEnabled = true;
                }
            }
        }

        /// <summary>
        /// Based on the checked state add or remove the Gen2.LockAction.KILL_LOCK item from lock actions selected
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkbxLockKillPwd_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                if ((bool)chkbxLockKillPwd.IsChecked)
                {
                    if ((bool)chkbxPermaLockKillPwd.IsChecked)
                    {
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.KILL_PERMALOCK.ToString());
                    }
                    else
                    {
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.KILL_LOCK.ToString());
                    }

                    // uncheck the rest of the lock action checkboxes of the kill pwd memory
                    chkbxUnLockKillPwd.IsChecked = false;
                    chkbxUnLockKillPwd.IsEnabled = false;
                }
                else
                {
                    if ((bool)chkbxPermaLockKillPwd.IsChecked)
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.KILL_PERMALOCK.ToString());
                    }
                    else
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.KILL_LOCK.ToString());
                    }
                    chkbxUnLockKillPwd.IsChecked = false;
                    chkbxUnLockKillPwd.IsEnabled = true;
                }
            }
        }

        /// <summary>
        /// Based on the checked state add or remove the Gen2.LockAction.KILL_PERMALOCK item from lock actions selected
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkbxPermaLockKillPwd_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                if ((bool)chkbxPermaLockKillPwd.IsChecked)
                {
                    if ((bool)chkbxLockKillPwd.IsChecked)
                    {
                        if (selectedLockActions.Contains("Gen2.LockAction." + Gen2.LockAction.KILL_LOCK.ToString()))
                        {
                            selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.KILL_LOCK.ToString());
                        }
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.KILL_PERMALOCK.ToString());
                    }
                    if ((bool)chkbxUnLockKillPwd.IsChecked)
                    {
                        if (selectedLockActions.Contains("Gen2.LockAction." + Gen2.LockAction.KILL_UNLOCK.ToString()))
                        {
                            selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.KILL_UNLOCK.ToString());
                        }
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.KILL_PERMAUNLOCK.ToString());
                    }
                }
                else
                {
                    if ((bool)chkbxLockKillPwd.IsChecked)
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.KILL_PERMALOCK.ToString());
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.KILL_LOCK.ToString());
                    }
                    if ((bool)chkbxUnLockKillPwd.IsChecked)
                    {
                        selectedLockActions.Remove("Gen2.LockAction." + Gen2.LockAction.KILL_PERMAUNLOCK.ToString());
                        selectedLockActions.Add("Gen2.LockAction." + Gen2.LockAction.KILL_UNLOCK.ToString());
                    }
                }
            }
        }

        /// <summary>
        /// Accept only hex characters for access password
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void txtbxAccesspaasword_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            int hexNumber;
            e.Handled = !int.TryParse(e.Text, NumberStyles.HexNumber, CultureInfo.CurrentCulture, out hexNumber);
            btnWriteAccessPwd.IsEnabled = !e.Handled;
            grpbxLockTgLockActns.IsEnabled = btnWriteAccessPwd.IsEnabled;
            grpbxLockTgApplyLocks.IsEnabled = btnWriteAccessPwd.IsEnabled;
        }        

        /// <summary>
        /// Don't accept space for access password
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void txtbxAccesspaasword_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Space)
            {
                e.Handled = true;
            }
        }

        private void btnWriteAccessPwd_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Mouse.SetCursor(Cursors.Wait);

                if (txtbxAccesspaasword.Text.Length <= 0)
                {
                    MessageBox.Show("Access password can't be empty", "Universal Reader Assistant", MessageBoxButton.OK, MessageBoxImage.Information);
                    return;
                }

                if ((bool)rbFirstTagLockTagTb.IsChecked)
                {
                    antenna = ((null != GetSelectedAntennaList()) ? (GetSelectedAntennaList()[0]) : antenna);
                }

                objReader.ParamSet("/reader/tagop/antenna", antenna);

                if ((bool)rbSelectedTagLockTagTb.IsChecked)
                {
                    TagFilter searchSelect = null;


                    if (selectMemBank == Gen2.Bank.EPC)
                    {
                        searchSelect = new TagData(txtEpc.Text);
                    }
                    else
                    {
                        byte[] SearchSelectData = ByteFormat.FromHex(txtData.Text);
                        if (null == SearchSelectData)
                        {
                            dataLength = 0;
                        }
                        else
                        {
                            dataLength = SearchSelectData.Length;
                        }

                        searchSelect = new Gen2.Select(false, selectMemBank, Convert.ToUInt16(accessPwddStartAddress * 16), Convert.ToUInt16(dataLength * 8), SearchSelectData);
                    }
                    WriteAccessPassword(searchSelect);
                }
                else
                {
                    WriteAccessPassword(null);
                }
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
        /// Write the access password in the reserved memory
        /// </summary>
        /// <param name="filter"> data to be written on the specified tag which is used as filter</param>
        private void WriteAccessPassword(TagFilter filter)
        {
            try
            {
                objReader.ParamSet("/reader/tagop/protocol", TagProtocol.GEN2);
                ushort[] dataToBeWritten = null;
                dataToBeWritten = ByteConv.ToU16s(ByteFormat.FromHex(txtbxAccesspaasword.Text.Replace(" ","")));
                objReader.ExecuteTagOp(new Gen2.WriteData(Gen2.Bank.RESERVED, 2, dataToBeWritten), filter);
                MessageBox.Show("Access Password has successfully been set to 0x"+txtbxAccesspaasword.Text.Replace(" ",""), "Info", MessageBoxButton.OK, MessageBoxImage.Information);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message.ToString(), "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void btnApplyLockActions_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                statusLockActions.Clear();

                // Atleast one lock action need be selected to perform apply
                if (!(selectedLockActions.Count > 0))
                {
                    MessageBox.Show("Please select atleast one lock action to perform", "Universal Reader Assistant", MessageBoxButton.OK, MessageBoxImage.Information);
                    return;
                }

                // Either of the lock/unlock need to be selected, if Permanent? is selected.
                if ((bool)chkbxPermaLockAccessPwd.IsChecked)
                {
                    if (false == ((bool)chkbxLockAccessPwd.IsChecked || (bool)chkbxUnLocAccessPwd.IsChecked))
                    {
                        MessageBox.Show("Read/Write Lock or UnLock need to be selected, if Permanent? for Access Password is checked", "Universal Reader Assistant", MessageBoxButton.OK, MessageBoxImage.Information);
                        return;
                    }
                }

                // Either of the lock/unlock need to be selected, if Permanent? is selected.
                if ((bool)chkbxPermaLockKillPwd.IsChecked)
                {
                    if (false == ((bool)chkbxLockKillPwd.IsChecked || (bool)chkbxUnLockKillPwd.IsChecked))
                    {
                        MessageBox.Show("Read/Write Lock or UnLock need to be selected, if Permanent? for Kill Password is checked", "Universal Reader Assistant", MessageBoxButton.OK, MessageBoxImage.Information);
                        return;
                    }
                }

                // Either of the lock/unlock need to be selected, if Permanent? is selected.
                if ((bool)chkbxPermaLockTIDMem.IsChecked)
                {
                    if (false == ((bool)chkbxLockTIDMem.IsChecked || (bool)chkbxUnLockTIDMem.IsChecked))
                    {
                        MessageBox.Show("Write Lock or UnLock need to be selected, if Permanent? for TID Memory is checked", "Universal Reader Assistant", MessageBoxButton.OK, MessageBoxImage.Information);
                        return;
                    }
                }

                // Either of the lock/unlock need to be selected, if Permanent? is selected.
                if ((bool)chkbxPermaLockUserMem.IsChecked)
                {
                    if (false == ((bool)chkbxLockUserMem.IsChecked || (bool)chkbxUnLockUserMem.IsChecked))
                    {
                        MessageBox.Show("Write Lock or UnLock need to be selected, if Permanent? for User Memory is checked", "Universal Reader Assistant", MessageBoxButton.OK, MessageBoxImage.Information);
                        return;
                    }
                }

                // Either of the lock/unlock need to be selected, if Permanent? is selected.
                if ((bool)chkbxPermaLockEPCMem.IsChecked)
                {
                    if (false == ((bool)chkbxLockEPCMem.IsChecked || (bool)chkbxUnLockEPCMem.IsChecked))
                    {
                        MessageBox.Show("Write Lock or UnLock need to be selected, if Permanent? for EPC Memory is checked", "Universal Reader Assistant", MessageBoxButton.OK, MessageBoxImage.Information);
                        return;
                    }
                }

                // Access password can't be empty
                if (txtbxAccesspaasword.Text.Length <= 0)
                {
                    MessageBox.Show("Access password can't be empty", "Universal Reader Assistant", MessageBoxButton.OK, MessageBoxImage.Information);
                    return;
                }

                // Locking the tag with zero access password provides no security
                if (ByteConv.ToU32(ByteFormat.FromHex(txtbxAccesspaasword.Text.Replace(" ", "")), 0) == 0)
                {
                    MessageBoxResult result = MessageBox.Show("Locking with Access Password = 0 provides no security, memory will be fully accessible. Continue?", "Universal Reader Assistant", MessageBoxButton.OKCancel, MessageBoxImage.Information);
                    if (result.Equals(MessageBoxResult.Cancel))
                    {
                        return;
                    }
                }

                List<string> temp = new List<string>();
                temp = selectedLockActions.ToList<string>();
                foreach (string lck in temp)
                {
                    if (lck.Contains("_PERMALOCK"))
                    {
                        MessageBoxResult result = MessageBox.Show("Are you sure you want the locking action "+lck+" to be made Permanent?. This cannot be undone.", "Universal Reader Assistant", MessageBoxButton.OKCancel, MessageBoxImage.Information);
                        if (result.Equals(MessageBoxResult.Cancel))
                        {
                            selectedLockActions.Remove(lck);
                        }
                    }
                }

                if (selectedLockActions.Contains("Gen2.LockAction.ACCESS_LOCK") || selectedLockActions.Contains("Gen2.LockAction.ACCESS_PERMALOCK"))
                {
                    SaveAccessPassword();
                }               

                Mouse.SetCursor(Cursors.Wait);

                if ((bool)rbFirstTagLockTagTb.IsChecked)
                {
                    antenna = ((null != GetSelectedAntennaList()) ? (GetSelectedAntennaList()[0]) : antenna);
                }

                objReader.ParamSet("/reader/tagop/antenna", antenna);

                if ((bool)rbSelectedTagLockTagTb.IsChecked)
                {
                    TagFilter searchSelect = null;


                    if (selectMemBank == Gen2.Bank.EPC)
                    {
                        searchSelect = new TagData(txtEpc.Text);
                    }
                    else
                    {
                        byte[] SearchSelectData = ByteFormat.FromHex(txtData.Text);
                        if (null == SearchSelectData)
                        {
                            dataLength = 0;
                        }
                        else
                        {
                            dataLength = SearchSelectData.Length;
                        }

                        searchSelect = new Gen2.Select(false, selectMemBank, Convert.ToUInt16(accessPwddStartAddress * 16), Convert.ToUInt16(dataLength * 8), SearchSelectData);
                    }
                    ApplyLock(searchSelect);
                }
                else
                {
                    ApplyLock(null);
                }
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
        /// Current access password, epc and tid need to be saved before applying the lock on access password"
        /// </summary>
        private void SaveAccessPassword()
        {
            try
            {
                MessageBox.Show("Current access password need to be saved before applying the lock on Access Password","Universal Reader Assistant", MessageBoxButton.OK, MessageBoxImage.Information);            
                string accessPwdSaveFileDialogFileName =  "AccessPwd";
                SaveFileDialog accessPwdSaveFileDialog = new SaveFileDialog();
                accessPwdSaveFileDialog.Filter = "Text Files (.txt)|*.txt";
                accessPwdSaveFileDialog.FileName = accessPwdSaveFileDialogFileName;
                if ((bool)accessPwdSaveFileDialog.ShowDialog())
                {
                    accessPwdSaveFileDialogFileName = accessPwdSaveFileDialog.FileName;
                    TextWriter txtWriter = new StreamWriter(accessPwdSaveFileDialogFileName);
                    StringBuilder stringAccesspAssword = new StringBuilder();               

                    Mouse.SetCursor(Cursors.Wait);
                    if ((bool)rbFirstTagLockTagTb.IsChecked)
                    {
                        antenna = GetSelectedAntennaList()[0];
                    }

                    objReader.ParamSet("/reader/tagop/antenna", antenna);
                    TagFilter searchSelect = null;
                    stringAccesspAssword.Append("EPC: "+currentEPC);
                    stringAccesspAssword.Append(Environment.NewLine);
                    if ((bool)rbSelectedTagLockTagTb.IsChecked)
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

                            searchSelect = new Gen2.Select(false, selectMemBank, Convert.ToUInt16(accessPwddStartAddress * 16), Convert.ToUInt16(dataLength * 8), SearchSelectData);
                        }
                    }
                    else
                    {
                        searchSelect = new TagData(currentEPC);                        
                    }

                    //Read TID bank data
                    ushort[] tidData = null;
                    TagOp op;
                    try
                    {
                        string tidBankData = string.Empty;
                        if (model.Equals("M5e") || model.Equals("M5e EU") || model.Equals("M5e Compact") || model.Equals("Astra"))
                        {
                            ReadData(Gen2.Bank.TID, searchSelect, out tidData);
                        }
                        else
                        {
                            op = new Gen2.ReadData(Gen2.Bank.TID, 0, 0);
                            tidData = (ushort[])objReader.ExecuteTagOp(op, searchSelect);
                        }

                        if (null != tidData)
                            tidBankData = ByteFormat.ToHex(ByteConv.ConvertFromUshortArray(tidData), "", " ");

                        if (tidBankData.Length > 0)
                        {
                            stringAccesspAssword.Append("TID: "+tidBankData.Replace(" ",""));
                        }
                    }
                    catch (Exception ex)
                    {
                        if (ex is FAULT_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC_Exception)
                        {
                            stringAccesspAssword.Append("TID: Read Error");
                        }
                        else
                        {
                            stringAccesspAssword.Append("TID: "+ex.Message);
                        }
                    }
                    stringAccesspAssword.Append(Environment.NewLine);
                    stringAccesspAssword.Append("AccessPassword: 0x"+txtbxAccesspaasword.Text.Replace(" ",""));
                    stringAccesspAssword.Append(Environment.NewLine);
                    txtWriter.WriteLine(stringAccesspAssword.ToString());
                    txtWriter.Close();
                }
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
        /// Apply selected lock actions on the tag
        /// </summary>
        /// <param name="filter"></param>
        private void ApplyLock(TagFilter filter)
        {
            try
            {
                statusLockActions.Clear();
                objReader.ParamSet("/reader/tagop/protocol", TagProtocol.GEN2);
                foreach (string lck in selectedLockActions)
                {
                    Gen2.LockAction lockAction = null;
                    try
                    {                       
                        switch (lck)
                        {
                            case "Gen2.LockAction.ACCESS_LOCK":
                                lockAction = new Gen2.LockAction(Gen2.LockAction.ACCESS_LOCK);
                                break;
                            case "Gen2.LockAction.ACCESS_PERMALOCK":
                                lockAction = new Gen2.LockAction(Gen2.LockAction.ACCESS_PERMALOCK);
                                break;
                            case "Gen2.LockAction.ACCESS_UNLOCK":
                                lockAction = new Gen2.LockAction(Gen2.LockAction.ACCESS_UNLOCK);
                                break;
                            case "Gen2.LockAction.EPC_LOCK":
                                lockAction = new Gen2.LockAction(Gen2.LockAction.EPC_LOCK);
                                break;
                            case "Gen2.LockAction.EPC_PERMALOCK":
                                lockAction = new Gen2.LockAction(Gen2.LockAction.EPC_PERMALOCK);
                                break;
                            case "Gen2.LockAction.EPC_UNLOCK":
                                lockAction = new Gen2.LockAction(Gen2.LockAction.EPC_UNLOCK);
                                break;
                            case "Gen2.LockAction.KILL_LOCK":
                                lockAction = new Gen2.LockAction(Gen2.LockAction.KILL_LOCK);
                                break;
                            case "Gen2.LockAction.KILL_PERMALOCK":
                                lockAction = new Gen2.LockAction(Gen2.LockAction.KILL_PERMALOCK);
                                break;
                            case "Gen2.LockAction.KILL_UNLOCK":
                                lockAction = new Gen2.LockAction(Gen2.LockAction.KILL_UNLOCK);
                                break;
                            case "Gen2.LockAction.TID_LOCK":
                                lockAction = new Gen2.LockAction(Gen2.LockAction.TID_LOCK);
                                break;
                            case "Gen2.LockAction.TID_PERMALOCK":
                                lockAction = new Gen2.LockAction(Gen2.LockAction.TID_PERMALOCK);
                                break;
                            case "Gen2.LockAction.TID_UNLOCK":
                                lockAction = new Gen2.LockAction(Gen2.LockAction.TID_UNLOCK);
                                break;
                            case "Gen2.LockAction.USER_LOCK":
                                lockAction = new Gen2.LockAction(Gen2.LockAction.USER_LOCK);
                                break;
                            case "Gen2.LockAction.USER_PERMALOCK":
                                lockAction = new Gen2.LockAction(Gen2.LockAction.USER_PERMALOCK);
                                break;
                            case "Gen2.LockAction.USER_UNLOCK":
                                lockAction = new Gen2.LockAction(Gen2.LockAction.USER_UNLOCK);
                                break;
                            case "Gen2.LockAction.KILL_PERMAUNLOCK":
                                lockAction = new Gen2.LockAction(Gen2.LockAction.KILL_PERMAUNLOCK);
                                break;
                            case "Gen2.LockAction.TID_PERMAUNLOCK":
                                lockAction = new Gen2.LockAction(Gen2.LockAction.TID_PERMAUNLOCK);
                                break;
                            case "Gen2.LockAction.USER_PERMAUNLOCK":
                                lockAction = new Gen2.LockAction(Gen2.LockAction.USER_PERMAUNLOCK);
                                break;
                            case "Gen2.LockAction.ACCESS_PERMAUNLOCK":
                                lockAction = new Gen2.LockAction(Gen2.LockAction.ACCESS_PERMAUNLOCK);
                                break;
                            case "Gen2.LockAction.EPC_PERMAUNLOCK":
                                lockAction = new Gen2.LockAction(Gen2.LockAction.EPC_PERMAUNLOCK);
                                break;
                        }                        
                        objReader.ExecuteTagOp(new Gen2.Lock(ByteConv.ToU32(ByteFormat.FromHex(txtbxAccesspaasword.Text.Replace(" ","")), 0), lockAction), filter);
                        statusLockActions.Add("Success");
                        txtblkErrorAccessPassword.Text = "";
                        txtblkErrorAccessPassword.Visibility = System.Windows.Visibility.Collapsed;
                    }
                    catch (Exception ex)
                    {
                        statusLockActions.Add("Error: "+ex.Message);                        
                    }
                }                
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message.ToString(), "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        /// <summary>
        /// Reset previous error
        /// </summary>
        public void ResetLockActionCheckBoxes()
        {
            txtblkErrorAccessPassword.Text = "";
            txtblkErrorAccessPassword.Visibility = System.Windows.Visibility.Collapsed;
            // Clear access password
            txtbxAccesspaasword.Text = "";

            // uncheck all the lock actions checkboxes            
            chkbxLockAccessPwd.IsChecked = false;
            chkbxLockEPCMem.IsChecked = false;
            chkbxLockKillPwd.IsChecked = false;
            chkbxLockTIDMem.IsChecked = false;
            chkbxLockUserMem.IsChecked = false;

            chkbxPermaLockAccessPwd.IsChecked = false;
            chkbxPermaLockEPCMem.IsChecked = false;
            chkbxPermaLockKillPwd.IsChecked = false;
            chkbxPermaLockTIDMem.IsChecked = false;
            chkbxPermaLockUserMem.IsChecked = false;

            chkbxUnLocAccessPwd.IsChecked = false;
            chkbxUnLockEPCMem.IsChecked = false;
            chkbxUnLockKillPwd.IsChecked = false;
            chkbxUnLockTIDMem.IsChecked = false;
            chkbxUnLockUserMem.IsChecked = false;
            statusLockActions.Clear();
        }

        /// <summary>
        ///  Reset locktag tab to default values
        /// </summary>
        public void ResetLockTagTab()
        {
            if (null != objReader)
            {
                lblSelectFilter.Content = "Showing tag:";
                btnRead.Content = "Read";
                rbSelectedTagLockTagTb.IsEnabled = false;
                rbFirstTagLockTagTb.IsChecked = true;
                rbFirstTagLockTagTb.IsEnabled = true;
                grpbxLockTgLockActns.IsEnabled = false;
                grpbxLockTgApplyLocks.IsEnabled = false;
                grpbxLockTgAccessPwd.IsEnabled = false;
                ResetLockActionCheckBoxes();

                lblLockTagError.Content = "";
                lblLockTagError.Visibility = System.Windows.Visibility.Collapsed;                
            }
        }

        private void rbFirstTagLockTagTb_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                ResetLockTagTab();
            }
        }

        private void grpbxLockTgLockActns_GotFocus(object sender, RoutedEventArgs e)
        {
            // Clear the error status when new lock action is selected
            statusLockActions.Clear();
        }
    }
}

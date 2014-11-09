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
    /// Interaction logic for ucWriteEPC.xaml
    /// </summary>
    public partial class ucWriteEPC : UserControl
    {
        Reader objReader;
        uint startAddress = 0;
        int dataLength = 0;
        int antenna = 0;
        Gen2.Bank selectMemBank;

        public ucWriteEPC()
        {
            InitializeComponent();
        }

        public void LoadEPC(Reader reader)
        {
            objReader = reader;
        }

        public void Load(Reader reader,uint address, int length, Gen2.Bank selectedBank,TagReadRecord selectedTagRed)
        {
            InitializeComponent();
            objReader = reader;
            startAddress = address;
            dataLength = length;
            selectMemBank = selectedBank;

            spWriteEPC.IsEnabled = true;
            rbSelectedTag.IsChecked = true;
            rbSelectedTag.IsEnabled = true;
            
            string[] stringData = selectedTagRed.Data.Split(' ');
            txtEpc.Text = selectedTagRed.EPC;
            txtData.Text = string.Join("", stringData);
            Window mainWindow = App.Current.MainWindow;
            ucTagResults tagResults = (ucTagResults)mainWindow.FindName("TagResults");
            switch(selectedBank)
            {
                case Gen2.Bank.EPC:                    
                    if (tagResults.txtSelectedCell.Text == "Data")
                    {
                        lblSelectFilter.Content = "EPC Memory, Decimal Address = " + address.ToString() + " and Data = " + txtData.Text;
                    }
                    else
                    {
                        lblSelectFilter.Content = "EPC ID = " + selectedTagRed.EPC;
                    }
                    break;
                case Gen2.Bank.TID:
                    if (tagResults.txtSelectedCell.Text == "Data")
                    {
                        lblSelectFilter.Content = "TID Memory, Decimal Address = " + address.ToString() + " and Data = " + txtData.Text;
                    }
                    else
                    {
                        lblSelectFilter.Content = "EPC ID = " + selectedTagRed.EPC;
                    }
                    break;
                case Gen2.Bank.USER:
                    if (tagResults.txtSelectedCell.Text == "Data")
                    {
                        lblSelectFilter.Content = "User Memory, Decimal Address = " + address.ToString() + " and Data = " + txtData.Text;
                    }
                    else
                    {
                        lblSelectFilter.Content = "EPC ID = " + selectedTagRed.EPC;
                    }                    
                    break;
            }            
            txtCurrentEpc.Text = selectedTagRed.EPC;
            currentEpc = txtCurrentEpc.Text;
            antenna = selectedTagRed.Antenna;
        }

        private void btnRead_Click(object sender, RoutedEventArgs e)
        {
            Mouse.SetCursor(Cursors.Wait);
            TagReadData[] tagReads;
            try
            {
                if ((bool)rbFirstTag.IsChecked)
                {
                    SimpleReadPlan srp = new SimpleReadPlan(((null != GetSelectedAntennaList())?(new int[] { GetSelectedAntennaList()[0]}):null), TagProtocol.GEN2, null, 0);
                    objReader.ParamSet("/reader/read/plan", srp);
                    tagReads = objReader.Read(500);
                }
                else
                {
                    SetReadPlan();
                    tagReads = objReader.Read(500);
                }

                if ((null != tagReads) && (tagReads.Length > 0))
                {
                    if ((bool)rbASCIIRep.IsChecked)
                    {
                        txtCurrentEpc.Text = Utilities.HexStringToAsciiString(tagReads[0].EpcString);                        
                    }
                    else if ((bool)rbReverseBase36Rep.IsChecked)
                    {
                        txtCurrentEpc.Text = Utilities.ConvertHexToBase36(tagReads[0].EpcString);
                    }
                    else
                    {
                        txtCurrentEpc.Text = tagReads[0].EpcString;
                    }
                    if (tagReads.Length > 1)
                    {
                        lblError.Content = "Warning: More than one tag responded";
                        lblError.Visibility = System.Windows.Visibility.Visible;
                    }
                    else
                    {
                        lblError.Visibility = System.Windows.Visibility.Collapsed;
                    }
                    currentEpc = txtCurrentEpc.Text;
                }
                else
                {
                    txtCurrentEpc.Text = "";
                    MessageBox.Show("No tags found", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
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

        private void btnWrite_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Mouse.SetCursor(Cursors.Wait);
                
                if ((bool)rbFirstTag.IsChecked)
                {
                    antenna = ((null != GetSelectedAntennaList()) ? ( GetSelectedAntennaList()[0]) : antenna);
                }

                objReader.ParamSet("/reader/tagop/antenna", antenna);                

                if ((bool)rbSelectedTag.IsChecked)
                {
                    TagFilter searchSelect = null;


                    if (lblSelectFilter.Content.ToString().Contains("EPC ID"))
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
                        
                        searchSelect = new Gen2.Select(false, selectMemBank, Convert.ToUInt16(startAddress * 16), Convert.ToUInt16(dataLength * 8), SearchSelectData);
                    }
                    WriteEPC(searchSelect, txtWriteEPC.Text);
                }
                else
                {
                    WriteEPC(null, txtWriteEPC.Text);
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

        private void WriteEPC(TagFilter filter, string epc)
        {
            try
            {
                objReader.ParamSet("/reader/tagop/protocol", TagProtocol.GEN2);
                if ((bool)rbASCIIRep.IsChecked)
                {
                    if (validateWriteEPCLength(Utilities.AsciiStringToHexString(epc)))
                    {
                        objReader.WriteTag(filter, new TagData(Utilities.AsciiStringToHexString(epc)));
                    }
                    else
                    {
                        return;
                    }
                }
                else if ((bool)rbReverseBase36Rep.IsChecked)
                {
                    if (validateWriteEPCLength(Utilities.ConvertBase36ToHex(epc)))
                    {
                        objReader.WriteTag(filter, new TagData(Utilities.ConvertBase36ToHex(epc)));
                    }
                    else
                    {
                        return;
                    }
                }
                else
                {
                    if (validateWriteEPCLength(epc))
                    {
                        objReader.WriteTag(filter, new TagData(epc));
                    }
                    else
                    {
                        return;
                    }
                }
                MessageBox.Show("Write epc is successfull", "Info", MessageBoxButton.OK, MessageBoxImage.Information);                
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message.ToString(), "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private bool validateWriteEPCLength(string validEPC)
        {
            if (((validEPC.Length % 4) != 0) || validEPC.Contains(" "))
            {
                MessageBox.Show("Please enter a valid New EPC", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return false;
            }
            else
            {
                return true;
            }
        }

        private void SetReadPlan()
        {
           
            TagFilter filter = null;


            if (selectMemBank == Gen2.Bank.EPC) 
            {
                if (txtEpc.Text != "")
                {
                    filter = new TagData(txtEpc.Text);
                }
            }
            else
            {
                byte[] data = ByteFormat.FromHex(txtData.Text);

                if (null == data)
                {
                    dataLength = 0;
                }
                else
                {
                    dataLength = data.Length;
                }

                filter = new Gen2.Select(false, selectMemBank, Convert.ToUInt16(startAddress * 16), Convert.ToUInt16(dataLength * 8), data);
            }
            
            SimpleReadPlan srp = new SimpleReadPlan(new int[]{antenna}, TagProtocol.GEN2, filter, 1000);
            objReader.ParamSet("/reader/read/plan", srp);
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

        private void txtCurrentEpc_TextChanged(object sender, TextChangedEventArgs e)
        {
            string input = txtCurrentEpc.Text;
            if ((bool)rbASCIIRep.IsChecked)
            {
                for (int count = 0; count < input.Length; count++)
                {
                    int asciiValue = (int)input[count];
                    if ((asciiValue < 32) || (asciiValue > 127))
                    {
                        txtCurrentEpc.Text = txtCurrentEpc.Text.Replace(input[count].ToString(), Convert.ToString(((char)9633)));
                    }
                }
            }
        }

        string currentEpcRep = "Hex";
        string currentEpc = string.Empty;
        private void rbHexRep_Checked(object sender, RoutedEventArgs e)
        {
            try
            {
                if (currentEpcRep != "Hex")
                {
                    if (currentEpcRep == "Ascii")
                    {
                        if (txtCurrentEpc.Text != "")
                        {
                            txtCurrentEpc.Text = Utilities.AsciiStringToHexString(txtCurrentEpc.Text);
                        }
                        if (txtWriteEPC.Text != "")
                        {
                            txtWriteEPC.Text = Utilities.AsciiStringToHexString(txtWriteEPC.Text);
                        }
                    }
                    else if (currentEpcRep == "Base36")
                    {
                        if (txtCurrentEpc.Text != "")
                        {
                            txtCurrentEpc.Text = Utilities.ConvertBase36ToHex(txtCurrentEpc.Text);
                        }
                        if (txtWriteEPC.Text != "")
                        {
                            txtWriteEPC.Text = Utilities.ConvertBase36ToHex(txtWriteEPC.Text);
                        }
                    }
                }
                currentEpcRep = "Hex";
            }
            catch (Exception ex)
            {
                rbHexRep.IsChecked = true;
                MessageBox.Show(ex.Message.ToString(), "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void rbASCIIRep_Checked(object sender, RoutedEventArgs e)
        {
            try
            {
                if (currentEpcRep == "Hex")
                {
                    if (txtCurrentEpc.Text != "")
                    {
                        txtCurrentEpc.Text = Utilities.HexStringToAsciiString(txtCurrentEpc.Text);
                    }
                    if (txtWriteEPC.Text != "")
                    {
                        txtWriteEPC.Text = Utilities.HexStringToAsciiString(txtWriteEPC.Text);
                    }
                }
                else if (currentEpcRep == "Base36")
                {
                    if (txtCurrentEpc.Text != "")
                    {
                        txtCurrentEpc.Text = Utilities.HexStringToAsciiString(Utilities.ConvertBase36ToHex(txtCurrentEpc.Text));
                    }
                    if (txtWriteEPC.Text != "")
                    {
                        txtWriteEPC.Text = Utilities.HexStringToAsciiString(Utilities.ConvertBase36ToHex(txtWriteEPC.Text));
                    }
                }
                currentEpcRep = "Ascii";
            }
            catch (Exception ex)
            {
                if (currentEpcRep == "Hex")
                    rbHexRep.IsChecked = true;
                else if (currentEpcRep == "Base36")
                    rbReverseBase36Rep.IsChecked = true;
                MessageBox.Show(ex.Message.ToString(), "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void rbReverseBase36Rep_Checked(object sender, RoutedEventArgs e)
        {
            try
            {
                if (currentEpcRep == "Hex")
                {
                    if (txtCurrentEpc.Text != "")
                    {
                        txtCurrentEpc.Text = Utilities.ConvertHexToBase36(txtCurrentEpc.Text);
                    }
                    if (txtWriteEPC.Text != "")
                    {
                        txtWriteEPC.Text = Utilities.ConvertHexToBase36(txtWriteEPC.Text);
                    }
                }
                else if (currentEpcRep == "Ascii")
                {
                    if (txtCurrentEpc.Text != "")
                    {
                        txtCurrentEpc.Text = Utilities.ConvertHexToBase36(Utilities.AsciiStringToHexString(txtCurrentEpc.Text));
                    }
                    if (txtWriteEPC.Text != "")
                    {
                        txtWriteEPC.Text = Utilities.ConvertHexToBase36(Utilities.AsciiStringToHexString(txtWriteEPC.Text));
                    }
                }
                currentEpcRep = "Base36";
            }
            catch (Exception ex)
            {
                if (currentEpcRep == "Hex")
                    rbHexRep.IsChecked = true;
                else if (currentEpcRep == "Ascii")
                    rbASCIIRep.IsChecked = true;
                MessageBox.Show(ex.Message.ToString(), "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void txtCurrentEpc_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            e.Handled = true;
        }

        private void CommandBinding_CanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = false;
            e.Handled = true;

        }

        private void txtCurrentEpc_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if ((e.Key == Key.Back) || (e.Key == Key.Delete))
            {
                e.Handled = true;
            }
        }

        /// <summary>
        /// Reset writeepc tab to default values
        /// </summary>
        public void ResetWriteEPCTab()
        {
            if (null != objReader)
            {
                rbSelectedTag.IsEnabled = false;
                txtEpc.Text = "";
                txtData.Text = "";
                txtCurrentEpc.Text = "";
                txtWriteEPC.Text = "";
                lblError.Content = "";
                rbFirstTag.IsChecked = true;
                rbHexRep.IsChecked = true;
            }
        }

        private void rbFirstTag_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                ResetWriteEPCTab();
            }
        }
    }
}

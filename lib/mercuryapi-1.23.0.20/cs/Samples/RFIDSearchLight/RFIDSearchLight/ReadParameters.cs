using System;
using System.Linq;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Media;
using System.Text;
using System.Windows.Forms;
using Microsoft.WindowsCE.Forms;

namespace ThingMagic.RFIDSearchLight
{
    public partial class ReadParameters : Form
    {
        log4net.ILog logger = log4net.LogManager.GetLogger("ReadParameters");
        private bool isSettingsChanged = false;

        private Dictionary<string, string> properties;
        OpenFileDialog openFile = new OpenFileDialog();

        public ReadParameters()
        {
            InitializeComponent();
        }

        private void ReadParameters_Load(object sender, EventArgs e)
        {
            InputModeEditor.SetInputMode(txtScanDuration, InputMode.Numeric);
            properties = Utilities.GetProperties();
            txtScanDuration.Text = properties["scanduration"];
            txtScanDuration.TextChanged += cbValidateTxtScanDuration;
            ValidateTxtScanDuration();
            cbAudibleAlerts.SelectedText = properties["audiblealert"];
            cbSeparator.SelectedText = properties["metadataseparator"];
            string[] metadataArray = properties["metadatatodisplay"].Split(',');
            chkTimeStamp.Checked = false;
            chkRSSI.Checked = false;
            chkPosition.Checked = false;
            foreach (string metadata in metadataArray)
            {
                switch (metadata)
                {
                    case "timestamp":
                        chkTimeStamp.Checked = true;
                        break;
                    case "rssi":
                        chkRSSI.Checked = true;
                        break;
                    case "position":
                        chkPosition.Checked = true;
                        break;
                    default: break;
                }
            }
            txtDecodeFile.Text = properties["decodewavefile"];
            txtStartScanFile.Text = properties["startscanwavefile"];
            txtEndScanFile.Text = properties["endscanwavefile"];
            openFile.Filter = "wave Files (.wav)|*.wav";

            // Ignore any change events fired during initialization
            isSettingsChanged = false;
        }

        private void btnSave_Click(object sender, EventArgs e)
        {
            SaveSettings();
        }
        private bool SaveSettings()
        {
            if (false == IsValidScanDuration(txtScanDuration.Text))
            {
                MessageBox.Show("Please give a valid scan duration");
                return false;
            }
            properties["scanduration"] = txtScanDuration.Text;
            properties["audiblealert"] = cbAudibleAlerts.Text;
            properties["metadataseparator"] = cbSeparator.Text;
            string metadata = string.Empty;
            if (chkTimeStamp.Checked)
            {
                metadata = "timestamp,";
            }
            if(chkRSSI.Checked)
            {
                metadata += "rssi,";
            }
            if (chkPosition.Checked)
            {
                metadata += "position";
            }
            properties["metadatatodisplay"] = metadata;
            properties["decodewavefile"] = txtDecodeFile.Text;
            properties["startscanwavefile"] = txtStartScanFile.Text;
            properties["endscanwavefile"] = txtEndScanFile.Text;

            Utilities.SaveConfigurations(properties);
            isSettingsChanged = false;
            this.Close();
            return true;
        }

        private bool SoundFileIsGood(string filename)
        {
            SoundPlayer player = new SoundPlayer(filename);
            try
            {
                player.Load();
                player.Play();
                return true;
            }
            catch (Exception ex)
            {
                MessageBox.Show(
                    "Can't play " + filename + "\r\n" +
                    ex.ToString()
                    );
                return false;
            }
        }

        /// <summary>
        /// Test string input for validity.  Valid scan durations are positive floating-point values.
        /// </summary>
        /// <param name="value"></param>
        /// <returns>True if value is valid input for scan duration</returns>
        private bool IsValidScanDuration(string value)
        {
            try
            {
                Double.Parse(value);
                return true;
            }
            catch (Exception ex)
            {
                return false;
            }
        }

        private void btnChooseDecode_Click(object sender, EventArgs e)
        {
            openFile.ShowDialog();
            if ("" != openFile.FileName.ToString())
            {
                if (SoundFileIsGood(openFile.FileName.ToString()))
                {
                    txtDecodeFile.Text = openFile.FileName.ToString();
                }
            }
        }

        private void btnChooseStartScan_Click(object sender, EventArgs e)
        {
            openFile.ShowDialog();
            if ("" != openFile.FileName.ToString())
            {
                if (SoundFileIsGood(openFile.FileName.ToString()))
                {
                    txtStartScanFile.Text = openFile.FileName.ToString();
                }
            }
        }

        private void btnChooseEndScan_Click(object sender, EventArgs e)
        {
            openFile.ShowDialog();
            {
                if ("" != openFile.FileName.ToString())
                    if (SoundFileIsGood(openFile.FileName.ToString()))
                    {
                        txtEndScanFile.Text = openFile.FileName.ToString();
                    }
            }
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            isSettingsChanged = false;
            this.Close();
        }

        private void btnRestoreDefaults_Click(object sender, EventArgs e)
        {
            txtScanDuration.Text = "3";
            cbAudibleAlerts.SelectedItem = "Yes";
            cbSeparator.SelectedItem = "Space";
            chkTimeStamp.Checked = false;
            chkPosition.Checked = false;
            chkRSSI.Checked = false;

            cbAudibleAlerts.SelectedItem = "Yes";
            txtDecodeFile.Text = @"\Windows\Loudest.wav";
            txtStartScanFile.Text = @"\Windows\Quietest.wav";
            txtEndScanFile.Text = @"\Windows\Splat.wav";
        }

        private void ReadParameters_Closing(object sender, CancelEventArgs e)
        {
            if (isSettingsChanged)
            {
                switch (MessageBox.Show("Do you wish to save it", "Settings are changed", MessageBoxButtons.YesNo, MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button1))
                {
                    case DialogResult.Yes:
                        if (false == SaveSettings())
                        {
                            e.Cancel = true;
                        }
                        break;
                }
            }
        }

        private void SettingsChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void cbValidateTxtScanDuration(object sender, EventArgs e)
        {
            ValidateTxtScanDuration();
        }
        private bool ValidateTxtScanDuration()
        {
            bool isValid = IsValidScanDuration(txtScanDuration.Text);
            txtScanDuration.ForeColor = isValid
                ? SystemColors.WindowText
                : Color.Red;
            return isValid;
        }
    }
}
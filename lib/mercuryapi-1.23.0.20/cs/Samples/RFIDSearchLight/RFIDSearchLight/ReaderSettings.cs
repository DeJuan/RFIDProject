using System;
using System.Linq;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Microsoft.WindowsCE.Forms;

namespace ThingMagic.RFIDSearchLight
{
    public partial class ReaderSettings : Form
    {
        log4net.ILog logger = log4net.LogManager.GetLogger("ReaderSettings");
        private Dictionary<string, string> properties;
        private bool isKey8Pressed = false;
        private bool isKey9Pressed = false;
        private bool isSettingsChanged = false;
        bool isAdressNumeric = true;        
        
        public ReaderSettings()
        {
            InitializeComponent();
            properties = Utilities.GetProperties();            
        }

        private void ReaderSettings_Load(object sender, EventArgs e)
        {
            InputModeEditor.SetInputMode(txtAddress, InputMode.Numeric);
            try
            {
                string tagPopulation = properties["tagpopulation"];
                switch (tagPopulation)
                {
                    case "small":
                        rbSmallTags.Checked = true;
                        break;
                    case "medium":
                        rbMediumTags.Checked = true;
                        break;
                    case "large":
                        rbLargeTags.Checked = true;
                        break;
                    default: break;
                }
                if (properties["selectionaddress"] == "")
                    txtAddress.Text = "0";
                else
                    txtAddress.Text = properties["selectionaddress"];
                if (properties["readpower"] != "")
                {
                    cbPowerInDBM.SelectedItem = (Convert.ToInt32(properties["readpower"])/100).ToString();
                }
                txtEpc.Text = properties["selectionmask"];
                cbTagSelection.SelectedItem = properties["tagselection"];
                if (properties["ismaskselected"] == "yes")
                {
                    chkMask.Checked = true;
                }
                else
                {
                    chkMask.Checked = false;
                }
                isSettingsChanged = false;
            }
            catch (Exception ex)
            {
                logger.Error("In ReaderSettings_Load: " + ex.ToString());
                MessageBox.Show(ex.ToString());
            }
        }
        
        private void btnSave_Click(object sender, EventArgs e)
        {
            if (0 != (txtEpc.Text.Length % 2))
            {
                MessageBox.Show("Please give even number of bytes for mask", "Error");
                return;
            }
             isAdressNumeric = Utilities.IsWholeNumber(txtAddress.Text);
            if (!isAdressNumeric)
            {
                MessageBox.Show("Please give a valid address");
                return;
            }
            isSettingsChanged = false;
            try
            {
                properties["gen2q"] = "staticq";
                if (rbLargeTags.Checked)
                {
                    properties["tagpopulation"] = "large";
                    properties["gen2session"] = "s1";
                    properties["tagencoding"] = "m2";
                    properties["staticqvalue"] = "6";
                }
                else if (rbMediumTags.Checked)
                {
                    properties["tagpopulation"] = "medium";
                    properties["gen2session"] = "s1";
                    properties["tagencoding"] = "m4";
                    properties["staticqvalue"] = "4";
                }
                else if (rbSmallTags.Checked)
                {
                    properties["tagpopulation"] = "small";
                    properties["gen2session"] = "s0";
                    properties["tagencoding"] = "m4";
                    properties["staticqvalue"] = "2";
                }
                properties["selectionaddress"] = txtAddress.Text;
                properties["readpower"] = (Convert.ToInt32(cbPowerInDBM.Text)*100).ToString();
                properties["selectionmask"] = txtEpc.Text;
                if (chkMask.Checked)
                {
                    properties["ismaskselected"] = "yes";
                }
                else
                {
                    properties["ismaskselected"] = "no";
                }
                properties["tagselection"] = cbTagSelection.Text;
                Utilities.SaveConfigurations(properties);
                isSettingsChanged = false;
            }
            catch (Exception ex)
            {
                logger.Error("In btnSave_Click: " + ex.ToString());
                if (isSettingsChanged)
                    isSettingsChanged = true;
                MessageBox.Show(ex.Message);
            }
            finally
            {
                this.Close();
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            cbPowerInDBM.SelectedItem = "21";
            cbTagSelection.SelectedItem = "None";
            txtEpc.Text = "";
            txtAddress.Text = "";
            chkMask.Checked = false;
        }

        private void ReaderSettings_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyValue == 56)
            {
                isKey8Pressed = true;
            }
            if (e.KeyValue == 57)
            {
                isKey9Pressed = true;
            }
            if (isKey8Pressed && isKey9Pressed)
            {
                if (!txtEpc.Focused)
                {
                    if (isSettingsChanged)
                    {
                        btnSave_Click(sender, e);
                    }
                    AdvancedReaderSettings advReaderSettings = new AdvancedReaderSettings();
                    advReaderSettings.Show();
                    isKey8Pressed = false;
                    isKey9Pressed = false;
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
            cbPowerInDBM.SelectedItem = "23";
            rbSmallTags.Checked = true;
            cbTagSelection.SelectedItem = "None";
            txtAddress.Text = "0";
            txtEpc.Text = "";
            chkMask.Checked = false;
        }

        private void cbTagSelection_SelectedIndexChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
            if (cbTagSelection.Text == "None")
            {
                txtEpc.Text = "";
                txtAddress.Text = "0";
                chkMask.Checked = false;
            }
        }

        private void txtAddress_TextChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void txtEpc_TextChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void cbPowerInDBM_SelectedIndexChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void rbLargeTags_CheckedChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void rbMediumTags_CheckedChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void rbSmallTags_CheckedChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void chkMask_CheckStateChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void ReaderSettings_Closing(object sender, CancelEventArgs e)
        {
            if (isSettingsChanged && isAdressNumeric)
            {
                switch (MessageBox.Show("Do you wish to save it", "Settings are changed", MessageBoxButtons.YesNo, MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button1))
                {
                    case DialogResult.Yes:
                        btnSave_Click(sender, e);
                        break;
                }
            }
        }
    }
}
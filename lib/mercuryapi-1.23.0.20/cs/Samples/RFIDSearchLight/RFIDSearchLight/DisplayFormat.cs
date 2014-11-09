using System;
using System.Linq;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Reflection;

namespace ThingMagic.RFIDSearchLight
{
    public partial class DisplayFormat : Form
    {
        private bool isSettingsChanged = false;

        private Dictionary<string, string> properties;
        
        public Dictionary<string, string> Properties
        {
            get { return properties; }
            set { properties = value; }
        }

        public DisplayFormat()
        {
            InitializeComponent();
        }

        private void DisplayFormat_Load(object sender, EventArgs e)
        {
            Properties = Utilities.GetProperties();
            if ("base36" == Properties["displayformat"])
            {
                rbEPCinBase36.Checked = true;
                txtToolTip.Text = "Displays the EPC in Base36 format. Base36 is a positional numeral system that uses 36 as the radix. For Example - RKXJGKK1 which is the Base36 representation of 0x3028354D8202001CAB8890BB";                
            }
            else if ("hex" == Properties["displayformat"])
            {
                rbEpcInHex.Checked = true;
                txtToolTip.Text = "Displays the EPC in Hex format. For Example - 0x3028354D8202001CAB8890BB";
            }
            // Ignore any change events fired during initialization
            isSettingsChanged = false;
        }
        private void btnSave_Click(object sender, EventArgs e)
        {
            if (rbEPCinBase36.Checked)
            {
                Properties["displayformat"] = "base36";
            }
            else
            {
                Properties["displayformat"] = "hex";
            }
            Utilities.SaveConfigurations(Properties);
            isSettingsChanged = false;
            this.Close();
        }

        private void rbEpcInHex_CheckedChanged(object sender, EventArgs e)
        {
            txtToolTip.Text = "Displays the EPC in Hex format. For Example - 0x3028354D8202001CAB8890BB";
            isSettingsChanged = true;
        }

        private void rbEPCinBase36_CheckedChanged(object sender, EventArgs e)
        {
            txtToolTip.Text = "Displays the EPC in Base36 format. Base36 is a positional numeral system that uses 36 as the radix. For Example - RKXJGKK1 which is the Base36 representation of 0x3028354D8202001CAB8890BB";
            isSettingsChanged = true;
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            isSettingsChanged = false;
            this.Close();
        }

        private void btnRestoreToDefaults_Click(object sender, EventArgs e)
        {
            rbEpcInHex.Checked = true;
        }

        private void DisplayFormat_Closing(object sender, CancelEventArgs e)
        {
            if (isSettingsChanged)
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
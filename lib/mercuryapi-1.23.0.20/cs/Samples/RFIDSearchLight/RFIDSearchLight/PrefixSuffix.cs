using System;
using System.Linq;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace ThingMagic.RFIDSearchLight
{
    public partial class PrefixSuffix : Form
    {
        private bool isSettingsChanged = false;

        private Dictionary<string, string> properties;
        public PrefixSuffix()
        {
            InitializeComponent();
            properties = Utilities.GetProperties();
            txtPrefix.Text = properties["prefix"].ToString();
            txtSuffix.Text = properties["suffix"].ToString();
            cbSeparator.SelectedItem = properties["multipletagseparator"].ToString();

            // Ignore any change events fired during initialization
            isSettingsChanged = false;
        }

        private void btnSave_Click(object sender, EventArgs e)
        {
            properties["prefix"] = txtPrefix.Text;
            properties["suffix"] = txtSuffix.Text;
            properties["multipletagseparator"] = cbSeparator.Text;
            Utilities.SaveConfigurations(properties);
            isSettingsChanged = false;
            this.Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            isSettingsChanged = false;
            this.Close();
        }

        private void btnRestoreToDefaults_Click(object sender, EventArgs e)
        {
            txtPrefix.Text = "";
            txtSuffix.Text = "";
            cbSeparator.SelectedItem = "Space";
        }

        private void PrefixSuffix_Closing(object sender, CancelEventArgs e)
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

        private void SettingsChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }
    }
}
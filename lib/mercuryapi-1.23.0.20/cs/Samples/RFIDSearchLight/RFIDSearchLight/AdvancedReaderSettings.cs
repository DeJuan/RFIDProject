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
    public partial class AdvancedReaderSettings : Form
    {
        public static log4net.ILog logger = log4net.LogManager.GetLogger("AdvancedReaderSettings");
        private Dictionary<string, string> properties;
        private bool isSettingsChanged = false;
        bool isAdressNumeric = true;

        public AdvancedReaderSettings()
        {
            InitializeComponent();
            properties = Utilities.GetProperties();
            InputModeEditor.SetInputMode(txtRFOff, InputMode.Numeric);
            InputModeEditor.SetInputMode(txtRFOn, InputMode.Numeric);
        }

        private void AdvancedReaderSettings_Load(object sender, EventArgs e)
        {
            try
            {            
                using (ReadMgr.Session rsess = ReadMgr.GetSession())
                {
                    if (properties["tagencoding"] == "")
                        properties["tagencoding"] = rsess.Reader.ParamGet("/reader/gen2/tagEncoding").ToString();
                    if (properties["gen2target"] == "")
                        properties["gen2target"] = rsess.Reader.ParamGet("/reader/gen2/target").ToString();
                    if (properties["gen2session"] == "")
                        properties["gen2session"] = rsess.Reader.ParamGet("/reader/gen2/session").ToString();
                    if (properties["gen2q"] == "")
                        properties["gen2q"] = rsess.Reader.ParamGet("/reader/gen2/q").ToString();
                    //Populate tagencoding value
                    switch (properties["tagencoding"].ToLower())
                    {
                        case "fm0":
                            //rbFM0.Checked = true;
                            break;
                        case "m2":
                            rbM2.Checked = true;
                            break;
                        case "m4":
                            rbM4.Checked = true;
                            break;
                        case "m8":
                            rbM8.Checked = true;
                            break;
                        default: break;
                    }
                    //Populate target value
                    switch (properties["gen2target"].ToLower())
                    {
                        case "a":
                            rbA.Checked = true;
                            break;
                        case "b":
                            rbB.Checked = true;
                            break;
                        case "ab":
                            rbAB.Checked = true;
                            break;
                        case "ba":
                            rbBA.Checked = true;
                            break;
                    }
                    //Populate gen2 session value
                    switch (properties["gen2session"].ToLower())
                    {
                        case "s0":
                            rbS0.Checked = true;
                            break;
                        case "s1":
                            rbS1.Checked = true;
                            break;
                        case "s2":
                            rbS2.Checked = true;
                            break;
                        case "s3":
                            rbS3.Checked = true;
                            break;
                    }
                    //Populate Q value
                    switch (properties["gen2q"].ToLower())
                    {
                        case "dynamicq":
                            rbDynamicQ.Checked = true;
                            break;
                        case "staticq":
                            rbStaticQ.Checked = true;
                            cbStaticQ.SelectedItem = properties["staticqvalue"];
                            cbStaticQ.Enabled = true;
                            break;
                    }
                    if (properties["recordhighestrssi"].ToLower() == "yes")
                        rbRecordHighestRSSI.Checked = true;
                    else
                        rbRecordHighestRSSI.Checked = false;
                    if (properties["recordrssioflasttagread"].ToLower() == "yes")
                        rbRecordRSSIoflastTagRead.Checked = true;
                    else
                        rbRecordRSSIoflastTagRead.Checked = false;
                    txtRFOn.Text = properties["rfontime"];
                    txtRFOff.Text = properties["rfofftime"];
                    isSettingsChanged = false;
                }
            }
            catch (Exception ex)
            {
                logger.Error("In AdvancedReaderSettings_Load: " + ex.ToString());
                MessageBox.Show(ex.Message);
            }
        }

        private void btnSave_Click(object sender, EventArgs e)
        {
            string session = string.Empty, target = string.Empty, q = string.Empty, tagEncoding = string.Empty;
            isAdressNumeric = Utilities.IsWholeNumber(txtRFOff.Text);
            if (!isAdressNumeric)
           {
               MessageBox.Show("Please give a valid RF Off time", "Error");
               return;
           }
             isAdressNumeric = Utilities.IsWholeNumber(txtRFOn.Text);
            if (!Utilities.IsWholeNumber(txtRFOn.Text))
           {
               MessageBox.Show("Please give a valid RF On time", "Error");
               return;
           }
            try
            {
                isSettingsChanged = false;
                //Tag encoding
                if (rbFM0.Checked)
                    tagEncoding = "fm0";
                else if (rbM2.Checked)
                    tagEncoding = "m2";
                else if (rbM4.Checked)
                    tagEncoding = "m4";
                else if (rbM8.Checked)
                    tagEncoding = "m8";
                //Session
                if (rbS0.Checked)
                    session = "s0";
                else if (rbS1.Checked)
                    session = "s1";
                else if (rbS2.Checked)
                    session = "s2";
                else if (rbS3.Checked)
                    session = "s3";
                //Target
                if (rbA.Checked)
                    target = "a";
                else if (rbB.Checked)
                    target = "b";
                else if (rbAB.Checked)
                    target = "ab";
                else if (rbBA.Checked)
                    target = "ba";
                //Q
                if (rbDynamicQ.Checked)
                {
                    q = "dynamicq";
                }
                else if (rbStaticQ.Checked)
                {
                    q = "staticq";
                    properties["staticqvalue"] = cbStaticQ.SelectedItem.ToString();
                }
                if (rbRecordHighestRSSI.Checked)
                    properties["recordhighestrssi"] = "yes";
                else
                    properties["recordhighestrssi"] = "no";
                if (rbRecordRSSIoflastTagRead.Checked)
                    properties["recordrssioflasttagread"] = "yes";
                else
                    properties["recordrssioflasttagread"] = "no";

                properties["gen2session"] = session;
                properties["gen2target"] = target;
                properties["tagencoding"] = tagEncoding;
                properties["gen2q"] = q;
                properties["rfontime"] = txtRFOn.Text;
                properties["rfofftime"] = txtRFOff.Text;
                Utilities.SaveConfigurations(properties);
                isSettingsChanged = false;
            }
            catch (Exception ex)
            {
                logger.Error("In btnSave_Click: " + ex.ToString());
                if (isSettingsChanged)
                    isSettingsChanged = false;
                MessageBox.Show(ex.Message);
            }
            finally
            {
                this.Close();
            }
        }

        private void rbDynamicQ_CheckedChanged(object sender, EventArgs e)
        {
            if (rbDynamicQ.Checked)
            {
                cbStaticQ.Enabled = false;
            }
            else
            {
                cbStaticQ.Enabled = true;
                if (cbStaticQ.SelectedIndex < 0)
                    cbStaticQ.SelectedIndex = 0;
            }
        }

        private void btnRestoreDefaults_Click(object sender, EventArgs e)
        {
            try
            {
                txtRFOn.Text = "250";
                txtRFOff.Text = "50";
                using (ReadMgr.Session rsess = ReadMgr.GetSession())
                {
                    ((SerialReader)rsess.Reader).CmdBootBootloader();
                    properties = Utilities.GetProperties();
                    ReadMgr.ForceDisconnect();
                }
                using (ReadMgr.Session rsess = ReadMgr.GetSession())
                {
                    properties["tagencoding"] = rsess.Reader.ParamGet("/reader/gen2/tagEncoding").ToString();
                    properties["gen2target"] = rsess.Reader.ParamGet("/reader/gen2/target").ToString();
                    properties["gen2session"] = rsess.Reader.ParamGet("/reader/gen2/session").ToString();
                    properties["gen2q"] = rsess.Reader.ParamGet("/reader/gen2/q").ToString();
                    properties["rfontime"] = "250";
                    properties["rfofftime"] = "50";
                    AdvancedReaderSettings_Load(sender, e);
                    isSettingsChanged = true;
                }
            }
            catch
            {
                System.Threading.Thread.Sleep(200);
                try
                {
                    ReadMgr.ForceReconnect();
                    btnRestoreDefaults_Click(sender, e);
                }
                catch (Exception ex)
                {
                    logger.Error("In btnRestoreDefaults_Click: " + ex.ToString());
                    MessageBox.Show(ex.Message);
                }
                
            }
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            ReadMgr.AllowDisconnect();
            isSettingsChanged = false;
            this.Close();
        }

        private void rbM2_CheckedChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void rbM4_CheckedChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void rbM8_CheckedChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void rbS0_CheckedChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void rbS1_CheckedChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void rbS2_CheckedChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void rbS3_CheckedChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void rbA_CheckedChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void rbB_CheckedChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void rbAB_CheckedChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void rbBA_CheckedChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void rbStaticQ_CheckedChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void cbStaticQ_SelectedIndexChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void txtRFOn_TextChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void txtRFOff_TextChanged(object sender, EventArgs e)
        {
            isSettingsChanged = true;
        }

        private void AdvancedReaderSettings_Closing(object sender, CancelEventArgs e)
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
            ReadMgr.AllowDisconnect();
        }
    }
}

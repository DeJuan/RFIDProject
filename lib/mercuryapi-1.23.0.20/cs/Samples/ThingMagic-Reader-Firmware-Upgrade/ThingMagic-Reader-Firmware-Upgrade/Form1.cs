/*
 * Copyright (c) 2009 ThingMagic, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.

 ****************************************************************
 * Software to Upgrade ThingMagic's embedded module.
 * Date: 03/04/2010
 * Author: Kapil Asher
 *****************************************************************
 */


using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Threading;
using System.IO;
using ThingMagic;

namespace ThingMagic_Reader_Firmware_Upgrade
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }
        Reader reader = null;
        SerialReader rdr = null;
        private Thread progressStatus = null;
        private void copyToolStripMenuItem_Click(object sender, EventArgs e)
        {

        }

        private void readerAddress_TextChanged(object sender, EventArgs e)
        {

        }

        private void disconnectReaderToolStripMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                rdr.Destroy();
            }
            catch { }
        }

        private void exitMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                rdr.Destroy();
            }
            catch
            {
            }

            Application.Exit();
        }

        OpenFileDialog openFile = new OpenFileDialog();

        private void chooseFileButton_Click(object sender, EventArgs e)
        {            
            openFile.ShowDialog();
            firmwarePath.Text = openFile.FileName.ToString();
        }

        private void firmwarePath_TextChanged(object sender, EventArgs e)
        {

        }

        private void progressBar1_Click(object sender, EventArgs e)
        {

        }

        private void upgradeButton_Click(object sender, EventArgs e)
        {
            try
            {
                if (firmwarePath.Text == "")
                {
                    MessageBox.Show("Please select a firmware to load", "Error!", MessageBoxButtons.OK);
                }
                else if (!firmwarePath.Text.Contains(".sim") && !firmwarePath.Text.Contains(".tmfw"))
                {
                    MessageBox.Show("Invalid File Extension", "Error!", MessageBoxButtons.OK);
                }
                else
                {
                    upgradeButton.Enabled = false;
                    chooseFileButton.Enabled = false;
                    label6.Text = "";
                    if (readerAddress.Text.Contains("COM") || readerAddress.Text.Contains("com"))
                    {
                        ///Creates a Reader Object for operations on the Reader.
                        reader = Reader.Create(string.Concat("tmr:///", readerAddress.Text));

                    }
                    else
                    {
                        //Creates a Reader Object for operations on the Reader.
                        reader = Reader.Create(string.Concat("tmr://", readerAddress.Text));
                    }
                    try
                    {
                        reader.Connect();
                    }
                    catch (FAULT_BL_INVALID_IMAGE_CRC_Exception)
                    {

                    }
                    thingMagicReaderToolStripMenuItem.Enabled = true;
                    reader.ParamSet("/reader/transportTimeout", 5000);
                    if (readerAddress.Text.Contains("COM") || readerAddress.Text.Contains("com"))
                    {
                        reader.ParamSet("/reader/baudRate", 115200);
                    }
                    thingMagicReaderToolStripMenuItem.Enabled = true;
                    System.IO.FileStream firmware = new System.IO.FileStream(firmwarePath.Text, System.IO.FileMode.Open, System.IO.FileAccess.Read);
                    startProgress();
                    Thread updateTrd = new Thread(delegate()
                    {
                        reader.FirmwareLoad(firmware);
                        reader.Destroy();
                        stopProgress();
                        label6.Invoke(new del1(updateStatus));
                    });
                    updateTrd.Start();
                    upgradeButton.Enabled = true;
                    chooseFileButton.Enabled = true;                   
                }
                
            }
            catch (System.IO.IOException)
            {
                if (!readerAddress.Text.Contains("COM"))
                {
                    MessageBox.Show("Application needs a valid Reader Address of type COMx", "Error", MessageBoxButtons.OK);
                    upgradeButton.Enabled = true;
                    chooseFileButton.Enabled = true;  
                }
                else
                {
                    MessageBox.Show("Reader not connected on " + readerAddress.Text, "Error", MessageBoxButtons.OK);
                    upgradeButton.Enabled = true;
                    chooseFileButton.Enabled = true;  
                }
            }
            catch (ReaderCodeException ex)
            {
                    MessageBox.Show("Error connecting to reader: " + ex.Message.ToString(), "Error!", MessageBoxButtons.OK);
                    label6.Text = "Upgrade Failed!";
                    upgradeButton.Enabled = true;
                    chooseFileButton.Enabled = true;
            }
            catch (System.UnauthorizedAccessException)
            {
                MessageBox.Show("Access to " + readerAddress.Text + " denied. Please check if another program is accessing this port", "Error!", MessageBoxButtons.OK);
                upgradeButton.Enabled = true;
                chooseFileButton.Enabled = true;  
            }
        
        }
        bool start;
        private void startProgress()
        {
            start = true;
            if (progressStatus == null)
            {
                this.progressStatus = new Thread(ProgressBarWork);
                this.progressStatus.IsBackground = true;
                this.progressStatus.Start();
            }
        }
        private void stopProgress()
        {
            start = false;
            if (progressStatus != null)
            {
                progressStatus.Join(100);
                progressStatus = null;
            }

        }

        delegate void del(int x);
        delegate void del1();
        void updateProgressBar(int y)
        {
            progressBar1.Value = y;
        }
        void updateStatus()
        {
            label6.Text = "Upgrade Successful!";
        }
        
        private void ProgressBarWork()
        {
            Thread.Sleep(1000);
            int i = 0;
            while (i < 101)
            {
                progressBar1.Invoke(new del(updateProgressBar), new object[] { i });
                if (start)
                    i++;
                else
                {
                    i = 102;
                    progressBar1.Invoke(new del(updateProgressBar), new object[] { progressBar1.Maximum });
                }                
                Thread.Sleep(350);
            }
        }
        private void thingMagicReaderToolStripMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                try
                {
                    string software = (string)rdr.ParamGet("/reader/version/software");
                    MessageBox.Show(string.Concat("Hardware: M6e", "  ", "Software: ", software), "About ThingMagic Reader...", MessageBoxButtons.OK);
                }
                catch (NullReferenceException)
                {
                    reader = Reader.Create(string.Concat("tmr:///", readerAddress.Text));
                    reader.Connect();
                    rdr = (SerialReader)reader;
                    string software = (string)rdr.ParamGet("/reader/version/software");
                    MessageBox.Show(string.Concat("Hardware: M6e", "  ", "Software: ", software), "About ThingMagic Reader...", MessageBoxButtons.OK);
                    rdr.Destroy();
                }
            }
            catch
            {
                MessageBox.Show("Connection to ThingMagic Reader not established", "Error!", MessageBoxButtons.OK);
            }
        }

        private void firmwareUpgradeUtilityToolStripMenuItem_Click(object sender, EventArgs e)
        {
            MessageBox.Show("ThingMagic Firmware Upgrade Utility\nVersion 1.2.1.7", "About...", MessageBoxButtons.OK);
        }

        private void pictureBox1_Click(object sender, EventArgs e)
        {
            ToolTip forWebsite = new ToolTip();
            forWebsite.SetToolTip(pictureBox1, "www.thingmagic.com");
            System.Diagnostics.Process.Start("http://www.thingmagic.com/manuals-firmware");      
        }

        private void Form1_Load(object sender, EventArgs e)
        {

        }

        private void label6_Click(object sender, EventArgs e)
        {

        }

        private void copyFirmwareToLocalToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (firmwarePath.Text == "")
            {
                MessageBox.Show("Please select a firmware to load", "Error!", MessageBoxButtons.OK);
            }
            else
            {
                folderBrowserDialog1.ShowDialog();
                string sourceFile = firmwarePath.Text;
                string targetPath = folderBrowserDialog1.SelectedPath;
                char[] sep = new char[1];
                sep[0] = '\\';
                string destFile = System.IO.Path.Combine(targetPath, sourceFile.Split(sep)[sourceFile.Split(sep).Length - 1]);
                System.IO.File.Copy(sourceFile, destFile, true);
            }
        }

        private void folderBrowserDialog1_HelpRequest(object sender, EventArgs e)
        {

        }

        private void upgradeAssistantToolStripMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                System.Reflection.Assembly execAssembly = System.Reflection.Assembly.GetExecutingAssembly();
                string path = Path.GetDirectoryName(execAssembly.Location);
                System.Diagnostics.Process.Start(path + "\\Readme.txt");
            }
            catch (System.ComponentModel.Win32Exception)
            {
                MessageBox.Show("Help file missing! Please re-compile the source code", "Error!", MessageBoxButtons.OK);

            }

        }

       
    }
}

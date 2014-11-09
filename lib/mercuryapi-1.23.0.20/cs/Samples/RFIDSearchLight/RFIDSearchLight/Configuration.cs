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
    public partial class Configuration : Form
    {
        private Dictionary<string, string> properties;

        public Dictionary<string, string> Properties
        {
            get { return properties; }
            set { properties = value; }
        }

        public Configuration()
        {
            InitializeComponent();
        }
        private NotifyIcon pvtNotifyIcon;
        public Configuration(NotifyIcon notifyIcon)
        {
            InitializeComponent();
            pvtNotifyIcon = notifyIcon;
        }

        private void menuItem2_Click(object sender, EventArgs e)
        {
            //AdvancedReaderSettings ars = new AdvancedReaderSettings();
            //ars.Show();
            pvtNotifyIcon.Remove();
            Application.Exit();
        }

        private void menuItem1_Click(object sender, EventArgs e)
        {
            this.Close();            
        }

        private void btnDisplayFormat_Click(object sender, EventArgs e)
        {
            DisplayFormat objDisplayFormat = new DisplayFormat();
            objDisplayFormat.Show();
        }

        private void btnReadParameters_Click(object sender, EventArgs e)
        {
            ReadParameters objReadParameters = new ReadParameters();
            objReadParameters.Show();
        }

        private void btnPrefixSuffix_Click(object sender, EventArgs e)
        {
            PrefixSuffix objPrefixSuffix = new PrefixSuffix();
            objPrefixSuffix.Show();
        }

        private void btnReaderSettings_Click(object sender, EventArgs e)
        {
            ReaderSettings objReaderSettings = new ReaderSettings();
            objReaderSettings.Show();
        }
    }
}
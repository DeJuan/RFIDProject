using System;
using System.Linq;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using ThingMagic;

namespace ThingMagic.RFIDSearchLight
{
    public partial class RegionSelection : Form
    {
        Reader objReader;
        private Dictionary<string, string> properties;
        Reader.Region[] regionsToBind;
        bool selectionMade = false;

        public RegionSelection()
        {
            InitializeComponent();
        }
        public RegionSelection(Reader reader,Reader.Region[] regions)
        {
            InitializeComponent();
            objReader = reader;
            regionsToBind = regions;
        }

        private void Region_Load(object sender, EventArgs e)
        {
            cbRegions.DataSource = regionsToBind;
            properties = Utilities.GetProperties();
        }

        private void btnOk_Click(object sender, EventArgs e)
        {
            properties["region"] = cbRegions.SelectedItem.ToString();
            Utilities.SaveConfigurations(properties);
            selectionMade = true;
            this.Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void RegionSelection_Closed(object sender, EventArgs e)
        {
            if (false == selectionMade)
            {
                Application.Exit();
            }
        }
    }
}
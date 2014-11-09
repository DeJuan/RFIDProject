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
    public partial class About : Form
    {
        public About(string text)
        {
            InitializeComponent();
            AboutText = text;
        }

        public string AboutText
        {
            set { aboutTextBox.Text = value; }
            get { return aboutTextBox.Text; }
        }
    }
}
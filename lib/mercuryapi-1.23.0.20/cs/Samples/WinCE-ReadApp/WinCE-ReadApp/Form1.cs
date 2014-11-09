using System;
using System.Linq;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using ThingMagic;

namespace WinceReadApp
{
    public partial class Form1 : Form
    {
        public Reader objReader = null;
        private ReadTags[] arr1 = new ReadTags[0];
        private ReadTags[] arr = null;
        public Form1()
        {
            InitializeComponent();
            UpdateComport();
            dataGrid1.DataSource = arr1;
            generatedatagrid();
        }
        public void generatedatagrid()
        {
            DataGridTableStyle tableStyle = new DataGridTableStyle();
            tableStyle.MappingName = arr1.GetType().Name;

            DataGridTextBoxColumn tbcName = new DataGridTextBoxColumn();
            tbcName.Width = dataGrid1.Width * 50 / 100;
            tbcName.MappingName = "EPC";
            tbcName.HeaderText = "EPC";
            tableStyle.GridColumnStyles.Add(tbcName);

            tbcName = new DataGridTextBoxColumn();
            tbcName.Width = dataGrid1.Width * 20 / 100;
            tbcName.MappingName = "Timestamp";
            tbcName.HeaderText = "Timestamp";
            tableStyle.GridColumnStyles.Add(tbcName);

            tbcName = new DataGridTextBoxColumn();
            tbcName.Width = dataGrid1.Width * 15 / 100;
            tbcName.MappingName = "ReadCount";
            tbcName.HeaderText = "ReadCount";
            tableStyle.GridColumnStyles.Add(tbcName);

            tbcName = new DataGridTextBoxColumn();
            tbcName.Width = dataGrid1.Width * 12 / 100;
            tbcName.MappingName = "Rssi";
            tbcName.HeaderText = "Rssi";
            tableStyle.GridColumnStyles.Add(tbcName);

            dataGrid1.TableStyles.Clear();
            dataGrid1.TableStyles.Add(tableStyle);
        }
        private void UpdateComport()
        {
            string[] comport = null;
            comboBox1.Items.Clear();
            comport = System.IO.Ports.SerialPort.GetPortNames();
            foreach (string comport1 in comport)
            {
                comboBox1.Items.Add(comport1);
            }
            comboBox1.SelectedIndex = 0;
        }

        private void btnReadOnce_Click(object sender, EventArgs e)
        {
            try
            {
                int TotalTagCount = 0;
                btnReadOnce.Enabled = false;
                TagReadData[] tagID = objReader.Read(int.Parse(tbxReadTimeout.Text));
                arr = new ReadTags[tagID.Length];
                for (int i = 0; i < tagID.Length; i++)
                {
                    arr[i] = new ReadTags(tagID[i]);
                    TotalTagCount = TotalTagCount + tagID[i].ReadCount;
                }
                dataGrid1.DataSource = arr;
                generatedatagrid();
                lblTotalTagCount.Text = TotalTagCount.ToString();
                lblUniqueTagCount.Text = tagID.Length.ToString();
                btnReadOnce.Enabled = true;
                btnReadOnce.Focus();

            }
            catch (IOException ex)
            {
                MessageBox.Show(ex.Message, "Error");
                objReader.Destroy();
                objReader = null;
                btnReadOnce.Enabled = false;
                btnConnect.Text = "Connect";
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Reader Message");
                btnReadOnce.Enabled = true;
            }

        }

        private void btnConnect_Click(object sender, EventArgs e)
        {

            try
            {
                if (btnConnect.Text.Equals("Connect"))
                {
                    String model = string.Empty;
                    string readeruri = comboBox1.SelectedItem.ToString();
                    objReader = Reader.Create(string.Concat("eapi:///", readeruri));
                    objReader.Connect();
                    model = (string)objReader.ParamGet("/reader/version/model");
                    Reader.Region regionToSet = (Reader.Region)objReader.ParamGet("/reader/region/id");
                    if (objReader is SerialReader)
                    {
                        if (regionToSet == Reader.Region.UNSPEC)
                        {
                            if (model.Equals("M6e PRC"))
                            {
                                regionToSet = Reader.Region.PRC;
                            }
                            else
                            {
                                regionToSet = Reader.Region.NA;
                            }
                        }
                    }
                    objReader.ParamSet("/reader/region/id", regionToSet);

                    if (model.Equals("M6e Micro"))
                    {
                        SimpleReadPlan plan = new SimpleReadPlan(new int[] { 1, 2 }, TagProtocol.GEN2);
                        objReader.ParamSet("/reader/read/plan", plan);
                    }
                    btnConnect.Text = "Disconnect";
                    btnReadOnce.Enabled = true;
                    comboBox1.Enabled = false;
                    btnRefresh.Enabled = false;
                    lblReadTimeout.Enabled = true;
                    tbxReadTimeout.Enabled = true;
                    UpdateGrid();
                }
                else
                {
                    objReader.Destroy();
                    objReader = null;
                    btnReadOnce.Enabled = false;
                    comboBox1.Enabled = true;
                    lblReadTimeout.Enabled = false;
                    tbxReadTimeout.Enabled = false;
                    btnConnect.Text = "Connect";
                    btnRefresh.Enabled = true;
                    lblTotalTagCount.Text = "0";
                    lblUniqueTagCount.Text = "0";
                }
            }
            catch (IOException ex)
            {
                MessageBox.Show(ex.Message, "Error");
                objReader.Destroy();
                objReader = null;
                btnReadOnce.Enabled = false;
                btnConnect.Text = "Connect";
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Reader message");
            }

        }

        private void btnRefresh_Click(object sender, EventArgs e)
        {

            UpdateComport();
        }

        private void btnClear_Click(object sender, EventArgs e)
        {
            UpdateGrid();
        }

        private void dataGrid1_Resize(object sender, EventArgs e)
        {
            generatedatagrid();
        }
        private void UpdateGrid()
        {
            dataGrid1.DataSource = null;
            dataGrid1.DataSource = arr1;
            lblTotalTagCount.Text = "0";
            lblUniqueTagCount.Text = "0";
        }

        private void tbxReadTimeout_TextChanged(object sender, EventArgs e)
        {
            try
            {
                if (Convert.ToInt32(tbxReadTimeout.Text) >= 0)
                {
                    objReader.ParamSet("/reader/transportTimeout", int.Parse(tbxReadTimeout.Text) + 5000);
                }
            }
            catch { }
        }

        private void tbxReadTimeout_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (char.IsDigit(e.KeyChar) || char.IsControl(e.KeyChar))
            {
                e.Handled = false;
            }
            else
            {
                e.Handled = true;
            } 
        }

        private void tbxReadTimeout_LostFocus(object sender, EventArgs e)
        {

            if (tbxReadTimeout.Text != "")
            {
                if ((bool)btnReadOnce.Enabled)
                {
                    if (Convert.ToInt32(tbxReadTimeout.Text) < 30)
                    {
                        MessageBox.Show("Please input read once timeout greater then 30 ms",
                            "Error", MessageBoxButtons.OK,MessageBoxIcon.Exclamation,MessageBoxDefaultButton.Button1);
                        tbxReadTimeout.Text = "500";
                        return;
                    }
                }
            }
            else
            {
                MessageBox.Show("Timeout (ms) can't be empty", "Universal Reader Assistant Message",
                    MessageBoxButtons.OK,MessageBoxIcon.Exclamation,MessageBoxDefaultButton.Button1);
                tbxReadTimeout.Text = "500";
            }
        }
    }
    public class ReadTags
    {
        private string epc;
        private string timestamp;
        private string readcount;
        public string rssi;

        public ReadTags()
        {
        }
        public ReadTags(TagReadData adddata)
        {
            epc = adddata.EpcString;
            timestamp = adddata.Time.ToString();
            readcount = adddata.ReadCount.ToString();
            rssi = adddata.Rssi.ToString();
        }
        public string EPC
        {
            get { return epc; }
        }
        public string Timestamp
        {
            get { return timestamp; }
        }
        public string ReadCount
        {
            get { return readcount; }
        }
        public string Rssi
        {
            get { return rssi; }
        }

    }
}
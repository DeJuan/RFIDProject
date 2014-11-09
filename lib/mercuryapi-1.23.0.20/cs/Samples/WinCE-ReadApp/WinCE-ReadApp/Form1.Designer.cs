namespace WinceReadApp
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            this.panel1 = new System.Windows.Forms.Panel();
            this.lblReadTimeout = new System.Windows.Forms.Label();
            this.tbxReadTimeout = new System.Windows.Forms.TextBox();
            this.lblUniqueTagCount = new System.Windows.Forms.Label();
            this.lblTotalTagCount = new System.Windows.Forms.Label();
            this.btnClear = new System.Windows.Forms.Button();
            this.btnRefresh = new System.Windows.Forms.Button();
            this.btnConnect = new System.Windows.Forms.Button();
            this.comboBox1 = new System.Windows.Forms.ComboBox();
            this.label3 = new System.Windows.Forms.Label();
            this.lblUTC = new System.Windows.Forms.Label();
            this.lblTTC = new System.Windows.Forms.Label();
            this.btnReadOnce = new System.Windows.Forms.Button();
            this.dataGrid1 = new System.Windows.Forms.DataGrid();
            this.panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.lblReadTimeout);
            this.panel1.Controls.Add(this.tbxReadTimeout);
            this.panel1.Controls.Add(this.lblUniqueTagCount);
            this.panel1.Controls.Add(this.lblTotalTagCount);
            this.panel1.Controls.Add(this.btnClear);
            this.panel1.Controls.Add(this.btnRefresh);
            this.panel1.Controls.Add(this.btnConnect);
            this.panel1.Controls.Add(this.comboBox1);
            this.panel1.Controls.Add(this.label3);
            this.panel1.Controls.Add(this.lblUTC);
            this.panel1.Controls.Add(this.lblTTC);
            this.panel1.Controls.Add(this.btnReadOnce);
            this.panel1.Location = new System.Drawing.Point(3, 3);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(632, 65);
            // 
            // lblReadTimeout
            // 
            this.lblReadTimeout.Enabled = false;
            this.lblReadTimeout.Location = new System.Drawing.Point(3, 32);
            this.lblReadTimeout.Name = "lblReadTimeout";
            this.lblReadTimeout.Size = new System.Drawing.Size(113, 20);
            this.lblReadTimeout.Text = "ReadTimeout(ms)";
            this.lblReadTimeout.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // tbxReadTimeout
            // 
            this.tbxReadTimeout.Enabled = false;
            this.tbxReadTimeout.Location = new System.Drawing.Point(122, 29);
            this.tbxReadTimeout.Name = "tbxReadTimeout";
            this.tbxReadTimeout.Size = new System.Drawing.Size(39, 23);
            this.tbxReadTimeout.TabIndex = 13;
            this.tbxReadTimeout.Text = "500";
            this.tbxReadTimeout.TextChanged += new System.EventHandler(this.tbxReadTimeout_TextChanged);
            this.tbxReadTimeout.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.tbxReadTimeout_KeyPress);
            this.tbxReadTimeout.LostFocus += new System.EventHandler(this.tbxReadTimeout_LostFocus);
            // 
            // lblUniqueTagCount
            // 
            this.lblUniqueTagCount.Location = new System.Drawing.Point(275, 26);
            this.lblUniqueTagCount.Name = "lblUniqueTagCount";
            this.lblUniqueTagCount.Size = new System.Drawing.Size(44, 20);
            this.lblUniqueTagCount.Text = "0";
            // 
            // lblTotalTagCount
            // 
            this.lblTotalTagCount.Location = new System.Drawing.Point(275, 6);
            this.lblTotalTagCount.Name = "lblTotalTagCount";
            this.lblTotalTagCount.Size = new System.Drawing.Size(45, 20);
            this.lblTotalTagCount.Text = "0";
            // 
            // btnClear
            // 
            this.btnClear.Location = new System.Drawing.Point(500, 32);
            this.btnClear.Name = "btnClear";
            this.btnClear.Size = new System.Drawing.Size(72, 20);
            this.btnClear.TabIndex = 9;
            this.btnClear.Text = "Clear";
            this.btnClear.Click += new System.EventHandler(this.btnClear_Click);
            // 
            // btnRefresh
            // 
            this.btnRefresh.Location = new System.Drawing.Point(408, 32);
            this.btnRefresh.Name = "btnRefresh";
            this.btnRefresh.Size = new System.Drawing.Size(72, 20);
            this.btnRefresh.TabIndex = 8;
            this.btnRefresh.Text = "Refresh";
            this.btnRefresh.Click += new System.EventHandler(this.btnRefresh_Click);
            // 
            // btnConnect
            // 
            this.btnConnect.Location = new System.Drawing.Point(500, 4);
            this.btnConnect.Name = "btnConnect";
            this.btnConnect.Size = new System.Drawing.Size(72, 20);
            this.btnConnect.TabIndex = 7;
            this.btnConnect.Text = "Connect";
            this.btnConnect.Click += new System.EventHandler(this.btnConnect_Click);
            // 
            // comboBox1
            // 
            this.comboBox1.Location = new System.Drawing.Point(394, 4);
            this.comboBox1.Name = "comboBox1";
            this.comboBox1.Size = new System.Drawing.Size(100, 23);
            this.comboBox1.TabIndex = 6;
            // 
            // label3
            // 
            this.label3.Location = new System.Drawing.Point(325, 6);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(77, 20);
            this.label3.Text = "ReaderPort";
            // 
            // lblUTC
            // 
            this.lblUTC.Location = new System.Drawing.Point(158, 26);
            this.lblUTC.Name = "lblUTC";
            this.lblUTC.Size = new System.Drawing.Size(120, 20);
            this.lblUTC.Text = "UniqueTagCount :";
            this.lblUTC.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // lblTTC
            // 
            this.lblTTC.Location = new System.Drawing.Point(171, 6);
            this.lblTTC.Name = "lblTTC";
            this.lblTTC.Size = new System.Drawing.Size(107, 20);
            this.lblTTC.Text = "TotalTagCount :";
            this.lblTTC.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // btnReadOnce
            // 
            this.btnReadOnce.Enabled = false;
            this.btnReadOnce.Location = new System.Drawing.Point(3, 7);
            this.btnReadOnce.Name = "btnReadOnce";
            this.btnReadOnce.Size = new System.Drawing.Size(158, 20);
            this.btnReadOnce.TabIndex = 0;
            this.btnReadOnce.Text = "ReadOnce";
            this.btnReadOnce.Click += new System.EventHandler(this.btnReadOnce_Click);
            // 
            // dataGrid1
            // 
            this.dataGrid1.BackgroundColor = System.Drawing.Color.FromArgb(((int)(((byte)(128)))), ((int)(((byte)(128)))), ((int)(((byte)(128)))));
            this.dataGrid1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.dataGrid1.Location = new System.Drawing.Point(0, 74);
            this.dataGrid1.Name = "dataGrid1";
            this.dataGrid1.Size = new System.Drawing.Size(638, 336);
            this.dataGrid1.TabIndex = 1;
            this.dataGrid1.Resize += new System.EventHandler(this.dataGrid1_Resize);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(638, 410);
            this.Controls.Add(this.dataGrid1);
            this.Controls.Add(this.panel1);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "Form1";
            this.Text = "Wince_ReadApp";
            this.panel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Label lblUTC;
        private System.Windows.Forms.Label lblTTC;
        private System.Windows.Forms.Button btnReadOnce;
        private System.Windows.Forms.ComboBox comboBox1;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Button btnClear;
        private System.Windows.Forms.Button btnRefresh;
        private System.Windows.Forms.Button btnConnect;
        private System.Windows.Forms.DataGrid dataGrid1;
        private System.Windows.Forms.Label lblUniqueTagCount;
        private System.Windows.Forms.Label lblTotalTagCount;
        private System.Windows.Forms.Label lblReadTimeout;
        private System.Windows.Forms.TextBox tbxReadTimeout;
    }
}


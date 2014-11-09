namespace ThingMagic.RFIDSearchLight
{
    partial class ReadParameters
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;
        private System.Windows.Forms.MainMenu mainMenu1;

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
            this.mainMenu1 = new System.Windows.Forms.MainMenu();
            this.panel1 = new System.Windows.Forms.Panel();
            this.txtScanDuration = new System.Windows.Forms.TextBox();
            this.lblScanDuration = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.btnChooseEndScan = new System.Windows.Forms.Button();
            this.btnChooseStartScan = new System.Windows.Forms.Button();
            this.btnChooseDecode = new System.Windows.Forms.Button();
            this.txtEndScanFile = new System.Windows.Forms.TextBox();
            this.txtStartScanFile = new System.Windows.Forms.TextBox();
            this.txtDecodeFile = new System.Windows.Forms.TextBox();
            this.label5 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.cbAudibleAlerts = new System.Windows.Forms.ComboBox();
            this.btnCancel = new System.Windows.Forms.Button();
            this.btnSave = new System.Windows.Forms.Button();
            this.btnRestoreDefaults = new System.Windows.Forms.Button();
            this.panel2 = new System.Windows.Forms.Panel();
            this.chkRSSI = new System.Windows.Forms.CheckBox();
            this.chkPosition = new System.Windows.Forms.CheckBox();
            this.chkTimeStamp = new System.Windows.Forms.CheckBox();
            this.label2 = new System.Windows.Forms.Label();
            this.cbSeparator = new System.Windows.Forms.ComboBox();
            this.label1 = new System.Windows.Forms.Label();
            this.lblMute = new System.Windows.Forms.Label();
            this.panel1.SuspendLayout();
            this.panel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.txtScanDuration);
            this.panel1.Controls.Add(this.lblScanDuration);
            this.panel1.Controls.Add(this.label7);
            this.panel1.Controls.Add(this.label6);
            this.panel1.Controls.Add(this.btnChooseEndScan);
            this.panel1.Controls.Add(this.btnChooseStartScan);
            this.panel1.Controls.Add(this.btnChooseDecode);
            this.panel1.Controls.Add(this.txtEndScanFile);
            this.panel1.Controls.Add(this.txtStartScanFile);
            this.panel1.Controls.Add(this.txtDecodeFile);
            this.panel1.Controls.Add(this.label5);
            this.panel1.Controls.Add(this.label4);
            this.panel1.Controls.Add(this.label3);
            this.panel1.Controls.Add(this.cbAudibleAlerts);
            this.panel1.Controls.Add(this.btnCancel);
            this.panel1.Controls.Add(this.btnSave);
            this.panel1.Controls.Add(this.btnRestoreDefaults);
            this.panel1.Controls.Add(this.panel2);
            this.panel1.Controls.Add(this.cbSeparator);
            this.panel1.Controls.Add(this.label1);
            this.panel1.Controls.Add(this.lblMute);
            this.panel1.Location = new System.Drawing.Point(0, 0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(227, 309);
            // 
            // txtScanDuration
            // 
            this.txtScanDuration.Location = new System.Drawing.Point(172, 137);
            this.txtScanDuration.Name = "txtScanDuration";
            this.txtScanDuration.Size = new System.Drawing.Size(46, 21);
            this.txtScanDuration.TabIndex = 84;
            this.txtScanDuration.Text = "3";
            this.txtScanDuration.TextChanged += new System.EventHandler(this.SettingsChanged);
            // 
            // lblScanDuration
            // 
            this.lblScanDuration.Location = new System.Drawing.Point(5, 137);
            this.lblScanDuration.Name = "lblScanDuration";
            this.lblScanDuration.Size = new System.Drawing.Size(151, 20);
            this.lblScanDuration.Text = "Scan Duration in Seconds";
            // 
            // label7
            // 
            this.label7.Location = new System.Drawing.Point(1, 100);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(226, 10);
            this.label7.Text = "-----------------------------------------------------------------------";
            // 
            // label6
            // 
            this.label6.Font = new System.Drawing.Font("Tahoma", 11F, System.Drawing.FontStyle.Regular);
            this.label6.ForeColor = System.Drawing.Color.RoyalBlue;
            this.label6.Location = new System.Drawing.Point(5, 111);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(177, 23);
            this.label6.Text = "Keyboard Wedge Settings";
            // 
            // btnChooseEndScan
            // 
            this.btnChooseEndScan.Location = new System.Drawing.Point(206, 78);
            this.btnChooseEndScan.Name = "btnChooseEndScan";
            this.btnChooseEndScan.Size = new System.Drawing.Size(21, 20);
            this.btnChooseEndScan.TabIndex = 76;
            this.btnChooseEndScan.Text = "...";
            this.btnChooseEndScan.Click += new System.EventHandler(this.btnChooseEndScan_Click);
            // 
            // btnChooseStartScan
            // 
            this.btnChooseStartScan.Location = new System.Drawing.Point(206, 56);
            this.btnChooseStartScan.Name = "btnChooseStartScan";
            this.btnChooseStartScan.Size = new System.Drawing.Size(21, 20);
            this.btnChooseStartScan.TabIndex = 75;
            this.btnChooseStartScan.Text = "...";
            this.btnChooseStartScan.Click += new System.EventHandler(this.btnChooseStartScan_Click);
            // 
            // btnChooseDecode
            // 
            this.btnChooseDecode.Location = new System.Drawing.Point(206, 30);
            this.btnChooseDecode.Name = "btnChooseDecode";
            this.btnChooseDecode.Size = new System.Drawing.Size(21, 20);
            this.btnChooseDecode.TabIndex = 74;
            this.btnChooseDecode.Text = "...";
            this.btnChooseDecode.Click += new System.EventHandler(this.btnChooseDecode_Click);
            // 
            // txtEndScanFile
            // 
            this.txtEndScanFile.Location = new System.Drawing.Point(116, 78);
            this.txtEndScanFile.Name = "txtEndScanFile";
            this.txtEndScanFile.Size = new System.Drawing.Size(88, 21);
            this.txtEndScanFile.TabIndex = 73;
            this.txtEndScanFile.Text = "\\windows\\Splat.wav";
            this.txtEndScanFile.TextChanged += new System.EventHandler(this.SettingsChanged);
            // 
            // txtStartScanFile
            // 
            this.txtStartScanFile.Location = new System.Drawing.Point(116, 54);
            this.txtStartScanFile.Name = "txtStartScanFile";
            this.txtStartScanFile.Size = new System.Drawing.Size(88, 21);
            this.txtStartScanFile.TabIndex = 72;
            this.txtStartScanFile.Text = "\\windows\\Quietest.wav";
            this.txtStartScanFile.TextChanged += new System.EventHandler(this.SettingsChanged);
            // 
            // txtDecodeFile
            // 
            this.txtDecodeFile.Location = new System.Drawing.Point(116, 29);
            this.txtDecodeFile.Name = "txtDecodeFile";
            this.txtDecodeFile.Size = new System.Drawing.Size(88, 21);
            this.txtDecodeFile.TabIndex = 71;
            this.txtDecodeFile.Text = "\\window\\Loudest.wav";
            this.txtDecodeFile.TextChanged += new System.EventHandler(this.SettingsChanged);
            // 
            // label5
            // 
            this.label5.Location = new System.Drawing.Point(0, 81);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(111, 20);
            this.label5.Text = "End Scan Wav File";
            // 
            // label4
            // 
            this.label4.Location = new System.Drawing.Point(0, 55);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(111, 20);
            this.label4.Text = "Start Scan Wav File";
            // 
            // label3
            // 
            this.label3.Location = new System.Drawing.Point(0, 30);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(111, 20);
            this.label3.Text = "Read Alert Wav File";
            // 
            // cbAudibleAlerts
            // 
            this.cbAudibleAlerts.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDown;
            this.cbAudibleAlerts.Items.Add("Select");
            this.cbAudibleAlerts.Items.Add("Yes");
            this.cbAudibleAlerts.Items.Add("No");
            this.cbAudibleAlerts.Location = new System.Drawing.Point(159, 3);
            this.cbAudibleAlerts.Name = "cbAudibleAlerts";
            this.cbAudibleAlerts.Size = new System.Drawing.Size(59, 22);
            this.cbAudibleAlerts.TabIndex = 54;
            this.cbAudibleAlerts.SelectedIndexChanged += new System.EventHandler(this.SettingsChanged);
            this.cbAudibleAlerts.TextChanged += new System.EventHandler(this.SettingsChanged);
            // 
            // btnCancel
            // 
            this.btnCancel.Location = new System.Drawing.Point(112, 261);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(70, 20);
            this.btnCancel.TabIndex = 53;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // btnSave
            // 
            this.btnSave.Location = new System.Drawing.Point(35, 261);
            this.btnSave.Name = "btnSave";
            this.btnSave.Size = new System.Drawing.Size(70, 20);
            this.btnSave.TabIndex = 52;
            this.btnSave.Text = "Save";
            this.btnSave.Click += new System.EventHandler(this.btnSave_Click);
            // 
            // btnRestoreDefaults
            // 
            this.btnRestoreDefaults.Location = new System.Drawing.Point(35, 285);
            this.btnRestoreDefaults.Name = "btnRestoreDefaults";
            this.btnRestoreDefaults.Size = new System.Drawing.Size(147, 20);
            this.btnRestoreDefaults.TabIndex = 51;
            this.btnRestoreDefaults.Text = "Restore Defaults";
            this.btnRestoreDefaults.Click += new System.EventHandler(this.btnRestoreDefaults_Click);
            // 
            // panel2
            // 
            this.panel2.Controls.Add(this.chkRSSI);
            this.panel2.Controls.Add(this.chkPosition);
            this.panel2.Controls.Add(this.chkTimeStamp);
            this.panel2.Controls.Add(this.label2);
            this.panel2.Location = new System.Drawing.Point(1, 192);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(219, 64);
            // 
            // chkRSSI
            // 
            this.chkRSSI.Location = new System.Drawing.Point(99, 16);
            this.chkRSSI.Name = "chkRSSI";
            this.chkRSSI.Size = new System.Drawing.Size(100, 20);
            this.chkRSSI.TabIndex = 13;
            this.chkRSSI.Text = "RSSI";
            this.chkRSSI.CheckStateChanged += new System.EventHandler(this.SettingsChanged);
            // 
            // chkPosition
            // 
            this.chkPosition.Location = new System.Drawing.Point(4, 42);
            this.chkPosition.Name = "chkPosition";
            this.chkPosition.Size = new System.Drawing.Size(187, 20);
            this.chkPosition.TabIndex = 12;
            this.chkPosition.Text = "Position(Lat, Long)";
            this.chkPosition.CheckStateChanged += new System.EventHandler(this.SettingsChanged);
            // 
            // chkTimeStamp
            // 
            this.chkTimeStamp.Location = new System.Drawing.Point(4, 16);
            this.chkTimeStamp.Name = "chkTimeStamp";
            this.chkTimeStamp.Size = new System.Drawing.Size(100, 20);
            this.chkTimeStamp.TabIndex = 11;
            this.chkTimeStamp.Text = "Time Stamp";
            this.chkTimeStamp.CheckStateChanged += new System.EventHandler(this.SettingsChanged);
            // 
            // label2
            // 
            this.label2.Location = new System.Drawing.Point(1, -3);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(151, 15);
            this.label2.Text = "Tag Metadata to Display";
            // 
            // cbSeparator
            // 
            this.cbSeparator.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDown;
            this.cbSeparator.Items.Add("Select");
            this.cbSeparator.Items.Add("Comma");
            this.cbSeparator.Items.Add("Space");
            this.cbSeparator.Location = new System.Drawing.Point(151, 164);
            this.cbSeparator.Name = "cbSeparator";
            this.cbSeparator.Size = new System.Drawing.Size(69, 22);
            this.cbSeparator.TabIndex = 50;
            this.cbSeparator.SelectedIndexChanged += new System.EventHandler(this.SettingsChanged);
            this.cbSeparator.TextChanged += new System.EventHandler(this.SettingsChanged);
            // 
            // label1
            // 
            this.label1.Location = new System.Drawing.Point(8, 166);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(130, 20);
            this.label1.Text = "Metadata Separator";
            // 
            // lblMute
            // 
            this.lblMute.Location = new System.Drawing.Point(0, 5);
            this.lblMute.Name = "lblMute";
            this.lblMute.Size = new System.Drawing.Size(100, 20);
            this.lblMute.Text = "Audible Alerts";
            // 
            // ReadParameters
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(240, 268);
            this.Controls.Add(this.panel1);
            this.Menu = this.mainMenu1;
            this.MinimizeBox = false;
            this.Name = "ReadParameters";
            this.Text = "Read Parameters";
            this.Load += new System.EventHandler(this.ReadParameters_Load);
            this.Closing += new System.ComponentModel.CancelEventHandler(this.ReadParameters_Closing);
            this.panel1.ResumeLayout(false);
            this.panel2.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.ComboBox cbAudibleAlerts;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Button btnSave;
        private System.Windows.Forms.Button btnRestoreDefaults;
        private System.Windows.Forms.Panel panel2;
        private System.Windows.Forms.CheckBox chkRSSI;
        private System.Windows.Forms.CheckBox chkPosition;
        private System.Windows.Forms.CheckBox chkTimeStamp;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.ComboBox cbSeparator;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label lblMute;
        private System.Windows.Forms.Button btnChooseEndScan;
        private System.Windows.Forms.Button btnChooseStartScan;
        private System.Windows.Forms.Button btnChooseDecode;
        private System.Windows.Forms.TextBox txtEndScanFile;
        private System.Windows.Forms.TextBox txtStartScanFile;
        private System.Windows.Forms.TextBox txtDecodeFile;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.TextBox txtScanDuration;
        private System.Windows.Forms.Label lblScanDuration;

    }
}
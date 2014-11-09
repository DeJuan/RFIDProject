namespace ThingMagic.RFIDSearchLight
{
    partial class AdvancedReaderSettings
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
            this.menuItem1 = new System.Windows.Forms.MenuItem();
            this.menuItem2 = new System.Windows.Forms.MenuItem();
            this.pnlAdvancedRdrSettings = new System.Windows.Forms.Panel();
            this.txtRFOff = new System.Windows.Forms.TextBox();
            this.lblRFOff = new System.Windows.Forms.Label();
            this.txtRFOn = new System.Windows.Forms.TextBox();
            this.lblRFOn = new System.Windows.Forms.Label();
            this.rbRecordHighestRSSI = new System.Windows.Forms.RadioButton();
            this.rbRecordRSSIoflastTagRead = new System.Windows.Forms.RadioButton();
            this.btnCancel = new System.Windows.Forms.Button();
            this.btnSave = new System.Windows.Forms.Button();
            this.btnRestoreDefaults = new System.Windows.Forms.Button();
            this.label2 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.lblQ = new System.Windows.Forms.Label();
            this.pnlQ = new System.Windows.Forms.Panel();
            this.cbStaticQ = new System.Windows.Forms.ComboBox();
            this.rbStaticQ = new System.Windows.Forms.RadioButton();
            this.rbDynamicQ = new System.Windows.Forms.RadioButton();
            this.lblTarget = new System.Windows.Forms.Label();
            this.pnlTarget = new System.Windows.Forms.Panel();
            this.rbBA = new System.Windows.Forms.RadioButton();
            this.rbAB = new System.Windows.Forms.RadioButton();
            this.rbB = new System.Windows.Forms.RadioButton();
            this.rbA = new System.Windows.Forms.RadioButton();
            this.pnlSession = new System.Windows.Forms.Panel();
            this.rbS3 = new System.Windows.Forms.RadioButton();
            this.rbS2 = new System.Windows.Forms.RadioButton();
            this.rbS1 = new System.Windows.Forms.RadioButton();
            this.rbS0 = new System.Windows.Forms.RadioButton();
            this.pnlTagEncoding = new System.Windows.Forms.Panel();
            this.rbM8 = new System.Windows.Forms.RadioButton();
            this.rbM4 = new System.Windows.Forms.RadioButton();
            this.rbM2 = new System.Windows.Forms.RadioButton();
            this.rbFM0 = new System.Windows.Forms.RadioButton();
            this.pnlAdvancedRdrSettings.SuspendLayout();
            this.pnlQ.SuspendLayout();
            this.pnlTarget.SuspendLayout();
            this.pnlSession.SuspendLayout();
            this.pnlTagEncoding.SuspendLayout();
            this.SuspendLayout();
            // 
            // mainMenu1
            // 
            this.mainMenu1.MenuItems.Add(this.menuItem1);
            this.mainMenu1.MenuItems.Add(this.menuItem2);
            // 
            // menuItem1
            // 
            this.menuItem1.Text = "";
            // 
            // menuItem2
            // 
            this.menuItem2.Text = "";
            // 
            // pnlAdvancedRdrSettings
            // 
            this.pnlAdvancedRdrSettings.AutoScroll = true;
            this.pnlAdvancedRdrSettings.Controls.Add(this.txtRFOff);
            this.pnlAdvancedRdrSettings.Controls.Add(this.lblRFOff);
            this.pnlAdvancedRdrSettings.Controls.Add(this.txtRFOn);
            this.pnlAdvancedRdrSettings.Controls.Add(this.lblRFOn);
            this.pnlAdvancedRdrSettings.Controls.Add(this.rbRecordHighestRSSI);
            this.pnlAdvancedRdrSettings.Controls.Add(this.rbRecordRSSIoflastTagRead);
            this.pnlAdvancedRdrSettings.Controls.Add(this.btnCancel);
            this.pnlAdvancedRdrSettings.Controls.Add(this.btnSave);
            this.pnlAdvancedRdrSettings.Controls.Add(this.btnRestoreDefaults);
            this.pnlAdvancedRdrSettings.Controls.Add(this.label2);
            this.pnlAdvancedRdrSettings.Controls.Add(this.label1);
            this.pnlAdvancedRdrSettings.Controls.Add(this.lblQ);
            this.pnlAdvancedRdrSettings.Controls.Add(this.pnlQ);
            this.pnlAdvancedRdrSettings.Controls.Add(this.lblTarget);
            this.pnlAdvancedRdrSettings.Controls.Add(this.pnlTarget);
            this.pnlAdvancedRdrSettings.Controls.Add(this.pnlSession);
            this.pnlAdvancedRdrSettings.Controls.Add(this.pnlTagEncoding);
            this.pnlAdvancedRdrSettings.Location = new System.Drawing.Point(4, 4);
            this.pnlAdvancedRdrSettings.Name = "pnlAdvancedRdrSettings";
            this.pnlAdvancedRdrSettings.Size = new System.Drawing.Size(233, 261);
            // 
            // txtRFOff
            // 
            this.txtRFOff.Location = new System.Drawing.Point(94, 244);
            this.txtRFOff.Name = "txtRFOff";
            this.txtRFOff.Size = new System.Drawing.Size(33, 21);
            this.txtRFOff.TabIndex = 62;
            this.txtRFOff.Text = "50";
            this.txtRFOff.TextChanged += new System.EventHandler(this.txtRFOff_TextChanged);
            // 
            // lblRFOff
            // 
            this.lblRFOff.Location = new System.Drawing.Point(14, 244);
            this.lblRFOff.Name = "lblRFOff";
            this.lblRFOff.Size = new System.Drawing.Size(70, 20);
            this.lblRFOff.Text = "RF Off (ms)";
            // 
            // txtRFOn
            // 
            this.txtRFOn.Location = new System.Drawing.Point(94, 221);
            this.txtRFOn.Name = "txtRFOn";
            this.txtRFOn.Size = new System.Drawing.Size(33, 21);
            this.txtRFOn.TabIndex = 60;
            this.txtRFOn.Text = "250";
            this.txtRFOn.TextChanged += new System.EventHandler(this.txtRFOn_TextChanged);
            // 
            // lblRFOn
            // 
            this.lblRFOn.Location = new System.Drawing.Point(14, 221);
            this.lblRFOn.Name = "lblRFOn";
            this.lblRFOn.Size = new System.Drawing.Size(70, 20);
            this.lblRFOn.Text = "RF On (ms) ";
            // 
            // rbRecordHighestRSSI
            // 
            this.rbRecordHighestRSSI.Location = new System.Drawing.Point(14, 268);
            this.rbRecordHighestRSSI.Name = "rbRecordHighestRSSI";
            this.rbRecordHighestRSSI.Size = new System.Drawing.Size(143, 20);
            this.rbRecordHighestRSSI.TabIndex = 50;
            this.rbRecordHighestRSSI.Text = "Record Highest RSSI";
            // 
            // rbRecordRSSIoflastTagRead
            // 
            this.rbRecordRSSIoflastTagRead.Location = new System.Drawing.Point(14, 290);
            this.rbRecordRSSIoflastTagRead.Name = "rbRecordRSSIoflastTagRead";
            this.rbRecordRSSIoflastTagRead.Size = new System.Drawing.Size(193, 20);
            this.rbRecordRSSIoflastTagRead.TabIndex = 49;
            this.rbRecordRSSIoflastTagRead.Text = "Record RSSI of last Tag Read";
            // 
            // btnCancel
            // 
            this.btnCancel.Location = new System.Drawing.Point(123, 314);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(61, 20);
            this.btnCancel.TabIndex = 40;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // btnSave
            // 
            this.btnSave.Location = new System.Drawing.Point(37, 314);
            this.btnSave.Name = "btnSave";
            this.btnSave.Size = new System.Drawing.Size(60, 20);
            this.btnSave.TabIndex = 39;
            this.btnSave.Text = "Save";
            this.btnSave.Click += new System.EventHandler(this.btnSave_Click);
            // 
            // btnRestoreDefaults
            // 
            this.btnRestoreDefaults.Location = new System.Drawing.Point(37, 340);
            this.btnRestoreDefaults.Name = "btnRestoreDefaults";
            this.btnRestoreDefaults.Size = new System.Drawing.Size(147, 20);
            this.btnRestoreDefaults.TabIndex = 38;
            this.btnRestoreDefaults.Text = "Restore Defaults";
            this.btnRestoreDefaults.Click += new System.EventHandler(this.btnRestoreDefaults_Click);
            // 
            // label2
            // 
            this.label2.Location = new System.Drawing.Point(127, 0);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(81, 20);
            this.label2.Text = "Gen2 Session";
            // 
            // label1
            // 
            this.label1.Location = new System.Drawing.Point(14, -1);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(81, 20);
            this.label1.Text = "Tag Encoding";
            // 
            // lblQ
            // 
            this.lblQ.Location = new System.Drawing.Point(125, 114);
            this.lblQ.Name = "lblQ";
            this.lblQ.Size = new System.Drawing.Size(81, 20);
            this.lblQ.Text = "Gen2 Q";
            // 
            // pnlQ
            // 
            this.pnlQ.BackColor = System.Drawing.Color.White;
            this.pnlQ.Controls.Add(this.cbStaticQ);
            this.pnlQ.Controls.Add(this.rbStaticQ);
            this.pnlQ.Controls.Add(this.rbDynamicQ);
            this.pnlQ.Location = new System.Drawing.Point(126, 122);
            this.pnlQ.Name = "pnlQ";
            this.pnlQ.Size = new System.Drawing.Size(84, 96);
            this.pnlQ.Tag = "Gen2 Q";
            // 
            // cbStaticQ
            // 
            this.cbStaticQ.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDown;
            this.cbStaticQ.Enabled = false;
            this.cbStaticQ.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.cbStaticQ.Items.Add("0");
            this.cbStaticQ.Items.Add("1");
            this.cbStaticQ.Items.Add("2");
            this.cbStaticQ.Items.Add("3");
            this.cbStaticQ.Items.Add("4");
            this.cbStaticQ.Items.Add("5");
            this.cbStaticQ.Items.Add("6");
            this.cbStaticQ.Items.Add("7");
            this.cbStaticQ.Items.Add("8");
            this.cbStaticQ.Items.Add("9");
            this.cbStaticQ.Items.Add("10");
            this.cbStaticQ.Items.Add("11");
            this.cbStaticQ.Items.Add("12");
            this.cbStaticQ.Items.Add("13");
            this.cbStaticQ.Items.Add("14");
            this.cbStaticQ.Items.Add("15");
            this.cbStaticQ.Location = new System.Drawing.Point(7, 55);
            this.cbStaticQ.Name = "cbStaticQ";
            this.cbStaticQ.Size = new System.Drawing.Size(59, 20);
            this.cbStaticQ.TabIndex = 15;
            this.cbStaticQ.SelectedIndexChanged += new System.EventHandler(this.cbStaticQ_SelectedIndexChanged);
            // 
            // rbStaticQ
            // 
            this.rbStaticQ.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.rbStaticQ.Location = new System.Drawing.Point(3, 33);
            this.rbStaticQ.Name = "rbStaticQ";
            this.rbStaticQ.Size = new System.Drawing.Size(80, 15);
            this.rbStaticQ.TabIndex = 14;
            this.rbStaticQ.TabStop = false;
            this.rbStaticQ.Text = "StaticQ";
            this.rbStaticQ.CheckedChanged += new System.EventHandler(this.rbStaticQ_CheckedChanged);
            // 
            // rbDynamicQ
            // 
            this.rbDynamicQ.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.rbDynamicQ.Location = new System.Drawing.Point(3, 13);
            this.rbDynamicQ.Name = "rbDynamicQ";
            this.rbDynamicQ.Size = new System.Drawing.Size(80, 15);
            this.rbDynamicQ.TabIndex = 13;
            this.rbDynamicQ.TabStop = false;
            this.rbDynamicQ.Text = "DynamicQ";
            this.rbDynamicQ.CheckedChanged += new System.EventHandler(this.rbDynamicQ_CheckedChanged);
            // 
            // lblTarget
            // 
            this.lblTarget.Location = new System.Drawing.Point(16, 114);
            this.lblTarget.Name = "lblTarget";
            this.lblTarget.Size = new System.Drawing.Size(81, 20);
            this.lblTarget.Text = "Gen2 Target";
            // 
            // pnlTarget
            // 
            this.pnlTarget.BackColor = System.Drawing.Color.White;
            this.pnlTarget.Controls.Add(this.rbBA);
            this.pnlTarget.Controls.Add(this.rbAB);
            this.pnlTarget.Controls.Add(this.rbB);
            this.pnlTarget.Controls.Add(this.rbA);
            this.pnlTarget.Location = new System.Drawing.Point(14, 122);
            this.pnlTarget.Name = "pnlTarget";
            this.pnlTarget.Size = new System.Drawing.Size(84, 96);
            // 
            // rbBA
            // 
            this.rbBA.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.rbBA.Location = new System.Drawing.Point(3, 75);
            this.rbBA.Name = "rbBA";
            this.rbBA.Size = new System.Drawing.Size(50, 15);
            this.rbBA.TabIndex = 12;
            this.rbBA.TabStop = false;
            this.rbBA.Text = "BA";
            this.rbBA.CheckedChanged += new System.EventHandler(this.rbBA_CheckedChanged);
            // 
            // rbAB
            // 
            this.rbAB.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.rbAB.Location = new System.Drawing.Point(2, 54);
            this.rbAB.Name = "rbAB";
            this.rbAB.Size = new System.Drawing.Size(50, 15);
            this.rbAB.TabIndex = 10;
            this.rbAB.TabStop = false;
            this.rbAB.Text = "AB";
            this.rbAB.CheckedChanged += new System.EventHandler(this.rbAB_CheckedChanged);
            // 
            // rbB
            // 
            this.rbB.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.rbB.Location = new System.Drawing.Point(3, 33);
            this.rbB.Name = "rbB";
            this.rbB.Size = new System.Drawing.Size(50, 15);
            this.rbB.TabIndex = 9;
            this.rbB.TabStop = false;
            this.rbB.Text = "B";
            this.rbB.CheckedChanged += new System.EventHandler(this.rbB_CheckedChanged);
            // 
            // rbA
            // 
            this.rbA.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.rbA.Location = new System.Drawing.Point(3, 13);
            this.rbA.Name = "rbA";
            this.rbA.Size = new System.Drawing.Size(50, 15);
            this.rbA.TabIndex = 8;
            this.rbA.TabStop = false;
            this.rbA.Text = "A";
            this.rbA.CheckedChanged += new System.EventHandler(this.rbA_CheckedChanged);
            // 
            // pnlSession
            // 
            this.pnlSession.BackColor = System.Drawing.Color.White;
            this.pnlSession.Controls.Add(this.rbS3);
            this.pnlSession.Controls.Add(this.rbS2);
            this.pnlSession.Controls.Add(this.rbS1);
            this.pnlSession.Controls.Add(this.rbS0);
            this.pnlSession.Location = new System.Drawing.Point(126, 10);
            this.pnlSession.Name = "pnlSession";
            this.pnlSession.Size = new System.Drawing.Size(84, 96);
            // 
            // rbS3
            // 
            this.rbS3.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.rbS3.Location = new System.Drawing.Point(3, 75);
            this.rbS3.Name = "rbS3";
            this.rbS3.Size = new System.Drawing.Size(50, 15);
            this.rbS3.TabIndex = 6;
            this.rbS3.TabStop = false;
            this.rbS3.Text = "S3";
            this.rbS3.CheckedChanged += new System.EventHandler(this.rbS3_CheckedChanged);
            // 
            // rbS2
            // 
            this.rbS2.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.rbS2.Location = new System.Drawing.Point(2, 54);
            this.rbS2.Name = "rbS2";
            this.rbS2.Size = new System.Drawing.Size(50, 15);
            this.rbS2.TabIndex = 6;
            this.rbS2.TabStop = false;
            this.rbS2.Text = "S2";
            this.rbS2.CheckedChanged += new System.EventHandler(this.rbS2_CheckedChanged);
            // 
            // rbS1
            // 
            this.rbS1.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.rbS1.Location = new System.Drawing.Point(3, 33);
            this.rbS1.Name = "rbS1";
            this.rbS1.Size = new System.Drawing.Size(50, 15);
            this.rbS1.TabIndex = 5;
            this.rbS1.TabStop = false;
            this.rbS1.Text = "S1";
            this.rbS1.CheckedChanged += new System.EventHandler(this.rbS1_CheckedChanged);
            // 
            // rbS0
            // 
            this.rbS0.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.rbS0.Location = new System.Drawing.Point(3, 13);
            this.rbS0.Name = "rbS0";
            this.rbS0.Size = new System.Drawing.Size(50, 15);
            this.rbS0.TabIndex = 4;
            this.rbS0.TabStop = false;
            this.rbS0.Text = "S0";
            this.rbS0.CheckedChanged += new System.EventHandler(this.rbS0_CheckedChanged);
            // 
            // pnlTagEncoding
            // 
            this.pnlTagEncoding.BackColor = System.Drawing.Color.White;
            this.pnlTagEncoding.Controls.Add(this.rbM8);
            this.pnlTagEncoding.Controls.Add(this.rbM4);
            this.pnlTagEncoding.Controls.Add(this.rbM2);
            this.pnlTagEncoding.Controls.Add(this.rbFM0);
            this.pnlTagEncoding.Location = new System.Drawing.Point(14, 9);
            this.pnlTagEncoding.Name = "pnlTagEncoding";
            this.pnlTagEncoding.Size = new System.Drawing.Size(84, 96);
            // 
            // rbM8
            // 
            this.rbM8.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.rbM8.Location = new System.Drawing.Point(3, 75);
            this.rbM8.Name = "rbM8";
            this.rbM8.Size = new System.Drawing.Size(50, 15);
            this.rbM8.TabIndex = 3;
            this.rbM8.TabStop = false;
            this.rbM8.Text = "M8";
            this.rbM8.CheckedChanged += new System.EventHandler(this.rbM8_CheckedChanged);
            // 
            // rbM4
            // 
            this.rbM4.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.rbM4.Location = new System.Drawing.Point(2, 54);
            this.rbM4.Name = "rbM4";
            this.rbM4.Size = new System.Drawing.Size(50, 15);
            this.rbM4.TabIndex = 2;
            this.rbM4.TabStop = false;
            this.rbM4.Text = "M4";
            this.rbM4.CheckedChanged += new System.EventHandler(this.rbM4_CheckedChanged);
            // 
            // rbM2
            // 
            this.rbM2.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.rbM2.Location = new System.Drawing.Point(3, 33);
            this.rbM2.Name = "rbM2";
            this.rbM2.Size = new System.Drawing.Size(50, 15);
            this.rbM2.TabIndex = 1;
            this.rbM2.TabStop = false;
            this.rbM2.Text = "M2";
            this.rbM2.CheckedChanged += new System.EventHandler(this.rbM2_CheckedChanged);
            // 
            // rbFM0
            // 
            this.rbFM0.Enabled = false;
            this.rbFM0.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.rbFM0.Location = new System.Drawing.Point(3, 13);
            this.rbFM0.Name = "rbFM0";
            this.rbFM0.Size = new System.Drawing.Size(50, 15);
            this.rbFM0.TabIndex = 0;
            this.rbFM0.TabStop = false;
            this.rbFM0.Text = "FM0";
            // 
            // AdvancedReaderSettings
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(240, 268);
            this.Controls.Add(this.pnlAdvancedRdrSettings);
            this.Menu = this.mainMenu1;
            this.MinimizeBox = false;
            this.Name = "AdvancedReaderSettings";
            this.Text = "Reader Settings (Advanced)";
            this.Load += new System.EventHandler(this.AdvancedReaderSettings_Load);
            this.Closing += new System.ComponentModel.CancelEventHandler(this.AdvancedReaderSettings_Closing);
            this.pnlAdvancedRdrSettings.ResumeLayout(false);
            this.pnlQ.ResumeLayout(false);
            this.pnlTarget.ResumeLayout(false);
            this.pnlSession.ResumeLayout(false);
            this.pnlTagEncoding.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.MenuItem menuItem1;
        private System.Windows.Forms.MenuItem menuItem2;
        private System.Windows.Forms.Panel pnlAdvancedRdrSettings;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label lblQ;
        private System.Windows.Forms.Panel pnlQ;
        private System.Windows.Forms.ComboBox cbStaticQ;
        private System.Windows.Forms.RadioButton rbStaticQ;
        private System.Windows.Forms.RadioButton rbDynamicQ;
        private System.Windows.Forms.Label lblTarget;
        private System.Windows.Forms.Panel pnlTarget;
        private System.Windows.Forms.RadioButton rbBA;
        private System.Windows.Forms.RadioButton rbAB;
        private System.Windows.Forms.RadioButton rbB;
        private System.Windows.Forms.RadioButton rbA;
        private System.Windows.Forms.Panel pnlSession;
        private System.Windows.Forms.RadioButton rbS3;
        private System.Windows.Forms.RadioButton rbS2;
        private System.Windows.Forms.RadioButton rbS1;
        private System.Windows.Forms.RadioButton rbS0;
        private System.Windows.Forms.Panel pnlTagEncoding;
        private System.Windows.Forms.RadioButton rbM8;
        private System.Windows.Forms.RadioButton rbM4;
        private System.Windows.Forms.RadioButton rbM2;
        private System.Windows.Forms.RadioButton rbFM0;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Button btnSave;
        private System.Windows.Forms.Button btnRestoreDefaults;
        private System.Windows.Forms.RadioButton rbRecordHighestRSSI;
        private System.Windows.Forms.RadioButton rbRecordRSSIoflastTagRead;
        private System.Windows.Forms.TextBox txtRFOn;
        private System.Windows.Forms.Label lblRFOn;
        private System.Windows.Forms.TextBox txtRFOff;
        private System.Windows.Forms.Label lblRFOff;
    }
}
namespace ThingMagic.RFIDSearchLight
{
    partial class ReaderSettings
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
            this.pnlSettings = new System.Windows.Forms.Panel();
            this.label4 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.btnCancel = new System.Windows.Forms.Button();
            this.chkMask = new System.Windows.Forms.CheckBox();
            this.cbPowerInDBM = new System.Windows.Forms.ComboBox();
            this.label2 = new System.Windows.Forms.Label();
            this.lblMediumTags = new System.Windows.Forms.Label();
            this.lblSmallTags = new System.Windows.Forms.Label();
            this.rbMediumTags = new System.Windows.Forms.RadioButton();
            this.rbSmallTags = new System.Windows.Forms.RadioButton();
            this.rbLargeTags = new System.Windows.Forms.RadioButton();
            this.btnSave = new System.Windows.Forms.Button();
            this.btnRestoreDefaults = new System.Windows.Forms.Button();
            this.txtEpc = new System.Windows.Forms.TextBox();
            this.lblMask = new System.Windows.Forms.Label();
            this.txtAddress = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.cbTagSelection = new System.Windows.Forms.ComboBox();
            this.lblTagSelection = new System.Windows.Forms.Label();
            this.lblReadPower = new System.Windows.Forms.Label();
            this.pnlSettings.SuspendLayout();
            this.SuspendLayout();
            // 
            // pnlSettings
            // 
            this.pnlSettings.Controls.Add(this.label4);
            this.pnlSettings.Controls.Add(this.label3);
            this.pnlSettings.Controls.Add(this.btnCancel);
            this.pnlSettings.Controls.Add(this.chkMask);
            this.pnlSettings.Controls.Add(this.cbPowerInDBM);
            this.pnlSettings.Controls.Add(this.label2);
            this.pnlSettings.Controls.Add(this.lblMediumTags);
            this.pnlSettings.Controls.Add(this.lblSmallTags);
            this.pnlSettings.Controls.Add(this.rbMediumTags);
            this.pnlSettings.Controls.Add(this.rbSmallTags);
            this.pnlSettings.Controls.Add(this.rbLargeTags);
            this.pnlSettings.Controls.Add(this.btnSave);
            this.pnlSettings.Controls.Add(this.btnRestoreDefaults);
            this.pnlSettings.Controls.Add(this.txtEpc);
            this.pnlSettings.Controls.Add(this.lblMask);
            this.pnlSettings.Controls.Add(this.txtAddress);
            this.pnlSettings.Controls.Add(this.label1);
            this.pnlSettings.Controls.Add(this.cbTagSelection);
            this.pnlSettings.Controls.Add(this.lblTagSelection);
            this.pnlSettings.Controls.Add(this.lblReadPower);
            this.pnlSettings.Location = new System.Drawing.Point(0, 1);
            this.pnlSettings.Name = "pnlSettings";
            this.pnlSettings.Size = new System.Drawing.Size(225, 299);
            // 
            // label4
            // 
            this.label4.Font = new System.Drawing.Font("Tahoma", 6F, System.Drawing.FontStyle.Regular);
            this.label4.Location = new System.Drawing.Point(94, 175);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(28, 18);
            this.label4.Text = "(Hex)";
            // 
            // label3
            // 
            this.label3.Font = new System.Drawing.Font("Tahoma", 6F, System.Drawing.FontStyle.Regular);
            this.label3.Location = new System.Drawing.Point(111, 157);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(40, 20);
            this.label3.Text = "(in bytes)";
            // 
            // btnCancel
            // 
            this.btnCancel.Location = new System.Drawing.Point(115, 245);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(70, 20);
            this.btnCancel.TabIndex = 32;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // chkMask
            // 
            this.chkMask.Location = new System.Drawing.Point(8, 219);
            this.chkMask.Name = "chkMask";
            this.chkMask.Size = new System.Drawing.Size(205, 20);
            this.chkMask.TabIndex = 7;
            this.chkMask.Text = "Select tags not matching mask";
            this.chkMask.CheckStateChanged += new System.EventHandler(this.chkMask_CheckStateChanged);
            // 
            // cbPowerInDBM
            // 
            this.cbPowerInDBM.Items.Add("10");
            this.cbPowerInDBM.Items.Add("11");
            this.cbPowerInDBM.Items.Add("12");
            this.cbPowerInDBM.Items.Add("13");
            this.cbPowerInDBM.Items.Add("14");
            this.cbPowerInDBM.Items.Add("15");
            this.cbPowerInDBM.Items.Add("16");
            this.cbPowerInDBM.Items.Add("17");
            this.cbPowerInDBM.Items.Add("18");
            this.cbPowerInDBM.Items.Add("19");
            this.cbPowerInDBM.Items.Add("20");
            this.cbPowerInDBM.Items.Add("21");
            this.cbPowerInDBM.Items.Add("22");
            this.cbPowerInDBM.Items.Add("23");
            this.cbPowerInDBM.Location = new System.Drawing.Point(161, 1);
            this.cbPowerInDBM.Name = "cbPowerInDBM";
            this.cbPowerInDBM.Size = new System.Drawing.Size(55, 22);
            this.cbPowerInDBM.TabIndex = 1;
            this.cbPowerInDBM.SelectedIndexChanged += new System.EventHandler(this.cbPowerInDBM_SelectedIndexChanged);
            // 
            // label2
            // 
            this.label2.Font = new System.Drawing.Font("Tahoma", 6F, System.Drawing.FontStyle.Regular);
            this.label2.Location = new System.Drawing.Point(21, 44);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(120, 10);
            this.label2.Text = "(Greater than 50 tags)";
            // 
            // lblMediumTags
            // 
            this.lblMediumTags.Font = new System.Drawing.Font("Tahoma", 6F, System.Drawing.FontStyle.Regular);
            this.lblMediumTags.Location = new System.Drawing.Point(21, 76);
            this.lblMediumTags.Name = "lblMediumTags";
            this.lblMediumTags.Size = new System.Drawing.Size(120, 10);
            this.lblMediumTags.Text = "(10 to 50 tags)";
            // 
            // lblSmallTags
            // 
            this.lblSmallTags.Font = new System.Drawing.Font("Tahoma", 6F, System.Drawing.FontStyle.Regular);
            this.lblSmallTags.Location = new System.Drawing.Point(20, 110);
            this.lblSmallTags.Name = "lblSmallTags";
            this.lblSmallTags.Size = new System.Drawing.Size(120, 13);
            this.lblSmallTags.Text = "(Less than 10 tags)";
            // 
            // rbMediumTags
            // 
            this.rbMediumTags.Location = new System.Drawing.Point(2, 56);
            this.rbMediumTags.Name = "rbMediumTags";
            this.rbMediumTags.Size = new System.Drawing.Size(216, 20);
            this.rbMediumTags.TabIndex = 2;
            this.rbMediumTags.Text = "Medium Tag Population";
            this.rbMediumTags.CheckedChanged += new System.EventHandler(this.rbMediumTags_CheckedChanged);
            // 
            // rbSmallTags
            // 
            this.rbSmallTags.Checked = true;
            this.rbSmallTags.Location = new System.Drawing.Point(2, 88);
            this.rbSmallTags.Name = "rbSmallTags";
            this.rbSmallTags.Size = new System.Drawing.Size(216, 20);
            this.rbSmallTags.TabIndex = 3;
            this.rbSmallTags.Text = "Small Tag Population";
            this.rbSmallTags.CheckedChanged += new System.EventHandler(this.rbSmallTags_CheckedChanged);
            // 
            // rbLargeTags
            // 
            this.rbLargeTags.Location = new System.Drawing.Point(2, 25);
            this.rbLargeTags.Name = "rbLargeTags";
            this.rbLargeTags.Size = new System.Drawing.Size(216, 20);
            this.rbLargeTags.TabIndex = 1;
            this.rbLargeTags.Text = "Large Tag Population";
            this.rbLargeTags.CheckedChanged += new System.EventHandler(this.rbLargeTags_CheckedChanged);
            // 
            // btnSave
            // 
            this.btnSave.Location = new System.Drawing.Point(38, 245);
            this.btnSave.Name = "btnSave";
            this.btnSave.Size = new System.Drawing.Size(72, 20);
            this.btnSave.TabIndex = 24;
            this.btnSave.Text = "Save";
            this.btnSave.Click += new System.EventHandler(this.btnSave_Click);
            // 
            // btnRestoreDefaults
            // 
            this.btnRestoreDefaults.Location = new System.Drawing.Point(38, 271);
            this.btnRestoreDefaults.Name = "btnRestoreDefaults";
            this.btnRestoreDefaults.Size = new System.Drawing.Size(147, 20);
            this.btnRestoreDefaults.TabIndex = 27;
            this.btnRestoreDefaults.Text = "Restore Defaults";
            this.btnRestoreDefaults.Click += new System.EventHandler(this.btnRestoreDefaults_Click);
            // 
            // txtEpc
            // 
            this.txtEpc.Location = new System.Drawing.Point(9, 196);
            this.txtEpc.Name = "txtEpc";
            this.txtEpc.Size = new System.Drawing.Size(207, 21);
            this.txtEpc.TabIndex = 6;
            this.txtEpc.TextChanged += new System.EventHandler(this.txtEpc_TextChanged);
            // 
            // lblMask
            // 
            this.lblMask.Location = new System.Drawing.Point(8, 173);
            this.lblMask.Name = "lblMask";
            this.lblMask.Size = new System.Drawing.Size(87, 20);
            this.lblMask.Text = "Selection Mask";
            // 
            // txtAddress
            // 
            this.txtAddress.Location = new System.Drawing.Point(176, 152);
            this.txtAddress.Name = "txtAddress";
            this.txtAddress.Size = new System.Drawing.Size(40, 21);
            this.txtAddress.TabIndex = 5;
            this.txtAddress.Text = "0";
            this.txtAddress.TextChanged += new System.EventHandler(this.txtAddress_TextChanged);
            // 
            // label1
            // 
            this.label1.Location = new System.Drawing.Point(8, 153);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(102, 20);
            this.label1.Text = "Selection Address ";
            // 
            // cbTagSelection
            // 
            this.cbTagSelection.Items.Add("None");
            this.cbTagSelection.Items.Add("On EPC");
            this.cbTagSelection.Items.Add("On TID");
            this.cbTagSelection.Items.Add("On User Data");
            this.cbTagSelection.Location = new System.Drawing.Point(116, 126);
            this.cbTagSelection.Name = "cbTagSelection";
            this.cbTagSelection.Size = new System.Drawing.Size(100, 22);
            this.cbTagSelection.TabIndex = 4;
            this.cbTagSelection.SelectedIndexChanged += new System.EventHandler(this.cbTagSelection_SelectedIndexChanged);
            // 
            // lblTagSelection
            // 
            this.lblTagSelection.Location = new System.Drawing.Point(6, 126);
            this.lblTagSelection.Name = "lblTagSelection";
            this.lblTagSelection.Size = new System.Drawing.Size(109, 20);
            this.lblTagSelection.Text = "Tag Selection";
            // 
            // lblReadPower
            // 
            this.lblReadPower.Location = new System.Drawing.Point(2, 4);
            this.lblReadPower.Name = "lblReadPower";
            this.lblReadPower.Size = new System.Drawing.Size(132, 20);
            this.lblReadPower.Text = "Read Power (dBm)";
            // 
            // ReaderSettings
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(240, 268);
            this.Controls.Add(this.pnlSettings);
            this.KeyPreview = true;
            this.Menu = this.mainMenu1;
            this.MinimizeBox = false;
            this.Name = "ReaderSettings";
            this.Text = "Reader Settings";
            this.Load += new System.EventHandler(this.ReaderSettings_Load);
            this.Closing += new System.ComponentModel.CancelEventHandler(this.ReaderSettings_Closing);
            this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.ReaderSettings_KeyDown);
            this.pnlSettings.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel pnlSettings;
        private System.Windows.Forms.CheckBox chkMask;
        private System.Windows.Forms.ComboBox cbPowerInDBM;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label lblMediumTags;
        private System.Windows.Forms.Label lblSmallTags;
        private System.Windows.Forms.RadioButton rbMediumTags;
        private System.Windows.Forms.RadioButton rbSmallTags;
        private System.Windows.Forms.RadioButton rbLargeTags;
        private System.Windows.Forms.Button btnSave;
        private System.Windows.Forms.Button btnRestoreDefaults;
        private System.Windows.Forms.TextBox txtEpc;
        private System.Windows.Forms.Label lblMask;
        private System.Windows.Forms.TextBox txtAddress;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ComboBox cbTagSelection;
        private System.Windows.Forms.Label lblTagSelection;
        private System.Windows.Forms.Label lblReadPower;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label3;

    }
}
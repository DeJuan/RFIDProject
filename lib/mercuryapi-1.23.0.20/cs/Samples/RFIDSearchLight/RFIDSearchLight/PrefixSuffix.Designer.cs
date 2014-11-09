namespace ThingMagic.RFIDSearchLight
{
    partial class PrefixSuffix
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
            this.txtPrefix = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.txtSuffix = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.cbSeparator = new System.Windows.Forms.ComboBox();
            this.btnCancel = new System.Windows.Forms.Button();
            this.btnSave = new System.Windows.Forms.Button();
            this.btnRestoreToDefaults = new System.Windows.Forms.Button();
            this.label4 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // txtPrefix
            // 
            this.txtPrefix.Location = new System.Drawing.Point(13, 69);
            this.txtPrefix.Name = "txtPrefix";
            this.txtPrefix.Size = new System.Drawing.Size(154, 21);
            this.txtPrefix.TabIndex = 0;
            this.txtPrefix.TextChanged += new System.EventHandler(this.SettingsChanged);
            // 
            // label1
            // 
            this.label1.Location = new System.Drawing.Point(13, 46);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(154, 20);
            this.label1.Text = "Prefix to send before RFID";
            // 
            // label2
            // 
            this.label2.Location = new System.Drawing.Point(13, 99);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(154, 20);
            this.label2.Text = "Suffix to send after RFID";
            // 
            // txtSuffix
            // 
            this.txtSuffix.Location = new System.Drawing.Point(13, 126);
            this.txtSuffix.Name = "txtSuffix";
            this.txtSuffix.Size = new System.Drawing.Size(154, 21);
            this.txtSuffix.TabIndex = 1;
            this.txtSuffix.TextChanged += new System.EventHandler(this.SettingsChanged);
            // 
            // label3
            // 
            this.label3.Location = new System.Drawing.Point(13, 176);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(137, 20);
            this.label3.Text = "Multiple Tag Separator";
            // 
            // cbSeparator
            // 
            this.cbSeparator.Items.Add("Enter");
            this.cbSeparator.Items.Add("Comma");
            this.cbSeparator.Items.Add("Tab");
            this.cbSeparator.Items.Add("Space");
            this.cbSeparator.Items.Add("Pipe");
            this.cbSeparator.Location = new System.Drawing.Point(143, 174);
            this.cbSeparator.Name = "cbSeparator";
            this.cbSeparator.Size = new System.Drawing.Size(94, 22);
            this.cbSeparator.TabIndex = 2;
            this.cbSeparator.SelectedIndexChanged += new System.EventHandler(this.SettingsChanged);
            this.cbSeparator.TextChanged += new System.EventHandler(this.SettingsChanged);
            // 
            // btnCancel
            // 
            this.btnCancel.Location = new System.Drawing.Point(122, 212);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(70, 20);
            this.btnCancel.TabIndex = 23;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // btnSave
            // 
            this.btnSave.Location = new System.Drawing.Point(45, 212);
            this.btnSave.Name = "btnSave";
            this.btnSave.Size = new System.Drawing.Size(70, 20);
            this.btnSave.TabIndex = 22;
            this.btnSave.Text = "Save";
            this.btnSave.Click += new System.EventHandler(this.btnSave_Click);
            // 
            // btnRestoreToDefaults
            // 
            this.btnRestoreToDefaults.Location = new System.Drawing.Point(45, 236);
            this.btnRestoreToDefaults.Name = "btnRestoreToDefaults";
            this.btnRestoreToDefaults.Size = new System.Drawing.Size(147, 20);
            this.btnRestoreToDefaults.TabIndex = 21;
            this.btnRestoreToDefaults.Text = "Restore Defaults";
            this.btnRestoreToDefaults.Click += new System.EventHandler(this.btnRestoreToDefaults_Click);
            // 
            // label4
            // 
            this.label4.Font = new System.Drawing.Font("Tahoma", 12F, System.Drawing.FontStyle.Regular);
            this.label4.ForeColor = System.Drawing.Color.RoyalBlue;
            this.label4.Location = new System.Drawing.Point(13, 15);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(196, 20);
            this.label4.Text = "Keyboard Wedge Settings";
            // 
            // PrefixSuffix
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(240, 268);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnSave);
            this.Controls.Add(this.btnRestoreToDefaults);
            this.Controls.Add(this.cbSeparator);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.txtSuffix);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.txtPrefix);
            this.Menu = this.mainMenu1;
            this.MinimizeBox = false;
            this.Name = "PrefixSuffix";
            this.Text = "Prefix Suffix";
            this.Closing += new System.ComponentModel.CancelEventHandler(this.PrefixSuffix_Closing);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TextBox txtPrefix;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox txtSuffix;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.ComboBox cbSeparator;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Button btnSave;
        private System.Windows.Forms.Button btnRestoreToDefaults;
        private System.Windows.Forms.Label label4;
    }
}
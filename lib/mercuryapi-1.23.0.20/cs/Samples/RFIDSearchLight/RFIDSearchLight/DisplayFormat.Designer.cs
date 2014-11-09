namespace ThingMagic.RFIDSearchLight
{
    partial class DisplayFormat
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
            this.txtToolTip = new System.Windows.Forms.TextBox();
            this.btnSave = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.btnRestoreToDefaults = new System.Windows.Forms.Button();
            this.rbEpcInHex = new System.Windows.Forms.RadioButton();
            this.rbEPCinBase36 = new System.Windows.Forms.RadioButton();
            this.label4 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // txtToolTip
            // 
            this.txtToolTip.Location = new System.Drawing.Point(7, 110);
            this.txtToolTip.Multiline = true;
            this.txtToolTip.Name = "txtToolTip";
            this.txtToolTip.ReadOnly = true;
            this.txtToolTip.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.txtToolTip.Size = new System.Drawing.Size(221, 87);
            this.txtToolTip.TabIndex = 2;
            // 
            // btnSave
            // 
            this.btnSave.Location = new System.Drawing.Point(46, 212);
            this.btnSave.Name = "btnSave";
            this.btnSave.Size = new System.Drawing.Size(70, 20);
            this.btnSave.TabIndex = 3;
            this.btnSave.Text = "Save";
            this.btnSave.Click += new System.EventHandler(this.btnSave_Click);
            // 
            // btnCancel
            // 
            this.btnCancel.Location = new System.Drawing.Point(123, 212);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(70, 20);
            this.btnCancel.TabIndex = 4;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // btnRestoreToDefaults
            // 
            this.btnRestoreToDefaults.Location = new System.Drawing.Point(46, 238);
            this.btnRestoreToDefaults.Name = "btnRestoreToDefaults";
            this.btnRestoreToDefaults.Size = new System.Drawing.Size(147, 20);
            this.btnRestoreToDefaults.TabIndex = 5;
            this.btnRestoreToDefaults.Text = "Restore Defaults";
            this.btnRestoreToDefaults.Click += new System.EventHandler(this.btnRestoreToDefaults_Click);
            // 
            // rbEpcInHex
            // 
            this.rbEpcInHex.Checked = true;
            this.rbEpcInHex.Location = new System.Drawing.Point(7, 44);
            this.rbEpcInHex.Name = "rbEpcInHex";
            this.rbEpcInHex.Size = new System.Drawing.Size(100, 20);
            this.rbEpcInHex.TabIndex = 0;
            this.rbEpcInHex.Text = "EPC in Hex";
            this.rbEpcInHex.CheckedChanged += new System.EventHandler(this.rbEpcInHex_CheckedChanged);
            // 
            // rbEPCinBase36
            // 
            this.rbEPCinBase36.Location = new System.Drawing.Point(7, 71);
            this.rbEPCinBase36.Name = "rbEPCinBase36";
            this.rbEPCinBase36.Size = new System.Drawing.Size(109, 20);
            this.rbEPCinBase36.TabIndex = 1;
            this.rbEPCinBase36.TabStop = false;
            this.rbEPCinBase36.Text = "EPC in Base36";
            this.rbEPCinBase36.CheckedChanged += new System.EventHandler(this.rbEPCinBase36_CheckedChanged);
            // 
            // label4
            // 
            this.label4.Font = new System.Drawing.Font("Tahoma", 12F, System.Drawing.FontStyle.Regular);
            this.label4.ForeColor = System.Drawing.Color.RoyalBlue;
            this.label4.Location = new System.Drawing.Point(7, 12);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(196, 20);
            this.label4.Text = "Keyboard Wedge Settings";
            // 
            // DisplayFormat
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(240, 268);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.rbEPCinBase36);
            this.Controls.Add(this.rbEpcInHex);
            this.Controls.Add(this.btnRestoreToDefaults);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnSave);
            this.Controls.Add(this.txtToolTip);
            this.Menu = this.mainMenu1;
            this.MinimizeBox = false;
            this.Name = "DisplayFormat";
            this.Text = "Display Format";
            this.Load += new System.EventHandler(this.DisplayFormat_Load);
            this.Closing += new System.ComponentModel.CancelEventHandler(this.DisplayFormat_Closing);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TextBox txtToolTip;
        private System.Windows.Forms.Button btnSave;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Button btnRestoreToDefaults;
        private System.Windows.Forms.RadioButton rbEpcInHex;
        private System.Windows.Forms.RadioButton rbEPCinBase36;
        private System.Windows.Forms.Label label4;
    }
}
namespace ThingMagic.RFIDSearchLight
{
    partial class Configuration
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
            this.btnPrefixSuffix = new System.Windows.Forms.Button();
            this.btnReadParameters = new System.Windows.Forms.Button();
            this.btnReaderSettings = new System.Windows.Forms.Button();
            this.btnDisplayFormat = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // mainMenu1
            // 
            this.mainMenu1.MenuItems.Add(this.menuItem1);
            this.mainMenu1.MenuItems.Add(this.menuItem2);
            // 
            // menuItem1
            // 
            this.menuItem1.Text = "Home";
            this.menuItem1.Click += new System.EventHandler(this.menuItem1_Click);
            // 
            // menuItem2
            // 
            this.menuItem2.Text = "Exit";
            this.menuItem2.Click += new System.EventHandler(this.menuItem2_Click);
            // 
            // btnPrefixSuffix
            // 
            this.btnPrefixSuffix.Location = new System.Drawing.Point(30, 149);
            this.btnPrefixSuffix.Name = "btnPrefixSuffix";
            this.btnPrefixSuffix.Size = new System.Drawing.Size(180, 20);
            this.btnPrefixSuffix.TabIndex = 11;
            this.btnPrefixSuffix.Text = "Prefix / Suffix";
            this.btnPrefixSuffix.Click += new System.EventHandler(this.btnPrefixSuffix_Click);
            // 
            // btnReadParameters
            // 
            this.btnReadParameters.Location = new System.Drawing.Point(30, 92);
            this.btnReadParameters.Name = "btnReadParameters";
            this.btnReadParameters.Size = new System.Drawing.Size(180, 20);
            this.btnReadParameters.TabIndex = 10;
            this.btnReadParameters.Text = "Read Parameters";
            this.btnReadParameters.Click += new System.EventHandler(this.btnReadParameters_Click);
            // 
            // btnReaderSettings
            // 
            this.btnReaderSettings.Location = new System.Drawing.Point(30, 206);
            this.btnReaderSettings.Name = "btnReaderSettings";
            this.btnReaderSettings.Size = new System.Drawing.Size(180, 20);
            this.btnReaderSettings.TabIndex = 12;
            this.btnReaderSettings.Text = "Reader Settings";
            this.btnReaderSettings.Click += new System.EventHandler(this.btnReaderSettings_Click);
            // 
            // btnDisplayFormat
            // 
            this.btnDisplayFormat.Location = new System.Drawing.Point(30, 34);
            this.btnDisplayFormat.Name = "btnDisplayFormat";
            this.btnDisplayFormat.Size = new System.Drawing.Size(180, 20);
            this.btnDisplayFormat.TabIndex = 9;
            this.btnDisplayFormat.Text = "Display Format";
            this.btnDisplayFormat.Click += new System.EventHandler(this.btnDisplayFormat_Click);
            // 
            // Configuration
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(240, 268);
            this.Controls.Add(this.btnPrefixSuffix);
            this.Controls.Add(this.btnReadParameters);
            this.Controls.Add(this.btnReaderSettings);
            this.Controls.Add(this.btnDisplayFormat);
            this.Menu = this.mainMenu1;
            this.MinimizeBox = false;
            this.Name = "Configuration";
            this.Text = "Configuration";
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button btnPrefixSuffix;
        private System.Windows.Forms.Button btnReadParameters;
        private System.Windows.Forms.Button btnReaderSettings;
        private System.Windows.Forms.Button btnDisplayFormat;
        private System.Windows.Forms.MenuItem menuItem1;
        private System.Windows.Forms.MenuItem menuItem2;
    }
}
namespace ThingMagic.RFIDSearchLight
{
    partial class Info
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Info));
            this.mainMenu1 = new System.Windows.Forms.MainMenu();
            this.menuItem2 = new System.Windows.Forms.MenuItem();
            this.menuItem1 = new System.Windows.Forms.MenuItem();
            this.pnlMain = new System.Windows.Forms.Panel();
            this.btnReadTags = new System.Windows.Forms.Button();
            this.btnConfigure = new System.Windows.Forms.Button();
            this.btnKeyboardWedge = new System.Windows.Forms.Button();
            this.pbLogo = new System.Windows.Forms.PictureBox();
            this.infoDebugLabel = new System.Windows.Forms.Label();
            this.infoGpsLabel = new System.Windows.Forms.Label();
            this.infoPowerLabel = new System.Windows.Forms.Label();
            this.pnlMain.SuspendLayout();
            this.SuspendLayout();
            // 
            // mainMenu1
            // 
            this.mainMenu1.MenuItems.Add(this.menuItem2);
            this.mainMenu1.MenuItems.Add(this.menuItem1);
            // 
            // menuItem2
            // 
            this.menuItem2.Text = "About";
            this.menuItem2.Click += new System.EventHandler(this.about_Click);
            // 
            // menuItem1
            // 
            this.menuItem1.Text = "Exit";
            this.menuItem1.Click += new System.EventHandler(this.exit_Click);
            // 
            // pnlMain
            // 
            this.pnlMain.AutoScroll = true;
            this.pnlMain.Controls.Add(this.btnReadTags);
            this.pnlMain.Controls.Add(this.btnConfigure);
            this.pnlMain.Controls.Add(this.btnKeyboardWedge);
            this.pnlMain.Controls.Add(this.pbLogo);
            this.pnlMain.Controls.Add(this.infoDebugLabel);
            this.pnlMain.Controls.Add(this.infoGpsLabel);
            this.pnlMain.Controls.Add(this.infoPowerLabel);
            this.pnlMain.Dock = System.Windows.Forms.DockStyle.Fill;
            this.pnlMain.Location = new System.Drawing.Point(0, 0);
            this.pnlMain.Name = "pnlMain";
            this.pnlMain.Size = new System.Drawing.Size(240, 268);
            // 
            // btnReadTags
            // 
            this.btnReadTags.Location = new System.Drawing.Point(20, 198);
            this.btnReadTags.Name = "btnReadTags";
            this.btnReadTags.Size = new System.Drawing.Size(200, 48);
            this.btnReadTags.TabIndex = 8;
            this.btnReadTags.Text = "Read RFID Tags";
            this.btnReadTags.Click += new System.EventHandler(this.btnReadTags_Click);
            // 
            // btnConfigure
            // 
            this.btnConfigure.Location = new System.Drawing.Point(20, 130);
            this.btnConfigure.Name = "btnConfigure";
            this.btnConfigure.Size = new System.Drawing.Size(200, 48);
            this.btnConfigure.TabIndex = 7;
            this.btnConfigure.Text = "Configure Application";
            this.btnConfigure.Click += new System.EventHandler(this.btnConfigure_Click);
            // 
            // btnKeyboardWedge
            // 
            this.btnKeyboardWedge.Location = new System.Drawing.Point(20, 61);
            this.btnKeyboardWedge.Name = "btnKeyboardWedge";
            this.btnKeyboardWedge.Size = new System.Drawing.Size(200, 48);
            this.btnKeyboardWedge.TabIndex = 6;
            this.btnKeyboardWedge.Text = "Enable RFID keyboard wedge";
            this.btnKeyboardWedge.Click += new System.EventHandler(this.btnKeyboardWedge_Click);
            // 
            // pbLogo
            // 
            this.pbLogo.Image = ((System.Drawing.Image)(resources.GetObject("pbLogo.Image")));
            this.pbLogo.Location = new System.Drawing.Point(3, 3);
            this.pbLogo.Name = "pbLogo";
            this.pbLogo.Size = new System.Drawing.Size(234, 52);
            // 
            // infoDebugLabel
            // 
            this.infoDebugLabel.Location = new System.Drawing.Point(3, 112);
            this.infoDebugLabel.Name = "infoDebugLabel";
            this.infoDebugLabel.Size = new System.Drawing.Size(234, 20);
            this.infoDebugLabel.Text = "<Debug Message>";
            // 
            // infoGpsLabel
            // 
            this.infoGpsLabel.Location = new System.Drawing.Point(3, 248);
            this.infoGpsLabel.Name = "infoGpsLabel";
            this.infoGpsLabel.Size = new System.Drawing.Size(234, 20);
            this.infoGpsLabel.Text = "<GPS Message>";
            this.infoGpsLabel.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // infoPowerLabel
            // 
            this.infoPowerLabel.Location = new System.Drawing.Point(0, 181);
            this.infoPowerLabel.Name = "infoPowerLabel";
            this.infoPowerLabel.Size = new System.Drawing.Size(234, 20);
            this.infoPowerLabel.Text = "<Power Message>";
            // 
            // Info
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(240, 268);
            this.Controls.Add(this.pnlMain);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.KeyPreview = true;
            this.Menu = this.mainMenu1;
            this.MinimizeBox = false;
            this.Name = "Info";
            this.Text = "RFID SearchLight";
            this.Load += new System.EventHandler(this.Info_Load);
            this.Closed += new System.EventHandler(this.Info_Closed);
            this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.Info_KeyDown);
            this.pnlMain.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel pnlMain;
        private System.Windows.Forms.PictureBox pbLogo;
        private System.Windows.Forms.MenuItem menuItem2;
        private System.Windows.Forms.MenuItem menuItem1;
        private System.Windows.Forms.Button btnReadTags;
        private System.Windows.Forms.Button btnConfigure;
        private System.Windows.Forms.Button btnKeyboardWedge;
        private System.Windows.Forms.Label infoDebugLabel;
        private System.Windows.Forms.Label infoGpsLabel;
        private System.Windows.Forms.Label infoPowerLabel;

    }
}
namespace ThingMagic.RFIDSearchLight
{
    partial class ReadTags
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ReadTags));
            this.mainMenu1 = new System.Windows.Forms.MainMenu();
            this.miGoToMain = new System.Windows.Forms.MenuItem();
            this.miExit = new System.Windows.Forms.MenuItem();
            this.btnStartReads = new System.Windows.Forms.Button();
            this.dgTagResult = new System.Windows.Forms.DataGrid();
            this.btnClearReads = new System.Windows.Forms.Button();
            this.btnSaveTags = new System.Windows.Forms.Button();
            this.txtTotalUniqueTags = new System.Windows.Forms.TextBox();
            this.lblTotalTags = new System.Windows.Forms.Label();
            this.tbTXPower = new System.Windows.Forms.TrackBar();
            this.btnLed = new System.Windows.Forms.Button();
            this.lblTagReadCoverage = new System.Windows.Forms.Label();
            this.saveTagsFileDialog = new System.Windows.Forms.SaveFileDialog();
            this.pbRay1 = new System.Windows.Forms.PictureBox();
            this.pbRay2 = new System.Windows.Forms.PictureBox();
            this.label1 = new System.Windows.Forms.Label();
            this.debugLabel = new System.Windows.Forms.Label();
            this.powerLabel = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // mainMenu1
            // 
            this.mainMenu1.MenuItems.Add(this.miGoToMain);
            this.mainMenu1.MenuItems.Add(this.miExit);
            // 
            // miGoToMain
            // 
            this.miGoToMain.Text = "Home";
            this.miGoToMain.Click += new System.EventHandler(this.miGoToMain_Click);
            // 
            // miExit
            // 
            this.miExit.Text = "Exit";
            this.miExit.Click += new System.EventHandler(this.miExit_Click);
            // 
            // btnStartReads
            // 
            this.btnStartReads.Location = new System.Drawing.Point(3, 3);
            this.btnStartReads.Name = "btnStartReads";
            this.btnStartReads.Size = new System.Drawing.Size(100, 34);
            this.btnStartReads.TabIndex = 0;
            this.btnStartReads.Text = "Start Reads";
            this.btnStartReads.Click += new System.EventHandler(this.btnStartReads_Click);
            // 
            // dgTagResult
            // 
            this.dgTagResult.BackgroundColor = System.Drawing.Color.FromArgb(((int)(((byte)(128)))), ((int)(((byte)(128)))), ((int)(((byte)(128)))));
            this.dgTagResult.Location = new System.Drawing.Point(3, 123);
            this.dgTagResult.Name = "dgTagResult";
            this.dgTagResult.Size = new System.Drawing.Size(234, 115);
            this.dgTagResult.TabIndex = 3;
            // 
            // btnClearReads
            // 
            this.btnClearReads.Location = new System.Drawing.Point(4, 245);
            this.btnClearReads.Name = "btnClearReads";
            this.btnClearReads.Size = new System.Drawing.Size(99, 20);
            this.btnClearReads.TabIndex = 4;
            this.btnClearReads.Text = "Clear Reads";
            this.btnClearReads.Click += new System.EventHandler(this.btnClearReads_Click);
            // 
            // btnSaveTags
            // 
            this.btnSaveTags.Location = new System.Drawing.Point(137, 245);
            this.btnSaveTags.Name = "btnSaveTags";
            this.btnSaveTags.Size = new System.Drawing.Size(100, 20);
            this.btnSaveTags.TabIndex = 5;
            this.btnSaveTags.Text = "Save Tags";
            this.btnSaveTags.Click += new System.EventHandler(this.btnSaveTags_Click);
            // 
            // txtTotalUniqueTags
            // 
            this.txtTotalUniqueTags.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.txtTotalUniqueTags.Font = new System.Drawing.Font("Tahoma", 18F, System.Drawing.FontStyle.Regular);
            this.txtTotalUniqueTags.ForeColor = System.Drawing.Color.Green;
            this.txtTotalUniqueTags.Location = new System.Drawing.Point(172, 85);
            this.txtTotalUniqueTags.Multiline = true;
            this.txtTotalUniqueTags.Name = "txtTotalUniqueTags";
            this.txtTotalUniqueTags.ReadOnly = true;
            this.txtTotalUniqueTags.Size = new System.Drawing.Size(65, 34);
            this.txtTotalUniqueTags.TabIndex = 2;
            this.txtTotalUniqueTags.Text = "0";
            this.txtTotalUniqueTags.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.txtTotalUniqueTags.WordWrap = false;
            // 
            // lblTotalTags
            // 
            this.lblTotalTags.Location = new System.Drawing.Point(4, 94);
            this.lblTotalTags.Name = "lblTotalTags";
            this.lblTotalTags.Size = new System.Drawing.Size(151, 20);
            this.lblTotalTags.Text = "Total Unique Tags Found";
            // 
            // tbTXPower
            // 
            this.tbTXPower.LargeChange = 1;
            this.tbTXPower.Location = new System.Drawing.Point(30, 55);
            this.tbTXPower.Maximum = 26;
            this.tbTXPower.Name = "tbTXPower";
            this.tbTXPower.Size = new System.Drawing.Size(169, 25);
            this.tbTXPower.TabIndex = 1;
            this.tbTXPower.Value = 21;
            this.tbTXPower.ValueChanged += new System.EventHandler(this.tbTXPower_ValueChanged);
            // 
            // btnLed
            // 
            this.btnLed.BackColor = System.Drawing.Color.White;
            this.btnLed.Enabled = false;
            this.btnLed.ForeColor = System.Drawing.Color.Black;
            this.btnLed.Location = new System.Drawing.Point(109, 3);
            this.btnLed.Name = "btnLed";
            this.btnLed.Size = new System.Drawing.Size(128, 34);
            this.btnLed.TabIndex = 113;
            // 
            // lblTagReadCoverage
            // 
            this.lblTagReadCoverage.Location = new System.Drawing.Point(4, 40);
            this.lblTagReadCoverage.Name = "lblTagReadCoverage";
            this.lblTagReadCoverage.Size = new System.Drawing.Size(115, 15);
            this.lblTagReadCoverage.Text = "Tag Read Coverage";
            // 
            // pbRay1
            // 
            this.pbRay1.Image = ((System.Drawing.Image)(resources.GetObject("pbRay1.Image")));
            this.pbRay1.Location = new System.Drawing.Point(4, 58);
            this.pbRay1.Name = "pbRay1";
            this.pbRay1.Size = new System.Drawing.Size(23, 22);
            // 
            // pbRay2
            // 
            this.pbRay2.Image = ((System.Drawing.Image)(resources.GetObject("pbRay2.Image")));
            this.pbRay2.Location = new System.Drawing.Point(203, 52);
            this.pbRay2.Name = "pbRay2";
            this.pbRay2.Size = new System.Drawing.Size(34, 27);
            // 
            // label1
            // 
            this.label1.Font = new System.Drawing.Font("Tahoma", 6F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Italic))));
            this.label1.Location = new System.Drawing.Point(110, 15);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(126, 10);
            this.label1.Text = "AbcgqxyZ";
            this.label1.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // debugLabel
            // 
            this.debugLabel.Font = new System.Drawing.Font("Tahoma", 6F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Italic))));
            this.debugLabel.Location = new System.Drawing.Point(110, 25);
            this.debugLabel.Name = "debugLabel";
            this.debugLabel.Size = new System.Drawing.Size(126, 10);
            this.debugLabel.Text = "debugLabel";
            this.debugLabel.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // powerLabel
            // 
            this.powerLabel.Font = new System.Drawing.Font("Tahoma", 6F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Italic))));
            this.powerLabel.Location = new System.Drawing.Point(54, 84);
            this.powerLabel.Name = "powerLabel";
            this.powerLabel.Size = new System.Drawing.Size(112, 12);
            this.powerLabel.Text = "powerLabel";
            this.powerLabel.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // ReadTags
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(240, 268);
            this.Controls.Add(this.powerLabel);
            this.Controls.Add(this.debugLabel);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.pbRay2);
            this.Controls.Add(this.pbRay1);
            this.Controls.Add(this.lblTagReadCoverage);
            this.Controls.Add(this.btnLed);
            this.Controls.Add(this.tbTXPower);
            this.Controls.Add(this.lblTotalTags);
            this.Controls.Add(this.txtTotalUniqueTags);
            this.Controls.Add(this.btnSaveTags);
            this.Controls.Add(this.btnClearReads);
            this.Controls.Add(this.dgTagResult);
            this.Controls.Add(this.btnStartReads);
            this.Menu = this.mainMenu1;
            this.MinimizeBox = false;
            this.Name = "ReadTags";
            this.Text = "Read Tags";
            this.Load += new System.EventHandler(this.ReadTags_Load);
            this.Closed += new System.EventHandler(this.ReadTags_Closed);
            this.Closing += new System.ComponentModel.CancelEventHandler(this.ReadTags_Closing);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button btnStartReads;
        private System.Windows.Forms.DataGrid dgTagResult;
        private System.Windows.Forms.Button btnClearReads;
        private System.Windows.Forms.Button btnSaveTags;
        private System.Windows.Forms.TextBox txtTotalUniqueTags;
        private System.Windows.Forms.Label lblTotalTags;
        private System.Windows.Forms.TrackBar tbTXPower;
        private System.Windows.Forms.MenuItem miGoToMain;
        private System.Windows.Forms.MenuItem miExit;
        private System.Windows.Forms.Button btnLed;
        private System.Windows.Forms.Label lblTagReadCoverage;
        private System.Windows.Forms.SaveFileDialog saveTagsFileDialog;
        private System.Windows.Forms.PictureBox pbRay1;
        private System.Windows.Forms.PictureBox pbRay2;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label debugLabel;
        private System.Windows.Forms.Label powerLabel;
    }
}
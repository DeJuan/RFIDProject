namespace TestPowerManagement
{
    partial class TestPMForm1
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
            this.leftButtonMenuItem = new System.Windows.Forms.MenuItem();
            this.appExitMenuItem = new System.Windows.Forms.MenuItem();
            this.formCloseMenuItem = new System.Windows.Forms.MenuItem();
            this.clearMenuItem = new System.Windows.Forms.MenuItem();
            this.openSerialMenuItem = new System.Windows.Forms.MenuItem();
            this.closeSerialMenuItem = new System.Windows.Forms.MenuItem();
            this.cmdVersionMenuItem = new System.Windows.Forms.MenuItem();
            this.saveMenuItem = new System.Windows.Forms.MenuItem();
            this.logEnabledMenuItem = new System.Windows.Forms.MenuItem();
            this.menuItem1 = new System.Windows.Forms.MenuItem();
            this.startUmtMenuItem = new System.Windows.Forms.MenuItem();
            this.stopUmtMenuItem = new System.Windows.Forms.MenuItem();
            this.gpsMenuItem = new System.Windows.Forms.MenuItem();
            this.gpsOpenMenuItem = new System.Windows.Forms.MenuItem();
            this.gpsCloseMenuItem = new System.Windows.Forms.MenuItem();
            this.gpsGetPositionMenuItem = new System.Windows.Forms.MenuItem();
            this.gpsGetDeviceStateMenuItem = new System.Windows.Forms.MenuItem();
            this.rightButtonMenuItem = new System.Windows.Forms.MenuItem();
            this.outputTextBox = new System.Windows.Forms.TextBox();
            this.statusTextBox = new System.Windows.Forms.TextBox();
            this.SuspendLayout();
            // 
            // mainMenu1
            // 
            this.mainMenu1.MenuItems.Add(this.leftButtonMenuItem);
            this.mainMenu1.MenuItems.Add(this.rightButtonMenuItem);
            // 
            // leftButtonMenuItem
            // 
            this.leftButtonMenuItem.MenuItems.Add(this.appExitMenuItem);
            this.leftButtonMenuItem.MenuItems.Add(this.formCloseMenuItem);
            this.leftButtonMenuItem.MenuItems.Add(this.clearMenuItem);
            this.leftButtonMenuItem.MenuItems.Add(this.openSerialMenuItem);
            this.leftButtonMenuItem.MenuItems.Add(this.closeSerialMenuItem);
            this.leftButtonMenuItem.MenuItems.Add(this.cmdVersionMenuItem);
            this.leftButtonMenuItem.MenuItems.Add(this.saveMenuItem);
            this.leftButtonMenuItem.MenuItems.Add(this.logEnabledMenuItem);
            this.leftButtonMenuItem.MenuItems.Add(this.menuItem1);
            this.leftButtonMenuItem.MenuItems.Add(this.gpsMenuItem);
            this.leftButtonMenuItem.Text = "Menu";
            // 
            // appExitMenuItem
            // 
            this.appExitMenuItem.Text = "App.Exit";
            this.appExitMenuItem.Click += new System.EventHandler(this.appExitMenuItem_Click);
            // 
            // formCloseMenuItem
            // 
            this.formCloseMenuItem.Text = "Form.Close";
            this.formCloseMenuItem.Click += new System.EventHandler(this.formCloseMenuItem_Click);
            // 
            // clearMenuItem
            // 
            this.clearMenuItem.Text = "Clear";
            this.clearMenuItem.Click += new System.EventHandler(this.clearMenuItem_Click);
            // 
            // openSerialMenuItem
            // 
            this.openSerialMenuItem.Text = "Open Serial";
            this.openSerialMenuItem.Click += new System.EventHandler(this.openSerialMenuItem_Click);
            // 
            // closeSerialMenuItem
            // 
            this.closeSerialMenuItem.Text = "Close Serial";
            this.closeSerialMenuItem.Click += new System.EventHandler(this.closeSerialMenuItem_Click);
            // 
            // cmdVersionMenuItem
            // 
            this.cmdVersionMenuItem.Text = "cmdVersion";
            this.cmdVersionMenuItem.Click += new System.EventHandler(this.cmdVersionMenuItem_Click);
            // 
            // saveMenuItem
            // 
            this.saveMenuItem.Text = "Save";
            this.saveMenuItem.Click += new System.EventHandler(this.saveMenuItem_Click);
            // 
            // logEnabledMenuItem
            // 
            this.logEnabledMenuItem.Checked = true;
            this.logEnabledMenuItem.Text = "LogEnabled";
            this.logEnabledMenuItem.Click += new System.EventHandler(this.logEnabledMenuItem_Click);
            // 
            // menuItem1
            // 
            this.menuItem1.MenuItems.Add(this.startUmtMenuItem);
            this.menuItem1.MenuItems.Add(this.stopUmtMenuItem);
            this.menuItem1.Text = "Test";
            // 
            // startUmtMenuItem
            // 
            this.startUmtMenuItem.Text = "Start Unattended Thread";
            this.startUmtMenuItem.Click += new System.EventHandler(this.startUmtMenuItem_Click);
            // 
            // stopUmtMenuItem
            // 
            this.stopUmtMenuItem.Text = "Stop Unattended Thread";
            this.stopUmtMenuItem.Click += new System.EventHandler(this.stopUmtMenuItem_Click);
            // 
            // gpsMenuItem
            // 
            this.gpsMenuItem.MenuItems.Add(this.gpsOpenMenuItem);
            this.gpsMenuItem.MenuItems.Add(this.gpsCloseMenuItem);
            this.gpsMenuItem.MenuItems.Add(this.gpsGetPositionMenuItem);
            this.gpsMenuItem.MenuItems.Add(this.gpsGetDeviceStateMenuItem);
            this.gpsMenuItem.Text = "GPS";
            // 
            // gpsOpenMenuItem
            // 
            this.gpsOpenMenuItem.Text = "Open";
            this.gpsOpenMenuItem.Click += new System.EventHandler(this.gpsOpenMenuItem_Click);
            // 
            // gpsCloseMenuItem
            // 
            this.gpsCloseMenuItem.Text = "Close";
            this.gpsCloseMenuItem.Click += new System.EventHandler(this.gpsCloseMenuItem_Click);
            // 
            // gpsGetPositionMenuItem
            // 
            this.gpsGetPositionMenuItem.Text = "GetPosition";
            this.gpsGetPositionMenuItem.Click += new System.EventHandler(this.gpsGetPositionMenuItem_Click);
            // 
            // gpsGetDeviceStateMenuItem
            // 
            this.gpsGetDeviceStateMenuItem.Text = "GetDeviceState";
            this.gpsGetDeviceStateMenuItem.Click += new System.EventHandler(this.gpsGetDeviceStateMenuItem_Click);
            // 
            // rightButtonMenuItem
            // 
            this.rightButtonMenuItem.Text = "Exit";
            this.rightButtonMenuItem.Click += new System.EventHandler(this.rightButtonMenuItem_Click);
            // 
            // outputTextBox
            // 
            this.outputTextBox.Location = new System.Drawing.Point(3, 21);
            this.outputTextBox.Multiline = true;
            this.outputTextBox.Name = "outputTextBox";
            this.outputTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.outputTextBox.Size = new System.Drawing.Size(234, 244);
            this.outputTextBox.TabIndex = 0;
            this.outputTextBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.outputTextBox_KeyDown);
            this.outputTextBox.KeyUp += new System.Windows.Forms.KeyEventHandler(this.outputTextBox_KeyUp);
            this.outputTextBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.outputTextBox_KeyPress);
            // 
            // statusTextBox
            // 
            this.statusTextBox.Location = new System.Drawing.Point(3, 3);
            this.statusTextBox.Name = "statusTextBox";
            this.statusTextBox.Size = new System.Drawing.Size(234, 21);
            this.statusTextBox.TabIndex = 1;
            // 
            // TestPMForm1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(240, 268);
            this.Controls.Add(this.statusTextBox);
            this.Controls.Add(this.outputTextBox);
            this.Menu = this.mainMenu1;
            this.Name = "TestPMForm1";
            this.Text = "TestPMForm1";
            this.Load += new System.EventHandler(this.TestPMForm1_Load);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TextBox outputTextBox;
        private System.Windows.Forms.MenuItem leftButtonMenuItem;
        private System.Windows.Forms.MenuItem clearMenuItem;
        private System.Windows.Forms.MenuItem openSerialMenuItem;
        private System.Windows.Forms.MenuItem closeSerialMenuItem;
        private System.Windows.Forms.MenuItem cmdVersionMenuItem;
        private System.Windows.Forms.MenuItem saveMenuItem;
        private System.Windows.Forms.TextBox statusTextBox;
        private System.Windows.Forms.MenuItem appExitMenuItem;
        private System.Windows.Forms.MenuItem formCloseMenuItem;
        private System.Windows.Forms.MenuItem logEnabledMenuItem;
        private System.Windows.Forms.MenuItem menuItem1;
        private System.Windows.Forms.MenuItem startUmtMenuItem;
        private System.Windows.Forms.MenuItem stopUmtMenuItem;
        private System.Windows.Forms.MenuItem gpsMenuItem;
        private System.Windows.Forms.MenuItem gpsOpenMenuItem;
        private System.Windows.Forms.MenuItem gpsCloseMenuItem;
        private System.Windows.Forms.MenuItem gpsGetPositionMenuItem;
        private System.Windows.Forms.MenuItem gpsGetDeviceStateMenuItem;
        private System.Windows.Forms.MenuItem rightButtonMenuItem;
    }
}


//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
using System;
using System.Drawing;
using System.Collections;
using System.Windows.Forms;
using System.Data;
using Microsoft.WindowsMobile.Samples.Location;

namespace GpsTest
{
    /// <summary>
    /// Summary description for Form1.
    /// </summary>
    public class Form1 : System.Windows.Forms.Form
    {
        private System.Windows.Forms.MenuItem exitMenuItem;
        private System.Windows.Forms.MainMenu mainMenu1;
        private System.Windows.Forms.Label status;
        private MenuItem menuItem2;
        private MenuItem startGpsMenuItem;
        private MenuItem stopGpsMenuItem;


        private EventHandler updateDataHandler;
        GpsDeviceState device = null;
        GpsPosition position = null;

        Gps gps = new Gps();

        public Form1()
        {
            //
            // Required for Windows Form Designer support
            //
            InitializeComponent();

            //
            // TODO: Add any constructor code after InitializeComponent call
            //
        }
        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        protected override void Dispose( bool disposing )
        {
            base.Dispose( disposing );
        }
        #region Windows Form Designer generated code
        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.mainMenu1 = new System.Windows.Forms.MainMenu();
            this.exitMenuItem = new System.Windows.Forms.MenuItem();
            this.menuItem2 = new System.Windows.Forms.MenuItem();
            this.startGpsMenuItem = new System.Windows.Forms.MenuItem();
            this.stopGpsMenuItem = new System.Windows.Forms.MenuItem();
            this.status = new System.Windows.Forms.Label();
            // 
            // mainMenu1
            // 
            this.mainMenu1.MenuItems.Add(this.exitMenuItem);
            this.mainMenu1.MenuItems.Add(this.menuItem2);
            // 
            // exitMenuItem
            // 
            this.exitMenuItem.Text = "Exit";
            this.exitMenuItem.Click += new System.EventHandler(this.exitMenuItem_Click);
            // 
            // menuItem2
            // 
            this.menuItem2.MenuItems.Add(this.startGpsMenuItem);
            this.menuItem2.MenuItems.Add(this.stopGpsMenuItem);
            this.menuItem2.Text = "GPS";
            // 
            // startGpsMenuItem
            // 
            this.startGpsMenuItem.Text = "Start GPS";
            this.startGpsMenuItem.Click += new System.EventHandler(this.startGpsMenuItem_Click);
            // 
            // stopGpsMenuItem
            // 
            this.stopGpsMenuItem.Enabled = false;
            this.stopGpsMenuItem.Text = "Stop GPS";
            this.stopGpsMenuItem.Click += new System.EventHandler(this.stopGpsMenuItem_Click);
            // 
            // status
            // 
            this.status.Location = new System.Drawing.Point(0, 0);
            this.status.Size = new System.Drawing.Size(237, 173);
            this.status.Text = "label1";
            // 
            // Form1
            // 
            this.ClientSize = new System.Drawing.Size(240, 268);
            this.Controls.Add(this.status);
            this.Menu = this.mainMenu1;
            this.Text = "Form1";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.Closed += new System.EventHandler(this.Form1_Closed);

        }
        #endregion

        /// <summary>
        /// The main entry point for the application.
        /// </summary>

        static void Main() 
        {
            Application.Run(new Form1());
        }

        private void exitMenuItem_Click(object sender, EventArgs e)
        {
            if (gps.Opened)
            {
                gps.Close();
            }

            Close();
        }

        private void Form1_Load(object sender, System.EventArgs e)
        {
            updateDataHandler = new EventHandler(UpdateData);
         
            status.Text = "";
            
            status.Width = Screen.PrimaryScreen.WorkingArea.Width;
            status.Height = Screen.PrimaryScreen.WorkingArea.Height;

            gps.DeviceStateChanged += new DeviceStateChangedEventHandler(gps_DeviceStateChanged);
            gps.LocationChanged += new LocationChangedEventHandler(gps_LocationChanged);
        }

        protected void gps_LocationChanged(object sender, LocationChangedEventArgs args)
        {
            position = args.Position;

            // call the UpdateData method via the updateDataHandler so that we
            // update the UI on the UI thread
            Invoke(updateDataHandler);

        }

        void gps_DeviceStateChanged(object sender, DeviceStateChangedEventArgs args)
        {
            device = args.DeviceState;

            // call the UpdateData method via the updateDataHandler so that we
            // update the UI on the UI thread
            Invoke(updateDataHandler);
        }

        void UpdateData(object sender, System.EventArgs args)
        {
            if (gps.Opened)
            {
                string str = "";
                if (device != null)
                {
                    str = device.FriendlyName + " " + device.ServiceState + ", " + device.DeviceState + "\n";
                }

                if (position != null)
                {

                    if (position.LatitudeValid)
                    {
                        str += "Latitude (DD):\n   " + position.Latitude + "\n";
                        str += "Latitude (D,M,S):\n   " + position.LatitudeInDegreesMinutesSeconds + "\n";
                    }

                    if (position.LongitudeValid)
                    {
                        str += "Longitude (DD):\n   " + position.Longitude + "\n";
                        str += "Longitude (D,M,S):\n   " + position.LongitudeInDegreesMinutesSeconds + "\n";
                    }

                    if (position.SatellitesInSolutionValid &&
                        position.SatellitesInViewValid &&
                        position.SatelliteCountValid)
                    {
                        str += "Satellite Count:\n   " + position.GetSatellitesInSolution().Length + "/" +
                            position.GetSatellitesInView().Length + " (" +
                            position.SatelliteCount + ")\n";
                    }

                    if (position.TimeValid)
                    {
                        str += "Time:\n   " + position.Time.ToString() + "\n";
                    }
                }

                status.Text = str;

            }
        }

        private void Form1_Closed(object sender, System.EventArgs e)
        {
            if (gps.Opened)
            {
                gps.Close();
            }
        }

        private void stopGpsMenuItem_Click(object sender, EventArgs e)
        {
            if (gps.Opened)
            {
                gps.Close();
            }

            startGpsMenuItem.Enabled = true;
            stopGpsMenuItem.Enabled = false;
        }

        private void startGpsMenuItem_Click(object sender, EventArgs e)
        {
            if (!gps.Opened)
            {
                gps.Open();
            }

            startGpsMenuItem.Enabled = false;
            stopGpsMenuItem.Enabled = true;
        }
    }
}

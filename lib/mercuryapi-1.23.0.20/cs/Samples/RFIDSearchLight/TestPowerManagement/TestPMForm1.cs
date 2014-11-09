using Microsoft.WindowsMobile.Samples.Location;
using System;
using System.Linq;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.IO.Ports;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using ThingMagic;
using ThingMagic.RFIDSearchLight;

namespace TestPowerManagement
{
    public partial class TestPMForm1 : Form
    {
        SerialReader rdr;
        StreamWriter log = new StreamWriter("log.txt", true);
        bool logEnabled = true;

        public TestPMForm1()
        {
            InitializeComponent();
            PrintLn("App started");
            Utilities.Logger.Log += new LogProvider.LogHandler(Utilities_Logger_Log);
            Utilities.Logger.Log += new LogProvider.LogHandler(Utilities_Status);
        }

        private void TestPMForm1_Load(object sender, EventArgs e)
        {
            PrintStatus("Initializing...");
            InitializePowerManager();
            logEnabledMenuItem.Checked = logEnabled;
            PrintStatus("Ready");
            initUmt();
            PrintLn("Initialization Complete");
        }

        void Utilities_Logger_Log(string message)
        {
            PrintLn("Utilities: " + message);
        }
        void Utilities_Status(string message)
        {
            PrintStatus(message);
        }

        private delegate void ClearTextCallback(Control control);
        public void ClearText(Control control)
        {
            if (control.InvokeRequired)
            {
                control.Invoke(new ClearTextCallback(ClearText), control);
            }
            else
            {
                control.Text = "";
            }
        }
        private delegate void PrintTextCallback(TextBox textbox, string text);
        public void PrintText(TextBox textbox, string text)
        {
            if (textbox.InvokeRequired)
            {
                textbox.Invoke(new PrintTextCallback(PrintText), textbox, text);
            }
            else
            {
                textbox.Text += text;
                textbox.SelectionStart = textbox.Text.Length;
                textbox.ScrollToCaret();
            }
        }
        void PrintLn(string text)
        {
            string message = Environment.TickCount + " " + text + "\r\n";
            PrintText(outputTextBox, message);
            if (logEnabled)
            {
                log.Write(message);
                log.Flush();
            }
        }

        private delegate void SetTextCallback(TextBox textbox, string text);
        public void SetText(TextBox textbox, string text)
        {
            if (textbox.InvokeRequired)
            {
                textbox.Invoke(new SetTextCallback(SetText), textbox, text);
            }
            else
            {
                textbox.Text = text;
            }
        }
        void PrintStatus(string text, bool copyToLog)
        {
            string message = text;
            SetText(statusTextBox, text);
            if (copyToLog)
            {
                PrintLn(message);
            }
        }
        void PrintStatus(string text)
        {
            PrintStatus(text, false);
        }

        private delegate void SaveTextCallback();
        public void SaveText()
        {
            if (outputTextBox.InvokeRequired)
            {
                outputTextBox.Invoke(new SaveTextCallback(SaveText));
            }
            else
            {
                using (StreamWriter writer = new StreamWriter("save.txt"))
                {
                    writer.Write(outputTextBox.Text);
                }
            }
        }

        PowerManager pwrMngr;
        private void InitializePowerManager()
        {
            // Create a new instance of the PowerManager class
            pwrMngr = new PowerManager();

            // Hook up the Power notify event. This event would not be activated 
            // until EnableNotifications is called. 
            pwrMngr.PowerNotify += new PowerManager.PowerEventHandler(OnPowerNotify);

            // Enable power notifications. This will cause a thread to start
            // that will fire the PowerNotify event when any power notification 
            // is received.
            pwrMngr.EnableNotifications();

            //// Get the current power state. 
            //StringBuilder systemStateName = new StringBuilder(20);
            //PowerManager.SystemPowerStates systemState;
            //int nError = pwrMngr.GetSystemPowerState(systemStateName, out systemState);
        }

        /// <summary>
        /// True if a system resume has been detected but we have not cleaned up after it yet.
        /// We cannot clean up immediately after a resume, because our COM port will not have been restored yet.
        /// Have to wait for one more Transistion event to pass.
        /// </summary>
        bool NeedResumeRecovery = false;
        bool SuspendTripped = false;
        int TransitionsSinceSuspend = 0;
        private void OnPowerNotify(object sender, PowerManager.PowerEventArgs e)
        {
            PowerManager.PowerInfo powerInfo = e.PowerInfo;

            PrintLn(
                powerInfo.Message.ToString()
                + " " + powerInfo.Flags.ToString("X")
                + " " + GetSerialPortList()
                );

            if (PowerManager.MessageTypes.Transition == powerInfo.Message)
            {
                // Ideally, we'd switch on the symbolic Flags enums,
                // but they don't cover all the observed values.
                switch ((uint)powerInfo.Flags)
                {
                    case 0x00400000:
                        // 0x00400000 is the lightest stage of power management,
                        // probably corresponding to backlight off.  It occurs
                        // even when USB doesn't get powered down.
                        //
                        // However, we should react to it because it is always
                        // the first thing our app sees when coming out of suspend.
                        // Sometimes, waking is very slow, and the user will be confused
                        // unless we trigger some feedback at the time of this first event.
                        PrintStatus("Power interruption detected.  Recalculating...", true);
                        CloseReader();
                        break;
                    case 0x00200000:
                        // 0x00200000 == SystemPowerStates.Suspend
                        PrintStatus("Suspend detected", true);
                        SuspendTripped = true;
                        TransitionsSinceSuspend = 0;
                        break;
                    case 0x10000000:
                        // 0x00100000 == SystemPowerStates.Idle
                        // Not sure why Idle is always observed *after* Suspend.
                        // I thought Suspend was the deeper sleep state; e.g.,
                        // http://nicolasbesson.blogspot.com/2008/04/power-management-under-window-ce-part.html
                        break;
                    case 0x12010000:
                        // 0x12010000 occurs when subsystems are powering up
                        // ActiveSync is one of these subsystems, so we can't do
                        // too much with this event, or we'll get false positives.
                        //
                        // The first 1201000 after Resume tends to correlate with the
                        // FTDI serial port being back up, but not sure if order is guaranteed.
                        //
                        // There are usually 2 1201000 transitions after waking from resume.
                        // There is a third one, several second later, when ActiveSync enables.
                        TransitionsSinceSuspend += 1;
                        PrintStatus("Detected " + TransitionsSinceSuspend + " transitions since suspend", true);
                        if (SuspendTripped && (2 <= TransitionsSinceSuspend))
                        {
                            if (null != rdr)
                            {
                                PrintStatus("Cleaning up reader connection...", true);
                                CloseReader();
                                PrintStatus("Reopening reader...", true);
                                OpenReader();
                            }
                            SuspendTripped = false;
                        }
                        break;
                }
            }

            //switch (powerInfo.Message)
            //{
            //    case PowerManager.MessageTypes.Resume:
            //        PrintLn("Resume Detected");
            //        NeedResumeRecovery = true;
            //        break;
            //    case PowerManager.MessageTypes.Transition:
            //        if (NeedResumeRecovery)
            //        {
            //            // Reader's serial port becomes invalid during suspend
            //            // Restore it here (if it was already open before suspend)
            //            if (null != rdr)
            //            {
            //                PrintStatus("Reconnecting...");

            //                // See how much we can get away with
            //                try
            //                {
            //                    GetVersion();
            //                }
            //                catch (Exception ex)
            //                {
            //                    // We can catch the serial write exception here, but I'm not sure why.
            //                    // Isn't it supposed to happen in the GUI thread due to Invoke?
            //                    PrintLn("Caught Exception trying to use broken reader: " + ex.ToString());
            //                }
            //                // Clean up old reader (to prevent SerialReader.Dispose method
            //                // from trying to talk to it and throwing an exception somewhere 
            //                // we can't handle it.
            //                PrintStatus("Cleaning up old reader...");
            //                PrintLn("Cleaning up old reader...");
            //                CloseReader();
            //                PrintStatus("Reopening reader...");
            //                PrintLn("Reopening reader...");
            //                OpenReader();
            //                PrintStatus("Connected");
            //            }
            //            NeedResumeRecovery = false;
            //        }
            //        break;
            //}
        }

        string GetSerialPortList()
        {
            return String.Join(" ", SerialPort.GetPortNames());
        }

        private void rightButtonMenuItem_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void clearMenuItem_Click(object sender, EventArgs e)
        {
            ClearText(outputTextBox);
        }

        private void openSerialMenuItem_Click(object sender, EventArgs e)
        {
            if (null != rdr)
            {
                PrintLn("Reader already opened");
            }
            else
            {
                OpenReader();
            }
        }

        private void OpenReader()
        {
            PrintLn("Opening reader...");
            rdr = (SerialReader)Utilities.ConnectReader();
            //rdr = (SerialReader)Reader.Create("eapi:///" + serialPortName);
            rdr.ParamSet("/reader/baudRate", 9600);
            //rdr.Log += new SerialReader.LogHandler(rdr_Log);
            rdr.Connect();
            PrintLn("Opened reader on port " + Utilities.ReaderPortName);
        }

        void rdr_Log(string message)
        {
            PrintLn("SerialReader: " + message);
        }

        private void closeSerialMenuItem_Click(object sender, EventArgs e)
        {
            if (null == rdr)
            {
                PrintLn("Reader not open");
            }
            else
            {
                CloseReader();
            }
        }

        private void CloseReader()
        {
            if (null == rdr)
            {
                PrintLn("Reader already closed");
            }
            else
            {
                PrintLn("Closing reader...");
                string portName = Utilities.ReaderPortName;
                try
                {
                    rdr.Destroy();
                }
                catch (Exception ex)
                {
                    PrintLn("Caught Exception closing reader: " + ex);
                }
                finally
                {
                    rdr = null;
                }
                PrintLn("Closed reader on serial port " + portName);
            }
        }

        private void cmdVersionMenuItem_Click(object sender, EventArgs e)
        {
            if (null == rdr)
            {
                PrintLn("Reader not open");
            }
            else
            {
                GetVersion();
            }
        }

        private void GetVersion()
        {
            PrintLn("Firmware Version: " + rdr.CmdVersion().Firmware.ToString());
        }

        private void saveMenuItem_Click(object sender, EventArgs e)
        {
            SaveText();
        }

        private void appExitMenuItem_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void formCloseMenuItem_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void logEnabledMenuItem_Click(object sender, EventArgs e)
        {
            logEnabledMenuItem.Checked = !logEnabledMenuItem.Checked;
            logEnabled = logEnabledMenuItem.Checked;
        }

        // Unattended Mode test
        Thread umtThread = null;
        ManualResetEvent umtRun = new ManualResetEvent(false);
        System.Media.SoundPlayer umtSound = new System.Media.SoundPlayer("\\Windows\\Voicbeep.wav");
        private void initUmt()
        {
            if (null == umtThread)
            {
                umtThread = new Thread(new ThreadStart(delegate()
                {
                    while (true)
                    {
                        umtRun.WaitOne();
                        umtSound.PlaySync();
                        Thread.Sleep(1000);
                    }
                }));
                umtThread.IsBackground = true;
                umtThread.Start();
            }
        }
        static IntPtr nullIntPtr = new IntPtr(0);
        IntPtr wavPowerReqHandle = nullIntPtr;
        private void startUmtMenuItem_Click(object sender, EventArgs e)
        {
            // See Unattended Mode example at http://stackoverflow.com/questions/336771/how-can-i-run-code-on-windows-mobile-while-being-suspended
            CoreDLL.PowerPolicyNotify(PPNMessage.PPN_UNATTENDEDMODE, 1);
            // Request that the audio device remain powered in unattended mode
            // NOTE: If you don't do this, the system suspends anyway 1 minute after you press the power button
            // (USB reader power light goes out, system publishes a Suspend notification.)
            // With this request, it seems to run indefinitely.
            wavPowerReqHandle = CoreDLL.SetPowerRequirement("wav1:", CEDeviceDriverPowerStates.D0,
                DevicePowerFlags.POWER_NAME | DevicePowerFlags.POWER_FORCE,
                new IntPtr(0), 0);
            umtRun.Set();
        }

        private void stopUmtMenuItem_Click(object sender, EventArgs e)
        {
            umtRun.Reset();
            if (nullIntPtr != wavPowerReqHandle)
            {
                CoreDLL.ReleasePowerRequirement(wavPowerReqHandle);
                wavPowerReqHandle = nullIntPtr;
            }
            CoreDLL.PowerPolicyNotify(PPNMessage.PPN_UNATTENDEDMODE, 0);
        }

        private Gps _gps = new Gps();
        private bool _gpsInitialized = false;
        GpsPosition GpsPosition;
        GpsDeviceState GpsDeviceState;

        private void RequireGpsOpen()
        {
            if (!_gpsInitialized) { InitGps(); }
            if (!_gps.Opened) { GpsOpen(); }
        }

        private void InitGps()
        {
            if (!_gpsInitialized)
            {
                //_gps.DeviceStateChanged += new DeviceStateChangedEventHandler(_gps_DeviceStateChanged);
                //_gps.LocationChanged += new LocationChangedEventHandler(_gps_LocationChanged);
                //_gpsInitialized = true;
            }
        }
        private void UninitGps()
        {
            if (_gpsInitialized)
            {
                _gps.DeviceStateChanged -= new DeviceStateChangedEventHandler(_gps_DeviceStateChanged);
                _gps.LocationChanged -= new LocationChangedEventHandler(_gps_LocationChanged);
                _gpsInitialized = false;
            }
        }

        string ToString(GpsPosition pos)
        {
            return ""
                + (pos.LatitudeValid ? "" : "(INVALID)")
                + pos.Latitude.ToString("F6")
                + ","
                + (pos.LongitudeValid ? "" : "(INVALID)")
                + pos.Longitude.ToString("F6");
        }
        void _gps_LocationChanged(object sender, LocationChangedEventArgs args)
        {
            GpsPosition = args.Position;
            PrintLn("GPS Location Changed: " + ToString(GpsPosition));
        }

        void _gps_DeviceStateChanged(object sender, DeviceStateChangedEventArgs args)
        {
            GpsDeviceState = args.DeviceState;
            PrintLn("GPS Device State Changed: " + GpsDeviceState);
        }

        private void gpsOpenMenuItem_Click(object sender, EventArgs e)
        {
            GpsOpen();
        }
        private void GpsOpen()
        {
            if (_gps.Opened)
            {
                PrintLn("GPS already opened");
            }
            else
            {
                _gps.Open();
                PrintLn("Opened GPS");
            }
        }

        private void gpsCloseMenuItem_Click(object sender, EventArgs e)
        {
            if (!_gps.Opened)
            {
                PrintLn("GPS is not open");
            }
            else
            {
                //UninitGps();
                _gps.Close();
                PrintLn("Closed GPS");
            }
        }

        private void gpsGetPositionMenuItem_Click(object sender, EventArgs e)
        {
            RequireGpsOpen();
            PrintLn(ToString(_gps.GetPosition()));
        }

        private void gpsGetDeviceStateMenuItem_Click(object sender, EventArgs e)
        {
            RequireGpsOpen();
            PrintLn(_gps.GetDeviceState().ToString());
        }

        private void outputTextBox_KeyPress(object sender, KeyPressEventArgs e)
        {
            PrintLn("KeyPress: " + e.KeyChar.ToString());
            e.Handled = true;
        }

        private void outputTextBox_KeyDown(object sender, KeyEventArgs e)
        {
            PrintLn("KeyDown:"
                + " KeyCode=" + e.KeyCode.ToString()
                + " KeyData=" + e.KeyData.ToString()
                + " KeyValue=" + e.KeyValue.ToString()
                + " Modifiers=" + e.Modifiers.ToString()
                );
            e.Handled = true;
        }

        private void outputTextBox_KeyUp(object sender, KeyEventArgs e)
        {
            PrintLn("KeyUp:"
                + " KeyCode=" + e.KeyCode.ToString()
                + " KeyData=" + e.KeyData.ToString()
                + " KeyValue=" + e.KeyValue.ToString()
                + " Modifiers=" + e.Modifiers.ToString()
                );
            e.Handled = true;
        }
    }
}
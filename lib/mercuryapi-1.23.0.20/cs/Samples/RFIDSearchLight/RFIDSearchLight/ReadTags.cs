using System;
using System.Linq;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Threading;
using System.IO;
using Microsoft.WindowsMobile.Samples.Location;
using System.Runtime.InteropServices;
using System.Media;
using ThingMagic;
using Microsoft.Win32;
using Microsoft.WindowsMobile;
using Microsoft.WindowsMobile.Status;
using System.Diagnostics;

namespace ThingMagic.RFIDSearchLight
{
    public partial class ReadTags : Form
    {
        log4net.ILog logger = log4net.LogManager.GetLogger("ReadTags");
        private Dictionary<string, string> properties;
        List<int> ant = new List<int>();
        string readerName = "Thingmagic USB Reader";
        delegate void OutputUpdateDelegate(TagReadData[] griddata);
        TagDatabase tagdb = new TagDatabase();
        delegate void OutputUpdateDelegateBigNum(string data);

        [DllImport("coredll.dll")]
        public static extern void SystemIdleTimerReset();

        [DllImport("aygshell.dll", SetLastError = true)]
        public static extern void SHIdleTimerReset();

        const byte KEYEVENTF_KEYUP = 0x0002;
        const byte KEYEVENTF_SILENT = 0x0004;
        const byte VK_LBUTTON = 0x01;

        [DllImport("coredll.dll")]
        static extern void keybd_event(byte bVk, byte bScan, int dwFlags, int dwExtraInfo);
        //IntPtr hcomport = IntPtr.Zero;
        System.Windows.Forms.Timer tmrBackLightControl;

        /// <summary>
        /// Define a variable for the transportlistener log
        /// </summary>
        TextWriter transportLogFile = null;

        //Pistol grip
        SystemState pistolTriggerEvent;
        //Sounds
        SoundPlayer playBeep = new SoundPlayer();
        SoundPlayer playStartAudio = new SoundPlayer();
        SoundPlayer playStopAudio = new SoundPlayer();

        CoreDLL.SYSTEM_POWER_STATUS_EX status;

        static void DiskLog(string message)
        {
            Utilities.DiskLog.Log("ReadTags: " + message);
        }

        public ReadTags()
        {
            InitializeComponent();
        }

        private NotifyIcon pvtNotifyIcon;

        public ReadTags(NotifyIcon notifyIcon)
        {
            try
            {
                InitializeComponent();
                dgTagResult.DataSource = tagdb.TagList;
                //Set the column width of datagrid
                AddGridStyles();

                properties = Utilities.GetProperties();

                playBeep.SoundLocation = properties["decodewavefile"];
                playBeep.LoadAsync();
                playStopAudio.SoundLocation = properties["endscanwavefile"];
                playStopAudio.LoadAsync();
                playStartAudio.SoundLocation = properties["startscanwavefile"];
                playStartAudio.LoadAsync();

                setStatus(Status.READY);
                pvtNotifyIcon = notifyIcon;

                //For battery level
                status = new CoreDLL.SYSTEM_POWER_STATUS_EX();

                tmrBackLightControl = new System.Windows.Forms.Timer();
                tmrBackLightControl.Interval = 1000 * 30;// Timer will tick every 30 seconds
                tmrBackLightControl.Tick += new EventHandler(tmrBackLightControl_Tick);

                //Pistol grip read handler
                pistolTriggerEvent = new SystemState(SystemProperty.HeadsetPresent);
                pistolTriggerEvent.Changed += new ChangeEventHandler(pistolTriggerEvent_Changed);
                pistolTriggerEvent.DisableApplicationLauncher();
            }
            catch (Exception ex)
            {
                logger.Error("In ReadTags: " + ex.ToString());
                MessageBox.Show(ex.Message);
            }
        }

        private void playStartSound()
        {
            try
            {
                playStartAudio.Play();
            }
            catch (Exception ex)
            {
                logger.Error("In playStartSound: " + ex.ToString());
            }
        }

        private void playStopSound()
        {
            try
            {
                playStopAudio.Play();
            }
            catch (Exception ex)
            {
                logger.Error("In playStopSound: " + ex.ToString());
            }
        }

        private void playBeepSound()
        {
            try
            {
                playBeep.Play();
            }
            catch (Exception ex)
            {
                logger.Error("In playBeepSound: " + ex.ToString());
            }
        }

        private void ReadMgrSubscribe()
        {
            ReadMgr.PowerLog.Log += new LogProvider.LogHandler(PowerLog_Log);
            ReadMgr.ReaderEvent += new ReadMgr.ReadMgrEventHandler(ReadMgr_ReaderEvent);
            ReadMgr.DebugLog.Log += SetDebug;
        }

        private void ReadMgrUnsubscribe()
        {
            ReadMgr.PowerLog.Log -= new LogProvider.LogHandler(PowerLog_Log);
            ReadMgr.ReaderEvent -= new ReadMgr.ReadMgrEventHandler(ReadMgr_ReaderEvent);
            ReadMgr.DebugLog.Log -= SetDebug;
        }

        ReadMgr.EventCode ReadMgr_ReaderEvent_State = ReadMgr.EventCode.READER_RECOVERED;
        void ReadMgr_ReaderEvent(ReadMgr.EventCode code)
        {
            BeginInvoke((Action)delegate
            {
                ReadMgr_ReaderEvent_State = code;
                switch (code)
                {
                    case ReadMgr.EventCode.POWER_INTERRUPTED:
                        Cursor.Current = Cursors.WaitCursor;
                        setStatus("Power Interrupted", Color.Yellow);
                        SetDebug("");
                        break;
                    case ReadMgr.EventCode.POWER_RESTORED:
                        setStatus("Power Restored");
                        Cursor.Current = Cursors.Default;
                        setStatus(Status.READY);
                        break;
                    case ReadMgr.EventCode.READER_SHUTDOWN:
                        logger.Debug("Calling GuiStopReads from ReadMgr_ReaderEvent");
                        GuiStopReads();
                        logger.Debug("Called GuiStopReads from ReadMgr_ReaderEvent");
                        setStatus(Status.IDLE);
                        break;
                    case ReadMgr.EventCode.READER_RECOVERING:
                        btnStartReads.Invoke((Action)delegate
                        {
                            btnStartReads.Text = "Start Reads";
                        });
                        Cursor.Current = Cursors.WaitCursor;
                        setStatus("Reconnecting...", Color.Yellow);
                        break;
                    case ReadMgr.EventCode.READER_RECOVERED:
                        try
                        {
                            //if (properties["isreading"] == "yes")
                            //{
                            //    // Give reader time to come up to working state
                            //    // before hammering it with Stop Reads commands
                            //    Thread.Sleep(1000);
                            //    StopReads();
                            //}
                            //setStatus(Status.READY);

                            setStatus(Status.READY);
                            if (properties["isreading"] == "yes")
                            {
                                ReadtheTags();
                            }
                        }
                        catch (Exception ex)
                        {
                            logger.Error("In ReadMgr_ReaderEvent" + ex.ToString());
                            setStatus(ex.Message, Color.Red);
                            Utilities.DiskLog.Log("Caught Exception in ReadTags.READER_RECOVERED:\r\n" + ex.ToString());
                        }
                        finally
                        {
                            Cursor.Current = Cursors.Default;
                        }
                        break;
                    case ReadMgr.EventCode.READER_RECOVERY_FAILED:
                        Cursor.Current = Cursors.Default;
                        setStatus(ReadMgr.lastErrorMessage, Color.Red);
                        break;
                }
            });
        }

        void PowerLog_Log(string message)
        {
            BeginInvoke((Action)delegate
            {
                powerLabel.Text = message;
                if (ReadMgr.EventCode.POWER_INTERRUPTED == ReadMgr_ReaderEvent_State)
                {
                    SetDebug(message);
                }
            });
        }

        private delegate void IntiatePistolRead();

        private void PistolRead()
        {
            try
            {
                //Read can be triggered either by pressing the pistolgrip button or by tapping
                // the "Start Reades". So when read is activated by one event, and other event
                // should not disturb it. So if reading is already in progress, just return.
                if (!readTriggeredByTap)
                {
                    readTriggeredByPistol = true;
                    ReadtheTags();
                }
            }
            catch (Exception ex)
            {
                logger.Error("In PistolRead: " + ex.ToString());
                Thread.Sleep(200);
                try
                {
                    logger.Debug("Reconnecting...");
                    ReadMgr.ForceReconnect();
                    logger.Debug("ReadMgr.ForceReconnect completed");
                    ReadtheTags();
                }
                catch (Exception ex2)
                {
                    logger.Error(ex2.ToString());
                    MessageBox.Show(ex2.Message);
                }
            }
        }

        void pistolTriggerEvent_Changed(object sender, ChangeEventArgs args)
        {
            try
            {
                this.BeginInvoke(new IntiatePistolRead(PistolRead));
            }
            catch (Exception ex)
            {
                logger.Error(ex.ToString());
                MessageBox.Show(ex.Message);
            }
        }

        void AddGridStyles()
        {
            DataGridTableStyle tableStyle = new DataGridTableStyle();
            tableStyle.MappingName = tagdb.TagList.GetType().Name;
            
            DataGridTextBoxColumn epc = new DataGridTextBoxColumn();
            epc.Width = 280;
            epc.MappingName = "EPC";
            epc.HeaderText = "EPC";
            tableStyle.GridColumnStyles.Add(epc);

            DataGridTextBoxColumn rssi = new DataGridTextBoxColumn();
            rssi.Width = 75;
            rssi.MappingName = "RSSI";
            rssi.HeaderText = "RSSI";
            tableStyle.GridColumnStyles.Add(rssi);

            DataGridTextBoxColumn readCount = new DataGridTextBoxColumn();
            readCount.Width = 130;
            readCount.MappingName = "ReadCount";
            readCount.HeaderText = "Read Count";
            tableStyle.GridColumnStyles.Add(readCount);

            DataGridTextBoxColumn timeStamp = new DataGridTextBoxColumn();
            timeStamp.Width = 180;
            timeStamp.MappingName = "TimeStamp";
            timeStamp.HeaderText = "Time Stamp";
            tableStyle.GridColumnStyles.Add(timeStamp);

            DataGridTextBoxColumn tbcName = new DataGridTextBoxColumn();
            tbcName.Width = 275;
            tbcName.MappingName = "TagId";
            tbcName.HeaderText = "Tag Id";
            tableStyle.GridColumnStyles.Add(tbcName);

            DataGridTextBoxColumn readerName = new DataGridTextBoxColumn();
            readerName.Width = 180;
            readerName.MappingName = "ReaderName";
            readerName.HeaderText = "Reader Name";
            tableStyle.GridColumnStyles.Add(readerName);

            DataGridTextBoxColumn position = new DataGridTextBoxColumn();
            position.Width = 240;
            position.MappingName = "Position";
            position.HeaderText = "Position";
            tableStyle.GridColumnStyles.Add(position);

            dgTagResult.TableStyles.Clear();
            dgTagResult.TableStyles.Add(tableStyle);
        }

        private void tmrBackLightControl_Tick(object sender, EventArgs e)
        {
            SystemIdleTimerReset();
            SHIdleTimerReset();
            keybd_event(VK_LBUTTON, 0, KEYEVENTF_SILENT, 0);
            keybd_event(VK_LBUTTON, 0, KEYEVENTF_KEYUP | KEYEVENTF_SILENT, 0);
        }

        private void ReadTags_Load(object sender, EventArgs e)
        {
            try
            {
                ReadMgrSubscribe();
                debugLabel.Text = "";
                powerLabel.Hide();
                int readPower = Convert.ToInt32(properties["readpower"].ToString());
                tbTXPower.Value = (readPower - 1000) / 50;
                properties["isreading"] = "no";
                Utilities.SaveConfigurations(properties);
            }
            catch (Exception ex)
            {
                logger.Error(ex.ToString());
                MessageBox.Show(ex.Message);
                //Supply the power to usb port
                //CoreDLL.PowerPolicyNotify(PPNMessage.PPN_UNATTENDEDMODE, -1);
                //CoreDLL.SetPowerRequirement(properties["comport"] + ":", CEDeviceDriverPowerStates.D0, DevicePowerFlags.POWER_NAME, IntPtr.Zero, 0);
            }
        }

        bool readTriggeredByTap = false;
        bool readTriggeredByPistol = false;

        private void btnStartReads_Click(object sender, EventArgs e)
        {
            try
            {
                //Read can be triggered either by pressing the pistolgrip button or by tapping
                // the "Start Reades". So when read is activated by one event, and other event
                // should not disturb it. So if reading is already in progress, just return.
                if (!readTriggeredByPistol)
                {
                    readTriggeredByTap = true;
                    ReadtheTags();
                    CancelEventArgs eClose = new CancelEventArgs(true);
                }
            }
            catch (Exception ex)
            {
                logger.Error(ex.ToString());
                try
                {
                    ReadMgr.ForceReconnect();
                    ReadtheTags();
                }
                catch (Exception ex2)
                {
                    logger.Error(ex2.ToString());
                    //do nothing
                }
            }
        }

        private void ReadtheTags()
        {
            try
            {
                // Make sure reader is connected
                ReadMgr.GetReader();

                System.Text.ASCIIEncoding encoding = new System.Text.ASCIIEncoding();

                if (btnStartReads.Text == "Start Reads")
                {
                    Cursor.Current = Cursors.WaitCursor;
                    try
                    {
                        //Check the battery power level
                        if (CoreDLL.GetSystemPowerStatusEx(status, false) == 1)
                        {
                            if (status.BatteryLifePercent <= 5)
                            {
                                if (status.ACLineStatus == 0)
                                {
                                    MessageBox.Show("Battery level is low to read tags");
                                    return;
                                }
                            }
                        }
                        properties["isreading"] = "yes";
                        Utilities.SaveConfigurations(properties);

                        //disable read power coverage
                        tbTXPower.Enabled = false;

                        ReadMgr.GetReader().ParamSet("/reader/transportTimeout", 2000);
                        int powerLevel = Convert.ToInt32(properties["readpower"]);
                        ReadMgr.GetReader().ParamSet("/reader/radio/readPower", powerLevel);
                        Utilities.SwitchRegion(properties["region"]);
                        ReadMgr.GetReader().ParamSet("/reader/antenna/txRxMap", new int[][] { new int[] { 1, 1, 1 } });
                        ant.Add(1);
                        SimpleReadPlan plan = new SimpleReadPlan(ant.ToArray(), TagProtocol.GEN2);
                        ReadMgr.GetReader().ParamSet("/reader/read/plan", plan);
                        //int readPower = Convert.ToInt32(properties["readpower"].ToString()) * 100;
                        //tbTXPower.Value = (readPower - 1000) / 50;

                        tmrBackLightControl.Enabled = true;
                        miGoToMain.Enabled = false;

                        //set properties
                        ReadMgr.GetReader().ParamSet("/reader/read/asyncOffTime", 50);
                        ReadMgr.GetReader().ParamSet("/reader/powerMode", Reader.PowerMode.FULL);

                        //set the tag population settings
                        ReadMgr.GetReader().ParamSet("/reader/gen2/target", Gen2.Target.A);//default target
                        string tagPopulation = properties["tagpopulation"];
                        switch (tagPopulation)
                        {
                            case "small":
                                ReadMgr.GetReader().ParamSet("/reader/gen2/q", new Gen2.StaticQ(2));
                                ReadMgr.GetReader().ParamSet("/reader/gen2/session", Gen2.Session.S0);
                                ReadMgr.GetReader().ParamSet("/reader/gen2/tagEncoding", Gen2.TagEncoding.M4);
                                break;
                            case "medium":
                                ReadMgr.GetReader().ParamSet("/reader/gen2/q", new Gen2.StaticQ(4));
                                ReadMgr.GetReader().ParamSet("/reader/gen2/session", Gen2.Session.S1);
                                ReadMgr.GetReader().ParamSet("/reader/gen2/tagEncoding", Gen2.TagEncoding.M4);
                                break;
                            case "large":
                                ReadMgr.GetReader().ParamSet("/reader/gen2/q", new Gen2.StaticQ(6));
                                ReadMgr.GetReader().ParamSet("/reader/gen2/session", Gen2.Session.S1);
                                ReadMgr.GetReader().ParamSet("/reader/gen2/tagEncoding", Gen2.TagEncoding.M2);
                                break;
                            default: break;
                        }

                        if (null != properties)
                        {
                            Utilities.SetReaderSettings(ReadMgr.GetReader(), properties);
                        }
                        else
                            MessageBox.Show("properties are null");
                        //set the read plan and filter
                        TagFilter filter;
                        int addressToRead = int.Parse(properties["selectionaddress"]);
                        Gen2.Bank bank = Gen2.Bank.EPC;
                        switch (properties["tagselection"].ToLower())
                        {
                            case "None":
                            case "epc": bank = Gen2.Bank.EPC; break;
                            case "tid": bank = Gen2.Bank.TID; break;
                            case "user": bank = Gen2.Bank.USER; break;
                            case "reserved": bank = Gen2.Bank.RESERVED; break;
                            default: break;

                        }
                        if ("yes" == properties["ismaskselected"])
                        {
                            filter = new Gen2.Select(true, bank, (uint)addressToRead * 8, (ushort)(properties["selectionmask"].Length * 4), ByteFormat.FromHex(properties["selectionmask"]));
                        }
                        else
                        {
                            filter = new Gen2.Select(false, bank, (uint)addressToRead * 8, (ushort)(properties["selectionmask"].Length * 4), ByteFormat.FromHex(properties["selectionmask"]));
                        }

                        SimpleReadPlan srp;
                        if (properties["tagselection"].ToLower() == "none")
                        {
                            srp = new SimpleReadPlan(new int[] { 1 }, TagProtocol.GEN2, null, 0);
                        }
                        else
                        {
                            srp = new SimpleReadPlan(new int[] { 1 }, TagProtocol.GEN2, filter, 0);
                        }
                        ReadMgr.GetReader().ParamSet("/reader/read/plan", srp);

                        btnStartReads.Text = "Stop Reads";
                        setStatus("Reading", System.Drawing.Color.DarkGoldenrod);
                        ReadMgr.GetReader().ReadException += ReadException;
                        ReadMgr.GetReader().TagRead += PrintTagRead;
                        ReadMgr.GetReader().StartReading();
                        if (properties["audiblealert"].ToLower() == "yes")
                        {
                            if (readTriggeredByTap)
                            {
                                playStartSound();
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        logger.Error(ex.ToString());
                        tbTXPower.Enabled = true;
                        //MessageBox.Show("Error connecting to reader: " + ex.Message.ToString(), "Error!", MessageBoxButtons.OK, MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button1);
                        btnStartReads.Text = "Start Reads";
                        setStatus(Status.IDLE);
                        ReadMgr.GetReader().ParamSet("/reader/powerMode", Reader.PowerMode.MAXSAVE);
                        miGoToMain.Enabled = true;
                        tmrBackLightControl.Enabled = false;
                        properties["isreading"] = "no";
                        Utilities.SaveConfigurations(properties);
                        throw ex;
                    }
                    finally
                    {
                        Cursor.Current = Cursors.Default;
                    }
                }
                else if (btnStartReads.Text == "Stop Reads")
                {
                    logger.Debug("Stop Reads pressed: Calling StopReads from ReadtheTags");
                    StopReads();
                    logger.Debug("Stop Reads pressed: Called StopReads from ReadtheTags");
                }
            }
            catch (Exception ex)
            {
                logger.Error(ex.ToString());
                if (-1 != ex.Message.IndexOf("RFID reader was not found"))
                {
                    MessageBox.Show(ex.Message, "Error");
                }
                else
                {
                    btnStartReads.Text = "Start Reads";
                    setStatus(Status.IDLE);
                    properties["isreading"] = "no";
                    Utilities.SaveConfigurations(properties);
                    throw ex;
                }
            }
        }

        private void StopReads()
        {
            Invoke((Action)delegate
            {
                if (properties["isreading"] == "yes")
                {
                    tbTXPower.Enabled = true;

                    miGoToMain.Enabled = true;
                    tbTXPower.Enabled = true;
                    logger.Debug("Calling StopReading from StopReads");
                    ReadMgr.GetReader().StopReading();
                    logger.Debug("Called StopReading from StopReads");
                    ReadMgr.GetReader().TagRead -= PrintTagRead;
                    ReadMgr.GetReader().ReadException -= ReadException;
                    setStatus(Status.READY);
                    ReadMgr.GetReader().ParamSet("/reader/powerMode", Reader.PowerMode.MINSAVE);
                    properties["isreading"] = "no";
                    properties["powermode"] = "minsave";
                    Utilities.SaveConfigurations(properties);
                    ReadMgr.AllowDisconnect();

                    logger.Debug("Calling GuiStopReads from StopReads");
                    GuiStopReads();
                    logger.Debug("Called GuiStopReads from StopReads");

                    //Change to initial values
                    readTriggeredByTap = false;
                    readTriggeredByPistol = false;
                    tmrBackLightControl.Enabled = false;
                }
            });
        }

        /// <summary>
        /// Update UI to indicate that reading has stopped
        /// </summary>
        private void GuiStopReads()
        {
            btnStartReads.Text = "Start Reads";
            if (properties["audiblealert"].ToLower() == "yes")
            {
                if (readTriggeredByTap)
                {
                    playStopSound();
                }
            }
        }

        private void setStatus(string message)
        {
            label1.BeginInvoke((Action)delegate
            {
                label1.Text = message;
            });
        }
        private void setStatus(Color color)
        {
            label1.BeginInvoke((Action)delegate
            {
                btnLed.BackColor = color;
                label1.BackColor = color;
                debugLabel.BackColor = color;
            });
        }
        private void setStatus(string message, Color color)
        {
            setStatus(message);
            setStatus(color);
        }
        /// <summary>
        /// Canned status values -- makes it easier to maintain consistency for often-used messages
        /// </summary>
        private enum Status
        {
            IDLE,
            READY,
        }
        private void setStatus(Status value)
        {
            switch (value)
            {
                case Status.IDLE:
                    setStatus("Idle", Color.Transparent);
                    break;
                case Status.READY:
                    setStatus("Ready", Color.LightGreen);
                    SetDebug("");
                    break;
            }
        }

        /// <summary>
        /// Validates EPC String and formats valid strings into EPC
        /// </summary>
        /// <param name="epcString">Valid/Invalid EPC String</param>
        /// <returns>Valid TagData used for selection</returns>
        private TagData validateEpcStringAndReturnTagData(string epcString)
        {
            TagData validTagData = null;
            if (epcString.Contains(" "))
            {
                validTagData = null;
            }
            else
            {
                if (epcString.Length == 0)
                {
                    validTagData = null;
                }
                else
                {
                    if (epcString.Contains("0x"))
                    {
                        validTagData = new TagData(epcString.Remove(0, 2));
                    }
                    validTagData = new TagData(epcString);
                }
            }
            return validTagData;
        }
        
        /// <summary> 
        /// Function that processes the Tag Data produced by StartReading();
        /// </summary>
        /// <param name="read"></param>
        void PrintTagRead(Object sender, TagReadDataEventArgs e)
        {
            BeginInvoke((Action)delegate
            {
                if (properties["audiblealert"].ToLower() == "yes")
                {
                    if (readTriggeredByTap)
                    {
                        playBeepSound();
                    }
                }
                TagReadData read = e.TagReadData;
                UpdateReadTagIDBox(new TagReadData[] { read });
                if (overTempFlag)
                {
                    this.BeginInvoke(new OverTemperatureChange(ShowOverTemperature));
                }
            });
        }

        private delegate void OverTemperatureChange();

        private void ShowOverTemperature()
        {
            if (overTempFlag)
            {
                label1.Width = 40;
                setStatus("Reading", System.Drawing.Color.DarkGoldenrod);
                btnLed.Width = 51;
            }
            else
            {
                label1.Width = 100;
                setStatus("Overheat", System.Drawing.Color.Red);
                btnLed.Width = 100;
                overTempFlag = true;
            }
        }

        static bool overTempFlag = false;

        private void ReadException(Object sender, ReaderExceptionEventArgs e)
        {
            if (-1 != e.ReaderException.Message.IndexOf("DEBUG"))
            {
                logger.Info("ReadException listener: " + e.ReaderException.Message);
                return;
            }
            logger.Error("ReadException listener: " + e.ReaderException.ToString());
            if (-1 != e.ReaderException.Message.IndexOf("temperature"))
            {
                this.BeginInvoke(new OverTemperatureChange(ShowOverTemperature));
            }
            else
            {
                SetDebug(e.ReaderException.Message);
                logger.Debug("Calling StopReads in ReadException listener...");
                StopReads();
                logger.Debug("StopReads completed in ReadException listener");
            }
        }

        /// <summary>
        /// Method to invoke OutputUpdateDelegate
        /// </summary>
        /// <param name="data">String Data</param>       
        public void UpdateReadTagIDBox(TagReadData[] tags)
        {
            if (dgTagResult.InvokeRequired)
            {
                dgTagResult.BeginInvoke(new OutputUpdateDelegate(OutputUpdateReadTagID), new object[] { tags });
            }
            else
            {
                OutputUpdateReadTagID(tags);
            }
        }
        /// <summary>
        /// Method for updating Read Tag ID text box
        /// </summary>
        /// <param name="data">String Data</param>
        private void OutputUpdateReadTagID(TagReadData[] tags)
        {
            if (tags != null && tags.Length != 0)
            {
                foreach (TagReadData read in tags)
                {
                    tagdb.Add(read, GpsMgr.LatLonString, readerName);
                }
            }
            UpdateBigNumBox(tagdb.TagList.Count.ToString());
        }
        public void UpdateBigNumBox(string data)
        {
            if (txtTotalUniqueTags.InvokeRequired)
            {
                txtTotalUniqueTags.BeginInvoke(new OutputUpdateDelegateBigNum(OutputUpdateBigNumBox), new object[] { data });
            }
            else
            {
                OutputUpdateBigNumBox(data);
            }
        }
        private void OutputUpdateBigNumBox(string data)
        {
            if (data.Length != 0)
            {
                txtTotalUniqueTags.Text = data;
            }
            else
            {
                txtTotalUniqueTags.Text = "0";
            }
        }

        private void miExit_Click(object sender, EventArgs e)
        {
            try
            {
                ReadMgr.GetReader().StopReading();
                ReadMgr.AllowDisconnect();
                ReleaseEvents();
                pvtNotifyIcon.Remove();
                //Enable audio recording toolbar
                Registry.SetValue(@"HKEY_LOCAL_MACHINE\System\AudioRecording", "Enabled", 1, RegistryValueKind.DWord);
                Application.Exit();
            }
            catch (Exception ex)
            {
                logger.Error(ex.ToString());
                logger.Info("Calling Application.Exit...");
                Application.Exit();
            }
        }

        private void btnClearReads_Click(object sender, EventArgs e)
        {
            lock (tagdb)
            {
                tagdb.Clear();
            }
            UpdateBigNumBox("0");
        }

        private void btnSaveTags_Click(object sender, EventArgs e)
        {
            string strDestinationFile = string.Empty;
            try
            {
                saveTagsFileDialog.Filter = "TXT Files (.txt)|*.txt";
                strDestinationFile = "TagResults" + DateTime.Now.ToString("yyyy-MM-dd-HH-mm-ss") + ".txt";
                saveTagsFileDialog.FileName = strDestinationFile;
                saveTagsFileDialog.ShowDialog();
                strDestinationFile = saveTagsFileDialog.FileName;
                string header = "TagId,TimeStamp,ReaderName,Position,ReadCount,RSSI,EPC";
                strDestinationFile = saveTagsFileDialog.FileName;
                TextWriter tw = new StreamWriter(strDestinationFile);
                tw.WriteLine(header);
                foreach (TagReadRecord tr in tagdb.TagList)
                {
                    tw.WriteLine(tr.ToString());
                }
                tw.Close();
            }
            catch (Exception ex)
            {
                logger.Error(ex.ToString());
                MessageBox.Show(ex.Message);
            }
        }

        private void miGoToMain_Click(object sender, EventArgs e)
        {
            try
            {
                ReleaseEvents();
            }
            catch (Exception ex)
            {
                logger.Error("Ignoring exception: " + ex.ToString());
                //do nothing
            }
            finally
            {
                this.Close();
            }
        }

        private void tbTXPower_ValueChanged(object sender, EventArgs e)
        {
            try
            {
                int powerLevel = 1000 + ((TrackBar)sender).Value * 50;

                properties["readpower"] = (powerLevel).ToString();
                Utilities.SaveConfigurations(properties);
            }
            catch (Exception ex)
            {
                logger.Error("Ignoring exception: " + ex.ToString());
                // do nothing
            }
        }

        private void SerialListener(Object sender, TransportListenerEventArgs e)
        {
            transportLogFile.Write(String.Format("{0} {1}",
                DateTime.Now.ToString("MM/dd/yyyy hh:mm:ss.fff tt"), e.Tx ? "Sending" : "Received"));
            for (int i = 0; i < e.Data.Length; i++)
            {
                if ((i & 15) == 0)
                {
                    transportLogFile.WriteLine();
                    transportLogFile.Write("  ");
                }
                transportLogFile.Write("  " + e.Data[i].ToString("X2"));
            }
            transportLogFile.WriteLine();
            transportLogFile.Flush();
        }

        private void ReadTags_Closed(object sender, EventArgs e)
        {
            try
            {
                if (properties["isreading"] == "no")
                {
                    ReleaseEvents();
                }
            }
            catch (Exception ex)
            {
                logger.Error("Ignoring exception: " + ex.ToString());
                logger.Info("Calling Close...");
                this.Close();
            }
        }

        private void ReadTags_Closing(object sender, CancelEventArgs e)
        {
            if (properties["isreading"] == "no")
            {
                e.Cancel = false;
                ReadMgrUnsubscribe();
            }
            else
            {
                e.Cancel = true;
            }
        }

        private void ReleaseEvents()
        {
            if (null != pistolTriggerEvent)
            {
                pistolTriggerEvent.Changed -= new ChangeEventHandler(pistolTriggerEvent_Changed);
            }
        }

        private void SetDebug(string message)
        {
            if (null != debugLabel)
            {
                debugLabel.BeginInvoke((Action)delegate
                {
                    debugLabel.Text = message;
                });
            }
        }
    }
}
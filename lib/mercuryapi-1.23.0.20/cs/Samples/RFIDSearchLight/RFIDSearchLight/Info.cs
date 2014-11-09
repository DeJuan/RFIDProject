using System;
using System.Linq;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Microsoft.WindowsCE.Forms;
using System.IO;
using System.Media;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading;
using System.Diagnostics;
using Microsoft.WindowsMobile.Samples.Location;
using Microsoft.Win32;
using Microsoft.WindowsMobile;
using Microsoft.WindowsMobile.Status;

namespace ThingMagic.RFIDSearchLight
{
    public partial class Info : Form
    {
        private static log4net.ILog logger = log4net.LogManager.GetLogger("Info");

        ReadMgr.Session rsess;
        //IntPtr hcomport = IntPtr.Zero;
        private Dictionary<string, string> properties = new Dictionary<string,string>();
        protected string ReaderInfoText;
        const int KEYEVENTF_KEYPRESS = 0x0000;
        const int KEYEVENTF_KEYUP = 0x0002;
        
        [DllImport("coredll.dll")]
        static extern void keybd_event(byte bVk, byte bScan, int dwFlags, int dwExtraInfo);

        [DllImport("coredll.dll")]
        private static extern IntPtr FindWindow(string lpClassName, string lpWindowName);

        [DllImport("coredll.dll")]
        static extern int ShowWindow(IntPtr hWnd, int nCmdShow);
        const int SW_MINIMIZED = 6;

        private NotifyIcon notifyIcon;
        InputPanel inputPanel = new InputPanel();

        private string savedRegion = string.Empty;

        private SoundPlayer hotkeySound = null;

        //[DllImport("coredll.dll", CharSet = CharSet.Auto)]
        //static extern IntPtr SendMessage(IntPtr hWnd, UInt32 Msg, IntPtr wParam, StringBuilder lParam);

        public Info()
        {
            InitializeComponent();
        }

        // Stuff that used to be in the constructor, but the catch clause fails
        // since MessageBox.Show is not allowed there (MissingMethodException)
        // call from Load event instead and use its catch clause.
        private void Initialize2()
        {
            notifyIcon = new NotifyIcon();
            notifyIcon.Click += new EventHandler(notifyIcon_Click);

            properties = Utilities.GetProperties();
            savedRegion = properties["region"];
            //Kill the old instance
            if (properties["processid"] != "")
            {
                logger.Info("Detected old instance.  Kill PID " + properties["processid"]);
                try
                {
                    int oldProcessId = Convert.ToInt32(properties["processid"]);
                    Process oldProcess = Process.GetProcessById(oldProcessId);
                    oldProcess.Kill();
                }
                catch //If the old instance not running,it throws ArgumentException
                {
                    //do nothing
                }
            }
            //Add the process id to config file
            Process currentPprocess = Process.GetCurrentProcess();
            properties["processid"] = currentPprocess.Id.ToString();
            Utilities.SaveConfigurations(properties);
        }

        class MyMessageWindow : MessageWindow
        {
            Info parentForm;
            public MyMessageWindow(Info parent)
            {
                parentForm = parent;
            }

            protected override void WndProc(ref Message m)
            {
                if (m.Msg == HotKeys.WM_HOTKEY_MSG_ID)
                {
                    int id = m.WParam.ToInt32();
                    parentForm.BeginInvoke((Action)delegate
                    {
                        parentForm.HandleHotKey(id);
                    });
                }
                base.WndProc(ref m);
            }
        }

        private MyMessageWindow msgwin;
        HotKeys.Manager hotkeyMan;
        int hotkeyDisableId = 0;

        protected void HandleHotKey(int id)
        {
            if (null != hotkeySound) { hotkeySound.Play(); }
            //MessageBox.Show("Hotkey " + id + " pressed");
            if (id == hotkeyDisableId)
            {
                hotkeyMan.Disable();
                MessageBox.Show("Disabled hotkeys");
            }

            KeyboardWedgeRead();
        }

        /// <summary>
        /// Form load
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void Info_Load(object sender, EventArgs e)
        {
            logger.Info("Info form loaded");
            try
            {
                Initialize2();
                logger.Debug("Initialize2 completed");
                InitializeHotkeys();
                logger.Debug("InitializeHotkeys completed");
                GpsMgr.Init();
                logger.Debug("GpsMgr.Init completed");

                try
                {
                    rsess = ReadMgr.GetSession();
                    logger.Debug("ReadMgr.GetSession completed");
                    ReadMgr.ReaderEvent += new ReadMgr.ReadMgrEventHandler(ReadMgr_ReaderEvent);
                    logger.Debug("ReadMgr.ReaderEvent subscribed to");
                }
                catch (Exception ex)
                {
                    logger.Error("In Info_Load ReadMgr: " + ex.ToString());
                    MessageBox.Show(ex.Message.ToString());
                    Application.Exit();
                    return;
                }

                System.Windows.Forms.Timer infoTimer = new System.Windows.Forms.Timer();
                infoTimer.Interval = 1000;
                infoTimer.Tick += new EventHandler(infoTimer_Tick);
                infoTimer.Enabled = true;
#if DEBUG
#else
                infoDebugLabel.Visible = false;
                infoGpsLabel.Visible = false;
                //infoGpsLabel.Text = "";
                infoPowerLabel.Visible = false;
#endif

                properties = Utilities.GetProperties();

                //Set the region
                SetRegion();
                
                Utilities.SaveConfigurations(properties);                    

                //Load the reader information
                LoadReaderInfo();
                InitializeReaderDefaults();
                Utilities.SaveConfigurations(properties);

                //Disable the launching of audio recording tool bar
                Registry.SetValue(@"HKEY_LOCAL_MACHINE\System\AudioRecording", "Enabled", 0, RegistryValueKind.DWord);

                // Force on-screen keyboard away to ensure that our menus are visible
                inputPanel.Enabled = false;
            }
            catch (ReaderCommException rcom)
            {
                logger.Error("In Info_Load: " + rcom.ToString());
                MessageBox.Show(rcom.Message.ToString());
            }
            catch (Exception ex)
            {
                logger.Error("In Info_Load: " + ex.ToString());
                MessageBox.Show(ex.Message.ToString());
            }
            finally
            {
                if (null != rsess)
                {
                    logger.Debug("Calling rsess.Dispose...");
                    rsess.Dispose();
                    logger.Debug("rsess.Dispose completed");
                }
            }
        }

        private void InitializeHotkeys()
        {
            //hotkeySound = new SoundPlayer("/Windows/TimerSound.wav");
            //hotkeySound.LoadAsync();

            msgwin = new MyMessageWindow(this);
            hotkeyMan = new HotKeys.Manager();
            List<HotKeys.Spec> hotkeys = new List<HotKeys.Spec>();
            hotkeys.Add(new HotKeys.Spec(HotKeys.Mod.WIN, HotKeys.Nomad.BUTTON_PISTOL_TRIGGER, msgwin));
            hotkeys.Add(new HotKeys.Spec(HotKeys.Mod.WIN, HotKeys.Nomad.BUTTON_LEFT_SOFTKEY, msgwin));
            hotkeyMan.SetKeys(hotkeys);
        }

        void ReadMgr_ReaderEvent(ReadMgr.EventCode code)
        {
            Invoke((Action)delegate
            {
                infoPowerLabel.Text = code.ToString();
            });
        }

        void infoTimer_Tick(object sender, EventArgs e)
        {
            string gpsString = (0 == GpsMgr.LatLonString.Length)
                ? "GPS not locked"
                : "GPS Position: " + GpsMgr.LatLonString;
            infoGpsLabel.Text = gpsString;
            infoDebugLabel.Text = Utilities.ReaderPortName;
        }

        const int USB_READER = 1;
        const int NA_ANTENNA = 2;
        const int EU_ANTENNA = 3;
        //Set the region by timezone on the nomad
        private void SetRegion()
        {
            try
            {
                //Get the regions based on the Windows settings 
                Hashtable regions = Utilities.GenerateRegionList();

                if (null != regions)
                {
                    try
                    {
                        string r = TimeZone.CurrentTimeZone.GetUtcOffset(DateTime.Now).ToString();
                        properties["region"] = regions[r].ToString();
                    }
                    catch
                    {
                        RegionSelection objRegion = new RegionSelection(rsess.Reader, (Reader.Region[])rsess.Reader.ParamGet("/reader/region/supportedRegions"));
                        objRegion.ShowDialog();
                        properties = Utilities.GetProperties();
                    }
                }
                //If the saved region is empty or saved region is conflicted with timezone selection
                if ((savedRegion == "") || (savedRegion != properties["region"]))
                {
                    //Set the region based on the productGroupID 
                    UInt16 productGroupID = (UInt16)rsess.Reader.ParamGet("/reader/version/productGroupID");
                    if (productGroupID == 2)//NA
                    {

                        UInt16 productid = (UInt16)rsess.Reader.ParamGet("/reader/version/productID");
                        switch (productid)
                        {
                            case USB_READER: break;
                            case NA_ANTENNA:
                                //Antenna NA (product id 2)
                                List<Reader.Region> regionsSuuportedbyAntennaNA = new List<Reader.Region>();
                                regionsSuuportedbyAntennaNA.Add(Reader.Region.NA);
                                regionsSuuportedbyAntennaNA.Add(Reader.Region.AU);
                                regionsSuuportedbyAntennaNA.Add(Reader.Region.PRC);
                                
                                if (!regionsSuuportedbyAntennaNA.Contains((Reader.Region)Enum.Parse(typeof(Reader.Region), properties["region"].ToString(), true)))
                                {
                                    RegionSelection objRegion = new RegionSelection(rsess.Reader, regionsSuuportedbyAntennaNA.ToArray());
                                    objRegion.ShowDialog();
                                    properties = Utilities.GetProperties();
                                }
                                //If user not selected the region in region selection screen
                                if (properties["region"] == "")
                                {                                 
                                    properties["region"] = ((savedRegion == "")?"na":savedRegion);
                                }
                                break;
                            case EU_ANTENNA:
                                //Antenna EU(product id 3)
                                List<Reader.Region> regionsSuuportedbyAntennaEU = new List<Reader.Region>();
                                regionsSuuportedbyAntennaEU.Add(Reader.Region.EU3);
                                regionsSuuportedbyAntennaEU.Add(Reader.Region.IN);
                                regionsSuuportedbyAntennaEU.Add(Reader.Region.NZ);

                                if (!regionsSuuportedbyAntennaEU.Contains((Reader.Region)Enum.Parse(typeof(Reader.Region), properties["region"].ToString(), true)))
                                {
                                    RegionSelection objRegion = new RegionSelection(rsess.Reader, regionsSuuportedbyAntennaEU.ToArray());
                                    objRegion.ShowDialog();
                                    properties = Utilities.GetProperties();
                                }
                                //If user not selected the region in region selection screen
                                if (properties["region"] == "")
                                {
                                    properties["region"] = ((savedRegion == "") ? "eu3" : savedRegion);
                                }
                                break;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                logger.Error("In SetRegion(): " + ex.ToString());
                MessageBox.Show(ex.Message);
            }
        }

        /// <summary>
        /// Add a line of reader info.
        /// Adds to ReaderInfoText (new scheme) and specified control's Text (old scheme)
        /// </summary>
        /// <param name="label">Name of field; e.g., (FW Date, FW Ver).
        /// If provided, will be prepended to value with a colon; e.g., "label: value".
        /// If not provided, value will be displayed alone; e.g., "value"</param>
        /// <param name="value">Value of field.
        /// To append blank line, set value=null.</param>
        /// <param name="control">Optional control to append value to</param>
        private void ReaderInfoAdd(string label, string value, Control control)
        {
            if (null != value)
            {
                if (null != label)
                {
                    ReaderInfoText += label + ": ";
                }
                ReaderInfoText += value;
                if (null != control)
                {
                    control.Text += value;
                }
            }
            ReaderInfoText += "\r\n";
        }
        private void LoadReaderInfo()
        {
            Assembly asm = System.Reflection.Assembly.GetExecutingAssembly();
            AssemblyName asmName = asm.GetName();
            ReaderInfoText = "";
            ReaderInfoAdd(null, asmName.Name + " " + asmName.Version.ToString(), null);
            Assembly mercuryApi = Assembly.LoadFrom("MercuryAPICE.dll");
            ReaderInfoAdd("Mercury API", mercuryApi.GetName().Version.ToString(), null);

            ReaderInfoAdd(null, null, null);
            ReaderInfoAdd(null, "ThingMagic " + (string)rsess.Reader.ParamGet("/reader/version/productGroup")
                + " on " + Utilities.ReaderPortName, null);
            ReaderInfoAdd("Serial Number", (string)rsess.Reader.ParamGet("/reader/version/serial"), null);
            ReaderInfoAdd("HWID", (string)rsess.Reader.ParamGet("/reader/version/hardware"), null);
            string[] software = rsess.Reader.ParamGet("/reader/version/software").ToString().Split('-');
            ReaderInfoAdd("BL Ver", software[2], null);
            ReaderInfoAdd("FW Ver", software[0], null);
            ReaderInfoAdd("FW Date", software[1], null);
            rsess.Reader.ParamSet("/reader/powerMode", Reader.PowerMode.MINSAVE);
        }

        private void InitializeReaderDefaults()
        {
            if (properties["readpower"] == "")
            {
                properties["readpower"] = rsess.Reader.ParamGet("/reader/radio/readPower").ToString();
            }

            Utilities.SwitchRegion(properties["region"]);

            if (properties["gen2q"] == "")
            {
                if (-1 != rsess.Reader.ParamGet("/reader/gen2/q").ToString().IndexOf("Dynamic"))
                {
                    properties["gen2q"] = "dynamicq";
                }
                else
                {
                    properties["gen2q"] = "staticq";
                    properties["staticqvalue"] = rsess.Reader.ParamGet("/reader/gen2/q").ToString().Split('(')[1].Substring(0, 1);
                }
            }
            if (properties["gen2session"] == "")
            {
                properties["gen2session"] = rsess.Reader.ParamGet("/reader/gen2/session").ToString();
            }
            if (properties["gen2target"] == "")
            {
                properties["gen2target"] = rsess.Reader.ParamGet("/reader/gen2/target").ToString();
            }
            if (properties["tagencoding"] == "")
            {
                properties["tagencoding"] = rsess.Reader.ParamGet("/reader/gen2/tagEncoding").ToString();
            }
        }

        private void ToggleKeyboardWedge()
        {
            properties = Utilities.GetProperties();
            if (btnKeyboardWedge.Text == "Enable RFID keyboard wedge")
            {
                switch (MessageBox.Show("Do you wish to continue?", "Increases Power Consumption", MessageBoxButtons.YesNo, MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button1))
                {
                    case DialogResult.Yes:
                        properties["iswedgeappdisabled"] = "no";
                        Utilities.SaveConfigurations(properties);
                        Minimize();
                        AddIcon();
                        btnKeyboardWedge.Text = "Disable RFID keyboard wedge";
                        hotkeyMan.Enable();
                        break;
                    case DialogResult.No:
                        return;
                }
            }
            else
            {
                DisableKeyboardWedge();
            }
        }

        private void DisableKeyboardWedge()
        {
            MessageBox.Show("Keyboard Wedge will now be disabled", "Warning");
            hotkeyMan.Disable();
            properties = Utilities.GetProperties();
            properties["iswedgeappdisabled"] = "yes";
            btnKeyboardWedge.Text = "Enable RFID keyboard wedge";
            this.MinimizeBox = false;
            Utilities.SaveConfigurations(properties);
        }

        private void ConfigureApp()
        {
            Configuration objConfiguration = new Configuration(notifyIcon);
            objConfiguration.Show();
        }

        private void ReadTags()
        {
            if (btnKeyboardWedge.Text == "Disable RFID keyboard wedge")
            {
                DisableKeyboardWedge();
            }
            ReadTags objReadTags = new ReadTags(notifyIcon);
            objReadTags.Show();
        }

        private static byte CharToVk(char ch)
        {
            byte vk = 0;
            if (Char.IsLetter(ch))
            {
                if (Char.IsLower(ch))
                {
                    ch = (char)(ch + ('A' - 'a'));
                }
                vk = (byte)(ch - 'A' + Keys.A);
            }
            else if (Char.IsDigit(ch))
            {
                vk = (byte)(ch - '0' + Keys.D0);
            }
            else
            {
                switch (ch)
                {
                    case ';':
                    case ':':
                        vk = 0xBA; break;
                    case '+':
                        vk = 0xBB; break;
                    case ',':
                        vk = 0xBC; break;
                    case '-':
                        vk = 0xBD; break;
                    case '.':
                        vk = 0xBE; break;
                    case '/':
                    case '?':
                        vk = 0xBF; break;
                    case ' ':
                        vk = 0x20; break;
                }
            }
            return vk;
        }

        private static SendInputWrapper.INPUT MakeKeybdInput(byte vk)
        {
            return MakeKeybdInput(vk, 0);
        }

        private static SendInputWrapper.INPUT MakeKeybdInput(byte vk, SendInputWrapper.KEYEVENTF dwFlags)
        {
            return new SendInputWrapper.INPUT
            {
                type = SendInputWrapper.INPUT_KEYBOARD,
                u = new SendInputWrapper.InputUnion
                {
                    ki = new SendInputWrapper.KEYBDINPUT
                    {
                        wVk = vk,
                        wScan = 0,
                        dwFlags = dwFlags,
                    }
                }
            };
        }

        private static SendInputWrapper.INPUT SHIFT_DOWN = MakeKeybdInput((byte)Keys.ShiftKey, 0);
        private static SendInputWrapper.INPUT SHIFT_UP = MakeKeybdInput((byte)Keys.ShiftKey, SendInputWrapper.KEYEVENTF.KEYUP);

        private static List<SendInputWrapper.INPUT> AddInput(
            List<SendInputWrapper.INPUT> inputs,
            SendInputWrapper.INPUT newInput)
        {
            if (null == inputs)
            {
                inputs = new List<SendInputWrapper.INPUT>();
            }
            inputs.Add(newInput);
            return inputs;
        }
        private static List<SendInputWrapper.INPUT> AddInputs(
            List<SendInputWrapper.INPUT> inputs,
            ICollection<SendInputWrapper.INPUT> newInputs)
        {
            if (null == inputs)
            {
                inputs = new List<SendInputWrapper.INPUT>();
            }
            inputs.AddRange(newInputs);
            return inputs;
        }

        private static List<SendInputWrapper.INPUT> AddKeybdInput(
            List<SendInputWrapper.INPUT> inputs,
            byte vk)
        {
            return AddKeybdInput(inputs, vk, 0);
        }
        private static List<SendInputWrapper.INPUT> AddKeybdInput(
            List<SendInputWrapper.INPUT> inputs,
            byte vk,
            SendInputWrapper.KEYEVENTF dwFlags)
        {
            if (null == inputs)
            {
                inputs = new List<SendInputWrapper.INPUT>();
            }
            inputs.Add(MakeKeybdInput(vk, dwFlags));
            return inputs;
        }

        private static List<SendInputWrapper.INPUT> AddKeypress(
            List<SendInputWrapper.INPUT> inputs,
            byte vk)
        {
            inputs = AddKeybdInput(inputs, vk, 0);
            inputs = AddKeybdInput(inputs, vk, SendInputWrapper.KEYEVENTF.KEYUP);
            return inputs;
        }
        private static List<SendInputWrapper.INPUT> AddKeypresses(
            List<SendInputWrapper.INPUT> inputs,
            ICollection<byte> vkList)
        {
            foreach (byte vk in vkList)
            {
                inputs = AddKeypress(inputs, vk);
            }
            return inputs;
        }
        private static List<SendInputWrapper.INPUT> AddKeypresses(
            List<SendInputWrapper.INPUT> inputs,
            string str)
        {
            List<byte> vkList = new List<byte>();
            foreach (char ch in str)
            {
                byte vk = CharToVk(ch);
                if (0 != vk)
                {
                    vkList.Add(vk);
                }
            }
            return AddKeypresses(inputs, vkList);
        }

        private static void SendInput(ICollection<SendInputWrapper.INPUT> inputList)
        {
            SendInputWrapper.SendInput(
                (uint)inputList.Count,
                inputList.ToArray(),
                Marshal.SizeOf(typeof(SendInputWrapper.INPUT)));
        }

        // TODO: Merge common functionality between KeyboardWedgeRead and ReadTags.ReadtheTags
        private void KeyboardWedgeRead()
        {
            List<SendInputWrapper.INPUT> inputList = new List<SendInputWrapper.INPUT>();
            string startIndicator = ".";

            // Signal start of read
            inputList.Clear();
            AddKeypresses(inputList, startIndicator);
            SendInput(inputList);

            properties = Utilities.GetProperties();

            SoundPlayer startSound = new SoundPlayer(properties["startscanwavefile"]);
            if (properties["audiblealert"].ToLower() == "yes")
            {
                startSound.Play();
            }
            SoundPlayer stopSound = new SoundPlayer(properties["endscanwavefile"]);
            stopSound.LoadAsync();

            CoreDLL.SYSTEM_POWER_STATUS_EX status = new CoreDLL.SYSTEM_POWER_STATUS_EX();
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
            try
            {
                TagReadData[] reads;

                //Utilities.PowerManager.PowerNotify += new PowerManager.PowerEventHandler(PowerManager_PowerNotify);
                using (ThingMagic.RFIDSearchLight.ReadMgr.Session rsess = ThingMagic.RFIDSearchLight.ReadMgr.GetSession())
                {
#if DEBUG
                    if (properties["audiblealert"].ToLower() == "yes")
                    {
                        startSound.Play();
                    }
#endif

                    int radioPower = 0;
                    if (properties["readpower"].ToString() == "")
                    {
                        radioPower = 2300;//While reading read power should be max
                    }
                    else
                    {
                        radioPower = Convert.ToInt32(properties["readpower"].ToString());
                    }
  
                    //Set the region
                    string region = properties["region"];
                    try
                    {
                        Utilities.SwitchRegion(region);
                    }
                    catch (ArgumentException)
                    {
                        MessageBox.Show(
                            "Unknown Region: " + region + "\r\n" +
                            "Please run RFIDSearchLight to initialize the region."
                            );
                    }
                    
                    rsess.Reader.ParamSet("/reader/powerMode", Reader.PowerMode.FULL);
                    rsess.Reader.ParamSet("/reader/radio/readPower", radioPower);
                    rsess.Reader.ParamSet("/reader/antenna/txRxMap", new int[][] { new int[] { 1, 1, 1 } });
                    List<int> ant = new List<int>();
                    ant.Add(1);
                    //set the tag population settings
                    rsess.Reader.ParamSet("/reader/gen2/target", Gen2.Target.A);//default target
                    string tagPopulation = properties["tagpopulation"];
                    switch (tagPopulation)
                    {
                        case "small":
                            rsess.Reader.ParamSet("/reader/gen2/q", new Gen2.StaticQ(2));
                            rsess.Reader.ParamSet("/reader/gen2/session", Gen2.Session.S0);
                            rsess.Reader.ParamSet("/reader/gen2/tagEncoding", Gen2.TagEncoding.M4);
                            break;
                        case "medium":
                            rsess.Reader.ParamSet("/reader/gen2/q", new Gen2.StaticQ(4));
                            rsess.Reader.ParamSet("/reader/gen2/session", Gen2.Session.S1);
                            rsess.Reader.ParamSet("/reader/gen2/tagEncoding", Gen2.TagEncoding.M4);
                            break;
                        case "large":
                            rsess.Reader.ParamSet("/reader/gen2/q", new Gen2.StaticQ(6));
                            rsess.Reader.ParamSet("/reader/gen2/session", Gen2.Session.S2);
                            rsess.Reader.ParamSet("/reader/gen2/tagEncoding", Gen2.TagEncoding.M2);
                            break;
                        default: break;
                    }
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
                    //set the read plan
                    SimpleReadPlan srp;
                    if (properties["tagselection"].ToLower() == "none")
                    {
                        srp = new SimpleReadPlan(new int[] { 1 }, TagProtocol.GEN2, null, 0);
                    }
                    else
                    {
                        srp = new SimpleReadPlan(new int[] { 1 }, TagProtocol.GEN2, filter, 0);
                    }
                    rsess.Reader.ParamSet("/reader/read/plan", srp);

                    double readDuration = Convert.ToDouble(properties["scanduration"].ToString()) * 1000;
                    int readTimeout = Convert.ToInt32(readDuration);

                    //Do a sync read for the readduration
#if DEBUG
                    if (properties["audiblealert"].ToLower() == "yes")
                    {
                        startSound.Play();
                    }
#endif
                    reads = rsess.Reader.Read(readTimeout);

                    rsess.Reader.ParamSet("/reader/powerMode", Reader.PowerMode.MINSAVE);
                    if (properties["audiblealert"].ToLower() == "yes")
                    {
                        stopSound.Play();
                    }
                    // Clear start indicator
                    inputList.Clear();
                    for (int i = 0; i < startIndicator.Length; i++)
                    {
                        AddKeypresses(inputList, new byte[] {
                            // Don't send Backspace -- that's one of our hotkeys,
                            // so it'll put us in an infinite loop.
                            (byte)Keys.Left,
                            (byte)Keys.Delete,
                        });
                    }
                    SendInput(inputList);
                }

                inputList.Clear();
                //HideWindow();
                bool timestamp = false, rssi = false, position = false;
                string[] metadata = properties["metadatatodisplay"].Split(',');
                //Metadata boolean variables
                foreach (string mdata in metadata)
                {
                    switch (mdata.ToLower())
                    {
                        case "timestamp": timestamp = true; break;
                        case "rssi": rssi = true; break;
                        case "position": position = true; break;
                        default: break;
                    }
                }
                string metadataseparator = properties["metadataseparator"];
                byte metadataseparatorInByte = 0x00;
                switch (metadataseparator.ToLower())
                {
                    //The byte representation of special characters can be found here - http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731(v=vs.85).aspx
                    case "comma": metadataseparatorInByte = 0xBC; break;
                    case "space": metadataseparatorInByte = 0x20; break;
                    case "enter": metadataseparatorInByte = 0x0D; break;
                    case "tab": metadataseparatorInByte = 0x09; break;
                    default: break;
                }
                //Print the epc in caps
                AddKeypress(inputList, (byte)Keys.Capital);  // Toggle Caps Lock

                //Print the tag reads
                foreach (TagReadData dat in reads)
                {
                    string tagData = string.Empty;
                    string epc = string.Empty;
                    if (properties["displayformat"] == "base36")
                    {
                        epc = ConvertEPC.ConvertHexToBase36(dat.EpcString);
                    }
                    else
                    {
                        epc = dat.EpcString;
                    }

                    AddKeypresses(inputList, properties["prefix"].ToUpper());
                    AddKeypresses(inputList, epc.ToUpper());
                    AddKeypresses(inputList, properties["suffix"].ToUpper());

                    if (timestamp)
                    {
                        AddKeypress(inputList, metadataseparatorInByte);
                        AddKeypresses(inputList, dat.Time.ToString("yyyy-MM-dd-HH-mm-ss"));
                    }
                    if (rssi)
                    {
                        AddKeypress(inputList, metadataseparatorInByte);
                        AddKeypresses(inputList, dat.Rssi.ToString());
                    }
                    if (position)
                    {
                        AddKeypress(inputList, metadataseparatorInByte);
                        AddKeypresses(inputList, GpsMgr.LatLonString);
                    }

                    switch (properties["multipletagseparator"].ToLower())
                    {
                        case "comma": AddKeypress(inputList, 0xBC); break;
                        case "space": AddKeypress(inputList, 0x20); break;
                        case "enter": AddKeypress(inputList, 0x0D); break;
                        case "tab": AddKeypress(inputList, 0x09); break;
                        case "pipe":
                            AddInput(inputList, SHIFT_DOWN);
                            AddKeypress(inputList, 0xDC);
                            AddInput(inputList, SHIFT_UP);
                            break;
                        default: break;
                    }
                    // Send keystrokes after each tag read record -- input buffer
                    // isn't big enough to hold more than a few lines
                    SendInput(inputList);
                    inputList.Clear();
                }
                //Turn caps lock back off
                AddKeypress(inputList, (byte)Keys.Capital);  // Toggle Caps Lock
                SendInput(inputList);
            }
            
            catch (Exception ex)
            {
                logger.Error("In KeyboardWedgeRead(): " + ex.ToString());
                //MessageBox.Show(ex.Message);
                //Debug.Log(ex.ToString());
            }
        }

        private void exit_Click(object sender, EventArgs e)
        {
            properties["iswedgeappdisabled"] = "yes";
            Utilities.SaveConfigurations(properties);
            ReadMgr.ForceDisconnect();
            RemoveIcon();
            //if (hcomport != IntPtr.Zero)
            //{
            //    CoreDLL.ReleasePowerRequirement(hcomport);
            //    hcomport = IntPtr.Zero;
            //}

            //Enable audio recording toolbar
            Registry.SetValue(@"HKEY_LOCAL_MACHINE\System\AudioRecording", "Enabled", 1, RegistryValueKind.DWord);
            Application.Exit();
        }
        public void HideWindow()
        {         
            ShowWindow(this.Handle, SW_MINIMIZED);
        }

        private void Info_KeyDown(object sender, KeyEventArgs e)
        {
            if ((e.KeyCode == System.Windows.Forms.Keys.Up))
            {
                // Up
            }
            if ((e.KeyCode == System.Windows.Forms.Keys.Down))
            {
                // Down
            }
            if ((e.KeyCode == System.Windows.Forms.Keys.Left))
            {
                // Left
            }
            if ((e.KeyCode == System.Windows.Forms.Keys.Right))
            {
                // Right
            }
            if ((e.KeyCode == System.Windows.Forms.Keys.Enter))
            {
                // Enter
            }

        }

        private void rbKeyboardWedge_CheckedChanged(object sender, EventArgs e)
        {
         
        }

        void Minimize()
        {
            // The Taskbar must be enabled to be able to do a Smart Minimize
            this.FormBorderStyle = FormBorderStyle.FixedDialog;
            this.WindowState = FormWindowState.Normal;
            this.ControlBox = true;
            this.MinimizeBox = true;
            this.MaximizeBox = true;

            // Since there is no WindowState.Minimize, we have to P/Invoke ShowWindow
            ShowWindow(this.Handle, SW_MINIMIZED);
        }

        private void AddIcon()
        {
            IntPtr hIcon = LoadIcon(GetModuleHandle(null), "#32512");
            notifyIcon.Add(hIcon);
            this.MaximizeBox = true;
        }

        private void RemoveIcon()
        {
            notifyIcon.Remove();
        }

        [DllImport("coredll.dll")]
        internal static extern IntPtr LoadIcon(IntPtr hInst, string IconName);

        [DllImport("coredll.dll")]
        internal static extern IntPtr GetModuleHandle(String lpModuleName);

        private void notifyIcon_Click(object sender, EventArgs e)
        {
            //MessageBox.Show("Icon Clicked!");
            this.Show();
        }

        //private void tmrPower_Tick(object sender, EventArgs e)
        //{
        //    try
        //    {
        //        properties = Utilities.GetProperties();
        //        if (properties["isreading"].ToLower() == "no")
        //        {
        //            if (properties["powermode"].ToLower() != "maxsave")
        //            {
        //                if (null == objReader)
        //                {
        //                    objReader = Utilities.ConnectReader();
        //                }
        //                else
        //                {
        //                    objReader.Connect();
        //                }
        //                objReader.ParamSet("/reader/powerMode", Reader.PowerMode.MAXSAVE);
        //            }
        //        }
        //    }
        //    catch
        //    {
        //        //do nothing
        //    }
        //    finally
        //    {
        //        if (null != objReader)
        //        {
        //            objReader.Destroy();
        //            objReader = null;
        //        }
        //    }
        //}

        private void Info_Closed(object sender, EventArgs e)
        {
            ReadMgr.ForceDisconnect();
            RemoveIcon();
            ////Release power to USB port
            //if (hcomport != IntPtr.Zero)
            //{
            //    CoreDLL.ReleasePowerRequirement(hcomport);
            //    hcomport = IntPtr.Zero;
            //}
            //Enable audio recording toolbar
            Registry.SetValue(@"HKEY_LOCAL_MACHINE\System\AudioRecording", "Enabled", 1, RegistryValueKind.DWord);
            Application.Exit();
        }

        private void about_Click(object sender, EventArgs e)
        {
            Form about = new About(ReaderInfoText);
            about.ShowDialog();
        }

        private void btnKeyboardWedge_Click(object sender, EventArgs e)
        {
            ToggleKeyboardWedge();
        }

        private void btnConfigure_Click(object sender, EventArgs e)
        {
            ConfigureApp();
        }

        private void btnReadTags_Click(object sender, EventArgs e)
        {
            ReadTags();
        }
    }
}

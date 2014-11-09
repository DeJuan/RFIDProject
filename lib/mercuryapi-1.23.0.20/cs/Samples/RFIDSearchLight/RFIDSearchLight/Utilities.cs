using System;
using System.Linq;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Text;
using System.Threading;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Text.RegularExpressions;
using Microsoft.WindowsMobile.Samples.Location;
using ThingMagic;

namespace ThingMagic.RFIDSearchLight
{
    public class DiskLog
    {
#if DEBUG
        StreamWriter log;
#endif
        public DiskLog(string filename)
        {
#if DEBUG
            // Add unique string to filename to allow multiple processes that use the same classes
            // TODO: Come up with a more controlled way to disambiguate processes
            filename = Environment.TickCount.ToString() + "-" + filename;
            log = new StreamWriter(filename, true);
#endif
        }
        public void Log(string message)
        {
#if DEBUG
            try
            {
                string text = Environment.TickCount.ToString() + " " + message + "\r\n";
                log.Write(text);
                log.Flush();
            }
            catch (Exception)
            {
                // Prevent stray exceptions if logger is called from weird places
                // (e.g., during application exit, after a crash.)  Makes it hard
                // to debug the debugger, but if you get to that point, you have 
                // bigger problems.
            }
#endif
        }
    }

    /// <summary>
    /// GPS connection manager
    /// 
    /// Manages a single connection to the Microsoft Managed GPS service
    /// </summary>
    public class GpsMgr
    {
        static Gps _gps = new Gps();
        static bool initialized = false;
        static string _latlonstr = "";

        public static void Init()
        {
            if (!initialized)
            {
                // Update GPS position once a second.
                // Don't use Gps.LocationChanged event -- that has a tendency to fire very rapidly, especially when GPS is not locked.
                System.Windows.Forms.Timer tmr = new System.Windows.Forms.Timer();
                tmr.Interval = 1000;
                tmr.Tick += new EventHandler(gpsTmrTick);
                tmr.Enabled = true;
            }
            if (!_gps.Opened)
            {
                _gps.Open();
            }
        }

        public static string LatLonString
        {
            get
            {
                lock (_latlonstr)
                {
                    return _latlonstr;
                }
            }
        }

        static void gpsTmrTick(object sender, EventArgs e)
        {
            UpdateLocation();
        }

        private static void UpdateLocation()
        {
            if (_gps.Opened)
            {
                GpsPosition pos = _gps.GetPosition();
                string posstr = "";
                if ((pos.LatitudeValid) && (pos.LongitudeValid))
                {
                    posstr = pos.Latitude.ToString("F6") +
                        " " + pos.Longitude.ToString("F6");
                }
                lock (_latlonstr)
                {
                    _latlonstr = posstr;
                }
            }
        }
        
        /// <summary>
        /// Open Managed GPS service
        /// </summary>
        public void Open()
        {
            if (!_gps.Opened)
            {
                _gps.Open();
            }
        }
        /// <summary>
        /// Close Managed GPS service
        /// 
        /// Don't need to explicitly call this.
        /// Exiting a program implicitly closes its Managed GPS connection.
        /// Use this only if you want to allow the GPS turn itself off.
        /// </summary>
        public void Close()
        {
            if (_gps.Opened)
            {
                _gps.Close();
            }
        }
    }

    /// <summary>
    /// Reader connection manager: Automatically manages Reader Connect and Destroy
    ///
    /// Maintains a single static Reader object to be shared by all users of ReadMgr.
    /// Automatically connects when reader object is requested (GetReader, Session.Reader)
    /// Automatically handles system power transitions by recovering the reader connection
    /// </summary>
    public class ReadMgr
    {
        static log4net.ILog logger = log4net.LogManager.GetLogger("ReadMgr");
        static Reader _rdr;
        private static readonly object readerLock = new object();
        private static CleanupMgr cleanupManager = new CleanupMgr();
        static bool subscriptionsDone = false;
        static IntPtr nullIntPtr = new IntPtr(0);
        static List<IntPtr> rdrPowerReqHandles = new List<IntPtr>();

        public static LogProvider DebugLog = new LogProvider();
        public static LogProvider StatusLog = new LogProvider();
        public static LogProvider PowerLog = new LogProvider();
        public delegate void ReadMgrEventHandler(EventCode code);
        public static event ReadMgrEventHandler ReaderEvent;
        public static string lastErrorMessage = "";

        class CleanupMgr
        {
            public CleanupMgr() { }

            /// <summary>
            /// Cleanup actions to run when application exits
            /// </summary>
            ~CleanupMgr()
            {
                // Be sure to clear out unattended mode requests.
                // Those are reference-counted across the system,
                // so uncanceled ones will prevent the system from
                // ever sleeping again (until rebooted.)
                DisableUnattendedReaderMode();
            }
        }

        public enum EventCode
        {
            /// <summary>
            /// Notify app that power was interrupted.
            /// App should display wait cursor, because we're not sure
            /// how long it will take to process the next power event.
            /// </summary>
            POWER_INTERRUPTED,
            /// Notify app that power was suspended.
            /// App should shut down FTDI peripherals to prevent driver corruption.
            /// </summary>
            POWER_SUSPEND,
            /// <summary>
            /// Power came back
            /// App can stop displaying wait cursor now.
            /// </summary>
            POWER_RESTORED,
            /// <summary>
            /// Reader connection has been shut down.
            /// </summary>
            READER_SHUTDOWN,
            /// <summary>
            /// Reader connection needs to be reset.
            /// App should display wait cursor.
            /// </summary>
            READER_RECOVERING,
            /// <summary>
            /// ReadMgr is done with reader recovery routine.
            /// App can stop displaying wait cursor now
            /// (unless it has its own long wait to indicate.)
            /// </summary>
            READER_RECOVERED,
            /// <summary>
            /// ReadMgr is done with reader recovery routine, but it failed.
            /// App can stop displaying wait cursor now, but should display an error message.
            /// </summary>
            READER_RECOVERY_FAILED,
        }
        static void OnReaderEvent(EventCode code)
        {
            if (null != ReaderEvent) { ReaderEvent(code); }
        }

        public static Reader GetReader()
        {
            CheckConnection();
            return _rdr;
        }

        public static Session GetSession()
        {
            return GetSession(false);
        }
        public static Session GetSession(bool forceDisconnect)
        {
            CheckConnection();
            return new Session(forceDisconnect);
        }

        /// <summary>
        /// Check status of connection and restore, if down
        /// </summary>
        public static void CheckConnection()
        {
            if (!subscriptionsDone)
            {
                Utilities.PowerManager.PowerNotify += new PowerManager.PowerEventHandler(PowerManager_PowerNotify);
                DebugLog.Log += new LogProvider.LogHandler(DebugLog_Log_ToDisk);
                subscriptionsDone = true;
            }
            lock (readerLock)
            {
                if (null == _rdr)
                {
                    _rdr = Utilities.ConnectReader();
                    EnableUnattendedReaderMode(Utilities.ReaderPortName);

                    //_rdr.ParamSet("/reader/powerMode", Reader.PowerMode.MEDSAVE);
                    _rdr.ParamSet("/reader/powerMode", Reader.PowerMode.FULL);
                }
            }
        }

        /// <summary>
        /// Set up unattended power management mode so reader 
        /// can be shut down cleanly before handheld suspends.
        /// </summary>
        /// <param name="portName"></param>
        private static void EnableUnattendedReaderMode(string portName)
        {
            DebugLog_Log_ToDisk("EnableUnattendedReaderMode");
            lock (rdrPowerReqHandles)
            {
                if (0 == rdrPowerReqHandles.Count)
                {
                    // See Unattended Mode example at http://stackoverflow.com/questions/336771/how-can-i-run-code-on-windows-mobile-while-being-suspended
                    CoreDLL.PowerPolicyNotify(PPNMessage.PPN_UNATTENDEDMODE, 1);
                    DebugLog_Log_ToDisk("Set PPN_UNATTENDEDMODE to 1");
                    // Request that devices remain powered in unattended mode
                    // NOTE: If you don't do this, the system suspends anyway 1 minute after you press the power button
                    // (USB reader power light goes out, system publishes a Suspend notification.)
                    // With this request, it seems to run indefinitely.
                    foreach (string devName in new string[]{
                        portName+":",  // RFID Reader
                        "wav1:",  // Speaker
                        "com3:",  // GPS Intermediate Driver
                    })
                    {
                        IntPtr handle = CoreDLL.SetPowerRequirement(devName, CEDeviceDriverPowerStates.D0,
                            DevicePowerFlags.POWER_NAME | DevicePowerFlags.POWER_FORCE,
                            nullIntPtr, 0);
                        rdrPowerReqHandles.Add(handle);
                        DebugLog_Log_ToDisk("Set PowerRequirement " + handle.ToString() + " for " + devName);
                    }
                }
            }
        }
        /// <summary>
        /// Withdraw unattended power management requirements
        /// to allow handheld to fully suspend.
        /// </summary>
        private static void DisableUnattendedReaderMode()
        {
            lock (rdrPowerReqHandles)
            {
                if (0 < rdrPowerReqHandles.Count)
                {
                    foreach (IntPtr handle in rdrPowerReqHandles)
                    {
                        CoreDLL.ReleasePowerRequirement(handle);
                        DebugLog_Log_ToDisk("Released PowerRequirement " + handle.ToString());
                    }
                    rdrPowerReqHandles.Clear();
                    CoreDLL.PowerPolicyNotify(PPNMessage.PPN_UNATTENDEDMODE, 0);
                    DebugLog_Log_ToDisk("Set PPN_UNATTENDEDMODE to 0");
                }
            }
        }

        static void DebugLog_Log_ToDisk(string message)
        {
            Utilities.DiskLog.Log("DebugLog: " + message);
        }

        static void ReadMgrDiskLog(string message)
        {
            Utilities.DiskLog.Log("ReadMgr: " + message);
        }

        static bool resumeTripped = false;  // Is there an unhandled Resume event?
        static int transitionsSinceResume = 0;  // FTDI driver isn't up until 2 Transition 12010000s after a Resume
        static bool screenOffTripped = false;  // Is there an unhandled Screen Off event?
        static int transitionsSinceScreenOff = 0;
        static void PowerManager_PowerNotify(object sender, PowerManager.PowerEventArgs e)
        {
            try
            {
                // Get the information
                PowerManager.PowerInfo powerInfo = e.PowerInfo;
                string message = powerInfo.Message + " " + powerInfo.Flags.ToString("X");
                PowerLog.OnLog(message);
                ReadMgrDiskLog(message);

                // Power Management Strategy
                //  * Whenever display is shut down (Transition 00400000), shut down reader, too (ForceDisconnect)
                //    * NOTE: Doesn't apply to user turning off backlight (hold power button for 1s) -- that doesn't generate a Transisition 04000000
                //  * After resume, wait until reader is ready, then reconnect
                //    * If Nomad didn't make it to Suspend (Transition 00200000), wait until first Transition 12010000
                //    * If Nomad went through Suspend (Transition 00200000), wait for second Transition 12010000
                switch (powerInfo.Message)
                {
                    case PowerManager.MessageTypes.Resume:
                        resumeTripped = true;
                        transitionsSinceResume = 0;
                        break;
                    case PowerManager.MessageTypes.Transition:
                        switch ((uint)powerInfo.Flags)
                        {
                            case 0x00400000:
                            // Backlight Off
                                screenOffTripped = true;
                                transitionsSinceScreenOff = 0;
                                OnReaderEvent(EventCode.POWER_INTERRUPTED);
                                ReadMgrDiskLog("POWER_INTERRUPTED");
                                ForceDisconnect();
                                //Cursor.Current = Cursors.WaitCursor;
                                //StatusLog.OnLog("Power Interrupted");
                                break;
                            case 0x00200000:
                            // Suspend
                                OnReaderEvent(EventCode.POWER_SUSPEND);
                                ReadMgrDiskLog("POWER_SUSPEND");
                                break;
                            case 0x12010000:
                            // Subsystem On
                                transitionsSinceResume += 1;
                                transitionsSinceScreenOff += 1;
                                if (!resumeTripped && (1 == transitionsSinceScreenOff))
                                {
                                    OnReaderEvent(EventCode.POWER_RESTORED);
                                    ReadMgrDiskLog("POWER_RESTORED");
                                    ExecuteReconnect();
                                }
                                if (resumeTripped && (2 == transitionsSinceResume))
                                {
                                    // Reconnect on 2nd powerup transition after Resume
                                    //Cursor.Current = Cursors.WaitCursor;
                                    //StatusLog.OnLog("Reconnecting...");
                                    ExecuteReconnect();
                                }
                                break;
                        }
                        break;
                }
            }
            catch (Exception ex)
            {
                logger.Error("In PowerManager_PowerNotify: " + ex.ToString());
                ReadMgrDiskLog("In PowerManager_PowerNotify: " + ex.ToString());
                //Application.Exit();
            }
        }

        private static void ExecuteReconnect()
        {
            try
            {
                OnReaderEvent(EventCode.READER_RECOVERING);
                ReadMgrDiskLog("READER_RECOVERING");
                DebugLog.OnLog("Recovering Reader...");
                //DebugLog.OnLog("Waiting...");
                //Thread.Sleep(3000);
                //MessageBox.Show("Waiting for serial driver recovery.  Press ok to continue.");
                //DebugLog.OnLog("Calling RecoverReader()...");
                ForceReconnect();
                DebugLog.OnLog("Reader Recovered");
                OnReaderEvent(EventCode.READER_RECOVERED);
                ReadMgrDiskLog("READER_RECOVERED");
                //Cursor.Current = Cursors.Default;
                resumeTripped = false;
            }
            catch (Exception ex)
            {
                logger.Error("In ExecuteReconnect(): " + ex.ToString());
                if (-1 != ex.Message.IndexOf("RFID reader was not found"))
                {
                    lastErrorMessage = "Connection failed";
                }
                else
                {
                    lastErrorMessage = ex.Message;
                }
                //StatusLog.OnLog(lastErrorMessage);
                OnReaderEvent(EventCode.READER_RECOVERY_FAILED);
                ReadMgrDiskLog("READER_RECOVERY_FAILED");
            }
        }

        /// <summary>
        /// Allow, but do not require, disconnection from the reader.
        /// 
        /// For example, if our app is in a maximum power-saving mode, it may choose 
        /// to disconnect as often as possible in order to power down the reader module.
        /// But it is also free to leave the connection open to minimize latency.
        /// </summary>
        public static void AllowDisconnect()
        {
        }
        /// <summary>
        /// Require disconnection from the reader.
        /// 
        /// For example, if we are exiting the application and want the reader to go into maximum shutdown mode.
        /// </summary>
        public static void ForceDisconnect()
        {
            lock (readerLock)
            {
                if (null != _rdr)
                {
                    try
                    {
                        ReadMgrDiskLog("Trying Reader.Destroy");
                        _rdr.Destroy();
                        ReadMgrDiskLog("Destroyed Reader");
                    }
                    catch (IOException ex)
                    {
                        logger.Error("Ignoring IOException in ForceDisconnect(): " + ex.ToString());
                        // IOExceptions are tolerable on Destroy.
                        // They are normal if the serial chipset
                        // has been shut down without the driver being closed
                        // (e.g., host goes to sleep while a connection is open)
                        ReadMgrDiskLog("Caught IOException in ForceDisconnect: " + ex.ToString());
                    }
                    catch (Exception ex)
                    {
                        logger.Error("Caught Exception in ForceDisconnect: " + ex.ToString());
                        ReadMgrDiskLog("Caught Exception in ForceDisconnect: " + ex.ToString());
                        throw;
                    }
                    finally
                    {
                        _rdr = null;
                        ReadMgrDiskLog("Waiting for SerialPort threads to spin down...");
                        Thread.Sleep(2000);
                        DisableUnattendedReaderMode();
                        OnReaderEvent(EventCode.READER_SHUTDOWN);
                    }
                }
            }
        }
        public static void ForceReconnect()
        {
            DebugLog.OnLog("Disconnect Reader");
            ForceDisconnect();
            DebugLog.OnLog("Wait for Serial Reset");
            // Wait for serial driver to recover
            //Thread.Sleep(1000);
            DebugLog.OnLog("Reconnect Reader");
            CheckConnection();
            DebugLog.OnLog("Reader Reconnected");
        }

        /// <summary>
        /// Automatically manage reader connect/disconnect via using-dispose syntax
        /// </summary>
        public class Session : IDisposable
        {
            bool _disposed;
            bool _forceDisconnect;

            /// <summary>
            /// 
            /// </summary>
            /// <param name="forceDisconnect">Force disconnection when session is over?</param>
            public Session(bool forceDisconnect)
            {
                _disposed = false;
                _forceDisconnect = forceDisconnect;
            }

            public void Dispose()
            {
                Dispose(true);
                GC.SuppressFinalize(this);
            }
            public void Dispose(bool disposing)
            {
                // If you need thread safety, use a lock around these 
                // operations, as well as in your methods that use the resource.
                if (!_disposed)
                {
                    if (disposing)
                    {
                        if (_forceDisconnect)
                        {
                            ForceDisconnect();
                        }
                        else
                        {
                            AllowDisconnect();
                        }
                    }

                    _disposed = true;
                }
            }

            public Reader Reader
            { 
                get 
                {
                    CheckConnection();
                    return _rdr;
                } 
            }
        }
    }

    /// <summary>
    /// DataGridView adapter for a TagReadData
    /// </summary>
    public class TagReadRecord : INotifyPropertyChanged
    {
        protected TagReadData RawRead;
        protected string gps;
        protected string readerName;
        protected string item;
        public TagReadRecord(TagReadData data)
        {
            RawRead = data;
        }
        /// <summary>
        /// Merge new tag read with existing one
        /// </summary>
        /// <param name="data">New tag read</param>
        public void Update(TagReadData newData)
        {
            newData.ReadCount += ReadCount;
            RawRead = newData;
            OnPropertyChanged(null);
        }
        public void Update(TagReadData newData, string gpsInfo)
        {
            newData.ReadCount += ReadCount;
            RawRead = newData;
            gps = gpsInfo;
            OnPropertyChanged(null);
        }
        public void Update(TagReadData newData, string gpsInfo, string ReaderName)
        {            
            newData.ReadCount += ReadCount;
            RawRead = newData;
            gps = gpsInfo;
            readerName = ReaderName;
            OnPropertyChanged(null);
        }
        public void Update(string currentItem)
        {
            item = currentItem;
            OnPropertyChanged(null);
        }
        public string TagId
        {
            get { return ConvertEPC.ConvertHexToBase36(RawRead.EpcString).ToUpper(); }
        }
        public DateTime TimeStamp
        {
            get { return RawRead.Time; }
        }
        public string ReaderName
        {
            get { return readerName; }
        }
        public string Position
        {
            get { return gps; }
        }
        public int ReadCount
        {
            get { return RawRead.ReadCount; }
        }
        //public int Antenna
        //{
        //    get { return RawRead.Antenna; }
        //}
        //public TagProtocol Protocol
        //{
        //    get { return RawRead.Tag.Protocol; }
        //}
        public int RSSI
        {
            get { return RawRead.Rssi; }
        }
        //public string Item
        //{
        //    get { return item; }
        //}

        public string EPC
        {
            get { return RawRead.EpcString.ToUpper(); }
        }

        #region INotifyPropertyChanged Members

        public event PropertyChangedEventHandler PropertyChanged;

        private void OnPropertyChanged(string name)
        {
            if (null != PropertyChanged)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(name));
            }
        }

        #endregion
        /// <summary>
        /// Returns a String that represents the current Object.
        /// </summary>
        /// <returns>A String that represents the current Object.</returns>
        public override string ToString()
        {
            return String.Format("{0},{1},{2},{3},{4},{5},{6}",  new object[]{TagId, TimeStamp, ReaderName, Position, ReadCount, RSSI, EPC});
        }
    }
    public class SorregionTableBindingList<T> : BindingList<T>
    {
        protected override bool SupportsSortingCore
        {
            get
            {
                return true;
            }
        }

        protected override bool IsSortedCore
        {
            get
            {
                return _isSorted;
            }
        }
        private bool _isSorted = false;

        protected override PropertyDescriptor SortPropertyCore
        {
            get
            {
                return _sortProperty;
            }
        }
        private PropertyDescriptor _sortProperty;

        protected override ListSortDirection SortDirectionCore
        {
            get
            {
                return _sortDirection;
            }
        }
        private ListSortDirection _sortDirection;

        protected override void ApplySortCore(PropertyDescriptor prop, ListSortDirection direction)
        {
            //if (null != prop.PropertyType.GetInterface("IComparable"))

            {
                List<T> itemsList = (List<T>)this.Items;
                Comparison<T> comparer = GetComparer(prop);
                itemsList.Sort(comparer);
                if (direction == ListSortDirection.Descending)
                {
                    itemsList.Reverse();
                }
                _isSorted = true;
                _sortProperty = prop;
                _sortDirection = direction;
                this.OnListChanged(new ListChangedEventArgs(ListChangedType.Reset, -1));
            }
        }

        protected virtual Comparison<T> GetComparer(PropertyDescriptor prop)
        {
            throw new NotImplementedException();
        }

        protected override void OnListChanged(ListChangedEventArgs e)
        {
            if (null != SortPropertyCore)
            {
                if (!_insideListChangedHandler)
                {
                    _insideListChangedHandler = true;
                    ApplySortCore(SortPropertyCore, SortDirectionCore);
                    _insideListChangedHandler = false;
                }
            }
            base.OnListChanged(e);
        }
        private bool _insideListChangedHandler = false;
    }
    public class TagReadRecordBindingList : SorregionTableBindingList<TagReadRecord>
    {
        protected override Comparison<TagReadRecord> GetComparer(PropertyDescriptor prop)
        {
            Comparison<TagReadRecord> comparer = null;
            switch (prop.Name)
            {
                case "TagId":
                    comparer = new Comparison<TagReadRecord>(delegate(TagReadRecord a, TagReadRecord b)
                    {
                        return String.Compare(a.TagId, b.TagId);
                    });
                    break;
                case "TimeStamp":
                    comparer = new Comparison<TagReadRecord>(delegate(TagReadRecord a, TagReadRecord b)
                    {
                        return DateTime.Compare(a.TimeStamp, b.TimeStamp);
                    });
                    break;
                //case "ReaderName":
                //    comparer = new Comparison<TagReadRecord>(delegate(TagReadRecord a, TagReadRecord b)
                //    {
                //        return String.Compare(a.ReaderName, b.ReaderName);
                //    });
                //    break;
                case "Position":
                    {
                        comparer = new Comparison<TagReadRecord>(delegate(TagReadRecord a, TagReadRecord b)
                        {
                            return String.Compare(a.Position, b.Position);
                        });
                        break;
                    }
                case "ReadCount":
                    comparer = new Comparison<TagReadRecord>(delegate(TagReadRecord a, TagReadRecord b)
                    {
                        return a.ReadCount - b.ReadCount;
                    });
                    break;
                //case "Antenna":
                //    comparer = new Comparison<TagReadRecord>(delegate(TagReadRecord a, TagReadRecord b)
                //    {
                //        return a.Antenna - b.Antenna;
                //    });
                //    break;
                //case "Protocol":
                //    comparer = new Comparison<TagReadRecord>(delegate(TagReadRecord a, TagReadRecord b)
                //    {
                //        return String.Compare(a.Protocol.ToString(), b.Protocol.ToString());
                //    });
                //    break;
                case "RSSI":
                    comparer = new Comparison<TagReadRecord>(delegate(TagReadRecord a, TagReadRecord b)
                    {
                        return a.RSSI - b.RSSI;
                    });
                    break;
                //case "Item":
                //    comparer = new Comparison<TagReadRecord>(delegate(TagReadRecord a, TagReadRecord b)
                //    {
                //        return String.Compare(a.Item, b.Item);
                //    });
                //    break;
                case "EPC":
                    comparer = new Comparison<TagReadRecord>(delegate(TagReadRecord a, TagReadRecord b)
                    {
                        return String.Compare(a.EPC, b.EPC);
                    });
                    break;
            }
            return comparer;
        }
    }
    public class TagDatabase
    {
        /// <summary>
        /// TagReadData model (backs data grid display)
        /// </summary>
        TagReadRecordBindingList _tagList = new TagReadRecordBindingList();

        /// <summary>
        /// EPC index into tag list
        /// </summary>
        Dictionary<string, TagReadRecord> EpcIndex = new Dictionary<string, TagReadRecord>();

        public TagReadRecordBindingList TagList
        {
            get { return _tagList; }
        }

        public void Clear()
        {
            EpcIndex.Clear();
            _tagList.Clear();
            // Clear doesn't fire notifications on its own
            _tagList.ResetBindings();
        }

        public void Add(TagReadData data)
        {
            string key = data.EpcString;
            if (!EpcIndex.ContainsKey(key))
            {
                TagReadRecord value = new TagReadRecord(data);
                _tagList.Add(value);
                EpcIndex.Add(key, value);
            }
            else
            {
                EpcIndex[key].Update(data);
            }
        }
        public void Add(TagReadData data, string gps)
        {
            string key = data.EpcString;
            if (!EpcIndex.ContainsKey(key))
            {
                TagReadRecord value = new TagReadRecord(data);
                _tagList.Add(value);
                EpcIndex.Add(key, value);
            }
            else
            {
                EpcIndex[key].Update(data, gps);
            }
        }

        public void AddRange(ICollection<TagReadData> reads)
        {
            foreach (TagReadData read in reads)
            {
                Add(read);
            }
        }
        public void Add(TagReadData data, string gps, string readerName)
        {
            string key = data.EpcString;
            if (!EpcIndex.ContainsKey(key))
            {
                TagReadRecord value = new TagReadRecord(data);
                _tagList.Add(value);
                //Adding TagReadData to list
                EpcIndex.Add(key, value);
                //Update the reader name and GPS values
                EpcIndex[key].Update(data, gps, readerName);
            }
            else
            {
                EpcIndex[key].Update(data, gps, readerName);
            }
        }
        public void AddRange(ICollection<TagReadData> reads, string gps)
        {
            foreach (TagReadData read in reads)
            {
                Add(read, gps);
            }
        }
    }

    public class Utilities
    {
        static log4net.ILog logger = log4net.LogManager.GetLogger("Utilities");
        static log4net.ILog transportLogger = log4net.LogManager.GetLogger("Transport");
        static string appPath = Path.GetDirectoryName(Assembly.GetExecutingAssembly().GetName().CodeBase) + @"/Config.txt";
        public static LogProvider Logger = new LogProvider();
        private static PowerManager _powerMgr = null;
        public static DiskLog DiskLog = new DiskLog("Utilities.log");

        /// <summary>
        /// Read the properties from TestConfiguration.properties file
        /// </summary>
        /// <param name="path">file path</param>
        /// <returns>Properties</returns>
        public static Dictionary<string, string> GetProperties()
        {
            Dictionary<string, string> Properties = new Dictionary<string, string>();
            try
            {
                FileStream fs = File.OpenRead(appPath);

                using (StreamReader sr = File.OpenText(appPath))
                {
                    string line;
                    while ((line = sr.ReadLine()) != null)
                    {
                        if ((!string.IsNullOrEmpty(line)) &&
                            (!line.StartsWith(";")) &&
                            (!line.StartsWith("#")) &&
                            (!line.StartsWith("'")) &&
                            (!line.StartsWith("/")) &&
                            (!line.StartsWith("*")) &&
                            (line.Contains('=')))
                        {
                            int index = line.IndexOf('=');
                            string key = line.Substring(0, index).Trim();
                            string value = line.Substring(index + 1).Trim();

                            if ((value.StartsWith("\"") && value.EndsWith("\"")) ||
                                (value.StartsWith("'") && value.EndsWith("'")))
                            {
                                value = value.Substring(1, value.Length - 2);
                            }
                            Properties.Add(key, value);
                        }
                    }
                }
                fs.Close();
            }
            catch (FileNotFoundException)
            {
                logger.Info(appPath + " not found.  Using default properties.");
                Properties = BuildDefaultProperties();
                SaveConfigurations(Properties);
            }
          
            return Properties;
        }

        private static Dictionary<string, string> BuildDefaultProperties()
        {
            Dictionary<string, string> Properties = new Dictionary<string, string>();
            Properties.Add("iswedgeappdisabled", "yes"); 
            Properties.Add("comport",""); 
            Properties.Add("region",""); 
            Properties.Add("displayformat","hex");                
            Properties.Add("scanduration","3");
            Properties.Add("audiblealert","yes");
            Properties.Add("metadataseparator","space");
            Properties.Add("multipletagseparator","space");
            Properties.Add("metadatatodisplay","timestamp");
            Properties.Add("decodewavefile",@"\Windows\Loudest.wav");
            Properties.Add("startscanwavefile",@"\Windows\Quietest.wav");
            Properties.Add("endscanwavefile",@"\Windows\Splat.wav");
            Properties.Add("prefix","");
            Properties.Add("suffix","");
            Properties.Add("readpower","");
            Properties.Add("tagpopulation","small");
            Properties.Add("tagselection","None");
            Properties.Add("selectionaddress","0");
            Properties.Add("selectionmask","");
            Properties.Add("ismaskselected","");
            Properties.Add("tagencoding","");
            Properties.Add("gen2session","");
            Properties.Add("gen2target","");
            Properties.Add("gen2q","");
            Properties.Add("staticqvalue","");
            Properties.Add("recordhighestrssi","yes");
            Properties.Add("recordrssioflasttagread","yes");
            Properties.Add("confighureapp","demo");
            Properties.Add("powermode","maxsave");
            Properties.Add("isreading","no");
            Properties.Add("rfontime","250");
            Properties.Add("rfofftime","50");
            return Properties;
        }

        public static void SaveConfigurations(Dictionary<string,string> Properties)
        { 
            try
            {
                using (StreamWriter sw = File.CreateText(appPath))
                {
                    foreach (KeyValuePair<string, string> item in Properties)
                    {
                        sw.WriteLine(item.Key + "=" + item.Value);
                    }
                    sw.Flush();
                    sw.Close();
                }
            }
            catch (Exception ex)
            {
                logger.Error("In SaveConfigurations: " + ex.ToString());
                throw ex;
            }
        }

        /// <summary>
        /// Set the properties on the reader
        /// </summary>
        /// <param name="objReader">Reader</param>
        /// <param name="Properties">configurations</param>
        public static void SetReaderSettings(Reader objReader, Dictionary<string, string> Properties)
        {
            using (ReadMgr.Session rsess = ReadMgr.GetSession())
            {
                //Set tagencoding value
                switch (Properties["tagencoding"].ToLower())
                {
                    case "fm0":
                        rsess.Reader.ParamSet("/reader/gen2/tagEncoding", Gen2.TagEncoding.FM0);
                        break;
                    case "m2":
                        rsess.Reader.ParamSet("/reader/gen2/tagEncoding", Gen2.TagEncoding.M2);
                        break;
                    case "m4":
                        rsess.Reader.ParamSet("/reader/gen2/tagEncoding", Gen2.TagEncoding.M4);
                        break;
                    case "m8":
                        rsess.Reader.ParamSet("/reader/gen2/tagEncoding", Gen2.TagEncoding.M8);
                        break;
                }
                //Set target value
                switch (Properties["gen2target"].ToLower())
                {
                    case "a":
                        rsess.Reader.ParamSet("/reader/gen2/target", Gen2.Target.A);
                        break;
                    case "b":
                        rsess.Reader.ParamSet("/reader/gen2/target", Gen2.Target.B);
                        break;
                    case "ab":
                        rsess.Reader.ParamSet("/reader/gen2/target", Gen2.Target.AB);
                        break;
                    case "ba":
                        rsess.Reader.ParamSet("/reader/gen2/target", Gen2.Target.BA);
                        break;
                }
                //Set gen2 session value
                switch (Properties["gen2session"].ToLower())
                {
                    case "s0":
                        rsess.Reader.ParamSet("/reader/gen2/session", Gen2.Session.S0);
                        break;
                    case "s1":
                        rsess.Reader.ParamSet("/reader/gen2/session", Gen2.Session.S1);
                        break;
                    case "s2":
                        rsess.Reader.ParamSet("/reader/gen2/session", Gen2.Session.S2);
                        break;
                    case "s3":
                        rsess.Reader.ParamSet("/reader/gen2/session", Gen2.Session.S3);
                        break;
                }
                //Set Q value
                switch (Properties["gen2q"].ToLower())
                {
                    case "dynamicq":
                        rsess.Reader.ParamSet("/reader/gen2/q", new Gen2.DynamicQ()); break;
                    case "staticq":
                        rsess.Reader.ParamSet("/reader/gen2/q", new Gen2.StaticQ(Convert.ToByte(Properties["staticqvalue"].ToString()))); break;
                    default:
                        break;
                }
                rsess.Reader.ParamSet("/reader/read/asyncOffTime", Convert.ToInt32(Properties["rfofftime"]));
                rsess.Reader.ParamSet("/reader/read/asyncOnTime", Convert.ToInt32(Properties["rfontime"]));
            }
        }
        public static PowerManager PowerManager
        {
            get
            {
                if (null == _powerMgr)
                {
                    // Create a new instance of the PowerManager class
                    _powerMgr = new PowerManager();

                    // Enable power notifications. This will cause a thread to start
                    // that will fire the PowerNotify event when any power notification 
                    // is received.
                    _powerMgr.EnableNotifications();
                }
                return _powerMgr;
            }
        }

        private static string _readerPortName = "";
        public static string ReaderPortName
        {
            get { return _readerPortName; }
        }

        public static Reader ConnectReader()
        {
            Reader objReader = null;
            string readerPort = null;
            List<string> ports = new List<string>();

            logger.Debug("Starting ConnectReader");

            // First, try last known-good port
            Dictionary<string, string> properties = Utilities.GetProperties();
            string lastPort = properties["comport"];
            logger.Debug("lastPort = " + lastPort);
            if (lastPort != "")
            {
                int retries = 3;
                while ((0 < retries) && (null == objReader))
                {
                    logger.Debug("Trying last-known port: " + lastPort
                        + " (" + retries.ToString() + " tries left)");
#if DEBUG
                    DiskLog.Log("Trying last-known port: " + lastPort
                        + " (" + retries.ToString() + " tries left)");
#endif
                    retries -= 1;
                    readerPort = lastPort;
                    objReader = ConnectReader(lastPort);
                    if (null != objReader) { break; }
                    Thread.Sleep(1000);
                }
            }
            if (null == objReader)
            { 
                // Next, try all known ports
                DiskLog.Log("Enumerating serial ports...");
                string[] portnames = System.IO.Ports.SerialPort.GetPortNames();
                Array.Reverse(portnames);
                ports.AddRange(portnames);
                {
                    StringBuilder sb = new StringBuilder();
                    sb.Append("Detected Ports:");
                    foreach (string port in ports)
                    {
                        sb.Append(" " + port.ToString());
                    }
                    logger.Debug(sb.ToString());
                }
    #if DEBUG
                {
                    StringBuilder sb = new StringBuilder();
                    sb.Append("Detected Ports:");
                    foreach (string port in ports)
                    {
                        sb.Append(" " + port.ToString());
                    }
                    DiskLog.Log(sb.ToString());
                }
    #endif
                foreach (string port in ports)
                {
                    switch (port.ToUpper())
                    {
                        case "COM1":  // physical DB-9 port
                        case "COM2":  // GPS port
                        case "COM3":  // GPS intermediate driver
                            logger.Debug("Skipping port " + port);
                            continue;
                    }
                    logger.Debug("Trying port " + port);
#if DEBUG
                    DiskLog.Log("Trying port " + port);
#endif
                    //Connect to the usb reader
                    readerPort = port;
                    objReader = ConnectReader(port);
                    if (null != objReader)
                    {
                        StringBuilder portBlacklistSB = new StringBuilder();
                        foreach (string portName in ports)
                        {
                            if (portName != readerPort)
                            {
                                portBlacklistSB.Append(" " + portName + " ");
                            }
                        }
                        string portBlacklist = portBlacklistSB.ToString();

#if DEBUG
                        string message = 
                            "Found a reader on port " + readerPort + "\r\n"
                            + "portBlacklist: " + portBlacklist;
                        DiskLog.Log(message);
#endif
                        break;
                    }
                }
            }

            // Did we find a reader?
            if (objReader == null)
            {
                string message = "RFID reader was not found. Please check the USB connection or re-install the FTDI driver";
                DiskLog.Log(message);
                logger.Error(message);
                throw new Exception(message);
            }
            else
            {
#if DEBUG
                DiskLog.Log("Saving reader port " + readerPort);
#endif
                properties["comport"] = readerPort;
                Utilities.SaveConfigurations(properties);
#if DEBUG
                DiskLog.Log("Connected: " + readerPort);
#endif
                logger.Info("Connected to reader on port " + readerPort);
                return objReader;
            }
        }

        public static Reader ConnectReader(string port)
        {
            Reader objReader = null;
            try
            {
                logger.Info("Starting ConnectReader(" + port + ")...");
                DiskLog.Log("Starting ConnectReader(" + port + ")...");
                //objReader = Reader.Create(@"eapi:///" + port);
                objReader = new SerialReader(@"/" + port);
                DiskLog.Log("Created new SerialReader(" + @"/" + port + ")");
                objReader.ParamSet("/reader/baudRate", 9600);
                DiskLog.Log("Set baud rate to 9600");
                objReader.Connect();
                //objReader.ParamSet("/reader/transportTimeout", 2000);
                _readerPortName = port;
                logger.Info("Connected to SerialReader on " + port);
                DiskLog.Log("Connected to SerialReader on " + port);
                //objReader.Transport += new EventHandler<TransportListenerEventArgs>(objReader_Transport);
            }
            catch (Exception ex)
            {
                logger.Error("In ConnectReader(" + port + "): " + ex.ToString());
                DiskLog.Log("Caught exception in ConnectReader(" + port + "): " + ex.ToString());
                objReader = null;
            }
            return objReader;
        }

        static void objReader_Transport(object sender, TransportListenerEventArgs e)
        {
            transportLogger.Debug(String.Format(
                "{0}: {1} (timeout={2:D}ms)",
                e.Tx ? "TX" : "RX",
                ByteFormat.ToHex(e.Data, "", " "),
                e.Timeout
                ));
        }

        //Generate region id list according to the time zone 
        public static Hashtable GenerateRegionList()
        {
            //region = table[local.GetUtcOffset(DateTime.Now).ToString()].ToString();
            Hashtable regionTable = new Hashtable();
            
            //Hawaii-Aleutian Standard Time
            regionTable.Add("-10:00:00", "NA");
            //Hawaii-Aleutian Daylight Time and Alaska Standard Time
            regionTable.Add("-09:00:00", "NA");
            //Pacific Standard Time and Alaska Daylight Time
            regionTable.Add("-08:00:00", "NA");
            //Pacific Daylight Time and Mountain Standard Time
            regionTable.Add("-07:00:00", "NA");
            //Mountain Daylight Time and Central Standard Time
            regionTable.Add("-06:00:00", "NA");
            //Central Daylight Time and Eastern Standard Time
            regionTable.Add("-05:00:00", "NA");
            //Eastern Daylight Time and Atlantic Standard Time
            regionTable.Add("-04:00:00", "NA");
            //Newfoundland Standard Time
            regionTable.Add("-03:30:00", "NA");
            //West Greenland Time, Atlantic Daylight Time and Pierre & Miquelon Standard Time
            regionTable.Add("-03:00:00", "NA");
            //Newfoundland Daylight Time
            regionTable.Add("-02:30:00", "NA");
            //Western Greenland Summer Time and Pierre & Miquelon Daylight Time
            regionTable.Add("-02:00:00", "NA");
            //East Greenland Time
            regionTable.Add("-01:00:00", "NA");
            //Greenwich Mean Time
            regionTable.Add("00:00:00", "EU3");

            //British Summer Time,Central European Time, Irish Standard Time and Western European Summer Time
            regionTable.Add("01:00:00", "EU3");
            //Central European Summer Time and 	Eastern European Time
            regionTable.Add("02:00:00", "EU3");
            //Eastern European Summer Time and Moscow Standard Time
            regionTable.Add("03:00:00", "EU3");
            //Kuybyshev Time,Moscow Daylight Time and Samara Time
            regionTable.Add("04:00:00", "EU3");

            //India Standard Time
            regionTable.Add("05:30:00", "IN");

            //Japan And Korea Standard Time
            regionTable.Add("09:00:00", "KR-JP");

            //China Standard Time
            regionTable.Add("08:00:00", "PRC");

            //Central Standard Time
            regionTable.Add("09:30:00", "AU");
            //Eastern Standard Time
            regionTable.Add("10:00:00", "AU");
            //Lord Howe Standard Time and Central Daylight Time
            regionTable.Add("10:30:00", "AU");
            //Eastern Daylight Time and Lord Howe Daylight Time
            regionTable.Add("11:00:00", "AU");
            //Norfolk Time
            regionTable.Add("11:30:00", "AU");

            //New Zealand Standard Time
            regionTable.Add("12:00:00", "NZ");
            regionTable.Add("13:00:00", "NZ");
            return regionTable;
        }

        /// <summary>
        /// Switch reader's region setting
        /// </summary>
        /// <param name="regionName">Name of region, as stored in properties["region"]</param>
        public static void SwitchRegion(string regionName)
        {
            switch (regionName.ToLower())
            {
                case "na": ReadMgr.GetReader().ParamSet("/reader/region/id", Reader.Region.NA); break;
                case "in": ReadMgr.GetReader().ParamSet("/reader/region/id", Reader.Region.IN); break;
                case "au": ReadMgr.GetReader().ParamSet("/reader/region/id", Reader.Region.AU); break;
                case "eu": ReadMgr.GetReader().ParamSet("/reader/region/id", Reader.Region.EU); break;
                case "eu2": ReadMgr.GetReader().ParamSet("/reader/region/id", Reader.Region.EU2); break;
                case "eu3": ReadMgr.GetReader().ParamSet("/reader/region/id", Reader.Region.EU3); break;
                case "jp": ReadMgr.GetReader().ParamSet("/reader/region/id", Reader.Region.JP); break;
                case "kr": ReadMgr.GetReader().ParamSet("/reader/region/id", Reader.Region.KR); break;
                case "kr2": ReadMgr.GetReader().ParamSet("/reader/region/id", Reader.Region.KR2); break;
                case "prc": ReadMgr.GetReader().ParamSet("/reader/region/id", Reader.Region.PRC); break;
                case "prc2": ReadMgr.GetReader().ParamSet("/reader/region/id", Reader.Region.PRC2); break;
                case "nz":
                    // ReadMgr.GetReader().ParamSet("/reader/region/id", Reader.Region.NZ);
                    // NZ Region not supported on M5ec module, so hack it here.
                    ReadMgr.GetReader().ParamSet("/reader/region/id", Reader.Region.OPEN);
                    ReadMgr.GetReader().ParamSet("/reader/region/hopTable", new int[] {
                    864400,
                    865150, 
                    864900,
                    865900,
                    865400,
                    864650, 
                    865650,
                });
                    break;
                case "open": ReadMgr.GetReader().ParamSet("/reader/region/id", Reader.Region.OPEN); break;
                case "unspec": ReadMgr.GetReader().ParamSet("/reader/region/id", Reader.Region.UNSPEC); break;
                default:
                    throw new ArgumentException("Unknown Region: " + regionName);
            }
        }

        // Function to test for Positive Integers with zero inclusive 
        public static bool IsWholeNumber(String strNumber)
        {
            Regex objNotWholePattern = new Regex("[^0-9]");
            return !objNotWholePattern.IsMatch(strNumber);
        }
    }

    public class SendInputWrapper
    {
        // via http://stackoverflow.com/questions/8962850/sendinput-fails-on-64bit
        // Usage:
        //   INPUT[] inputs = new INPUT[]
        //   {
        //       new INPUT
        //       {
        //           type = INPUT_KEYBOARD,
        //           u = new InputUnion
        //           {
        //               ki = new KEYBDINPUT
        //               {
        //                   wVk = key,
        //                   wScan = 0,
        //                   dwFlags = 0,
        //                   dwExtraInfo = GetMessageExtraInfo(),
        //               }
        //           }
        //       }
        //   };
        //
        //   SendInput((uint)inputs.Length, inputs, Marshal.SizeOf(typeof(INPUT)));

        public const int INPUT_MOUSE = 0;
        public const int INPUT_KEYBOARD = 1;
        public const int INPUT_HARDWARE = 2;

        public enum KEYEVENTF
        {
            EXTENDEDKEY = 0x0001,
            KEYUP = 0x0002,
            UNICODE = 0x0004,
            SCANCODE = 0x0008,
        };

        public struct INPUT
        {
            public int type;
            public InputUnion u;
        }

        [StructLayout(LayoutKind.Explicit)]
        public struct InputUnion
        {
            [FieldOffset(0)]
            public MOUSEINPUT mi;
            [FieldOffset(0)]
            public KEYBDINPUT ki;
            [FieldOffset(0)]
            public HARDWAREINPUT hi;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct MOUSEINPUT
        {
            public int dx;
            public int dy;
            public uint mouseData;
            public uint dwFlags;
            public uint time;
            public IntPtr dwExtraInfo;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct KEYBDINPUT
        {
            /*Virtual Key code.  Must be from 1-254.  If the dwFlags member specifies KEYEVENTF_UNICODE, wVk must be 0.*/
            public ushort wVk;
            /*A hardware scan code for the key. If dwFlags specifies KEYEVENTF_UNICODE, wScan specifies a Unicode character which is to be sent to the foreground application.*/
            public ushort wScan;
            /*Specifies various aspects of a keystroke.  See the KEYEVENTF_ constants for more information.*/
            public KEYEVENTF dwFlags;
            /*The time stamp for the event, in milliseconds. If this parameter is zero, the system will provide its own time stamp.*/
            public uint time;
            /*An additional value associated with the keystroke. Use the GetMessageExtraInfo function to obtain this information.*/
            public IntPtr dwExtraInfo;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct HARDWAREINPUT
        {
            public uint uMsg;
            public ushort wParamL;
            public ushort wParamH;
        }

        [DllImport("coredll.dll", SetLastError = true)]
        public static extern uint SendInput(uint nInputs, INPUT[] pInputs, int cbSize);
    }
}

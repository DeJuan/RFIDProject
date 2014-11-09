#region Using
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.ComponentModel;
using System.Data;
using System.Reflection;
using System.IO;
using System.Threading;
using System.Timers;
using System.Windows.Threading;
using System.Xml;
using System.Collections.ObjectModel;
using System.Windows.Controls.Primitives;
using System.Collections;
using System.Windows.Media.Animation;
using Microsoft.Win32;
using ThingMagic;
using ThingMagic.URA2.ViewModel;
using ThingMagic.URA2.BL;
using System.Globalization;
using Bonjour;
using System.Net.Sockets;
using System.Net;
using System.Management;
using System.Text.RegularExpressions;
#endregion Using

namespace ThingMagic.URA2
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class Main : Window
    {
        #region Fields
        // Cache async read progress state
        private bool isAsyncReadGoingOn = false;
        // Cache sync read progress state
        private bool isSyncReadGoingOn = false;
        /// <summary>
        /// Firmware Update failed flag
        /// </summary>
        public bool isFirmwareUpdateFailed = false;
        /// <summary>
        /// Iso18k6bCheckbox checked state
        /// </summary>
        public bool iso6bCheckBoxChecked = false;
        /// <summary>
        ///  Read time gets accumulated across all Start and Stop Reads,
        ///  until Clear Reads is pressed.
        /// </summary>
        public double continuousreadElapsedTime = 0.0;
        /// <summary>
        /// Read once time
        /// </summary>
        public double readonceElapsedTime = 0.0;
        // Stream for logging
        StreamWriter log;

        /// <summary>
        /// Reader model
        /// </summary>
        public string model = string.Empty;

        /// <summary>
        /// TMR log file created time
        /// </summary>
        public string tmrLogStartTime = string.Empty;

        /// <summary>
        /// Read rate tag count per second
        /// </summary>
        public long readRatePerSecondCount = 0;

        // Supported protocols by the connected reader
        private TagProtocol[] supportedProtocols = null;
        // Load save configuration object
        private LoadSaveConfiguration loadSaveConfig = null;
        
        /// <summary>
        /// Check bonjour services are installed on the host machine or no
        /// </summary>
        private bool isBonjourServicesInstalled = true;
        
        /// <summary>
        /// Connection lost count
        /// </summary>
        static int connectionLostCount = 0;
        
        //To remember start async read time
        DateTime startAsyncReadTime;

        // Dummy columns for layers 0 and 1:
        ColumnDefinition column1CloneForLayer0 = null;
        ColumnDefinition column2CloneForLayer0 = null;
        ColumnDefinition column2CloneForLayer1 = null;

        // To re read all the tags from the database and clear the memory
        DispatcherTimer dispatchtimer = null;

        //To display tag read rate per second
        DispatcherTimer readRatePerSec = null;

        /// <summary>
        /// Save the target in temporary variable before changing it to target AB, when fast search is enabled 
        /// and Change to the original target, when fast search is disabled.. 
        /// </summary>
        private string tempTarget = string.Empty;

        /// <summary>
        /// Define a reader variable
        /// </summary>
        Reader objReader = null;

        /// <summary>
        /// Define a region variable
        /// </summary>
        Reader.Region regionToSet = new Reader.Region();

        /// <summary>
        /// Define a list of simple read plans for Multiple Read Plan
        /// </summary>
        List<ReadPlan> simpleReadPlans = new List<ReadPlan>();

        /// <summary>
        /// Define a protocol variable for access commands
        /// </summary>
        List<TagProtocol> tagOpProto = new List<TagProtocol>();

        /// <summary>
        /// Define a variable for the transport listener log
        /// </summary>
        TextWriter transportLogFile = null;
        // Cache warning text status
        string warningText;
        System.Timers.Timer myTimer = new System.Timers.Timer();

        /// <summary>
        /// Define a variable for selection criteria
        /// </summary>
        TagFilter selectionOnEPC = null;

        // Whether the settings need to be loaded from the profile or 
        // from the connected reader
        private bool initialReaderSettingsLoaded = true;

        /// <summary>
        /// Delegates 
        /// </summary>
        delegate void del();
        private delegate void EmptyDelegate();

        /// <summary>
        /// If a flurry of read exceptions occurs, only want to show one MessageBox
        /// </summary>
        private bool showingAsyncReadExceptionMessageBox;
        private Object showingAsyncReadExceptionMessageBoxLock = new Object();        

        // Tag database object
        TagDatabase tagdb = new TagDatabase();

        public ObservableCollection<string> Positions { get; set; }

        public ObservableCollection<ColumnSelectionForTagResult> selectedColumnList { get; set; }

        // Optimal tag reads settings
        Dictionary<string, string> OptimalReaderSettings = null;

        // List of configuration settings to be saved
        Dictionary<string, string> SaveConfigurations = null;

        // Bonjour initialization fields
        string uri = string.Empty;
        private DNSSDEventManager eventManager = null;
        private DNSSDService service = null;
        private DNSSDService browser = null;
        private DNSSDService resolver = null;

        private int _backgroundNotifierCallbackCount = 0;
        private Object _backgroundNotifierLock = new Object();
        List<String> servicesList = new List<String>();

        /// <summary>
        /// URA custom message box object
        /// </summary>
        URACustomMessageBoxWindow CustomizedMessageBox = null;

        //Socket for tcp streaming
        Socket tagStreamSock;
        // listener to open Tcp connection
        private TcpListener listener;
        // Tcp Server thread 
        private Thread serverThread;
        // Array to store tagread objects
        TagReadData[] trd;
        // Boolean variable to check Tcp Client connect or not
        bool clientConnected = false;
        // Bollean variable to check wait for client connection or not
        public bool waitforclient = true;
        // ManualResetEvent class for Thread synchronization 
        private ManualResetEvent suspendChangedEvent = new ManualResetEvent(false);

        #endregion

        public Main()
        {
            try
            {
                string folderpath = System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "URA");
                bool isExists = System.IO.Directory.Exists(folderpath);
                // If folder doesn't exist. Create the new folder with name URA 
                if (!isExists)
                {
                    System.IO.Directory.CreateDirectory(folderpath);
                }
                tmrLogStartTime = DateTime.Now.ToString("yyyyMMdd_HHmmss_fff");
                string filename = System.IO.Path.Combine(folderpath, "tmrlog_" + tmrLogStartTime + ".txt");
                log = new StreamWriter(filename, true);                
                // WPF draws the screen at a continuous pace of 60 frames per second rate by default. 
                // So if we are using lots of graphics and images, application will eventually take a 
                // lots CPU utilization because of these frame rates. Hence reducing the frame-rates to 
                // 10 frames per second
                Timeline.DesiredFrameRateProperty.OverrideMetadata(
                typeof(Timeline),
                new FrameworkPropertyMetadata { DefaultValue = 10 });
       
                AppDomain.CurrentDomain.UnhandledException += new UnhandledExceptionEventHandler(
                    CurrentDomain_UnhandledException);
                Application.Current.Dispatcher.UnhandledException += new DispatcherUnhandledExceptionEventHandler(
                    Dispatcher_UnhandledException);                
                InitializeComponent();
                InitializeDummyClmnsforDocking();
                dispatchtimer = new DispatcherTimer();
                readRatePerSec = new DispatcherTimer();
                readRatePerSec.Interval = TimeSpan.FromMilliseconds(900);
                dispatchtimer.Interval = TimeSpan.FromMilliseconds(50);
                readRatePerSec.Tick += new EventHandler(readRatePerSec_Tick);
                dispatchtimer.Tick += new EventHandler(dispatchtimer_Tick);
                //GenerateColmnsForDataGrid();  
                regioncombo.ItemsSource = null;
                ConfigureAntennaBoxes(null);
                ConfigureProtocols(null);
                loadSaveConfig = new LoadSaveConfiguration();
                SaveConfigurations = new Dictionary<string, string>();
                InitializeColumnSelectionCbx();
                txtTagsNum.Text = _numValue.ToString();                
                //Display URA version and Api version, when application gets initialized
                InitializeRdrDiagnostics();
                eventManager = new DNSSDEventManager();
                eventManager.ServiceFound += new _IDNSSDEvents_ServiceFoundEventHandler(this.ServiceFound);
                eventManager.ServiceResolved += new _IDNSSDEvents_ServiceResolvedEventHandler(this.ServiceResolved);
                eventManager.ServiceLost += new _IDNSSDEvents_ServiceLostEventHandler(this.ServiceLost);
                service = new DNSSDService();
                btnRefreshReadersList.Visibility = System.Windows.Visibility.Visible;    
            }            
            catch (Exception bonjEX)
            {
                Mouse.SetCursor(Cursors.Arrow);
                if (-1 != bonjEX.Message.IndexOf("80040154 Class not registered"))
                {
                    isBonjourServicesInstalled = false;
                    btnRefreshReadersList.Visibility = System.Windows.Visibility.Collapsed;
                    //do nothing
                }
                else
                {
                    MessageBox.Show(bonjEX.Message);
                }               
            }
        }        

        #region Bonjour
        /// <summary>
        /// ServiceLost
        /// </summary>
        public void ServiceLost(DNSSDService browser, DNSSDFlags flags, uint ifIndex, string serviceName, string regtype, string domain)
        {
            cmbFixedReaderAddr.Items.Clear();
            servicesList.Clear();
            cmbFixedReaderAddr.InvalidateVisual();
            Onlog("Bonjour feature: Network Lost");
            //lblWarning.Dispatcher.BeginInvoke(new ThreadStart(delegate()
            //{
            //    warningText = "Network Lost";
            //    GUIshowWarning();
            //}));
        }

        // ServiceFound
        /// <summary>
        /// This call is invoked by the DNSService core.  We create
        /// a BrowseData object and invoked the appropriate method
        /// in the GUI thread so we can update the UI
        /// </summary>
        /// <param name="sref"></param>
        /// <param name="flags"></param>
        /// <param name="ifIndex"></param>
        /// <param name="serviceName"></param>
        /// <param name="regType"></param>
        /// <param name="domain"></param>
        public void ServiceFound(DNSSDService sref, DNSSDFlags flags, uint ifIndex, String serviceName, String regType, String domain)
        {
            int index = servicesList.IndexOf(serviceName);

            //
            // Check to see if we've seen this service before. If the machine has multiple
            // interfaces, we could potentially get called back multiple times for the
            // same service. Implementing a simple reference counting scheme will address
            // the problem of the same service showing up more than once in the browse list.
            //
            if (index == -1)
            {
                lock (_backgroundNotifierLock)
                    _backgroundNotifierCallbackCount++;
                BrowseData data = new BrowseData();

                data.InterfaceIndex = ifIndex;
                data.Name = serviceName;
                data.Type = regType;
                data.Domain = domain;
                data.Refs = 1;
                servicesList.Add(serviceName);
                resolver = service.Resolve(0, data.InterfaceIndex, data.Name, data.Type, data.Domain, eventManager);                
            }
            else
            {
                BrowseData data = new BrowseData();
                data.InterfaceIndex = ifIndex;
                data.Name = servicesList[index];
                data.Name = serviceName;
                data.Type = regType;
                data.Domain = domain;                
                resolver = service.Resolve(0, data.InterfaceIndex, data.Name, data.Type, data.Domain, eventManager); 
                data.Refs++;
            }
            lblWarning.Dispatcher.BeginInvoke(new ThreadStart(delegate()
            {
                GUIturnoffWarning();
            }));
        }

        // BrowseData
        /// <summary>
        /// This class is used to store data associated
        /// with a DNSService.Browse() operation 
        /// </summary>
        public class BrowseData
        {
            public uint InterfaceIndex;
            public String Name;
            public String Type;
            public String Domain;
            public int Refs;

            public override String
            ToString()
            {
                return Name;
            }

            public override bool
            Equals(object other)
            {
                bool result = false;

                if (other != null)
                {
                    result = (this.Name == other.ToString());
                }

                return result;
            }

            public override int
            GetHashCode()
            {
                return Name.GetHashCode();
            }
        };

        // ResolveData                       
        /// <summary>
        /// This class is used to store data associated
        /// with a DNSService.Resolve() operation
        /// </summary>
        public class ResolveData
        {
            public uint InterfaceIndex;
            public String FullName;
            public String HostName;
            public int Port;
            public TXTRecord TxtRecord;

            public override String
                ToString()
            {
                return FullName;
            }
        };

        /// <summary>
        /// Populate the comports or ip addresses in the combo-box when resolved
        /// </summary>
        /// <param name="sref"></param>
        /// <param name="flags"></param>
        /// <param name="ifIndex"></param>
        /// <param name="fullName"></param>
        /// <param name="hostName"></param>
        /// <param name="port"></param>
        /// <param name="txtRecord"></param>
        public void ServiceResolved (DNSSDService sref, DNSSDFlags flags, uint ifIndex,String fullName, String hostName,ushort port,TXTRecord txtRecord)
        {
            //cmbReaderAddr.Items.Add(hostName);
           ResolveData data = new ResolveData();

            data.InterfaceIndex = ifIndex;
            data.FullName = fullName;
            data.HostName = hostName;
            data.Port = port;
            data.TxtRecord = txtRecord;

            //Adding host name
            string[] hostnameArray = hostName.Split('.');

            if (hostnameArray.Length > 0)
            {
                if (!(cmbFixedReaderAddr.Items.Contains(hostnameArray[0])))
                {
                    cmbFixedReaderAddr.Items.Add(hostnameArray[0]);                    
                }
                cmbFixedReaderAddr.SelectedIndex = 0;
            }

            //for (uint idx = 0; idx < txtRecord.GetCount(); idx++)
            //{
            //    String key;

            //    key = txtRecord.GetKeyAtIndex(idx);
            //    object value = txtRecord.GetValueAtIndex(idx);

            //    if (key.Length > 0)
            //    {
            //        String val = "";

            //       if (key == "LanIp")
            //        {
            //            foreach (Byte b in (Byte[])value)
            //            {
            //                val += b.ToString() + ".";
            //            }
            //            System.Diagnostics.Debug.WriteLine("Reader uri:" + val.TrimEnd('.'));
            //            cmbFixedReaderAddr.Items.Add(val.TrimEnd('.'));
            //            break;
            //        }
                    
            //    }
            //}

            //
            // Don't forget to stop the resolver. This eases the burden on the network
            //
            if (null != resolver)
            {
                resolver.Stop();
                resolver = null;
            }
            
            lock (_backgroundNotifierLock)
                _backgroundNotifierCallbackCount--;
            //System.Diagnostics.Debug.WriteLine("Service resolved:" + _backgroundNotifierCallbackCount);
            
        }

        #endregion Bonjour

        #region ExceptionHandling
        /// <summary>
        /// Logging the error messages into the file
        /// </summary>
        /// <param name="message"></param>
        void Onlog(string message)
        {
            string text =  DateTime.Now.ToString("yyyy-MM-dd-HH-mm-ss") +" : "+ message + "\r\n";
            log.Write(text);
            log.Flush();
        }

        /// <summary>
        /// Application.DispatcherUnhandledException will handle exceptions thrown on the main UI thread in a WPF application
        /// </summary>
        void Dispatcher_UnhandledException(object sender, DispatcherUnhandledExceptionEventArgs e)
        {
            Onlog(e.Exception.Message);
            Onlog(e.Exception.StackTrace);
            if (e.Exception is ReaderCommException)
            {
                MessageBox.Show("Unable to communicate through COM port");
            }
            else if (e.Exception is InvalidOperationException)
            {
                //MessageBox.Show("This operation is not supported");
            }
            else if (e.Exception is IOException)
            {
                MessageBox.Show(e.Exception.Message,"Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
            //Log the message into text file
            Onlog(e.Exception.Message);
            Onlog(e.Exception.StackTrace);
            e.Handled = true;
        }

        /// <summary>
        /// AppDomain.UnhandledException will handle exceptions thrown on any thread and never caught
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void CurrentDomain_UnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            Exception ex = (Exception)e.ExceptionObject;
            Onlog("Exceptions thrown on the main UI thread in a URA2 application");
            Onlog(ex.Message);
            Onlog(ex.StackTrace);
            if (ex.Message.Contains("The device does not recognize the command."))
            {
                MessageBox.Show(ex.Message + "Due to unavoidable error, restarting the application",
                    "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                System.Diagnostics.Process.Start(System.Reflection.Assembly.GetExecutingAssembly().Location);
                Environment.Exit(0);
            }
        }

        /// <summary>
        /// This exception is thrown when antenna for tag operation is not selected
        /// </summary>
        public class TagOpAntennaException : ReaderException
        {
            public TagOpAntennaException(string message) : base(message) { }
        }

        /// <summary>
        /// This exception is thrown when protocol for tag operation is not selected
        /// </summary>
        public class TagOpProtocolException : ReaderException
        {
            public TagOpProtocolException(string message) : base(message) { }
        }

        /// <summary>
        /// Synchronous Reads Exception
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void ReadException(Object sender, ReaderExceptionEventArgs e)
        {
            // Clear previous shown error message on status bar
            ClearMessageOnStatusBar();

            // Log the exception
            Onlog(e.ReaderException.Message + ", Time :"+DateTime.Now.ToString());

            if (e.ReaderException.Message.Equals("No response from tag"))
            {
                // Display error message on the status bar
                DisplayMessageOnStatusBar("No response from tag", Brushes.Red);
                return;
            }

            if (e.ReaderException.Message.Equals("Operation not supported. M5e does not support zero-length read."))
            {
                // Display error message on the status bar
                DisplayMessageOnStatusBar(e.ReaderException.Message, Brushes.Red);
                return;
            }

            if (e.ReaderException is FAULT_TAG_ID_BUFFER_FULL_Exception)
            {
                // Display error message on the status bar
                DisplayMessageOnStatusBar("Tag ID Buffer Full", Brushes.Red);
            }
            else if ((e.ReaderException.Message.Contains("The operation has timed out.")))
            {
                Onlog(e.ReaderException.Message);
                // Display error message on the status bar
                DisplayMessageOnStatusBar(e.ReaderException.Message, Brushes.Red);

                // Disconnect the reader when exception is received
                DisconnectReader();
            }
            else if (e.ReaderException.Message.Contains("The port '"+uri+"' does not exist."))
            {
                Onlog("Error :" + e.ReaderException.Message);
            }
            else if ((e.ReaderException is ReaderCodeException)
                && ((((ReaderCodeException)e.ReaderException).Code == 0x504)
                || (((ReaderCodeException)e.ReaderException).Code == 0x505)))
            {
                switch (((ReaderCodeException)e.ReaderException).Code)
                {
                    case 0x504:
                        warningText = "Over Temperature";
                        break;
                    case 0x505:
                        warningText = "High Return Loss";
                        break;
                    default:
                        warningText = "warning";
                        break;

                }
                // Display custom message box with "Don't ask me again" check-box especially for the 
                // M6e-Micro, its not apparent enough to show the error only in status bar and some 
                // customers simply think it slow performance.
                if (model.Equals("M6e Micro") && (warningText == "Over Temperature"))
                {
                    Dispatcher.BeginInvoke(new ThreadStart(delegate()
                    {
                        // If 'don't ask me again' is checked. Don't display the message just
                        // log the message and show the error in status bar
                        if (!CustomizedMessageBox.DoNotAskMeAgainChecked)
                        {
                            if (this != CustomizedMessageBox.Owner)
                            {
                                CustomizedMessageBox.Owner = this;
                            }
                            if (!CustomizedMessageBox.MessageBoxOpened)
                            {
                                CustomizedMessageBox.Show();
                            }
                        }
                        else
                        {
                            GUIshowWarning();
                        }
                    }));
                }
                else
                {
                    // For rest of the readers just log and display the error in status bar
                    lblWarning.Dispatcher.BeginInvoke(new ThreadStart(delegate()
                    {
                        GUIshowWarning();
                    }));
                }
            }
            else if ((-1 != (e.ReaderException.Message.IndexOf("The I/O operation has been aborted"
                       + " because of either a thread exit or an application request."))))
            {
                Onlog("Error :" + e.ReaderException.Message);
                MessageBox.Show(e.ReaderException.Message + "The port '"+uri+"' does not exist.",
                                 "Error", MessageBoxButton.OK, MessageBoxImage.Error);

                // Disconnect the reader when exception is received
                DisconnectReader();
            }
            else if ((-1 != e.ReaderException.Message.IndexOf("Specified port does not exist")))
            {
                Onlog("Error :" + e.ReaderException.Message);
                MessageBox.Show(e.ReaderException.Message,
                                 "Error", MessageBoxButton.OK, MessageBoxImage.Error);

                // Disconnect the reader when exception is received
                DisconnectReader();
            }
            else if (-1 != e.ReaderException.Message.IndexOf("The port is closed."))
            {
                Onlog("Error :" + e.ReaderException.Message);
            }
            else if ((e.ReaderException.Message.Contains("Connection Lost"))
                    || (e.ReaderException.Message.Contains("Request timed out")))
            {
                Onlog("Error :" + e.ReaderException.Message + "Time : " + DateTime.Now.ToString());
                if (!isAsyncReadGoingOn)
                {
                    btnConnect.Dispatcher.BeginInvoke(new ThreadStart(delegate()
                    {
                        btnConnect.Content = "Disconnect";
                        btnConnect_Click(sender, new RoutedEventArgs());
                    }));

                    // Display error message on the status bar
                    DisplayMessageOnStatusBar("Connection lost", Brushes.Red);
                }
                else if (isAsyncReadGoingOn)
                {
                    HandleAsyncReadException(e.ReaderException);
                    if (isStopReadingBtnPressed)
                    {
                        btnConnect.Dispatcher.BeginInvoke(new ThreadStart(delegate()
                        {
                            Mouse.SetCursor(Cursors.AppStarting);
                            btnConnect.Content = "Disconnect";
                            btnConnect_Click(sender, new RoutedEventArgs());
                            // Remove any older status messages if exists
                            DisplayMessageOnStatusBar(e.ReaderException.Message, Brushes.Red);
                        }));
                    }
                    else
                    {
                        ++connectionLostCount;
                        if (connectionLostCount == 1)
                        {
                            // Consider the elapsed time till the first occurrence of the 
                            // connection lost exception and not till we receive the 5th 
                            // connection lost exception
                            Dispatcher.BeginInvoke(new ThreadStart(delegate()
                            {
                                continuousreadElapsedTime = CalculateElapsedTime();
                            }));
                        }
                        // Disable the read/stop-reading button when connection is lost
                        btnConnect.Dispatcher.BeginInvoke(new ThreadStart(delegate()
                        {
                            btnRead.IsEnabled = false;
                            cmbFixedReaderAddr.Text = uri;
                        }));
                        // Wait for 5 connection lost exception. If 5 exceptions, 
                        // assume reader has been restarted. hence restart the URA
                        if (connectionLostCount >= 5)
                        {
                            if (null != objReader)
                            {
                                objReader.Destroy();
                            }
                            objReader = null;
                            //Reconnect the reader
                            ReconnectReader(sender, new RoutedEventArgs());
                            // Reset connection lost exception counter
                            connectionLostCount = 0;
                        }
                    }
                }
                else
                {
                    try
                    {
                        stopReading();
                    }
                    finally
                    {
                        ShutdownStartReads(e.ReaderException);
                    }
                }
            }
            else
            {
                Onlog("Error :" + e.ReaderException.Message + "Time : " + DateTime.Now.ToString());
            }            
        }

        /// <summary>
        /// Displays error message or message on the status bar
        /// </summary>
        /// <param name="errorMessage"> error message or message to be displayed</param>
        /// <param name="colour">color to indicate the severity of the error</param>
        private void DisplayMessageOnStatusBar(string errorMessage, SolidColorBrush colour)
        {
            warningText = errorMessage;
            lblWarning.Dispatcher.BeginInvoke(new ThreadStart(delegate()
            {
                GUIshowWarning(colour);
            }));
        }

        /// <summary>
        /// Clear previously shown error message or message on the status bar
        /// </summary>
        private void ClearMessageOnStatusBar()
        {
            lblWarning.Dispatcher.BeginInvoke(new ThreadStart(delegate()
            {
                GUIturnoffWarning();
            }));
        }

        /// <summary>
        /// Disconnect the reader when I/O operation has been aborted 
        /// or the operation has timed out exception is occurred.
        /// </summary>
        private void DisconnectReader()
        {
            // Stop read rate per sec calculation
            readRatePerSec.Stop();
            btnConnect.Dispatcher.BeginInvoke(new ThreadStart(delegate()
            {
                isAsyncReadGoingOn = false;
                btnConnect.Content = "Disconnect";
                btnRead.Content = "Read";
                btnRead.ToolTip = "Start Async Read";
                rdBtnReadOnce.IsEnabled = true;
                AntennasGroupBox.IsEnabled = true;
                ProtocolsGroupBox.IsEnabled = true;
                ReadDataGroupBox.IsEnabled = true;
                FilterGroupBox.IsEnabled = true;
                // Disconnect the reader
                btnConnect_Click(this, new RoutedEventArgs());
                objReader = null;
            }));
        }

        /// <summary>
        /// Attempt to reconnect to the reader when connection is lost
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ReconnectReader(Object sender, RoutedEventArgs e)
        {
            lock (new Object())
            {
                int retryCount;
                for (retryCount = 1; retryCount < 6; retryCount++)
                {
                    try
                    {
                        objReader = Reader.Create(string.Concat("tmr://", uri));
                        Onlog("Attempting to reconnect - " + (retryCount) + ", after connection lost");
                        // Display error message on the status bar
                        DisplayMessageOnStatusBar("Attempting to reconnect - " + (retryCount), Brushes.Red);
                        objReader.Connect();
                        warningText = "";
                        lblWarning.Dispatcher.BeginInvoke(new ThreadStart(delegate()
                        {
                            cmbFixedReaderAddr.Text = uri;
                            // Enable the read/stop-reading button when URA is able to connect to the reader.
                            btnRead.IsEnabled = true;
                            GUIturnoffWarning();
                            btnRead.Content = "Read";
                            AntennasGroupBox.IsEnabled = true;
                            ProtocolsGroupBox.IsEnabled = true;
                            ReadDataGroupBox.IsEnabled = true;
                            FilterGroupBox.IsEnabled = true;
                            if ((bool)rdBtnReadContinuously.IsChecked)
                            {
                                OnStartRead("Reconnecting");
                            }
                        }));
                        break;
                    }
                    catch(Exception ex)
                    {
                        Onlog(ex.Message);
                         continue; 
                    }
                }

                if (retryCount >= 5)
                {
                    btnConnect.Dispatcher.BeginInvoke(new ThreadStart(delegate()
                    {
                        btnConnect.Content = "Disconnect";
                        btnConnect_Click(sender, e);
                    }));
                }
            }
        }

        /// <summary>
        /// Handle connection lost exception.
        /// </summary>
        /// <param name="ex"></param>
        private void HandleAsyncReadException(Exception ex)
        {
            bool alreadyShowing;
            lock (showingAsyncReadExceptionMessageBoxLock)
            {
                alreadyShowing = showingAsyncReadExceptionMessageBox;
                showingAsyncReadExceptionMessageBox = true;
            }
            if (!alreadyShowing)
            {
                this.Dispatcher.Invoke(new ThreadStart(delegate()
                {
                    Mouse.SetCursor(Cursors.Arrow);
                    readRatePerSec.Stop();

                    if (ex.Message.Contains("Connection Lost"))
                    {
                        // Display error message on the status bar
                        DisplayMessageOnStatusBar(ex.Message + "!. Trying to reconnect...", Brushes.Red);
                    }
                    else
                    {
                        // Display error message on the status bar
                        DisplayMessageOnStatusBar(ex.Message, Brushes.Red);
                    }

                }));
                lock (showingAsyncReadExceptionMessageBoxLock)
                {
                    showingAsyncReadExceptionMessageBox = false;
                }
            }            
        }

        /// <summary>
        /// Shows dialog box when Reader Comm exception is found
        /// </summary>
        /// <param name="exp">Reader Comm Exception</param>
        /// <param name="e"></param>
        public void handleReaderCommException(ReaderCommException exp, RoutedEventArgs e)
        {
            Mouse.SetCursor(Cursors.Arrow);
            switch (MessageBox.Show(exp.Message.ToString() + "\nDo you want to reset the reader now?",
                "Error", MessageBoxButton.YesNoCancel, MessageBoxImage.Error))
            {
                case MessageBoxResult.Yes:
                    {
                        // disconnectReader_Click(this, e);
                        btnConnect_Click(this, e);
                        break;
                    }
                case MessageBoxResult.No:
                    {
                        break;
                    }
                case MessageBoxResult.Cancel:
                    {
                        exitURA_Click(this, e);
                        break;
                    }
                default:
                    {
                        break;
                    }
            }
        }
        
        #endregion ExceptionHandling

        #region Initialize

        /// <summary>
        /// Populate Column selection combo-box in display options 
        /// </summary>
        private void InitializeColumnSelectionCbx()
        {
            selectedColumnList = new ObservableCollection<ColumnSelectionForTagResult>();
            selectedColumnList.Add(new ColumnSelectionForTagResult(false, "Antenna"));
            selectedColumnList.Add(new ColumnSelectionForTagResult(false, "Frequency"));
            selectedColumnList.Add(new ColumnSelectionForTagResult(false, "Phase"));
            selectedColumnList.Add(new ColumnSelectionForTagResult(false, "Protocol"));
            cbxcolumnSelection.ItemsSource = selectedColumnList;
        }

        /// <summary>
        /// // Initialize the dummy columns used when docking
        /// </summary>
        private void InitializeDummyClmnsforDocking()
        {
            column1CloneForLayer0 = new ColumnDefinition();
            column1CloneForLayer0.SharedSizeGroup = "column1";
            column2CloneForLayer0 = new ColumnDefinition();
            column2CloneForLayer0.SharedSizeGroup = "column2";
            column2CloneForLayer1 = new ColumnDefinition();
            column2CloneForLayer1.SharedSizeGroup = "column2";
        }

        /// <summary>
        /// Populate reader uri box with the port numbers
        /// </summary>
        private void InitializeReaderUriBox()
        {
            try
            {
                Mouse.SetCursor(Cursors.Wait);
                if ((bool)rdbtnLocalConnection.IsChecked)
                {
                    List<string> portNames = new List<string>();
                    using (var searcher = new ManagementObjectSearcher("root\\CIMV2", "SELECT * FROM Win32_PnPEntity WHERE ConfigManagerErrorCode = 0"))
                    {
                        foreach (ManagementObject queryObj in searcher.Get())
                        {
                            if (queryObj["Name"].ToString().Contains("(COM"))
                                portNames.Add(queryObj["Name"].ToString());
                        }
                        cmbReaderAddr.ItemsSource = portNames;
                        if (portNames.Count > 0)
                        {
                            cmbReaderAddr.Text = portNames[0];
                        }
                    }
                }
                else if (isBonjourServicesInstalled)
                {
                    _backgroundNotifierCallbackCount = 0;
                    if (browser != null)
                    {
                        browser.Stop();
                        servicesList.Clear();
                    }

                    string[] serviceTypes = { "_llrp._tcp", "_m4api._udp." };//, 

                    foreach (string serviceType in serviceTypes)
                    {
                        browser = service.Browse(0, 0, serviceType, null, eventManager);
                    }
                    Thread.Sleep(500);
                    while (0 < _backgroundNotifierCallbackCount)
                    {
                        Thread.Sleep(100);
                    }
                }
                Mouse.SetCursor(Cursors.Arrow);
            }
            catch (Exception bonjEX)
            {
                Mouse.SetCursor(Cursors.Arrow);               
                    throw bonjEX;                
            }
        }

        /// <summary>
        /// Populate optimal settings based on reader
        /// </summary>
        private void InitializeOptimalSettings()
        {
            OptimalReaderSettings = new Dictionary<string, string>();
            Gen2Settings_InitialLoad();
        }

        /// <summary>
        /// Get all the current gen2 settings and set the radio buttons accordingly
        /// </summary>
        private void Gen2Settings_InitialLoad()
        {
            //Get all the current settings and set the radio buttons accordingly

            try
            {
                Gen2.LinkFrequency blf = (Gen2.LinkFrequency)objReader.ParamGet("/reader/gen2/BLF");
                switch (blf)
                {
                    case Gen2.LinkFrequency.LINK250KHZ:
                        OptimalReaderSettings["/reader/gen2/BLF"] = "LINK250KHZ"; break;
                    case Gen2.LinkFrequency.LINK640KHZ:
                        OptimalReaderSettings["/reader/gen2/BLF"] = "LINK640KHZ";; break;
                    default:
                        OptimalReaderSettings.Add("/reader/gen2/BLF", ""); break;
                }
            }
            catch (ArgumentException)
            {
                OptimalReaderSettings.Add("/reader/gen2/BLF", "");
            }            
            
            try
            {

                Gen2.Tari tariVal = (Gen2.Tari)objReader.ParamGet("/reader/gen2/Tari");
                switch (tariVal)
                {
                    case Gen2.Tari.TARI_6_25US:
                        OptimalReaderSettings["/reader/gen2/tari"] = "TARI_6_25US"; break;
                    case Gen2.Tari.TARI_12_5US:
                        OptimalReaderSettings["/reader/gen2/tari"] = "TARI_12_5US"; break;
                    case Gen2.Tari.TARI_25US:
                        OptimalReaderSettings["/reader/gen2/tari"] = "TARI_25US"; break;
                    default: 
                        OptimalReaderSettings.Add("/reader/gen2/tari", ""); break;
                }
            }
            catch (ArgumentException)
            {
                OptimalReaderSettings.Add("/reader/gen2/tari", "");
            }
            catch (ReaderCodeException)
            {
            }

            try
            {
                Gen2.TagEncoding tagencoding = (Gen2.TagEncoding)objReader.ParamGet("/reader/gen2/tagEncoding");
                switch (tagencoding)
                {
                    case Gen2.TagEncoding.FM0:
                        OptimalReaderSettings["/reader/gen2/tagEncoding"] = "FM0"; break;
                    case Gen2.TagEncoding.M2:
                        OptimalReaderSettings["/reader/gen2/tagEncoding"] = "M2"; break;
                    case Gen2.TagEncoding.M4:
                        OptimalReaderSettings["/reader/gen2/tagEncoding"] = "M4"; break;
                    case Gen2.TagEncoding.M8:
                        OptimalReaderSettings["/reader/gen2/tagEncoding"] = "M8"; break;
                    default:
                        OptimalReaderSettings.Add("/reader/gen2/tagEncoding", ""); break;
                }
            }
            catch (ArgumentException)
            {
                OptimalReaderSettings.Add("/reader/gen2/tagEncoding", "");
            }
            try
            {
                Gen2.Session session = (Gen2.Session)objReader.ParamGet("/reader/gen2/session");
                switch (session)
                {
                    case Gen2.Session.S0:
                        OptimalReaderSettings["/reader/gen2/session"] = "S0"; break;
                    case Gen2.Session.S1:
                       OptimalReaderSettings["/reader/gen2/session"] = "S1"; break;
                    case Gen2.Session.S2:
                        OptimalReaderSettings["/reader/gen2/session"] = "S2"; break;
                    case Gen2.Session.S3:
                        OptimalReaderSettings["/reader/gen2/session"] = "S3"; break;
                    default: 
                        OptimalReaderSettings.Add("/reader/gen2/session", ""); break;
                }
            }
            catch (ArgumentException)
            {
                OptimalReaderSettings.Add("/reader/gen2/session", "");
            }
            try
            {
                Gen2.Target target = (Gen2.Target)objReader.ParamGet("/reader/gen2/Target");
                switch (target)
                {
                    case Gen2.Target.A:
                        OptimalReaderSettings["/reader/gen2/target"] = "A"; break;
                    case Gen2.Target.B:
                        OptimalReaderSettings["/reader/gen2/target"] = "B"; break;
                    case Gen2.Target.AB:
                        OptimalReaderSettings["/reader/gen2/target"] = "AB"; break;
                    case Gen2.Target.BA:
                        OptimalReaderSettings["/reader/gen2/target"] = "BA"; break;
                    default: OptimalReaderSettings.Add("Target", ""); break;
                }
            }
            catch (FeatureNotSupportedException)
            {
                OptimalReaderSettings.Add("Target", "");
            }
            try
            {
                Gen2.Q qval = (Gen2.Q)objReader.ParamGet("/reader/gen2/q");

                if (qval.GetType() == typeof(Gen2.DynamicQ))
                {
                    OptimalReaderSettings["/reader/gen2/q"] = "DynamicQ";
                }
                else if (qval.GetType() == typeof(Gen2.StaticQ))
                {
                    Gen2.StaticQ stqval = (Gen2.StaticQ)qval;
                    OptimalReaderSettings["/reader/gen2/q"] = "StaticQ";
                    int countQ = Convert.ToInt32(((Gen2.StaticQ)qval).InitialQ);
                    OptimalReaderSettings["/application/performanceTuning/staticQValue"] = countQ.ToString();
                }
                else
                {
                    OptimalReaderSettings.Add("/reader/gen2/q", "");
                }
            }
            catch (FeatureNotSupportedException)
            {
                OptimalReaderSettings.Add("/reader/gen2/q", "");
            }
        }

        /// <summary>
        /// Set maximum read power and initialize max and min read power to the read power slider.
        /// </summary>
        private void InitializeRdPwrSldrMaxNMinValue()
        {
            try
            {
                sldrReadPwr.Maximum = (Convert.ToDouble(
                    objReader.ParamGet("/reader/radio/powerMax").ToString()) / 100);
                if (regioncombo.SelectedItem.ToString().Equals("NA") 
                    && (objReader.ParamGet("/reader/version/model").ToString().Equals("Astra-EX")))
                {
                    if (sldrReadPwr.Maximum > 30.0)
                    {
                        // Since Astra-EX reader with NA region don't support maximum read power
                        // i.e 31.5 dBm on 1st antenna. Hence setting read power to 30 dBm for 
                        // all the antenna. 
                        sldrReadPwr.Value = 30.0;
                    }
                }
                else
                {
                    sldrReadPwr.Value = sldrReadPwr.Maximum;
                }                
                sldrReadPwr.Minimum = (Convert.ToDouble(
                    objReader.ParamGet("/reader/radio/powerMin").ToString()) / 100);
            }
            catch (ReaderException ex)
            {
                throw ex;
            }
            catch (Exception ex)
            {
                throw ex;
            }
        }

        #endregion Initialize

        #region EventHandler

        /// <summary>
        /// Refresh the data grid for every dispatcher interval set
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        private void dispatchtimer_Tick(Object sender, EventArgs args)
        {
            try
            {
                // Causes a control bound to the BindingSource to reread all the items 
                // in the list and refresh their displayed values.
                TagResults.tagagingColourCache.Clear();
                tagdb.Repaint();
                // Forces an immediate garbage collection from generation zero through
                // a specified generation.            
                GC.Collect(1);
                // Retrieves the number of bytes currently thought to be allocated. If 
                // the forceFullCollection parameter is true, this method waits a short
                // interval before returning while the system collects garbage and finalizes
                // objects.            
                long totalmem1 = GC.GetTotalMemory(true);
            }
            catch { }
        }

        /// <summary>
        /// Exit URA application
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void exitURA_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                // Don't go for tear down if the URA is not connected to any reader/module
                if ((null != objReader) && (lblshowStatus.Content.ToString() == "Connected"))
                {
                    if (objReader is SerialReader)
                    {
                        objReader.ParamSet("/reader/baudRate", 115200);
                    }
                    btnConnect_Click(sender, e);
                }
                else
                {
                    if (null != objReader)
                    {
                        if (objReader is SerialReader)
                        {
                            objReader.ParamSet("/reader/baudRate", 115200);
                        }
                        objReader.Destroy();
                    }
                }
            }
            catch { }
        }

        /// <summary>
        /// Copy feature implementation
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void copyCtrl_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Clipboard.Clear();
                string selectedText = "";
                Clipboard.SetText(selectedText);
            }
            catch { }
        }

        /// <summary>
        /// Paste feature implementation
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void pasteCtrl_Click(object sender, RoutedEventArgs e)
        {
            if (e.GetType().Equals(typeof(TextBox)))
            {
                TextBox tb = (TextBox)sender;
                tb.Paste();
            }
        }

        /// <summary>
        /// About info of the application
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void demoApplication_Click(object sender, RoutedEventArgs e)
        {
            MessageBox.Show(
                            String.Join("\n", new string[] {
                    String.Format("Universal Reader Assistant {0}", 
                    this.GetType().Assembly.GetName().Version.ToString()),
                    String.Format("Mercury API Version {0}", 
                    Assembly.GetAssembly(typeof(ThingMagic.Reader)).GetName().Version.ToString()),
                }),
                            "About Demo Application...",
                            MessageBoxButton.OK);
        }

        private void thingMagicReader_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                string software = (string)objReader.ParamGet("/reader/version/software");
                MessageBox.Show(string.Concat("Hardware: ", objReader.ParamGet("/reader/version/model").ToString(),
                    "  ", "Software: ", software), "About ThingMagic Reader...", MessageBoxButton.OK);
            }
            catch
            {
                MessageBox.Show("Connection to ThingMagic Reader not established", 
                    "Error!", MessageBoxButton.OK, MessageBoxImage.Error);
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
                    tagdb.Add(read);
                }
            }
        }

        /// <summary>
        /// Validates EPC String and formats valid strings into EPC
        /// </summary>
        /// <param name="epcString">Valid/Invalid EPC String</param>
        /// <returns>Valid TagData used for selection</returns>
        public TagData validateEpcStringAndReturnTagData(string epcString)
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
        /// Remove error status messages after specified interval
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void TimeEvent(object sender, ElapsedEventArgs e)
        {
            myTimer.Stop();
            lblWarning.Dispatcher.Invoke(new del(GUIturnoffWarning));
        }

        /// <summary>
        /// Show warning messages on to the status bar at the bottom
        /// of the application
        /// </summary>
        void GUIshowWarning()
        {
            lblWarning.Background = Brushes.Red;
            lblWarning.Content = warningText;
        }
        
        /// <summary>
        /// Show warning messages on to the status bar at the bottom
        /// of the application
        /// </summary>
        /// <param name="colour">Color to indicate the severity of the error</param>
        void GUIshowWarning(SolidColorBrush colour)
        {
            lblWarning.Background = colour;
            lblWarning.Content = warningText;
        }
        
        /// <summary>
        /// Clear warning messages on to the status bar at the bottom
        /// of the application
        /// </summary>
        void GUIturnoffWarning()
        {
            lblWarning.Background = Brushes.Transparent;
            lblWarning.Content = "";
        }

        void updateGUIatStop()
        {
            GUIshowWarning();
            btnRead.ToolTip = "Start Async Read";
            //readTag.IsEnabled = true;
            cbxBaudRate.IsEnabled = true;
            //thingMagicReader.IsEnabled = true;
        }

        /// <summary>
        /// Show error only once when async read is going on
        /// </summary>
        /// <param name="ex"></param>
        private void ShutdownStartReads(Exception ex)
        {
            this.btnRead.Dispatcher.BeginInvoke(new ThreadStart(delegate()
            {
                OnStopReadsClick();
            }));

            bool alreadyShowing;
            lock (showingAsyncReadExceptionMessageBoxLock)
            {
                alreadyShowing = showingAsyncReadExceptionMessageBox;
                showingAsyncReadExceptionMessageBox = true;
            }
            if (!alreadyShowing)
            {
                this.Dispatcher.Invoke(new ThreadStart(delegate()
                {
                    MessageBox.Show(ex.Message, "Reader Message", MessageBoxButton.OK, MessageBoxImage.Error);
                }));
                lock (showingAsyncReadExceptionMessageBoxLock)
                {
                    showingAsyncReadExceptionMessageBox = false;
                }
            }
        }

        /// <summary>
        /// Check control visibility i.e if control property is set to visible return 
        /// true, and if property is set to hidden or collapsed return false
        /// </summary>
        /// <param name="obj">control</param>
        /// <returns>bool</returns>
        private bool checkControlVisibility(Control obj)
        {
            if (obj.Visibility.Equals(Visibility.Visible))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        /// <summary>
        /// Clear all the ui controls and database related to tag reads 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnClearReads_Click(object sender, RoutedEventArgs e)
        {            
            ClearReads();
        }

        /// <summary>
        /// Clear all the ui controls and database related to tag reads 
        /// </summary>
        private void ClearReads()
        {
            Thread st = new Thread(delegate()
            {
                this.Dispatcher.BeginInvoke(new ThreadStart(delegate()
                {
                    lock (tagdb)
                    {
                        tagdb.Clear();
                        tagdb.Repaint();
                    }
                }
                ));
                Dispatcher.Invoke(new del(delegate()
                {
                    try
                    {
                        startAsyncReadTime = DateTime.Now;
                        GUIturnoffWarning();
                        CheckBox headerCheckBox = (CheckBox)GetChildControl(TagResults.dgTagResults, "headerCheckBox");
                        headerCheckBox.IsChecked = false;
                        txtTotalTagReads.Content = "0";
                        totalUniqueTagsReadTextBox.Content = "0";
                        totalTimeElapsedTextBox.Content = "0";
                        readRatePerSecondCount = 0;
                        readonceElapsedTime = 0.0;
                        txtbxReadRatePerSec.Content = "0";
                        continuousreadElapsedTime = 0.0;
                        TagResults.tagagingColourCache.Clear();
                    }
                    catch { }
                }));

            });
            st.Start();
        }

        /// <summary>
        /// Open the thingmagic support link when clicked on image
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void image1_MouseDown(object sender, MouseButtonEventArgs e)
        {
            ToolTip forWebsite = new ToolTip();
            forWebsite.Content = "www.thingmagic.com";
            System.Diagnostics.Process.Start("http://rfid.thingmagic.com/devkit");
        }

        /// <summary>
        /// Detect readers connected to each comports and suffix the name to the comports populated in the combo-box
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void probeReadNames_Click(object sender, RoutedEventArgs e)
        {
            // hourglass cursor
            probeReadNames.IsEnabled = false;
            Mouse.SetCursor(System.Windows.Input.Cursors.Wait);

            try
            {
                cmbReaderAddr.ItemsSource = "";
                InitializeReaderUriBox();
                List<string> readersList = new List<string>();
                for (int count = 0; count < cmbReaderAddr.Items.Count; count++)
                {
                    if (cmbReaderAddr.Items[count].ToString().Contains("COM") || cmbReaderAddr.Items[count].ToString().Contains("com"))
                    {
                        ///Creates a Reader Object for operations on the Reader.
                        objReader = Reader.Create(string.Concat("tmr:///", cmbReaderAddr.Items[count].ToString()));

                    }
                    else
                    {
                        //Creates a Reader Object for operations on the Reader.
                        objReader = Reader.Create(string.Concat("tmr://", cmbReaderAddr.Items[count].ToString()));

                    }

                    ///Assign  reader to reader
                    //reader = reader; //(Reader)reader;
                    try
                    {
                        objReader.Connect();
                    }
                    catch
                    {
                        continue;
                    }

                    UInt16 id = (UInt16)objReader.ParamGet("/reader/version/productGroupID");
                    string model = (string)objReader.ParamGet("/reader/version/model");
                    if (id == 1)
                    {
                        readersList.Add(cmbReaderAddr.Items[count].ToString() + " " + "M5e Ruggedized Reader");
                    }
                    else if (id == 2)
                    {
                        readersList.Add(cmbReaderAddr.Items[count].ToString() + " " + "-C USB Reader");
                    }
                    else if (model.Equals("M6e"))
                    {
                        readersList.Add(cmbReaderAddr.Items[count].ToString() + " " + "M6e Module");
                    }
                    else if (model.Equals("M5e"))
                    {
                        readersList.Add(cmbReaderAddr.Items[count].ToString() + " " + " M5e Module");
                    }
                    else if (model.Equals("M5e Compact"))
                    {
                        readersList.Add(cmbReaderAddr.Items[count].ToString() + " " + "-C USB Reader");
                    }
                    else
                    {
                        readersList.Add(cmbReaderAddr.Items[count].ToString());
                    }
                    objReader.Destroy();
                }
                if (readersList.Count > 0)
                {
                    cmbReaderAddr.Items.Clear();
                    foreach (string uri in readersList)
                    {
                        cmbReaderAddr.Items.Add(uri);
                    }
                    cmbReaderAddr.SelectedIndex = 0;
                }

                //string[] services = { "_llrp._tcp", "_m4api._udp.", "_telnet._tcp." };
                //foreach (string service in services)
                //{
                //    nsBrowser.SearchForService(service, "");
                //}
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
                probeReadNames.IsEnabled = true;
            }
            finally
            {
                Mouse.SetCursor(Cursors.Arrow);
            }
        }

        #region Docking of tool panel

        /// <summary>
        /// Toggle between docked and undocked states (Pane 1) 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void pane1Pin_Click(object sender, RoutedEventArgs e)
        {
            if (pane1Button.Visibility == Visibility.Collapsed)
                UndockPane(1);
            else
                DockPane(1);
        }

        /// <summary>
        /// Show Pane 1 when hovering over its button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void pane1Button_MouseEnter(object sender, RoutedEventArgs e)
        {
            layer1.Visibility = Visibility.Visible;
            // Adjust Z order to ensure the pane is on top:
            Grid.SetZIndex(layer1, 1);
            //Grid.SetZIndex(layer2, 0);
            //// Ensure the other pane is hidden if it is undocked
            //if (pane2Button.Visibility == Visibility.Visible)
            //    layer2.Visibility = Visibility.Collapsed;
        }

        /// <summary>
        /// Hide any undocked panes when the mouse enters Layer 0 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void layer0_MouseEnter(object sender, RoutedEventArgs e)
        {
            if (pane1Button.Visibility == Visibility.Visible)
                layer1.Visibility = Visibility.Collapsed;
            //if (pane2Button.Visibility == Visibility.Visible)
            //    layer2.Visibility = Visibility.Collapsed;
        }

        /// <summary>
        /// Hide the other pane if undocked when the mouse enters Pane 2
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void pane2_MouseEnter(object sender, RoutedEventArgs e)
        {
            // Ensure the other pane is hidden if it is undocked
            if (pane1Button.Visibility == Visibility.Visible)
                layer1.Visibility = Visibility.Collapsed;
        }

        /// <summary>
        /// Docks a pane, which hides the corresponding pane button 
        /// </summary>
        /// <param name="paneNumber"></param>
        public void DockPane(int paneNumber)
        {
            if (paneNumber == 1)
            {
                pane1Button.Visibility = Visibility.Collapsed;
                pane1PinImage.Source = new BitmapImage(new Uri("pack://application:,,,/Icons/pin.gif",
                    UriKind.RelativeOrAbsolute));
                // Add the cloned column to layer 0:
                layer0.ColumnDefinitions.Add(column1CloneForLayer0);
                // Add the cloned column to layer 1, but only if pane 2 is docked:
                //if (pane2Button.Visibility == Visibility.Collapsed)
                //layer1.ColumnDefinitions.Add(column2CloneForLayer1);
            }

        }

        /// <summary>
        /// Undocks a pane, which reveals the corresponding pane button 
        /// </summary>
        /// <param name="paneNumber"></param>
        public void UndockPane(int paneNumber)
        {
            if (paneNumber == 1)
            {
                layer1.Visibility = Visibility.Visible;
                pane1Button.Visibility = Visibility.Visible;
                pane1PinImage.Source = new BitmapImage
                (new Uri("pack://application:,,,/Icons/pinHorizontal.gif", UriKind.RelativeOrAbsolute));
                // Remove the cloned columns from layers 0 and 1:
                layer0.ColumnDefinitions.Remove(column1CloneForLayer0);
                // This won’t always be present, but Remove silently ignores bad columns:
                layer1.ColumnDefinitions.Remove(column2CloneForLayer1);
            }

        }
        #endregion

        /// <summary>
        /// Get the child control placed inside the control template 
        /// </summary>
        /// <param name="parent"></param>
        /// <param name="controlName"></param>
        /// <returns></returns>
        private object GetChildControl(DependencyObject parent, string controlName)
        {
            Object tempObj = null;
            int count = VisualTreeHelper.GetChildrenCount(parent);
            for (int counter = 0; counter < count; counter++)
            {
                //Get The Child Control based on Index
                tempObj = VisualTreeHelper.GetChild(parent, counter);

                //If Control's name Property matches with the argument control
                //name supplied then Return Control
                if ((tempObj as DependencyObject).GetValue(NameProperty).ToString() == controlName)
                    return tempObj;
                else //Else Search Recursively
                {
                    tempObj = GetChildControl(tempObj as DependencyObject, controlName);
                    if (tempObj != null)
                        return tempObj;
                }
            }
            return null;
        }

        /// <summary>
        /// Change the ToolTip content based on the state of header check-box in data grid
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void headerCheckBox_MouseEnter(object sender, MouseEventArgs e)
        {
            CheckBox ch = (CheckBox)sender;
            if ((bool)ch.IsChecked)
            {
                ch.ToolTip = "DeSelectALL";
            }
            else
            {
                ch.ToolTip = "SelectALL";
            }
        }

        /// <summary>
        /// Open connect expander
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnConnectExpander_Click(object sender, RoutedEventArgs e)
        {
            expdrConnect.IsExpanded = true;
            expdrConnect.Focus();
            //Dock the settings/status panel if not docked, to display connect options
            if (pane1Button.Visibility == System.Windows.Visibility.Visible)
            {
                pane1Button_MouseEnter(sender, e);
                pane1Pin_Click(sender, e);
            }
            if (expdrConnect.IsExpanded)
            {
                if (btnConnect.Content.ToString().Equals("Connect"))
                {
                    btnConnectExpander.Content = btnConnect.Content.ToString() + "...";
                }
                else
                {
                    btnConnectExpander.Content = btnConnect.Content.ToString() + "...";
                }
            }
        }

        /// <summary>
        /// Calculate tags read rate per second 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void readRatePerSec_Tick(object sender, EventArgs e)
        {
            if (txtTotalTagReads.Content.ToString() != "")
            {
                //Divide Total tag count at every 1 sec instant per difference value of
                //current time and start async read time
                UpdateReadRate(CalculateElapsedTime());
            }
        }

        /// <summary>
        /// Calculate total elapsed time between present time and start read command
        /// initiated
        /// </summary>
        /// <returns>Returns total time elapsed </returns>
        private double CalculateElapsedTime()
        {
            TimeSpan elapsed = (DateTime.Now - startAsyncReadTime);
            // elapsed time + previous cached async read time
            double totalseconds = continuousreadElapsedTime + elapsed.TotalSeconds;
            totalTimeElapsedTextBox.Content = Math.Round(totalseconds, 2).ToString();
            return totalseconds;
        }

        /// <summary>
        /// Display read rate per sec
        /// </summary>
        /// <param name="totalElapsedSeconds"> total elapsed time</param>
        private void UpdateReadRate(double totalElapsedSeconds)
        {
            long temp = Convert.ToInt64(txtTotalTagReads.Content.ToString());
            txtbxReadRatePerSec.Content = (Math.Round((temp / totalElapsedSeconds), 2)).ToString();
        }

        /// <summary>
        /// Warns the user when trying to close the app, when async read is going on 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void Window_Closing(object sender, CancelEventArgs e)
        {
            // If URA is still reading, notify user
            if (isAsyncReadGoingOn || isSyncReadGoingOn)
            {
                string msg = "Universal Reader Assistant is reading the tags. Please stop the reading?";
                MessageBoxResult result =
                  MessageBox.Show(
                    msg,
                    "Universal Reader Assistant Message",
                    MessageBoxButton.OK,
                    MessageBoxImage.Warning);
                e.Cancel = true;
            }
            else if (!btnUpdate.IsEnabled)
            {
                string msg = "Universal Reader Assistant is upgrading the firmware on reader. Please wait";
                MessageBoxResult result =
                  MessageBox.Show(
                    msg,
                    "Universal Reader Assistant Message",
                    MessageBoxButton.OK,
                    MessageBoxImage.Warning);
                e.Cancel = true;
            }
            else
            {
                exitURA_Click(sender, new RoutedEventArgs());
                Environment.Exit(1);
            }
        }

        #endregion EventHandler

        #region SaveTagResults
        ///<summary>
        ///Save the datagrid data to text file
        ///</summary>
        ///<param name="sender"></param>
        ///<param name="e"></param>        
        private void saveData_Click(object sender, RoutedEventArgs e)
        {
            string strDestinationFile = string.Empty;
            try
            {
                if (null != tcTagResults.SelectedItem)
                {
                    SaveFileDialog saveFileDialog1 = new SaveFileDialog();
                    saveFileDialog1.Filter = "CSV Files (*.csv)|*.csv";
                    if (((TabItem)tcTagResults.SelectedItem).Header.Equals("Tag Results"))
                    {
                        strDestinationFile = "UniversalReader_tagResults" 
                            + DateTime.Now.ToString("yyyy-MM-dd-HH-mm-ss") + @".csv";
                        saveFileDialog1.FileName = strDestinationFile;
                        if ((bool)saveFileDialog1.ShowDialog())
                        {
                            strDestinationFile = saveFileDialog1.FileName;
                            TagReadRecord rda;
                            // True, if any row is selected and only selected row is saved else 
                            // false and entire data grid is saved
                            bool flagSelectiveDataSave = false;
                            for (int rowCount = 0; rowCount <= TagResults.dgTagResults.Items.Count - 1; rowCount++)
                            {
                                rda = (TagReadRecord)TagResults.dgTagResults.Items.GetItemAt(rowCount);
                                if (rda.Checked)
                                {
                                    flagSelectiveDataSave = true;
                                    break;
                                }
                            }
                            TextWriter tw = new StreamWriter(strDestinationFile);
                            StringBuilder sb = new StringBuilder();
                            //writing the header
                            int columnCount = TagResults.dgTagResults.Columns.Count;
                            
                            for (int count = 1; count < columnCount ; count++)
                            {
                                string colHeader = TagResults.dgTagResults.Columns[count].Header.ToString();
                                if ((colHeader == "EPC(ASCII)") || (colHeader == "EPC(ReverseBase36)"))
                                {
                                    //Adding column header based on selection of Display options section
                                    if ( -1 != colHeader.IndexOf(cbxDisplayEPCAs.Text))
                                    {
                                        sb.Append(colHeader + ", "); 
                                    }
                                }
                                else if (colHeader == "Data(ASCII)")// || (colHeader == "Data(ReverseBase36)"))
                                {
                                    //Adding column header based on selection of Display options section
                                    if (-1 != colHeader.IndexOf(cbxDisplayEmbRdDataAs.Text))
                                    {
                                        sb.Append(colHeader + ", ");
                                    }
                                }
                                else
                                {
                                    if (count == columnCount-1)
                                    {
                                        sb.Append(colHeader); 
                                    }
                                    else
                                    {
                                        
                                        sb.Append(colHeader + ", ");
                                    }
                                }
                            }
                            tw.WriteLine(sb.ToString());
                            if (flagSelectiveDataSave)
                            {
                                //writing the data
                                rda = null;
                                for (int rowCount = 0; rowCount <= TagResults.dgTagResults.Items.Count - 1; rowCount++)
                                {
                                    rda = (TagReadRecord)TagResults.dgTagResults.Items.GetItemAt(rowCount);
                                    if (rda.Checked)
                                    {
                                        textWrite(tw, rda, rowCount + 1);
                                    }
                                }
                            }
                            else
                            {
                                //writing the data
                                rda = null;
                                for (int rowCount = 0; rowCount <= TagResults.dgTagResults.Items.Count - 1; rowCount++)
                                {
                                    rda = (TagReadRecord)TagResults.dgTagResults.Items.GetItemAt(rowCount);
                                    textWrite(tw, rda, rowCount + 1);
                                }
                            }
                            tw.Close();
                        }
                    }
                    else if (((TabItem)tcTagResults.SelectedItem).Header.Equals("Tag Inspector"))
                    {
                        saveFileDialog1.Filter = "CSV Files (*.csv)|*.csv";
                        strDestinationFile = "UniversalReader_TagInspection_Results" 
                            + DateTime.Now.ToString("yyyy-MM-dd-HH-mm-ss") + @".csv";
                        saveFileDialog1.FileName = strDestinationFile;
                        if ((bool)saveFileDialog1.ShowDialog())
                        {
                            strDestinationFile = saveFileDialog1.FileName;
                            using (StreamWriter sw = File.CreateText(strDestinationFile))
                            {

                                //sw.WriteLine("Tag inspected on EPC is: " + TagInspector.txtEpc.Text);
                                //sw.WriteLine("Reserved Memory Bank(0)");
                                //sw.WriteLine("-----------------------");
                                sw.WriteLine("Reserved, " + TagInspector.txtKillPassword.Text 
                                    + TagInspector.txtKillPassword.Text);
                                //sw.WriteLine("Access password: " + ;
                                //sw.WriteLine("       ");
                                //sw.WriteLine("EPC Memory Bank(1)");
                                //sw.WriteLine("------------------");
                                //sw.WriteLine("CRC: " + TagInspector.txtCRC.Text);
                                //sw.WriteLine("PC: " + TagInspector.txtPC.Text);
                                sw.WriteLine("EPC, " + TagInspector.txtEPCData.Text);
                                //sw.WriteLine("Unused portion?: " + TagInspector.txtEPCUnused.Text);
                                //sw.WriteLine("       ");
                                //sw.WriteLine("TID Memory Bank(2)");
                                //sw.WriteLine("------------------");
                                //sw.WriteLine("Cls ID: " + TagInspector.txtClsID.Text);
                                //sw.WriteLine("Vendor ID: " + TagInspector.txtVendorID.Text);
                                //sw.WriteLine("Model ID: " + TagInspector.txtModelID.Text);
                                //sw.WriteLine("Unique ID: " + TagInspector.txtUniqueIDValue.Text);
                                sw.WriteLine("TID, " + TagInspector.txtClsID.Text + TagInspector.txtVendorID.Text
                                    + TagInspector.txtModelID.Text + TagInspector.txtUniqueIDValue.Text);
                                sw.WriteLine("Tag type: " + TagInspector.txtVendorValue.Text + " " 
                                    + TagInspector.txtModeldIDValue.Text);
                                //sw.WriteLine("       ");
                                //sw.WriteLine("User Memory Bank(3)");
                                //sw.WriteLine("------------------");
                                //sw.WriteLine("Value: " + TagInspector.txtUserMemData.Text);
                                sw.WriteLine("User, " + TagInspector.txtUserMemData.Text);
                                sw.Flush();
                                sw.Close();
                        }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        /// <summary>
        /// For readability sake in the text file.
        /// </summary>
        /// <param name="tw"></param>
        /// <param name="rda"></param>
        private void textWrite(TextWriter tw, TagReadRecord rda, int rowNumber)
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(rowNumber + ", ") ;
            sb.Append(rda.EPC + ", ");
            
            switch (cbxDisplayEPCAs.Text)
            {
                case "Select":
                case "Hex": break;
                case "ASCII": sb.Append(rda.EPCInASCII + ", "); break;
                case "ReverseBase36": sb.Append(rda.EPCInReverseBase36 + ", "); break;
            }
            sb.Append(rda.Data + ", ");

            switch (cbxDisplayEmbRdDataAs.Text)
            {
                case "Select":
                case "Hex": break;
                case "ASCII": sb.Append(rda.DataInASCII + ", "); break;
                //case "ReverseBase36": sb.Append(rda.DataInReverseBase36 + ","); break;
            }
            
            sb.Append(rda.TimeStamp.ToString("MM-dd-yyyy HH:mm:ss:fff") + ", ");
            
            sb.Append(rda.RSSI + ", ");
            
            sb.Append(rda.ReadCount + ", ");
            
            sb.Append(rda.Antenna + ", ");
            
            sb.Append(rda.Protocol + ", ");
            
            sb.Append(rda.Frequency + ", ");
            
            sb.Append(rda.Phase);

            tw.Write(sb.ToString());
            tw.WriteLine();
        }
        #endregion SaveTagResults

        #region Connect
        /// <summary>
        /// Connect button event handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnConnect_Click(object sender, RoutedEventArgs e)
        {

            if (btnConnect.Content.ToString() == "Connect")
            {
                if((bool)rdbtnNetworkConnection.IsChecked)
                {
                    if ("" == cmbFixedReaderAddr.Text)
                    {
                        MessageBox.Show("Please input valid reader URI", "Error");
                        return;
                    }
                }
                App.Current.MainWindow.InvalidateVisual();                
                ProtocolsGroupBox.InvalidateVisual();
                AntennasGroupBox.InvalidateVisual();
                btnConnect.InvalidateVisual();

                try
                {
                    Mouse.SetCursor(Cursors.AppStarting);
                    tagdb.Clear();
                    // Clear warning on status bar if any
                    ClearMessageOnStatusBar();
                    btnConnect_Click_Body(sender, e);
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Inner Exception :"+ex.InnerException.ToString()+"Exception : "
                        +ex.Message+"Source :"+ex.Source, "ReaderException", MessageBoxButton.OK, MessageBoxImage.Error);
                }
                finally
                {
                    btnConnect.ToolTip = "Disconnect";
                    tiTagResults.Focus();
                    btnClearTagReads.IsEnabled = true;
                    saveData.IsEnabled = true;
                }
            }
            else
            {
                try
                {
                    if (btnRead.Content.ToString() == "Stop Reading")
                    {
                        startRead_Click(sender, e);
                    }
                    regionToSet = Reader.Region.UNSPEC;
                    lblReaderUri.Content = "Reader URI";
                    btnConnectExpander.ToolTip = "Connect";
                    regioncombo.ItemsSource = null;
                    btnConnect.IsEnabled = true;
                    chkDataExtensions.IsChecked = false;
                    if (!(objReader is SerialReader))
                    {
                        cbxBaudRate.IsEnabled = false;
                    }

                    btnRead.IsEnabled = false;

                    cmbReaderAddr.IsEnabled = true;
                    cmbFixedReaderAddr.IsEnabled = true;

                    if (null != objReader)
                    {
                        objReader.Destroy();
                        objReader = null;
                    }
                }
                catch (IOException ex)
                {
                    MessageBox.Show(ex.Message, "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                    Onlog(ex.Message + DateTime.Now.ToString());
                    Onlog(ex.StackTrace);
                }
                finally
                {
                    // Stop timer to render data on the grid
                    dispatchtimer.Stop();
                    readRatePerSec.Stop();
                    btnConnect.ToolTip = "Connect";
                    btnConnect.Content = "Connect";
                    ConfigureAntennaBoxes(null);
                    ConfigureProtocols(null);

                    // close the transport listener log re-enable the menu option but uncheck it                
                    chkEnableTransportLogging.IsChecked = false;
                    chkEnableTransportLogging.IsEnabled = true;
                    
                    // Enable Probe read names after disconnect
                    probeReadNames.IsEnabled = true;
                    
                    // Disable  read-once, readasyncread buttons                    
                    btnRead.IsEnabled = false;
                    btnRead.Visibility = System.Windows.Visibility.Hidden;
                    btnRefreshReadersList.IsEnabled = true;
                    //startRead.IsEnabled = false;
                    
                    // Enable fast search
                    chkEnableFastSearch.IsEnabled = true;
                    lblshowStatus.Content = "Disconnected";
                    InitializeRdrDiagnostics();
                    
                    // Enable firmware Update panel
                    stackPanelFirmwareUpdate.IsEnabled = true;

                    imgReaderStatus.Source = new BitmapImage(new Uri(@"..\Icons\LedRed.png",
                        UriKind.RelativeOrAbsolute));

                    rdbtnLocalConnection.IsEnabled = true;
                    rdbtnNetworkConnection.IsEnabled = true;
                    grpbxPerformanceTuning.IsEnabled = false;
                    chkCustomizeGen2Settings.IsChecked = false;
                    ReadDataGroupBox.IsEnabled = false;
                    gridPerformanceTuning.IsEnabled = false;
                    gridReadOptions.IsEnabled = false;
                    gridDataExtensions.IsEnabled = false;
                    regioncombo.IsEnabled = false;
                    gridDisplayOptions.IsEnabled = false;
                    btnConnectExpander.Content = btnConnect.Content+"...";

                    // Disable all the tabs except tag results tab
                    tiWriteEPC.IsEnabled = false;
                    tiTagInspector.IsEnabled = false;
                    tiUserMemory.IsEnabled = false;
                    tiLockTag.IsEnabled = false;
                    tiTagResults.Focus();
                    
                    // Reset protocols
                    iso6bCheckBox.IsChecked = false;
                    ipx256CheckBox.IsChecked = false;
                    ipx64CheckBox.IsChecked = false;
                    ataCheckBox.IsChecked = false;
                    
                    // Reset embedded read settings
                    chkEmbeddedReadData.IsChecked = false;
                    cbxReadDataBank.SelectedIndex = 0;
                    txtembReadStartAddr.Text = "0";
                    txtembReadLength.Text = "2";
                    chkUniqueByData.IsChecked = false;

                    // Reset filter settings
                    chkApplyFilter.IsChecked = false;
                    cbxFilterMemBank.SelectedIndex = 0;
                    txtFilterStartAddr.Text = "32";
                    txtFilterData.Text = "0";
                    chkFilterInvert.IsChecked = false;

                    // Re-initialized the read power slider control value to 0,
                    // when Disconnect Button on URA is pressed (Note : Only 
                    // re-initializing the slider control value and this doesn't
                    // set zero read power on the module while disconnecting)
                    sldrReadPwr.Value = 0;

                    // Release bonjour subscribed events
                    if (null != eventManager)
                    {
                        eventManager.ServiceFound -= new _IDNSSDEvents_ServiceFoundEventHandler(this.ServiceFound);
                        eventManager.ServiceResolved -= new _IDNSSDEvents_ServiceResolvedEventHandler(this.ServiceResolved);
                    }

                    if (null != transportLogFile)
                    {
                        transportLogFile.Close();
                    }

                    // Disable all the tabs except tag results tab
                    WriteEpc.spWriteEPC.IsEnabled = false;
                    TagInspector.spTagInspector.IsEnabled = false;
                    UserMemory.spUserMemory.IsEnabled = false;
                    LockTag.spLockTag.IsEnabled = false;

                    isAsyncReadGoingOn = false;
                    if (OptimalReaderSettings != null)
                    {
                        OptimalReaderSettings.Clear();
                    }

                    CustomizedMessageBox = null;

                    isLoadSavedConfigurations = false;

                    // Disable save configuration button
                    btnSaveConfig.IsEnabled = false;

                    // Enable load configuration button
                    btnLoadConfig.IsEnabled = true;

                    chkEnableFastSearch.IsChecked = false;
                    Mouse.SetCursor(Cursors.Arrow);

                    // Reset connection lost exception counter
                    connectionLostCount = 0;
                    // print this line in TMR log file for mentioning closing the port
                    log.WriteLine("*****************************************************************");
                    log.Flush();
                }
            }
        }

        /// <summary>
        /// Initializes the reader connected to the reader address field
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnConnect_Click_Body(object sender, EventArgs e)
        {
            try
            {
                //Starts the reader from default state                
                objReader.Destroy();
                ConfigureAntennaBoxes(null);
                ConfigureProtocols(null);
            }
            catch { }
            try
            {
                if ((bool)rdbtnLocalConnection.IsChecked)
                {
                    if (!ValidatePortNumber(cmbReaderAddr.Text))
                    {
                        throw new IOException();
                    }
                    if (cmbReaderAddr.Text == "")
                    {
                        throw new IOException();
                    }
                    // Creates a Reader Object for operations on the Reader.
                    string readerUri = cmbReaderAddr.Text;
                    //Regular Expression to get the com port number from comport name .
                    //for Ex: If The Comport name is "USB Serial Port (COM19)" by using this 
                    // regular expression will get com port number as "COM19".
                    MatchCollection mc = Regex.Matches(readerUri, @"(?<=\().+?(?=\))");
                    foreach (Match m in mc)
                    {
                        readerUri = m.ToString();
                    }
                    objReader = Reader.Create(string.Concat("tmr:///", readerUri));
                    uri = readerUri;
                    // Create TMR log Header
                    tmrLogHeader(uri);
                }
                else
                {
                    //Creates a Reader Object for operations on the Reader.
                    objReader = Reader.Create(string.Concat("tmr://", cmbFixedReaderAddr.Text));
                    uri = cmbFixedReaderAddr.Text;
                    // Create TMR log Header
                    tmrLogHeader(uri);
                }

                ///Assign  reader to reader
                //reader = reader; //(Reader)reader;

                // If Option selected add the serial-reader-specific message logger
                // before connecting, so we can see the initialization.
                if ((bool)chkEnableTransportLogging.IsChecked)
                {
                    SaveFileDialog saveFileDialog1 = new SaveFileDialog();
                    saveFileDialog1.Filter = "Text Files (.txt)|*.txt";
                    saveFileDialog1.Title = "Select a File to save transport layer logging";
                    string strDestinationFile = "UniversalReader_transportLog" 
                        + DateTime.Now.ToString("yyyy-MM-dd-HH-mm-ss") + @".txt";
                    saveFileDialog1.FileName = strDestinationFile;
                    // Show the Dialog.
                    // If the user clicked OK in the dialog and
                    // a .txt file was selected, open it.
                    if (saveFileDialog1.ShowDialog() == true)
                    {
                        StreamWriter writer = new StreamWriter(saveFileDialog1.FileName);
                        writer.AutoFlush = true;
                        transportLogFile = writer;
                        if (objReader is SerialReader)
                            objReader.Transport += SerialListener;
                        if (objReader is RqlReader)
                            objReader.Transport += RQLListener;
                        if (objReader is LlrpReader)
                            objReader.Transport += LLrpListener;
                    }
                    else
                    {
                        chkEnableTransportLogging.IsChecked = false;
                    }
                }
                chkEnableTransportLogging.IsEnabled = false;
                if (objReader is SerialReader)
                {
                    // Set the selected baud rate, so that api try's connecting to the 
                    // module with the selected baud rate first
                    SetBaudRate();
                }
                //Show the status
                lblshowStatus.Content = "Connecting..";                
                Mouse.SetCursor(Cursors.AppStarting);
                objReader.Connect();
                // Enable all the expander controls
                EnableDisableExpanderControl(true);
                // Enable save config button
                btnSaveConfig.IsEnabled = true;
                //
                TagResults.dgTagResults.CancelEdit();
                TagResults.dgTagResults.ItemsSource = null;
                TagResults.dgTagResults.ItemsSource = tagdb.TagList;
   
                //Enable all the disabled tabs
                tiWriteEPC.IsEnabled = true;
                tcTagResults.IsEnabled = true;
                tiTagInspector.IsEnabled = true;
                tiUserMemory.IsEnabled = true;
                tiLockTag.IsEnabled = true;
                
                GUIturnoffWarning();
                
                gridReadOptions.IsEnabled = true;
                regioncombo.IsEnabled = true;
                gridDisplayOptions.IsEnabled = true;
                gridDataExtensions.IsEnabled = true;

                //Clear tag counts and database
                btnClearReads_Click(sender, (RoutedEventArgs)e);

                //Get connect reader model
                model = objReader.ParamGet("/reader/version/model").ToString();

                //Set the embedded read data length for connected reader
                if (model.Equals("M5e") || model.Equals("M5e EU") || model.Equals("M5e Compact"))
                {
                    txtembReadLength.Text = "2";
                }
                else
                {
                    txtembReadLength.Text = "0";
                }

                //Show the status
                lblshowStatus.Content = "Connected";

                imgReaderStatus.Source = new BitmapImage(new Uri(@"..\Icons\LedOrange.png", UriKind.RelativeOrAbsolute));
                if ((bool)rdbtnLocalConnection.IsChecked)
                {
                    lblReaderUri.Content = uri;
                }
                else
                {
                    lblReaderUri.Content = cmbFixedReaderAddr.Text;
                }
                Mouse.SetCursor(Cursors.Arrow);
                btnConnectExpander.ToolTip = "Disconnect";
                grpbxPerformanceTuning.IsEnabled = true;
                grpbxRdrPwrSettngs.IsEnabled = true;
                ReadDataGroupBox.IsEnabled = true ;

                //Disable connected device / network reader radio button based on reader object
                if (objReader is SerialReader)
                {
                    rdbtnNetworkConnection.IsEnabled = false;
                }
                else
                {
                    rdbtnLocalConnection.IsEnabled = false;
                }

                //Disable the refresh button
                btnRefreshReadersList.IsEnabled = false;

                //Enable launch web api
                if (!(objReader is SerialReader))
                    btnWebUI.Visibility = System.Windows.Visibility.Visible;

                btnRead.Visibility = System.Windows.Visibility.Visible;

                //READER URI drop down should be disabled after URA is connected to a reader
                cmbReaderAddr.IsEnabled = false;
                cmbFixedReaderAddr.IsEnabled = false;

                //Disable Probe read names after connect
                probeReadNames.IsEnabled = false;

                //readerStatus.IsEnabled = true;
                regionToSet = (Reader.Region)objReader.ParamGet("/reader/region/id");
                if (objReader is SerialReader)
                {
                    if (regionToSet == Reader.Region.UNSPEC)
                    {
                        if (model.Equals("M6e PRC"))
                        {
                            regionToSet = Reader.Region.PRC;
                        }
                        else
                        {
                            regionToSet = Reader.Region.NA;
                        }
                    }

                }
                Reader.Region[] regions;
                if (objReader is LlrpReader || objReader is RqlReader)
                {
                    regions = new Reader.Region[] { regionToSet };
                }
                else
                {
                    regions = (Reader.Region[])objReader.ParamGet("/reader/region/supportedRegions");
                }
                regioncombo.ItemsSource = regions;

                //set the region on module
                regioncombo.SelectedItem = regioncombo.Items.GetItemAt(regioncombo.Items.IndexOf(regionToSet));


                //Initialize max and min read power for read power slider
                InitializeRdPwrSldrMaxNMinValue();

                //Initialize settings received o
                OptimalReaderSettings = null;
                InitializeOptimalSettings();

                objReader.ParamSet("/reader/transportTimeout", int.Parse(txtRFOnTimeout.Text) + 5000);
                //try setting a unique ID
                //reader.ParamSet("/reader/hostname", "this reader");
                if (objReader is SerialReader)
                {
                    cbxBaudRate.IsEnabled = true;
                }

                // Load Gen2 Settings 
                initialReaderSettingsLoaded = false;
                LoadGen2Settings();
                initialReaderSettingsLoaded = true;

                //Enable fast search on for mercury6, astra-ex, m6e. 
                if (model.Equals("Astra") || model.Equals("M5e") 
                    || model.Equals("M5e Compact") || model.Equals("M5e EU") 
                    || model.Equals("M4e"))
                {
                    chkEnableFastSearch.IsEnabled = false;
                }

                // Disable performance tuning and embedded read data options for model astra.
                if (model.Equals("Astra"))
                {
                    // // Disable performance tuning options
                    //// grpbxPerformanceTuning.IsEnabled = false;
                    // rdbtnAutoAdjstAsPoplChngs.IsEnabled = false;
                    // rdbtnOptmzExtmdNoTagsInField.IsEnabled = false;
                    // stkpnlOptmzExtmdNoTagsInField.IsEnabled = false;
                    // stkpnlRdDistVsrdRate.IsEnabled = false;
                    // rdBtnSlBstChforPoplSize.IsEnabled = false;
                    // rdBtnTagsRespondOption.IsEnabled = true;
                    // rdBtnTagsRespondOption.IsChecked = true;
                    // // Disable embedded read data options
                    // ReadDataGroupBox.IsEnabled = false;
                    // chkEmbeddedReadData.IsChecked = false;
                    DisableControlsForAstra();
                }
                else
                {
                    // Disable performance tuning options
                    // grpbxPerformanceTuning.IsEnabled = false;
                    rdbtnAutoAdjstAsPoplChngs.IsEnabled = true;
                    rdbtnOptmzExtmdNoTagsInField.IsEnabled = true;
                    stkpnlOptmzExtmdNoTagsInField.IsEnabled = true;
                    stkpnlRdDistVsrdRate.IsEnabled = true;
                    rdBtnSlBstChforPoplSize.IsEnabled = true;
                    rdBtnTagsRespondOption.IsEnabled = true;
                    rdBtnTagsRespondOption.IsChecked = true;
                    // Disable embedded read data options
                    ReadDataGroupBox.IsEnabled = true;
                }
                //thingMagicReader.IsEnabled = true;
                //startRead.IsEnabled = true;
                btnRead.IsEnabled = true;
                //advanceReaderSettings.IsEnabled = true;
                if (objReader is SerialReader)
                {
                    initializeBaudRate();
                    if (!(model.Equals("M6e") || model.Equals("M6e Micro") 
                        || model.Equals("M6e PRC")))
                    {
                        objReader.ParamSet("/reader/tagReadData/reportRssiInDbm", true);
                    }
                }
                ConfigureAntennaBoxes(objReader);
                supportedProtocols = (TagProtocol[])objReader.ParamGet("/reader/version/supportedProtocols");
                ConfigureProtocols(supportedProtocols);
                btnConnect.ToolTip = "Disconnect";
                btnConnect.Content = "Disconnect";
                btnConnectExpander.Content = btnConnect.Content+"...";

                //Enable save data, btnClearTagReads, read-once, readasyncread buttons
                saveData.IsEnabled = true;
                btnClearTagReads.IsEnabled = true;
                btnRead.IsEnabled = true;

                //startRead.IsEnabled = true;
                InitializeRdrDiagnostics();
                Mouse.SetCursor(Cursors.Arrow);
                CustomizedMessageBox = new URACustomMessageBoxWindow();

                // Clear firmware Update open file dialog status
                lblMessage.Text = "";
                txtFirmwarePath.Text = "";
            }
            catch (Exception ex)
            {
                string error = "Connect failed to saved reader URI [" + uri + "]. Please make sure reader "
                    +" is connected and try again. Or, connect to desired reader first then load any saved "
                    +"configuration to it";
                Mouse.SetCursor(Cursors.Arrow);
                btnConnect.IsEnabled = true;
                initialReaderSettingsLoaded = true;
                lblshowStatus.Content = "Disconnected";
                chkEnableTransportLogging.IsEnabled = true;
                gridReadOptions.IsEnabled = false;
                regioncombo.IsEnabled = false;
                gridDisplayOptions.IsEnabled = false;
                if (ex is IOException)
                {
                    if (!cmbReaderAddr.Text.Contains("COM"))
                    {
                        MessageBox.Show("Application needs a valid Reader Address of type COMx",
                            "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                    }
                    else
                    {
                        if (isLoadSavedConfigurations)
                        {
                            MessageBox.Show("Connect failed to saved reader URI [" +cmbReaderAddr.Text + "]. "
                                +"Please make sure reader is connected and try again. Or, connect to desired reader first then "
                                +"load any saved configuration to it", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                        }
                        else
                        {
                            MessageBox.Show("Reader not connected on " + cmbReaderAddr.Text, "Error",
                                MessageBoxButton.OK, MessageBoxImage.Error);
                        }
                    }
                    cmbReaderAddr.IsEnabled = true;
                    cmbFixedReaderAddr.IsEnabled = true;
                }
                else if (ex is ReaderException)
                {
                    if (-1 != ex.Message.IndexOf("target machine actively refused"))
                    {
                        if (isLoadSavedConfigurations)
                        {
                            MessageBox.Show("Error connecting to reader: " + error, "Reader Error", 
                                MessageBoxButton.OK, MessageBoxImage.Error);
                        }
                        else
                        {
                            MessageBox.Show("Error connecting to reader: " + "Connection attempt failed...",
                                "Reader Error", MessageBoxButton.OK, MessageBoxImage.Error);
                        }
                    }
                    else
                    {
                        if (isLoadSavedConfigurations)
                        {
                            MessageBox.Show("Error connecting to reader: " + ex.Message +" "+error,
                                "Reader Error", MessageBoxButton.OK, MessageBoxImage.Error);
                        }
                        else
                        {
                            MessageBox.Show("Error connecting to reader: " + ex.Message, "Reader Error",
                                MessageBoxButton.OK, MessageBoxImage.Error);
                        }
                    }
                }
                else if (ex is UnauthorizedAccessException)
                {
                    MessageBox.Show("Access to " + cmbReaderAddr.Text + " denied. Please check if another "
                        +"program is accessing this port", "Error!", MessageBoxButton.OK,
                        MessageBoxImage.Error);
                }
                else
                {
                    if (-1 != ex.Message.IndexOf("target machine actively refused"))
                    {
                        MessageBox.Show("Error connecting to reader: " + "Connection attempt failed...",
                            "Reader Error", MessageBoxButton.OK, MessageBoxImage.Error);
                    }
                    else
                    {
                        MessageBox.Show("Error connecting to reader: " + ex.Message, "Reader Error",
                            MessageBoxButton.OK, MessageBoxImage.Error);
                    }
                }
            }
        }

        /// <summary>
        /// Disable some of the UI controls when the connected reader is astra
        /// </summary>
        private void DisableControlsForAstra()
        {
            rdbtnAutoAdjstAsPoplChngs.IsEnabled = false;
            rdbtnOptmzExtmdNoTagsInField.IsEnabled = false;
            stkpnlOptmzExtmdNoTagsInField.IsEnabled = false;
            stkpnlRdDistVsrdRate.IsEnabled = false;
            rdBtnSlBstChforPoplSize.IsEnabled = false;
            rdBtnTagsRespondOption.IsEnabled = true;
            rdBtnTagsRespondOption.IsChecked = true;
            // Disable embedded read data options
            ReadDataGroupBox.IsEnabled = false;
            chkEmbeddedReadData.IsChecked = false;
        }

        /// <summary>
        /// Check for valid port numbers
        /// </summary>
        /// <param name="portNumber"></param>
        /// <returns></returns>
        private bool ValidatePortNumber(string portNumber)
        {
            List<string> portNames = new List<string>();
            List<string> portValues = new List<string>();
             //converting comport number from small letter to capital letter.Eg:com18 to COM18.
            portNumber =Regex.Replace(portNumber, @"[^a-zA-Z0-9_\\]", "").ToUpperInvariant(); 
            // getting the list of comports value and name which device manager shows
            using (var searcher = new ManagementObjectSearcher("root\\CIMV2", "SELECT * FROM Win32_PnPEntity WHERE ConfigManagerErrorCode = 0"))
            {
                foreach (ManagementObject queryObj in searcher.Get())
                {
                    if (queryObj["Name"].ToString().Contains("(COM"))
                        portNames.Add(queryObj["Name"].ToString());
                }
                for (int i = 0; i < portNames.Count; i++)
                {
                    MatchCollection mc = Regex.Matches(portNames[i], @"(?<=\().+?(?=\))");
                    foreach (Match m in mc)
                    {
                        portValues.Add(m.ToString());
                    }
                }
                if (portNames.Contains(cmbReaderAddr.Text))
                {
                    //Specified port number exist
                    return true;
                }
                else if (portValues.Contains(portNumber))
                {
                    return true;
                }
                else
                {
                    //Specified port number doesn't exist
                    return false;
                }
            }
        }

        /// <summary>
        /// Serial transport listener
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
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

        /// <summary>
        /// Rql transport listener
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void RQLListener(Object sender, TransportListenerEventArgs e)
        {
            transportLogFile.Write(String.Format("{0} {1}",
                DateTime.Now.ToString("hh:mm:ss.fff tt"),
                e.Tx ? "Sending" : "Received"));

            // Create an ASCII encoding.
            Encoding ascii = Encoding.ASCII;
            String decodedString = "  " + ascii.GetString(e.Data);
            transportLogFile.Write(decodedString);
            transportLogFile.WriteLine();
            transportLogFile.Flush();
        }

        /// <summary>
        /// Llrp transport listener
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void LLrpListener(Object sender, TransportListenerEventArgs e)
        {
            transportLogFile.Write(String.Format("{0} {1}",
                DateTime.Now.ToString("MM/dd/yyyy hh:mm:ss.fff tt"), e.Tx ? "Sending" : "Received"));

            // Create an ASCII encoding.
            Encoding ascii = Encoding.ASCII;
            String decodedString = "  " + ascii.GetString(e.Data);
            transportLogFile.Write(decodedString);
            transportLogFile.WriteLine();
            transportLogFile.Flush();
        }

        /// <summary>
        /// Select the reader set baud rate for the baud-rate combo-box
        /// </summary>
        private void initializeBaudRate()
        {
            int setBaudRate = 0;
            int cbxbaudrateindex = 0;
            setBaudRate = (int)this.objReader.ParamGet("/reader/baudRate");
            switch (setBaudRate)
            {
                case 9600:
                    cbxbaudrateindex = 1;
                    break;
                case 19200:
                    cbxbaudrateindex = 2;
                    break;
                case 38400:
                    cbxbaudrateindex = 3;
                    break;
                case 115200:
                    cbxbaudrateindex = 4;
                    break;
                case 230400:
                    cbxbaudrateindex = 5;
                    break;
                case 460800:
                    cbxbaudrateindex = 6;
                    break;
                case 921600:
                    cbxbaudrateindex = 7;
                    break;
                default:
                    cbxbaudrateindex = 0;
                    break;
            }
            cbxBaudRate.SelectedItem = cbxBaudRate.Items.GetItemAt(cbxbaudrateindex);
        }

        /// <summary>
        /// Fixed reader connection initialization
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void rdbtnNetworkConnection_Checked(object sender, RoutedEventArgs e)
        {
            try
            {
                if (null != lblbaudRate)
                {
                    lblbaudRate.Visibility = System.Windows.Visibility.Collapsed;
                    cbxBaudRate.Visibility = System.Windows.Visibility.Collapsed;
                    cmbReaderAddr.ItemsSource = "";
                    if (isBonjourServicesInstalled)
                    {
                        btnRefreshReadersList.Visibility = System.Windows.Visibility.Visible;
                    }
                    else
                    {
                        btnRefreshReadersList.Visibility = System.Windows.Visibility.Collapsed;
                    }
                    ToolTip networkRdrToolTip = new ToolTip();
                    networkRdrToolTip.Content = "Enter reader IPaddress/Hostname";
                    cmbReaderAddr.ToolTip = networkRdrToolTip;
                    btnRefreshReadersList.ToolTip = "Refresh host name list";
                    InitializeReaderUriBox();
                    cmbReaderAddr.Visibility = System.Windows.Visibility.Hidden;
                    cmbFixedReaderAddr.Visibility = System.Windows.Visibility.Visible;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        /// <summary>
        /// Serial reader connection initialization
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void rdbtnLocalConnection_Checked(object sender, RoutedEventArgs e)
        {
            try
            {
                if (lblbaudRate != null)
                {
                    cmbFixedReaderAddr.Visibility = System.Windows.Visibility.Hidden;
                    cmbReaderAddr.Visibility = System.Windows.Visibility.Visible;
                    lblbaudRate.Visibility = System.Windows.Visibility.Visible;
                    cbxBaudRate.Visibility = System.Windows.Visibility.Visible;
                    ToolTip serialRdrToolTip = new ToolTip();
                    serialRdrToolTip.Content = "Enter COM port or press Refresh to refresh comport list";
                    cmbReaderAddr.ToolTip = serialRdrToolTip;
                    btnRefreshReadersList.ToolTip = "Refresh Comport list";
                    InitializeReaderUriBox();
                    btnWebUI.Visibility = System.Windows.Visibility.Collapsed;
                    btnRefreshReadersList.Visibility = System.Windows.Visibility.Visible;
                    cbxBaudRate.IsEnabled = true;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        /// <summary>
        /// Set selected region to the connected reader
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void regioncombo_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (regioncombo.Items.Count > 0)
            {
                if (regioncombo.SelectedItem != null)
                {
                    if ("" != regioncombo.SelectedItem.ToString())
                    {
                        if (objReader is SerialReader)
                        {
                            objReader.ParamSet("/reader/region/id", 
                                Enum.Parse(typeof(Reader.Region), regioncombo.SelectedItem.ToString()));
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Set the baud rate selected to the serial port 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cbxBaudRate_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (null != objReader)
            {
                if (cbxBaudRate.Items.Count > 0)
                {
                    if (cbxBaudRate.SelectedItem != null)
                    {
                        if (objReader is SerialReader)
                        {
                            if (((ComboBoxItem)cbxBaudRate.SelectedItem).Content.ToString() != "Select")
                            {
                                objReader.ParamSet("/reader/baudRate", 
                                    Convert.ToInt32(((ComboBoxItem)cbxBaudRate.SelectedItem).Content.ToString()));
                            }
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Set selected region to the connected reader
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void regioncombo_SelectionChanged_1(object sender, SelectionChangedEventArgs e)
        {
            if (regioncombo.Items.Count > 0)
            {
                if (regioncombo.SelectedItem != null)
                {
                    if ("" != regioncombo.SelectedItem.ToString())
                    {
                        if (objReader is SerialReader)
                        {
                            objReader.ParamSet("/reader/region/id", 
                                Enum.Parse(typeof(Reader.Region), regioncombo.SelectedItem.ToString()));

                            // Reinitialize max and min read power for read power slider, when 
                            // region is set on the reader
                            InitializeRdPwrSldrMaxNMinValue();
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Set the baud rate selected to the serial port 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cbxBaudRate_SelectionChanged_1(object sender, SelectionChangedEventArgs e)
        {
            if (null != objReader)
            {
                SetBaudRate();
            }
        }

        /// <summary>
        /// Set the selected baud rate
        /// </summary>
        private void SetBaudRate()
        {
            if (cbxBaudRate.Items.Count > 0)
            {
                if (cbxBaudRate.SelectedItem != null)
                {
                    if (objReader is SerialReader)
                    {
                        if (((ComboBoxItem)cbxBaudRate.SelectedItem).Content.ToString() != "Select")
                        {
                            objReader.ParamSet("/reader/baudRate",
                                Convert.ToInt32(((ComboBoxItem)cbxBaudRate.SelectedItem).Content.ToString()));
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Open the web page of the connected reader if fixed reader
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnWebUI_Click(object sender, RoutedEventArgs e)
        {
            if (cmbFixedReaderAddr.Text != "")
            {
                System.Diagnostics.Process.Start("http://" + cmbFixedReaderAddr.Text);
            }
        }

        #endregion Connect

        #region ReadOptions

        /// <summary>
        /// Based on the protocol being picked to perform a select operation accordingly.
        /// For example if Gen2, then it should change select screen to word address. on
        /// the other hand if it is ISO18-6b, then it be byte address.
        /// </summary>
        private void OnProtocolSelect()
        {
            try
            {
                if ((bool)iso6bCheckBox.IsChecked)
                {
                    iso6bCheckBoxChecked = true;
                }
                else if (((bool)gen2CheckBox.IsChecked) 
                    || ((bool)ipx64CheckBox.IsChecked) 
                    || ((bool)ipx256CheckBox.IsChecked)
                    ||((bool)ataCheckBox.IsChecked))
                {
                    iso6bCheckBoxChecked = false;
                }
            }
            catch { }
        }

        /// <summary>
        /// Initialize read plans to perform read
        /// </summary>
        private void SetReadPlans()
        {
            try
            {
                List<int> ant = GetSelectedAntennaList();
                bool isFastSearchEnabled = (bool)chkEnableFastSearch.IsChecked;


                //Setup embedded tagOp settings if option checked
                TagOp embTagOp = null;
                Gen2.Bank opMemBank = Gen2.Bank.TID;
                if ((bool)chkEmbeddedReadData.IsChecked)
                {
                    //automatically enable the Data column if embedded tagOp is enabled
                    TagResults.dataColumn.Visibility = System.Windows.Visibility.Visible;
                    switch (cbxReadDataBank.Text)
                    {
                        case "Reserved":
                            opMemBank = Gen2.Bank.RESERVED;
                            break;
                        case "EPC":
                            opMemBank = Gen2.Bank.EPC;
                            break;
                        case "TID":
                            opMemBank = Gen2.Bank.TID;
                            break;
                        case "User":
                            opMemBank = Gen2.Bank.USER;
                            break;
                    };

                    embTagOp = new Gen2.ReadData(opMemBank, Convert.ToUInt32(Utilities.CheckHexOrDecimal(
                        txtembReadStartAddr.Text)), Convert.ToByte(Utilities.CheckHexOrDecimal(txtembReadLength.Text)));
                    objReader.ParamSet("/reader/tagreaddata/uniquebydata", (bool)chkUniqueByData.IsChecked);
                }
                else
                {
                        TagResults.dataColumn.Visibility = System.Windows.Visibility.Collapsed;
                }
                //END Setup embedded tagOp settings if option checked

                //Setup Select Filter settings if option checked
                Gen2.Select searchSelect = null;
                Gen2.Bank selectMemBank = Gen2.Bank.EPC;
                if ((bool)chkApplyFilter.IsChecked)
                {
                   
                    switch (cbxFilterMemBank.Text)
                    {
                        case "EPC":
                            selectMemBank = Gen2.Bank.EPC;
                            break;
                        case "TID":
                            selectMemBank = Gen2.Bank.TID;
                            break;
                        case "User":
                            selectMemBank = Gen2.Bank.USER;
                            break;
                    };

                    int discard;
                    byte[] SearchSelectData = Utilities.GetBytes(
                        Utilities.RemoveHexstringPrefix(txtFilterData.Text), out discard);
                    // Enter inside if condition, only if filter length is in odd nibbles(hex characters)
                    if (discard == 1 && SearchSelectData.Length == 0)
                    {
                        // If only one hex character(one nibble) is specified as a filter.
                        // For ex: 0xa, convert into byte array
                        byte objByte = (byte)(Utilities.HexToByte(
                            Utilities.RemoveHexstringPrefix(txtFilterData.Text).TrimEnd()) << 4);
                        SearchSelectData = new byte[] { objByte };
                    }
                    else if (discard == 1)
                    {
                        // If filter length is in odd nibbles. For ex: 0xabc , 0xabcde, 0xabcdefg
                        // after converting to byte array we get 0xab, 0xc0 if the specified filter is 0xabc
                        //Adding omitted character to byte array
                        Array.Resize(ref SearchSelectData, SearchSelectData.Length + 1);
                        byte objByte = (byte)(((Utilities.HexToByte(
                            Utilities.RemoveHexstringPrefix(txtFilterData.Text).Substring(
                            Utilities.RemoveHexstringPrefix(
                            txtFilterData.Text).Length - 1).TrimEnd())) << 4));
                        Array.Copy(new object[] {objByte }, 0, SearchSelectData, SearchSelectData.Length - 1, 1);
                    }

                    UInt16 dataLength;
                    if (txtFilterData.Text != "")
                        dataLength = Convert.ToUInt16(
                            Utilities.RemoveHexstringPrefix(txtFilterData.Text).Length * 4);//calculate the length in the form of nibbles
                    else
                        dataLength = 0;
                    
                    searchSelect = new Gen2.Select((bool)chkFilterInvert.IsChecked, selectMemBank,
                        Convert.ToUInt32(Utilities.CheckHexOrDecimal(txtFilterStartAddr.Text)),
                        dataLength, SearchSelectData);
                }
                else
                {
                    searchSelect = null;
                }
                //END Setup Select Filter settings if option checked



                if (0 == ant.Count)
                {
                    throw new Exception("Please select at least one antenna");
                }

                if ((bool)gen2CheckBox.IsChecked)
                {
                    if ((bool)rdBtnEqualSwitching.IsChecked)
                    {
                        SimpleReadPlan srpGen2 = null;
                        foreach (int a in ant)
                        {
                            srpGen2 = new SimpleReadPlan(new int[] { a }, 
                                TagProtocol.GEN2, searchSelect, embTagOp, isFastSearchEnabled, 100);
                            simpleReadPlans.Add(srpGen2);
                        }
                    }
                    else
                    {
                        SimpleReadPlan srpGen2 = new SimpleReadPlan(ant.ToArray(),
                            TagProtocol.GEN2, searchSelect, embTagOp, isFastSearchEnabled);
                        simpleReadPlans.Add(srpGen2);
                    }
                }
                if ((bool)iso6bCheckBox.IsChecked)
                {
                    SimpleReadPlan srpIso = new SimpleReadPlan(ant.ToArray(), 
                        TagProtocol.ISO180006B, null, null, isFastSearchEnabled);
                    simpleReadPlans.Add(srpIso);
                }
                if ((bool)ipx64CheckBox.IsChecked)
                {
                    SimpleReadPlan srpIpx64 = new SimpleReadPlan(ant.ToArray(),
                        TagProtocol.IPX64, null, null, isFastSearchEnabled);
                    simpleReadPlans.Add(srpIpx64);
                }
                if ((bool)ipx256CheckBox.IsChecked)
                {
                    SimpleReadPlan srpIpx256 = new SimpleReadPlan(ant.ToArray(), 
                        TagProtocol.IPX256, null, null, isFastSearchEnabled);
                    simpleReadPlans.Add(srpIpx256);
                }
                if ((bool)ataCheckBox.IsChecked)
                {
                    SimpleReadPlan srpAta = new SimpleReadPlan(ant.ToArray(),
                        TagProtocol.ATA, null, null, isFastSearchEnabled);
                    simpleReadPlans.Add(srpAta);
                }
                if (simpleReadPlans.ToArray().Length == 0)
                {
                    objReader.ParamSet("/reader/read/plan", new SimpleReadPlan(ant.ToArray(), 
                        TagProtocol.GEN2, searchSelect, embTagOp, isFastSearchEnabled));
                }
                else
                {
                    ReadPlan plan;
                    if (simpleReadPlans.Count == 1)
                    {
                        plan = (SimpleReadPlan)simpleReadPlans[0];
                    }
                    else
                    {
                        plan = new MultiReadPlan(simpleReadPlans);
                    }
                    objReader.ParamSet("/reader/read/plan", plan);
                }
                simpleReadPlans.Clear();
            }
            catch (Exception ex)
            {
                throw ex;
            }
        }

        /// <summary>
        /// Set read settings received from the connected read
        /// </summary>
        private void SetOptimalReaderSettings()
        {
            try
            {
                //To read maximum tags                    
                if (!model.Equals("Astra"))
                {
                    switch (OptimalReaderSettings["/reader/gen2/tagEncoding"])
                    {
                        case "FM0": objReader.ParamSet("/reader/gen2/tagEncoding", Gen2.TagEncoding.FM0); break;
                        case "M2": objReader.ParamSet("/reader/gen2/tagEncoding", Gen2.TagEncoding.M2); break;
                        case "M4": objReader.ParamSet("/reader/gen2/tagEncoding", Gen2.TagEncoding.M4); break;
                        case "M8": objReader.ParamSet("/reader/gen2/tagEncoding", Gen2.TagEncoding.M8); break;
                        default: break;
                    }
                    if (OptimalReaderSettings["/reader/gen2/q"] == "DynamicQ")
                    {
                        objReader.ParamSet("/reader/gen2/q", new Gen2.DynamicQ());
                    }
                    else
                    {
                        byte qValue = Convert.ToByte(
                            OptimalReaderSettings["/application/performanceTuning/staticQValue"]);
                        objReader.ParamSet("/reader/gen2/q", new Gen2.StaticQ(qValue));
                    }
                }
                if (model.Equals("M6e") || model.Equals("Mercury6") 
                    || model.Equals("Astra-EX") || model.Equals("M6e PRC")
                    || model.Equals("M6e Micro"))
                {
                    if (OptimalReaderSettings["/reader/gen2/BLF"].Equals("LINK640KHZ"))
                    {
                        objReader.ParamSet("/reader/gen2/BLF", Gen2.LinkFrequency.LINK640KHZ);
                    }
                    else
                    {
                        objReader.ParamSet("/reader/gen2/BLF", Gen2.LinkFrequency.LINK250KHZ);
                    }
                    //Set tari
                    switch (OptimalReaderSettings["/reader/gen2/tari"])
                    {
                        case "TARI_25US": objReader.ParamSet("/reader/gen2/tari",
                            Gen2.Tari.TARI_25US); break;
                        case "TARI_12_5US": objReader.ParamSet("/reader/gen2/tari",
                            Gen2.Tari.TARI_12_5US); break;
                        case "TARI_6_25US": objReader.ParamSet("/reader/gen2/tari",
                            Gen2.Tari.TARI_6_25US); break;
                        default: break;
                    }
                }
                switch (OptimalReaderSettings["/reader/gen2/session"])
                {
                    case "S0": objReader.ParamSet("/reader/gen2/session", Gen2.Session.S0); break;
                    case "S1": objReader.ParamSet("/reader/gen2/session", Gen2.Session.S1); break;
                    case "S2": objReader.ParamSet("/reader/gen2/session", Gen2.Session.S2); break;
                    case "S3": objReader.ParamSet("/reader/gen2/session", Gen2.Session.S3); break;
                    default: break;
                }
                switch (OptimalReaderSettings["/reader/gen2/target"])
                {
                    case "A": objReader.ParamSet("/reader/gen2/target", Gen2.Target.A); break;
                    case "AB": objReader.ParamSet("/reader/gen2/target", Gen2.Target.AB); break;
                    case "B": objReader.ParamSet("/reader/gen2/target", Gen2.Target.B); break;
                    case "BA": objReader.ParamSet("/reader/gen2/target", Gen2.Target.BA); break;
                    default: break;
                }
            }
            catch (Exception ex)
            {
                if ((-1 != ex.Message.IndexOf("Specified RFMode is not supported"))
                    || (ex is FAULT_MSG_INVALID_PARAMETER_VALUE_Exception))
                {
                    if (model.Equals("M6e") || model.Equals("Mercury6") 
                        || model.Equals("Astra-EX") || model.Equals("M6e PRC")
                        || model.Equals("M6e Micro"))
                    {
                        if (OptimalReaderSettings["/reader/gen2/BLF"].Equals("LINK640KHZ"))
                        {
                            objReader.ParamSet("/reader/gen2/BLF", Gen2.LinkFrequency.LINK640KHZ);
                        }
                        else
                        {
                            objReader.ParamSet("/reader/gen2/BLF", Gen2.LinkFrequency.LINK250KHZ);
                        }
                        //Set tari
                        switch (OptimalReaderSettings["/reader/gen2/tari"])
                        {
                            case "TARI_25US": objReader.ParamSet("/reader/gen2/tari", 
                                Gen2.Tari.TARI_25US); break;
                            case "TARI_12_5US": objReader.ParamSet("/reader/gen2/tari", 
                                Gen2.Tari.TARI_12_5US); break;
                            case "TARI_6_25US": objReader.ParamSet("/reader/gen2/tari",
                                Gen2.Tari.TARI_6_25US); break;
                            default: break;
                        }
                    }
                    SetOptimalReaderSettings();
                }
                else
                {
                    throw ex;
                }
            }

            
        }

        /// <summary>
        /// Initiates a Read Tag ID Single for the timeout given in the timeout Control Box.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>        
        private void readTag_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                // Clear warning on status bar if any
                ClearMessageOnStatusBar();

                // Set selected font size to tag-results
                SetFontSize();
                
                // Tag aging
                TagResults.enableTagAgingOnRead = true;
                TagResults.tagagingColourCache.Clear();

                // Set read plans 
                SetReadPlans();

                // Register exception handler
                objReader.ReadException += ReadException;

                // Load gen2 settings
                LoadGen2Settings();

                // Set optimal reader settings
                SetOptimalReaderSettings();

                // Disable read options when read is in progress
                EnableDisableSyncReadOptions(false);
                TagResultsViewModel trViewModel = new TagResultsViewModel();
                if (Convert.ToInt32(txtbxreadOnceTimeout.Text) <= 0)
                {
                    MessageBox.Show("Timeout value should be greater then 0", "Error",
                        MessageBoxButton.OK, MessageBoxImage.Error);
                    return;
                }
                string timeoutRdOnce = txtbxreadOnceTimeout.Text;
                Thread readSyncOnce = new Thread(new ThreadStart(delegate()
                    {
                        try
                        {
                            Dispatcher.BeginInvoke(new ThreadStart(delegate()
                            {
                                isSyncReadGoingOn = true;
                                lblshowStatus.Content = "Reading";
                                imgReaderStatus.Source = new BitmapImage(new Uri(@"..\Icons\LedGreen.png",
                                    UriKind.RelativeOrAbsolute));
                                if (clientConnected)
                                {
                                    imgTcpStreamStatus.Source = new BitmapImage(new Uri(@"..\Icons\broadcast-icon-on.png",
                                        UriKind.RelativeOrAbsolute));
                                    lblTcpStreamStatus.Visibility = Visibility.Visible;
                                }
                            }));
                            if (timeoutRdOnce != "")
                            {
                                trViewModel.ReadTags(ref tagdb, objReader, int.Parse(timeoutRdOnce),out trd);
                            }
                            else
                            {
                                timeoutRdOnce = "500";
                                trViewModel.ReadTags(ref tagdb, objReader, int.Parse(timeoutRdOnce),out trd);
                            }
                            if (clientConnected && tagStreamSock.Connected)
                            {
                                sendTagsToTcp(trd);
                            }
                            Dispatcher.BeginInvoke(new ThreadStart(delegate()
                                {
                                    tagdb.Repaint();
                                    isSyncReadGoingOn = false;
                                    btnRead.IsEnabled = true;
                                    readonceElapsedTime += Convert.ToDouble(txtbxreadOnceTimeout.Text);
                                    totalTimeElapsedTextBox.Content = Math.Round(((double)(readonceElapsedTime / 1000)),
                                        2).ToString();

                                    totalUniqueTagsReadTextBox.Content = tagdb.UniqueTagCount.ToString();

                                    txtTotalTagReads.Content = tagdb.TotalTagCount.ToString();
                                    txtbxReadRatePerSec.Content = (Math.Round(
                                        ((Convert.ToDouble(tagdb.TotalTagCount.ToString())) / ((readonceElapsedTime) / 1000)),
                                        2)).ToString();

                                    //Show the status
                                    lblshowStatus.Content = "Connected";
                                    imgReaderStatus.Source = new BitmapImage(new Uri(@"..\Icons\LedOrange.png",
                                        UriKind.RelativeOrAbsolute));
                                    imgTcpStreamStatus.Source = new BitmapImage(new Uri(@"..\Icons\broadcast-icon-off.png",
                                        UriKind.RelativeOrAbsolute));
                                    lblTcpStreamStatus.Visibility = Visibility.Hidden;

                                    TagResults.enableTagAgingOnRead = false;
                                    // Causes a control bound to the BindingSource to reread all the items 
                                    // in the list and refresh their displayed values.
                                    tagdb.Repaint();
                                    objReader.ReadException -= ReadException;
                                    CacheReadDataSettings();
                                    // Enable read options when read is in progress
                                    EnableDisableSyncReadOptions(true);
                                }));
                        }
                        catch (Exception ex)
                        {
                            btnConnect.Dispatcher.BeginInvoke(new ThreadStart(delegate()
                            {
                                btnRead.IsEnabled = true;
                                isSyncReadGoingOn = false;
                                TagResults.enableTagAgingOnRead = false;
                                //Show the status
                                lblshowStatus.Content = "Connected";
                                imgReaderStatus.Source = new BitmapImage(new Uri(@"..\Icons\LedOrange.png", 
                                    UriKind.RelativeOrAbsolute));

                                // Enable read options when read is in progress
                                EnableDisableSyncReadOptions(true);
                            }));
                            if (ex is ReaderCodeException)
                            {
                                if ((((ReaderCodeException)ex).Code == 0x504) 
                                    || (((ReaderCodeException)ex).Code == 0x505))
                                {
                                    switch (((ReaderCodeException)ex).Code)
                                    {
                                        case 0x504:
                                            warningText = "Over Temperature";
                                            break;
                                        case 0x505:
                                            warningText = "High Return Loss";
                                            break;
                                        default:
                                            warningText = "warning";
                                            break;
                                    }
                                    myTimer.Elapsed += new ElapsedEventHandler(TimeEvent);
                                    myTimer.Interval = 2000;
                                    myTimer.Start();
                                    lblWarning.Dispatcher.BeginInvoke(new ThreadStart(delegate()
                                    {
                                        GUIshowWarning();
                                    }));
                                }
                                else
                                {
                                    MessageBox.Show(ex.Message, "Reader Message", 
                                        MessageBoxButton.OK, MessageBoxImage.Error);
                                }
                            }
                            else if (ex is ReaderCommException)
                            {
                                MessageBox.Show(ex.Message, "Reader Message", 
                                    MessageBoxButton.OK, MessageBoxImage.Error);
                                return;
                            }
                            else if (ex is ReaderException)
                            {
                                DisplayMessageOnStatusBar(ex.Message, Brushes.Red);
                                return;
                            }
                            else if (ex is IOException)
                            {
                                MessageBox.Show("The I/O operation has been aborted, Disconnecting from reader",
                                    "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                                btnConnect.Dispatcher.BeginInvoke(new ThreadStart(delegate()
                                {
                                    btnConnect.Content = "Disconnect";
                                    btnConnect_Click(sender, new RoutedEventArgs());
                                }));
                            }
                            else
                            {
                                MessageBox.Show(ex.Message, "Reader Message", 
                                    MessageBoxButton.OK, MessageBoxImage.Error);
                                return;
                            }
                        }
                    }));
                readSyncOnce.Start();
                btnRead.IsEnabled = false;
            }
            catch (Exception ex)
            {
                // Enable read options when read is in progress
                EnableDisableSyncReadOptions(true);

                Onlog("Error :" + ex.Message);
                btnRead.IsEnabled = true;
                isSyncReadGoingOn = false;
                TagResults.enableTagAgingOnRead = false;
                //Show the status
                lblshowStatus.Content = "Connected";
                imgReaderStatus.Source = new BitmapImage(new Uri(@"..\Icons\LedOrange.png", UriKind.RelativeOrAbsolute));
                MessageBox.Show(ex.Message, "Reader Message", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        /// <summary>
        /// Enable or disable options when sync read is in progress
        /// </summary>
        /// <param name="option"></param>
        private void EnableDisableSyncReadOptions(bool option)
        {
            // Load config btn
            btnLoadConfig.IsEnabled = option;
            // Read continuous btn
            rdBtnReadContinuously.IsEnabled = option;
            // Baud rate combo-box
            cbxBaudRate.IsEnabled = option;
            // Fast search
            chkEnableFastSearch.IsEnabled = option;
            // Region combo-box
            regioncombo.IsEnabled = option;
            // Select all check box in tag-results
            Dispatcher.Invoke(new del(delegate()
            {
                CheckBox headerCheckBox = (CheckBox)GetChildControl(TagResults.dgTagResults, "headerCheckBox");
                headerCheckBox.IsEnabled = option;
            }));
            // Connect btn in expander
            btnConnect.IsEnabled = option;
            // Comport combo-box
            cmbReaderAddr.IsEnabled = option;
            // IP address combo-box
            cmbFixedReaderAddr.IsEnabled = option;
            // Font size
            txtfontSize.IsEnabled = option;
            // Rf off time
            txtRFOffTimeout.IsEnabled = option;
            // Rf on time
            txtRFOnTimeout.IsEnabled = option;
            // Disable all the buttons in settings, when async read is going on
            grpbxPerformanceTuning.IsEnabled = option;
            grpbxRdrPwrSettngs.IsEnabled = option;

            // Disable load config button when async read
            btnLoadConfig.IsEnabled = option;
            // Protocols
            ProtocolsGroupBox.IsEnabled = option;
            // Antennas
            AntennasGroupBox.IsEnabled = option;
            // Embedded read data
            ReadDataGroupBox.IsEnabled = option;
            // Filter
            FilterGroupBox.IsEnabled = option;
            // Disable firmware Update when async read
            stackPanelFirmwareUpdate.IsEnabled = option;
            // Read once timeout
            txtbxreadOnceTimeout.IsEnabled = option;
            // tag aging
            chkEnableTagAging.IsEnabled = option;
            // Refresh rate
            txtRefreshRate.IsEnabled = option;
            // WriteEPC tab
            tiWriteEPC.IsEnabled = option;
            // TagInspector tab
            tiTagInspector.IsEnabled = option;
            // UserMemory tab
            tiUserMemory.IsEnabled = option;
            // Lock tab
            tiLockTag.IsEnabled = option;
        }

        /// <summary>
        /// Get selected antenna list
        /// </summary>
        /// <returns></returns>
        private List<int> GetSelectedAntennaList()
        {
            CheckBox[] antennaBoxes = { Ant1CheckBox, Ant2CheckBox, Ant3CheckBox, Ant4CheckBox };
            List<int> ant = new List<int>();
            bool flagAntBoxVisibility;
            for (int antIdx = 0; antIdx < antennaBoxes.Length; antIdx++)
            {
                CheckBox antBox = antennaBoxes[antIdx];
                Visibility antBoxVisibility = antBox.Visibility;
                if (antBoxVisibility.Equals(Visibility.Visible))
                {
                    flagAntBoxVisibility = true;
                }
                else
                {
                    flagAntBoxVisibility = false;
                }
                if (flagAntBoxVisibility && (bool)antBox.IsEnabled && (bool)antBox.IsChecked)
                {
                    int antNum = antIdx + 1;
                    ant.Add(antNum);
                }
            }
            return ant;
        }

        /// <summary>
        /// Timeout used for synchronous and asynchronous reads
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void timeout_TextChanged(object sender, EventArgs e)
        {
        }

        /// <summary>
        /// Configure protocols 
        /// </summary>
        /// <param name="supportedProtocols"></param>
        void ConfigureProtocols(TagProtocol[] supportedProtocols)
        {
            CheckBox[] protocolBoxes = { gen2CheckBox, iso6bCheckBox, ipx64CheckBox, ipx256CheckBox ,ataCheckBox};
            foreach (CheckBox cb in protocolBoxes)
            {
                cb.Visibility = Visibility.Hidden;
            }
            if (null != supportedProtocols)
            {
                foreach (TagProtocol proto in supportedProtocols)
                {
                    switch (proto)
                    {
                        case TagProtocol.GEN2:
                            gen2CheckBox.Visibility = Visibility.Visible;
                            gen2CheckBox.IsChecked = true;
                            break;
                        case TagProtocol.ISO180006B:
                            iso6bCheckBox.Visibility = Visibility.Visible;
                            break;
                        case TagProtocol.IPX64:
                            ipx64CheckBox.Visibility = Visibility.Visible;
                            break;
                        case TagProtocol.IPX256:
                            ipx256CheckBox.Visibility = Visibility.Visible;
                            break;
                        case TagProtocol.ATA:
                            ataCheckBox.Visibility = Visibility.Visible;
                            break;
                    }
                }
                //At the start of application Based on the protocol being picked, 
                //change the label on select screen accordingly.
                //For example if Gen2, then change select screen to word address. on
                //the other hand if it is ISO18-6b, then change it to byte address.
                OnProtocolSelect();
            }
        }

        /// <summary>
        /// Configure antennas
        /// </summary>
        /// <param name="objReader"></param>
        public void ConfigureAntennaBoxes(Reader objReader)
        {
            // Cast int[] return values to IList<int> instead of int[] to get Contains method
            IList<int> existingAntennas = null;
            IList<int> detectedAntennas = null;
            IList<int> validAntennas = null;

            if (null == objReader)
            {
                int[] empty = new int[0];
                existingAntennas = detectedAntennas = validAntennas = empty;
            }
            else
            {
                bool checkPort;
                switch (model)
                {
                    case "Astra":
                        checkPort = true;
                        break;
                    default:
                        checkPort = (bool)objReader.ParamGet("/reader/antenna/checkPort");
                        break;
                }
                existingAntennas = (IList<int>)objReader.ParamGet("/reader/antenna/PortList");
                detectedAntennas = (IList<int>)objReader.ParamGet("/reader/antenna/connectedPortList");
                validAntennas = checkPort ? detectedAntennas : existingAntennas;
            }

            CheckBox[] antennaBoxes = { Ant1CheckBox, Ant2CheckBox, Ant3CheckBox, Ant4CheckBox };
            int antNum = 1;
            foreach (CheckBox cb in antennaBoxes)
            {
                if (existingAntennas.Contains(antNum))
                {
                    cb.Visibility = Visibility.Visible;
                }
                else
                {
                    cb.Visibility = Visibility.Hidden;
                }
                if (validAntennas.Contains(antNum))
                {
                    cb.IsEnabled = true;
                }
                else
                {
                    cb.IsEnabled = false;
                }
                if (detectedAntennas.Contains(antNum))
                {
                    cb.IsChecked = true;
                }
                else
                {
                    cb.IsChecked = false;
                }
                antNum++;
            }
        }

        /// <summary>
        /// Function that processes the Tag Data produced by StartReading();
        /// </summary>
        /// <param name="read"></param>
        void PrintTagRead(Object sender, TagReadDataEventArgs e)
        {
            Dispatcher.BeginInvoke(new ThreadStart(delegate()
           {
                if (clientConnected && tagStreamSock.Connected)
                {
                    PrintTagReads(e);
                }
                else
                {
                    lblTcpStreamStatus.Visibility = Visibility.Hidden;
                    imgTcpStreamStatus.Source = new BitmapImage(new Uri(@"..\Icons\broadcast-icon-off.png",
                           UriKind.RelativeOrAbsolute));
                }
           }));
            Dispatcher.BeginInvoke(new ThreadStart(delegate()
            {
                // Enable the read/stop-reading button when URA is able to connect 
                // to the reader or URA is able to get the tags.
                btnRead.IsEnabled = true;
                tagdb.Add(e.TagReadData);
                //If warning is there, remove it
                if (null != lblWarning.Content)
                {
                    if (lblWarning.Content.ToString() != "")
                    {
                        lblWarning.Dispatcher.BeginInvoke(new ThreadStart(delegate()
                        {
                            GUIturnoffWarning();
                        }));
                    }
                }
            }));
            Dispatcher.BeginInvoke(new ThreadStart(delegate()
            {
                txtTotalTagReads.Content = tagdb.TotalTagCount.ToString();
                totalUniqueTagsReadTextBox.Content = tagdb.UniqueTagCount.ToString();
            }
            ));
        }

        /// <summary>
        /// Initiate continuous read
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void startRead_Click(object sender, RoutedEventArgs e)
        {
            OnStartRead("");
        }

        /// <summary>
        /// Set selected font size to TagResults
        /// </summary>
        private void SetFontSize()
        {
            if (txtfontSize.Text == "")
            {
                txtfontSize.Text = "14";
                TagResults.dgTagResults.FontSize = Convert.ToDouble(txtfontSize.Text);
            }
            else if ((Convert.ToInt32(txtfontSize.Text) > 0) && (Convert.ToInt32(txtfontSize.Text) <= 20))
            {
                TagResults.dgTagResults.FontSize = Convert.ToDouble(txtfontSize.Text);
            }
        }

        /// <summary>
        /// Perform async read
        /// </summary>
        /// <param name="readStatus">Purpose of read async. "" if new read & "Reconnecting" 
        /// in case the ura lost its connection </param>
        private void OnStartRead( string readStatus)
        {
            bool triedStart = false;  // Did we go into the "Start Reads" clause?

            try
            {
                // Set the selected font size for content on tag results tab
                SetFontSize();

                Mouse.SetCursor(Cursors.Wait);
                if (btnRead.Content.ToString() == "Read")
                {
                    // Reset connection lost exception counter
                    connectionLostCount = 0;

                    // Variables used for synchronizations
                    isStopReadingBtnPressed = false;
                    isAsyncReadGoingOn = true;

                    // Display message on the status bar
                    DisplayMessageOnStatusBar("Applying initial settings", Brushes.LightBlue);

                    // Clear and set read plans based on the antenna configuration
                    simpleReadPlans.Clear();
                    SetReadPlans();

                    TagResults.enableTagAgingOnRead = true;
                    TagResults.tagagingColourCache.Clear();

                    triedStart = true;

                    // Disable all the enabled buttons which are not based on reader type
                    PerformAsyncReadInitialActions(false);
                    grpbxPerformanceTuning.IsEnabled = false;
                    ReadDataGroupBox.IsEnabled = false;

                    if ((("0" == txtRFOffTimeout.Text) && ("M6e".Equals(model)
                        || "M6e Micro".Equals(model) || "M6e PRC".Equals(model)))
                        ||(("Mercury6".Equals(model)) || ("Astra-EX".Equals(model))
                        || ("Astra".Equals(model))))
                    {
                        btnConnect.IsEnabled = false;
                        cmbReaderAddr.IsEnabled = false;
                        cmbFixedReaderAddr.IsEnabled = false;
                        txtfontSize.IsEnabled = false;
                        txtRFOffTimeout.IsEnabled = false;
                        txtRFOnTimeout.IsEnabled = false;
                    }

                    btnRead.ToolTip = "Stop Async Read";
                    btnRead.Content = "Stop Reading";

                    // Set RF off and on time 
                    objReader.ParamSet("/reader/read/asyncOnTime", int.Parse(txtRFOnTimeout.Text));
                    try
                    {                        
                        objReader.ParamSet("/reader/read/asyncOffTime", int.Parse(txtRFOffTimeout.Text));
                    }
                    catch(Exception ex)
                    {
                        if (-1 != ex.Message.IndexOf("M_UnsupportedParameter"))
                        {
                            MessageBox.Show("asyncOffTime unsupported parameter","Info",
                                MessageBoxButton.OK,MessageBoxImage.Information);
                            Onlog("asyncOffTime unsupported parameter");
                        }
                    }
                    
                    // To read maximum tags by default
                    LoadGen2Settings();

                    // Don't set the optimal reader settings when the ura is attempting
                    // to reconnect to the reader, when connection lost
                    if (readStatus.Length == 0)
                    {
                        SetOptimalReaderSettings();
                    }

                    // Register read exception and tag read listeners
                    objReader.ReadException += ReadException;
                    objReader.TagRead += PrintTagRead;

                    // Clear tag results
                    if (selectionOnEPC != null)
                    {
                        ClearReads();
                    }

                    // Cache current time
                    startAsyncReadTime = DateTime.Now;

                    // Display reading status
                    lblshowStatus.Content = "Reading";
                    imgReaderStatus.Source = new BitmapImage(new Uri(@"..\Icons\LedGreen.png",
                        UriKind.RelativeOrAbsolute));
                    if (clientConnected)
                        imgTcpStreamStatus.Source = new BitmapImage(new Uri(@"..\Icons\broadcast-icon-on.png",
                        UriKind.RelativeOrAbsolute));

                    // Start timer to render data on the grid and calculate read rate
                    dispatchtimer.Start();
                    readRatePerSec.Start();

                    Mouse.SetCursor(Cursors.Arrow);
                    // Call start async read
                    objReader.StartReading();

                    // Clear previous messages on status bar
                    ClearMessageOnStatusBar();
                }

                    // Call when stop reading button is pressed
                else if (btnRead.Content.ToString() == "Stop Reading")
                {
                    OnStopReadsClick();
                }
            }
            catch (Exception exp)
            {
                if (exp.Message.Contains("Please select at least one antenna"))
                {
                    MessageBox.Show(exp.Message, "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                    isAsyncReadGoingOn = false;
                    lblWarning.Dispatcher.BeginInvoke(new ThreadStart(delegate()
                    {
                        GUIturnoffWarning();
                    }));
                        return;
                }
                if (exp is ReaderCodeException)
                {
                    switch (MessageBox.Show(exp.Message.ToString() + "\nStart/Stop Tag Reads Failed. Try Again",
                        "Error", MessageBoxButton.YesNo, MessageBoxImage.Error))
                    {
                        case MessageBoxResult.Yes:
                            {
                                OnStartRead("");
                                btnRead.ToolTip = "Start Async Read (EPC Only)";
                                btnRead.Content = "Read";
                                rdBtnReadOnce.IsEnabled = true;
                                // Enable fast search
                                chkEnableFastSearch.IsEnabled = true;
                                isAsyncReadGoingOn = false;
                                TagResults.enableTagAgingOnRead = false;
                                // Enable firmware Update when async read is OFF
                                stackPanelFirmwareUpdate.IsEnabled = true;
                                OnStartRead("");
                                break;
                            }
                        default:
                            break;
                    }

                }
                if (exp is ReaderCommException)
                {
                    handleReaderCommException((ReaderCommException)exp, new RoutedEventArgs());
                }
                else
                {
                    if (triedStart)
                    {
                        ShutdownStartReads(exp);
                    }
                }
            }
        }

        /// <summary>
        /// Remember stop reading button press state, when connection lost 
        /// is occurred while performing async read 
        /// </summary>
        bool isStopReadingBtnPressed = false;

        /// <summary>
        /// Stop continuous read
        /// </summary>
        private void OnStopReadsClick()
        {
            // Variables used for synchronizations
            isStopReadingBtnPressed = true;

            btnRead.InvalidateVisual();

            // Stop timer to render data on the grid
            dispatchtimer.Stop();
            readRatePerSec.Stop();

            continuousreadElapsedTime = CalculateElapsedTime();
            try
            {
                Mouse.SetCursor(Cursors.AppStarting);

                // Call stop reading to stop async read
                if (null != objReader)
                {
                    objReader.StopReading();
                }
                // Causes a control bound to the BindingSource to reread all
                // the items in the list and refresh their displayed values.
                tagdb.Repaint();
            }
            finally
            {
                // Enable performance tuning controls
                grpbxPerformanceTuning.IsEnabled = true;

                // If connected reader is Astra, don't enable performance tunning
                // and read data grp bx. Since these parameters are not supported
                // by Astra
                if (!model.Equals("Astra"))
                {
                    ReadDataGroupBox.IsEnabled = true; ;
                }
                else
                {
                    DisableControlsForAstra();
                }

                // Variables used for synchronizations 
                isAsyncReadGoingOn = false;
                TagResults.enableTagAgingOnRead = false;

                Mouse.SetCursor(Cursors.Arrow);

                lblshowStatus.Content = "Stopped";
                imgReaderStatus.Source = new BitmapImage(new Uri(@"..\Icons\LedOrange.png",
                    UriKind.RelativeOrAbsolute));
                imgTcpStreamStatus.Source = new BitmapImage(new Uri(@"..\Icons\broadcast-icon-off.png",
                        UriKind.RelativeOrAbsolute));
                lblTcpStreamStatus.Visibility = Visibility.Hidden;
                btnRead.ToolTip = "Start Async Read";
                btnRead.Content = "Read";

                // De register read exception and tag read listeners
                objReader.TagRead -= PrintTagRead;
                objReader.ReadException -= ReadException;

                txtfontSize.IsEnabled = true;
                txtRFOffTimeout.IsEnabled = true;
                txtRFOnTimeout.IsEnabled = true;

                // Calculate read rate with time elapsed which is calculated before
                // calling StopReading() api method. So that we don't miss the 
                // remaining tags obtained in the tag database after calling 
                // stop reading() api method)
                UpdateReadRate(continuousreadElapsedTime);

                tiTagResults.Focus();

                // Cache reader settings such as start address, read length, gen2 
                // mem bank
                CacheReadDataSettings();
                // Clear previous messages on status bar
                ClearMessageOnStatusBar();

                // Enable all the disabled buttons which not based on reader
                // type
                PerformAsyncReadInitialActions(true);
            }
        }

        /// <summary>
        /// Performs async read initial actions. Ex. disable some buttons when async 
        /// read is in progress and enable when stopped 
        /// </summary>
        /// <param name="value">true or false</param>
        private void PerformAsyncReadInitialActions(bool value)
        {
            cbxBaudRate.IsEnabled = value;
            //Disable fast search
            chkEnableFastSearch.IsEnabled = value;
            //thingMagicReader.IsEnabled = false;
            regioncombo.IsEnabled = value;
            //Disable Select all check box
            Dispatcher.Invoke(new del(delegate()
            {
                CheckBox headerCheckBox = (CheckBox)GetChildControl(TagResults.dgTagResults, "headerCheckBox");
                headerCheckBox.IsEnabled = value;
            }));

            // All the buttons in the settings panel when async read is turned off
            grpbxRdrPwrSettngs.IsEnabled = value;

            // Dis bale load config button when async read
            btnLoadConfig.IsEnabled = value;

            ProtocolsGroupBox.IsEnabled = value;
            AntennasGroupBox.IsEnabled = value;
            FilterGroupBox.IsEnabled = value;
            rdBtnReadOnce.IsEnabled = value;
            btnConnect.IsEnabled = value;
            //Disable firmware Update when async read
            stackPanelFirmwareUpdate.IsEnabled = value;
        }

        /// <summary>
        /// Validate rf off time and throw exception if the value 
        /// is greater then the max limit
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void readDelay_TextChanged(object sender, TextChangedEventArgs e)
        {
            try
            {
                if (txtRFOffTimeout.Text != "")
                {
                    if (Convert.ToInt32(txtRFOffTimeout.Text) > 65535)
                    {
                        MessageBox.Show("Please input rf off timeout less then 65535", 
                            "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                        txtRFOffTimeout.Text = "0";
                        return;
                    }

                    if (Convert.ToInt32(txtRFOffTimeout.Text) < 0)
                    {
                        txtRFOffTimeout.Foreground = Brushes.Red;
                    }
                    else
                    {
                        txtRFOffTimeout.Foreground = Brushes.Black;
                    }
                    objReader.ParamSet("/reader/transportTimeout", int.Parse(txtRFOffTimeout.Text) + 5000);
                }
            }
            catch { }
        }

        /// <summary>
        /// Gen2 protocol
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void gen2CheckBox_CheckedChanged(object sender, RoutedEventArgs e)
        {
            //Based on the protocol being picked, change the label on select screen accordingly.
            //For example if Gen2, then change select screen to word address. on
            //the other hand if it is ISO18-6b, then change it to byte address.
            OnProtocolSelect();
        }

        /// <summary>
        /// ISO18-6b protocol
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void iso6bCheckBox_CheckedChanged(object sender, RoutedEventArgs e)
        {
            //Based on the protocol being picked, change the label on select screen accordingly.
            //For example if Gen2, then change select screen to word address. on
            //the other hand if it is ISO18-6b, then change it to byte address.
            OnProtocolSelect();
        }

        /// <summary>
        /// IPX64 protocol
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ipx64CheckBox_CheckedChanged(object sender, RoutedEventArgs e)
        {
            //Based on the protocol being picked, change the label on select screen accordingly.
            //For example if Gen2, then change select screen to word address. on
            //the other hand if it is ISO18-6b, then change it to byte address.
            OnProtocolSelect();
        }

        /// <summary>
        /// IPX256 protocol
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ipx256CheckBox_CheckedChanged(object sender, RoutedEventArgs e)
        {
            //Based on the protocol being picked, change the label on select screen accordingly.
            //For example if Gen2, then change select screen to word address. on
            //the other hand if it is ISO18-6b, then change it to byte address.
            OnProtocolSelect();
        }

        /// <summary>
        /// ATA protocol
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ataCheckBox_CheckedChanged(object sender, RoutedEventArgs e)
        {
            //Based on the protocol being picked, change the label on select screen accordingly.
            //For example if Gen2, then change select screen to word address. on
            //the other hand if it is ISO18-6b, then change it to byte address.
            OnProtocolSelect();

        }

        /// <summary>
        /// Sets up the tag operation protocol and antenna based on  Option Menu Bar
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void tagOpProtocolAntenna(object sender, EventArgs e)
        {
            {
                List<int> ant;
                try
                {
                    ant = GetSelectedAntennaList();
                    if ((ant.ToArray().Length == 0) || (ant.ToArray().Length > 1))
                    {
                        throw new ArgumentException();
                    }
                }
                catch (ArgumentException)
                {
                    throw new TagOpAntennaException("Please select a single antenna port for this operation");
                }

                try
                {
                    objReader.ParamSet("/reader/tagop/antenna", ant.ToArray()[0]);
                }
                catch (Exception)
                {
                    throw new TagOpAntennaException("Please select a single antenna port for this operation"); ;
                }
            }
            {
                tagOpProto.Clear();
                if ((bool)gen2CheckBox.IsChecked)
                {
                    tagOpProto.Add(TagProtocol.GEN2);
                }
                if ((bool)iso6bCheckBox.IsChecked)
                {
                    tagOpProto.Add(TagProtocol.ISO180006B);
                }
                if ((bool)ipx64CheckBox.IsChecked)
                {
                    tagOpProto.Add(TagProtocol.IPX64);
                }
                if ((bool)ipx256CheckBox.IsChecked)
                {
                    tagOpProto.Add(TagProtocol.IPX256);
                }
                if ((bool)ataCheckBox.IsChecked)
                {
                    tagOpProto.Add(TagProtocol.ATA);
                }
                if ((tagOpProto.ToArray().Length == 0) || (tagOpProto.ToArray().Length > 1))
                {
                    throw new TagOpProtocolException("Please select a single protocol for this operation");
                }
                else
                {
                    objReader.ParamSet("/reader/tagop/protocol", tagOpProto.ToArray()[0]);
                }
            }
        }

        /// <summary>
        /// Validate rf on time and throw exception if the timeout value is 
        /// greater then the max limit
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void timeout_TextChanged(object sender, TextChangedEventArgs e)
        {
            try
            {
                if (txtRFOnTimeout.Text != "")
                {
                    if (Convert.ToInt32(txtRFOnTimeout.Text) > 65535)
                    {
                        MessageBox.Show("Please input rf on timeout less then 65535", 
                            "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                        txtRFOnTimeout.Text = "1000";
                        return;
                    }
                    if (Convert.ToInt32(txtRFOnTimeout.Text) < 0)
                    {
                        txtRFOnTimeout.Foreground = Brushes.Red;
                    }
                    else
                    {
                        txtRFOnTimeout.Foreground = Brushes.Black;
                    }
                    objReader.ParamSet("/reader/transportTimeout", int.Parse(txtRFOnTimeout.Text) + 5000);
                }
            }
            catch { }
        }

        /// <summary>
        /// Perform read for specified timeout
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void rdBtnReadOnce_Checked(object sender, RoutedEventArgs e)
        {
            try
            {
                if ((bool)rdBtnReadOnce.IsChecked)
                {
                    if (btnConnect.Content.ToString() != "Connect")
                    {
                        //Show the status
                        lblshowStatus.Content = "Connected";
                        btnClearReads_Click(sender, e);
                        lblReadOnce.Visibility = System.Windows.Visibility.Visible;
                        txtbxreadOnceTimeout.Visibility = System.Windows.Visibility.Visible;
                        lblRfOn.Visibility = System.Windows.Visibility.Collapsed;
                        lblRfOff.Visibility = System.Windows.Visibility.Collapsed;
                        txtRFOffTimeout.Visibility = System.Windows.Visibility.Collapsed;
                        txtRFOnTimeout.Visibility = System.Windows.Visibility.Collapsed;
                        tiTagResults.Focus();
                    }
                }
            }
            catch { };
        }

        /// <summary>
        /// Perform continuous read until user press stop read
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void rdBtnReadContinuously_Checked(object sender, RoutedEventArgs e)
        {
            try
            {
                if ((bool)rdBtnReadContinuously.IsChecked)
                {
                    if (btnConnect.Content.ToString() != "Connect")
                    {
                        btnClearReads_Click(sender, e);
                        lblReadOnce.Visibility = System.Windows.Visibility.Collapsed;
                        txtbxreadOnceTimeout.Visibility = System.Windows.Visibility.Collapsed;
                        lblRfOn.Visibility = System.Windows.Visibility.Visible;
                        lblRfOff.Visibility = System.Windows.Visibility.Visible;
                        txtRFOffTimeout.Visibility = System.Windows.Visibility.Visible;
                        txtRFOnTimeout.Visibility = System.Windows.Visibility.Visible;
                        tiTagResults.Focus();
                    }
                }
            }
            catch { };
        }

        /// <summary>
        /// Read timeout event handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void txtbxreadOnceTimeout_TextChanged(object sender, TextChangedEventArgs e)
        {
            try
            {               
                if (Convert.ToInt32(txtbxreadOnceTimeout.Text) <= 0)
                {
                    txtbxreadOnceTimeout.Foreground = Brushes.Red;
                }
                else
                {
                    txtbxreadOnceTimeout.Foreground = Brushes.Black;
                    objReader.ParamSet("/reader/transportTimeout", int.Parse(txtRFOnTimeout.Text) + 5000);
                }
            }
            catch { }
        }

        /// <summary>
        /// Read button implementation. Performs read once or continuous read based
        /// on the read behavior 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnRead_Click(object sender, RoutedEventArgs e)
        {
            if (btnRead.Content.ToString() == "Read")
            {
                try
                {
                    ValidateRefreshRate();
                    // Validating TID filter for Higgs3 tag
                    bool status = validateTidFilterData();
                    if (!status)
                    {
                        return;
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message, "Error");
                    return;
                }
            }
             if (clientConnected)
            {
                imgTcpStreamStatus.Source = new BitmapImage(new Uri(@"..\Icons\broadcast-icon-on.png",
                    UriKind.RelativeOrAbsolute));
                lblTcpStreamStatus.Content = "Status:Data is streaming to port: " + txtTcpPort.Text;
                lblTcpStreamStatus.Foreground = new SolidColorBrush(Colors.Green);
                lblTcpStreamStatus.Visibility = Visibility.Visible;
            }
            if ((bool)rdBtnReadOnce.IsChecked)
            {
                //call read once
                btnRead.ToolTip = "Perform read based on the read behavior defined " 
                    + " by the settings in the Read Options sidebar.";
                imgReaderStatus.Source = new BitmapImage(new Uri(@"..\Icons\LedGreen.png",
                    UriKind.RelativeOrAbsolute));                
                readTag_Click(sender, e);
                imgReaderStatus.Source = new BitmapImage(new Uri(@"..\Icons\LedOrange.png",
                    UriKind.RelativeOrAbsolute));
            }
            if ((bool)rdBtnReadContinuously.IsChecked)
            {
                //call read continuous
                btnRead.ToolTip = "Start Async Read";                
                startRead_Click(sender, e);
            }
        }

        /// <summary>
        /// Stop continuous read if going on 
        /// </summary>
        void stopReading()
        {
            if (null != objReader)
            {
                objReader.StopReading();
                objReader.TagRead -= PrintTagRead;
                objReader.ReadException -= ReadException;
            }
        }

        /// <summary>
        /// When "Fast Search" is selected, the target should be changed to "AB" (required
        /// per the Fast Search HLD). (User should be able to manually change Target to
        /// something else if they wish.)
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkEnableFastSearch_Checked(object sender, RoutedEventArgs e)
        {
            if (null != chkEnableFastSearch)
            {
                if ((bool)chkEnableFastSearch.IsChecked)
                {
                    // Save the target in temporary variable before changing it to target AB,
                    // when fast search is enabled.
                    tempTarget = OptimalReaderSettings["/reader/gen2/target"];
                    OptimalReaderSettings["/reader/gen2/target"] = "AB";
                }
                else
                {
                    //Change to the original target, when fast search is disabled.
                    OptimalReaderSettings["/reader/gen2/target"] = tempTarget;
                }
            }
        }

        /// <summary>
        /// Enable antenna detection
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkbxAntennaDetection_Checked(object sender, RoutedEventArgs e)
        {
            SetCheckPort(sender, e, true);
        }

        /// <summary>
        /// Disable antenna detection
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkbxAntennaDetection_Unchecked(object sender, RoutedEventArgs e)
        {
            SetCheckPort(sender, e, false);
        }

        /// <summary>
        /// Antenna detection feature. Detect connected antennas on the reader
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        /// <param name="value"></param>
        private void SetCheckPort(Object sender, EventArgs e, bool value)
        {
            if (null != objReader)
            {
                try
                {
                    objReader.ParamSet("/reader/antenna/checkPort", value);
                    ConfigureAntennaBoxes(objReader);
                }
                catch (ReaderCodeException ex)
                {
                    MessageBox.Show(ex.Message 
                        + " Check supported reader configurations in Reader's Hardware Guide.",
                        "Unsupported Reader Configuration", MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }
        }

        /// <summary>
        /// Condition to check textbox to have only number
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void txtRFOnTimeout_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            e.Handled = !Utilities.AreAllValidNumericChars(e.Text);
            base.OnPreviewTextInput(e);
        }

        /// <summary>
        /// Validate read once timeout and throw exception when the timeout
        /// is less then the specified min limit
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void txtbxreadOnceTimeout_LostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            if (txtbxreadOnceTimeout.Text != "")
            {
                if ((bool)rdbtnNetworkConnection.IsChecked)
                {
                    if (Convert.ToInt32(txtbxreadOnceTimeout.Text) < 30)
                    {
                        MessageBox.Show("Please input read once timeout greater then 30 ms",
                            "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                        txtbxreadOnceTimeout.Text = "500";
                        return;
                    }
                }
            }
            else
            {
                MessageBox.Show("Timeout (ms) can't be empty", "Universal Reader Assistant Message",
                    MessageBoxButton.OK, MessageBoxImage.Error);
                txtbxreadOnceTimeout.Text = "500";
            }
        }

        #endregion ReadOptions

        #region DisplayOptions

        /// <summary>
        /// Enable bigNumUniqueTagCounts option and disable the remaining big num options
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void expAdvRdOpRdBtnUniqueTagCountBigNum_Checked(object sender, RoutedEventArgs e)
        {
            bigNumUniqueTagCounts.Visibility = System.Windows.Visibility.Visible;
            bigNumTotalTagCounts.Visibility = System.Windows.Visibility.Collapsed;
            gridCountsBigNum.Visibility = System.Windows.Visibility.Collapsed;
        }

        /// <summary>
        /// Enable bigNumTotalTagCounts option and disable the remaining big num options
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void expAdvRdOpRdBtnTotalTagCountBigNum_Checked(object sender, RoutedEventArgs e)
        {
            bigNumUniqueTagCounts.Visibility = System.Windows.Visibility.Collapsed;
            bigNumTotalTagCounts.Visibility = System.Windows.Visibility.Visible;
            gridCountsBigNum.Visibility = System.Windows.Visibility.Collapsed;
        }

        /// <summary>
        /// Enable bigNumSummary of read option and disable the remaining big num options
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void expAdvRdOpRdBtnCountsBigNum_Checked(object sender, RoutedEventArgs e)
        {
            bigNumUniqueTagCounts.Visibility = System.Windows.Visibility.Collapsed;
            bigNumTotalTagCounts.Visibility = System.Windows.Visibility.Collapsed;
            gridCountsBigNum.Visibility = System.Windows.Visibility.Visible;
        }

        /// <summary>
        /// Disable big num feature
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void expAdvRdOpRdBtnClearBigNumSelection_Checked(object sender, RoutedEventArgs e)
        {
            bigNumUniqueTagCounts.Visibility = System.Windows.Visibility.Collapsed;
            bigNumTotalTagCounts.Visibility = System.Windows.Visibility.Collapsed;
            gridCountsBigNum.Visibility = System.Windows.Visibility.Collapsed;
        }

        /// <summary>
        /// Column with check-box implementation
        /// </summary>
        public class ColumnSelectionForTagResult
        {
            protected bool ischecked = false;

            public ColumnSelectionForTagResult(bool isVisited, string cityName)
            {
                IsColumnChecked = isVisited;
                SelectedColumn = cityName;
            }

            public Boolean IsColumnChecked
            {
                get
                {
                    return ischecked;
                }
                set
                {
                    ischecked = value;
                }
            }

            public String SelectedColumn
            { get; set; }
        }

        /// <summary>
        /// Enable or disable the selected columns on tag results tab
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void AddColumnsToGrid(object sender, RoutedEventArgs e)
        {
            ColumnSelectionForTagResult rda;
            for (int rowCount = 0; rowCount <= cbxcolumnSelection.Items.Count - 1; rowCount++)
            {
                rda = (ColumnSelectionForTagResult)cbxcolumnSelection.Items.GetItemAt(rowCount);

                //re order column width based on column selection for readability
                if (rda.IsColumnChecked)
                {
                    //Adjust columns based on the content of the column header
                    TagResults.epcColumn.Width = new DataGridLength(1, DataGridLengthUnitType.Auto);
                    TagResults.timeStampColumn.Width = new DataGridLength(1, DataGridLengthUnitType.Auto);
                }
                else
                {
                    //Adjust columns in equal proportion of available space
                    TagResults.epcColumn.Width = new DataGridLength(1, DataGridLengthUnitType.Star);
                    TagResults.timeStampColumn.Width = new DataGridLength(1, DataGridLengthUnitType.Star);
                    TagResults.rssiColumn.Width = new DataGridLength(1, DataGridLengthUnitType.Star);
                    TagResults.readCountColumn.Width = new DataGridLength(1, DataGridLengthUnitType.Star);
                }

                switch (rda.SelectedColumn)
                {
                    case "Antenna":
                        if (rda.IsColumnChecked)
                        {
                            TagResults.antennaColumn.Visibility = System.Windows.Visibility.Visible;
                        }
                        else
                        {
                            TagResults.antennaColumn.Visibility = System.Windows.Visibility.Collapsed;
                        }
                        break;
                    case "Phase":
                        if (rda.IsColumnChecked)
                        {
                            TagResults.phaseColumn.Visibility = System.Windows.Visibility.Visible;
                        }
                        else
                        {
                            TagResults.phaseColumn.Visibility = System.Windows.Visibility.Collapsed;
                        }
                        break;
                    case "Frequency":
                        if (rda.IsColumnChecked)
                        {
                            TagResults.frequencyColumn.Visibility = System.Windows.Visibility.Visible;
                        }
                        else
                        {
                            TagResults.frequencyColumn.Visibility = System.Windows.Visibility.Collapsed;
                        }
                        break;
                    case "Protocol":
                        if (rda.IsColumnChecked)
                        {
                            TagResults.protocolColumn.Visibility = System.Windows.Visibility.Visible;
                        }
                        else
                        {
                            TagResults.protocolColumn.Visibility = System.Windows.Visibility.Collapsed;
                        }
                        break;
                }
            }
            TagResults.dgTagResults.InvalidateVisual();
        }

        /// <summary>
        /// Refresh fixed reader ip address
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnRefreshReadersList_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Mouse.SetCursor(Cursors.Wait);
                InitializeReaderUriBox();
                Mouse.SetCursor(Cursors.Arrow);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        /// <summary>
        /// To enable tag aging. Same event handler will be called for both checked and unchecked event.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkEnableTagAging_Checked(object sender, RoutedEventArgs e)
        {
            TagResults.chkEnableTagAging = (bool)chkEnableTagAging.IsChecked;
        }

        /// <summary>
        /// Enable or disable the selected big num option
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cbxBigNum_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (null != cbxBigNum)
            {
                try
                {
                    string text = ((ComboBoxItem)cbxBigNum.SelectedItem).Content.ToString();
                    switch (text)
                    {
                        case "Remove Big Num":
                            bigNumUniqueTagCounts.Visibility = System.Windows.Visibility.Collapsed;
                            bigNumTotalTagCounts.Visibility = System.Windows.Visibility.Collapsed;
                            gridCountsBigNum.Visibility = System.Windows.Visibility.Collapsed;
                            break;
                        case "Unique Tag Count":
                            bigNumUniqueTagCounts.Visibility = System.Windows.Visibility.Visible;
                            bigNumTotalTagCounts.Visibility = System.Windows.Visibility.Collapsed;
                            gridCountsBigNum.Visibility = System.Windows.Visibility.Collapsed;
                            break;
                        case "Total Tag Count":
                            bigNumUniqueTagCounts.Visibility = System.Windows.Visibility.Collapsed;
                            bigNumTotalTagCounts.Visibility = System.Windows.Visibility.Visible;
                            gridCountsBigNum.Visibility = System.Windows.Visibility.Collapsed;
                            break;
                        case "Summary of Tag Result":
                            bigNumUniqueTagCounts.Visibility = System.Windows.Visibility.Collapsed;
                            bigNumTotalTagCounts.Visibility = System.Windows.Visibility.Collapsed;
                            gridCountsBigNum.Visibility = System.Windows.Visibility.Visible;
                            break;
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message, "Universal Reader Assistant Message",
                        MessageBoxButton.OK, MessageBoxImage.Information);
                    cbxBigNum.SelectedIndex = 0;
                }
            }
        }

        /// <summary>
        /// Font size of the tag results tab
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void txtfontSize_TextChanged(object sender, TextChangedEventArgs e)
        {
            try
            {
                if ((Convert.ToInt32(txtfontSize.Text) <= 0) || (Convert.ToInt32(txtfontSize.Text) > 20))
                {
                    txtfontSize.Foreground = Brushes.Red;
                }
                else
                {
                    txtfontSize.Foreground = Brushes.Black;
                }
            }
            catch(Exception ex)
            {
                Onlog("Time:"+DateTime.Now.ToString());
                Onlog(ex.ToString());
            }
        }

         ///<summary>
         /// Change tagresults time stamp format based on the selected format in Time stamp combo-box
         ///</summary>
         ///<param name="sender"></param>
         ///<param name="e"></param>
        private void cbxTimestampFormat_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (TagResults.timeStampColumn.Binding != null)
            {
                try
                {
                    string text = ((ComboBoxItem)cbxTimestampFormat.SelectedItem).Content.ToString();
                    switch (text)
                    {
                        case "DD/MM/YYY HH:MM:Sec.MillSec":
                            TagResults.timeStampColumn.Binding = new Binding("TimeStamp");
                            TagResults.timeStampColumn.Binding.StringFormat = "{0:dd/MM/yyy hh:mm:ss:fff}";
                            //TagResults.timeStampColumn.Header = "TimeStamp (DD/MM/YYY HH:MM:Sec:MillSec)";
                            break;
                        case "MM/DD/YYY HH:MM:Sec.MillSec":
                            TagResults.timeStampColumn.Binding = new Binding("TimeStamp");
                            TagResults.timeStampColumn.Binding.StringFormat = "{0:MM/dd/yyy hh:mm:ss.fff}";
                            //TagResults.timeStampColumn.Header = "TimeStamp (MM/DD/YYY HH:MM:Sec)";
                            break;
                        case "YYY/DD/MM HH:MM:Sec.MillSec":
                            TagResults.timeStampColumn.Binding = new Binding("TimeStamp");
                            TagResults.timeStampColumn.Binding.StringFormat = "{0:yyy/dd/MM hh:mm:ss.fff}";
                            //TagResults.timeStampColumn.Header = "TimeStamp (YYY/DD/MM HH:MM:Sec)";
                            break;
                        case "HH:MM:Sec.MillSec":
                            TagResults.timeStampColumn.Binding = new Binding("TimeStamp");
                            TagResults.timeStampColumn.Binding.StringFormat = "{0:hh:mm:ss.fff}";
                            //TagResults.timeStampColumn.Header = "TimeStamp (HH:MM:Sec)";
                            break;
                    }
                }
                catch { MessageBox.Show("To set this, please clear the tag results",
                    "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Information);
                cbxTimestampFormat.SelectedIndex = 0;
                }
            }
        }

        /// <summary>
        /// Refresh rate for tag result grid 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void txtRefreshRate_LostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            try
            {
                ValidateRefreshRate();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Universal Reader Assistant",
                    MessageBoxButton.OK, MessageBoxImage.Error);
                txtRefreshRate.Text = "100";
                return;
            }
        }

        /// <summary>
        /// Validate refresh rate of tag results grid
        /// </summary>
        private void ValidateRefreshRate()
        {

            if (txtRefreshRate.Text != "")
            {
                int refreshrate = 0;
                try
                {
                    refreshrate = Convert.ToInt32(txtRefreshRate.Text.TrimEnd());
                }
                catch { throw new Exception("Please input the refresh rate between 100 and 999"); }
                if ((refreshrate < 100) || (refreshrate > 999))
                {
                    throw new Exception("Please input the refresh rate between 100 and 999");
                }
                else
                {
                    try
                    {
                        if (null != dispatchtimer)
                        {
                            dispatchtimer.Interval = TimeSpan.FromMilliseconds(Convert.ToDouble(txtRefreshRate.Text));
                        }
                    }
                    catch (Exception ex) { Onlog(ex.Message); };
                }
            }
            else
            {
                throw new Exception("Please input the refresh rate between 100 and 999");
            }
        }

        #endregion DisplayOptions

        #region Gen2PerformanceTuning
        
        // Up down ui control implementation
        #region NumberUpDown

        private int _numValue = 1;

        public int NumValue
        {
            get { return _numValue; }
            set
            {
                _numValue = value;
                txtTagsNum.Text = value.ToString();
            }
        }

        private void cmdUp_Click(object sender, RoutedEventArgs e)
        {
            NumValue++;            
            if (_numValue > 1)
            {
                cmdDown.IsEnabled = true;
            }
            if (_numValue > 99999)
            {
                NumValue--;
                cmdUp.IsEnabled = false;
            }
            UpDownCounterTextChange(sender);
        }

        private void cmdDown_Click(object sender, RoutedEventArgs e)
        {
            NumValue--;
            if (_numValue <= 1)
            {
                _numValue = 1;
                cmdDown.IsEnabled = false;
            }
            if (_numValue < 99999)
            {
                cmdUp.IsEnabled = true;
            }
            UpDownCounterTextChange(sender);
        }
        private void txtTagsNum_TextChanged(object sender, KeyboardFocusChangedEventArgs e)
        {
            UpDownCounterTextChange(sender);
        }

        private void UpDownCounterTextChange(object sender)
        {
            if (null != txtTagsNum)
            {
                if (!int.TryParse(txtTagsNum.Text, out _numValue))
                {
                    txtTagsNum.Text = _numValue.ToString();
                }
                if (Convert.ToInt32(txtTagsNum.Text) <= 0)
                {
                    MessageBox.Show("Number of tags should be greater then zero", 
                        "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Information);
                    txtTagsNum.Text = "1";
                    _numValue = 1;
                    cmdDown.IsEnabled = false;
                    rdbtnOptmzExtmdNoTagsInField_Checked(sender, new RoutedEventArgs());
                    return;
                }
                if (null != cmdDown)
                {
                    if (_numValue <= 1)
                    {
                        cmdDown.IsEnabled = false;
                        cmdUp.IsEnabled = true;
                    }
                    if (_numValue > 1)
                    {
                        cmdDown.IsEnabled = true;
                    }
                }
                rdbtnOptmzExtmdNoTagsInField_Checked(sender, new RoutedEventArgs());
            }
        }
        #endregion NumberUpDown

        /// <summary>
        /// To set tag population size. Automatically adjust as population changes.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void rdbtnAutoAdjstAsPoplChngs_Checked(object sender, RoutedEventArgs e)
        {
            try
            {
                if (null != objReader)
                {
                    if ((bool)rdbtnAutoAdjstAsPoplChngs.IsChecked)
                    {
                        // If tag population is selected as "auto", the Q setting should
                        // remain as "auto" i.e dynamic Q
                        objReader.ParamSet("/reader/gen2/q", new Gen2.DynamicQ());
                        OptimalReaderSettings["/reader/gen2/q"] = "DynamicQ";
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Reader Error", MessageBoxButton.OK, MessageBoxImage.Error);
            };
        }

        /// <summary>
        /// Read distance vs read rate 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void sldrRdDistVsrdRate_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            try
            {
                if (null != objReader)
                {
                    switch (((Int32)sldrRdDistVsrdRate.Value))
                    {
                        case 0:
                            //Maximize tag read distance
                            
                            OptimalReaderSettings["/reader/gen2/BLF"] = "LINK250KHZ";

                            if (!model.Equals("Astra"))
                            {
                                OptimalReaderSettings["/reader/gen2/tagEncoding"] = "M8";
                            }
                            if (!(model.Equals("M5e Compact") || model.Equals("M5e")))
                            {
                                OptimalReaderSettings["/reader/gen2/tari"] = "TARI_25US";
                            }
                            break;
                        case 1:
                            break;
                        case 2:
                            //Maximize tag read rate
                            if (!model.Equals("Astra"))
                            {
                                OptimalReaderSettings["/reader/gen2/tagEncoding"] = "FM0";
                            }
                            if (model.Equals("M6e") || model.Equals("Mercury6") 
                                || model.Equals("Astra-EX") || model.Equals("M6e Micro") 
                                || model.Equals("M6e PRC"))
                            {
                                OptimalReaderSettings["/reader/gen2/BLF"] = "LINK640KHZ";
                            }
                            if (!(model.Equals("M5e Compact") || model.Equals("M5e")))
                            {
                                OptimalReaderSettings["/reader/gen2/tari"] = "TARI_6_25US";
                            }
                            break;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Reader Error", MessageBoxButton.OK, MessageBoxImage.Error);
            };
        }

        /// <summary>
        /// Control tag response rate
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void sldrTagrspLessVsTagrspMore_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            SldrTagrspLessVsTagrspMoreValueChanged();
        }

        /// <summary>
        /// Set session and target based on the values set by the tag response less or more slider
        /// </summary>
        private void SldrTagrspLessVsTagrspMoreValueChanged()
        {
            try
            {
                if (null != objReader)
                {
                    switch (((Int32)sldrTagrspLessVsTagrspMore.Value))
                    {
                        case 0:
                            OptimalReaderSettings["/reader/gen2/session"] = "S2";
                            OptimalReaderSettings["/reader/gen2/target"] = "A";
                            break;
                        case 1:
                            OptimalReaderSettings["/reader/gen2/session"] = "S2";
                            OptimalReaderSettings["/reader/gen2/target"] = "AB";
                            break;
                        case 2:
                            OptimalReaderSettings["/reader/gen2/session"] = "S1";
                            OptimalReaderSettings["/reader/gen2/target"] = "A";
                            break;
                        case 3:
                            OptimalReaderSettings["/reader/gen2/session"] = "S1";
                            OptimalReaderSettings["/reader/gen2/target"] = "AB";
                            break;
                        case 4:
                            OptimalReaderSettings["/reader/gen2/session"] = "S0";
                            OptimalReaderSettings["/reader/gen2/target"] = "A";
                            break;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Reader Error", MessageBoxButton.OK, MessageBoxImage.Error);
            };
        }

        private void rdBtnTagsRespondOption_Checked(object sender, RoutedEventArgs e)
        {
            if ((bool)rdBtnTagsRespondOption.IsChecked)
            {
                stkPanelTagsRespondOption.Visibility = System.Windows.Visibility.Visible;
                txtblkTagrspLess.Visibility = System.Windows.Visibility.Visible;
                txtblkTagrspMore.Visibility = System.Windows.Visibility.Visible;
                sldrTagrspLessVsTagrspMore.Visibility = System.Windows.Visibility.Visible;
                SldrTagrspLessVsTagrspMoreValueChanged();
            }
            else if ((bool)(!rdBtnTagsRespondOption.IsChecked))
            {
                stkPanelTagsRespondOption.Visibility = System.Windows.Visibility.Collapsed;
                txtblkTagrspLess.Visibility = System.Windows.Visibility.Collapsed;
                txtblkTagrspMore.Visibility = System.Windows.Visibility.Collapsed;
                sldrTagrspLessVsTagrspMore.Visibility = System.Windows.Visibility.Collapsed;
            }
        }

        private void rdBtnSlBstChforPoplSize_Checked(object sender, RoutedEventArgs e)
        {
            try
            {
                if (null != objReader)
                {
                    if ((bool)rdBtnSlBstChforPoplSize.IsChecked)
                    {
                        if ((bool)rdbtnOptmzExtmdNoTagsInField.IsChecked)
                        {
                            int tagPopulationCount = Convert.ToInt32(txtTagsNum.Text);
                            switch (GetBLFTagEncodingEncodedValue())
                            {
                                case 1:
                                    if ((tagPopulationCount >= 1) && (tagPopulationCount <= 250))
                                    {
                                        OptimalReaderSettings["/reader/gen2/session"] = "S0";
                                    }
                                    else if ((tagPopulationCount >= 251) && (tagPopulationCount <= 1000))
                                    {
                                        OptimalReaderSettings["/reader/gen2/session"] = "S1";
                                    }
                                    else if (tagPopulationCount > 1000)
                                    {
                                        OptimalReaderSettings["/reader/gen2/session"] = "S2";
                                    }
                                    break;
                                case 2:
                                    if ((tagPopulationCount >= 1) && (tagPopulationCount <= 125))
                                    {
                                        OptimalReaderSettings["/reader/gen2/session"] = "S0";
                                    }
                                    else if ((tagPopulationCount >= 126) && (tagPopulationCount <= 500))
                                    {
                                        OptimalReaderSettings["/reader/gen2/session"] = "S1";
                                    }
                                    else if (tagPopulationCount > 500)
                                    {
                                        OptimalReaderSettings["/reader/gen2/session"] = "S2";
                                    }
                                    break;
                                case 3:
                                    if ((tagPopulationCount >= 1) && (tagPopulationCount <= 100))
                                    {
                                        OptimalReaderSettings["/reader/gen2/session"] = "S0";
                                    }
                                    else if ((tagPopulationCount >= 101) && (tagPopulationCount <= 400))
                                    {
                                        OptimalReaderSettings["/reader/gen2/session"] = "S1";
                                    }
                                    else if (tagPopulationCount > 400)
                                    {
                                        OptimalReaderSettings["/reader/gen2/session"] = "S2";
                                    }
                                    break;
                                case 4:
                                    if ((tagPopulationCount >= 1) && (tagPopulationCount <= 50))
                                    {
                                        OptimalReaderSettings["/reader/gen2/session"] = "S0";
                                    }
                                    else if ((tagPopulationCount >= 51) && (tagPopulationCount <= 200))
                                    {
                                        OptimalReaderSettings["/reader/gen2/session"] = "S1";
                                    }
                                    else if (tagPopulationCount > 200)
                                    {
                                        OptimalReaderSettings["/reader/gen2/session"] = "S2";
                                    }
                                    break;
                            }
                        }
                        else
                        {
                            //Select session 1 if no tag population size has been declared
                            OptimalReaderSettings["/reader/gen2/session"] = "S1";
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Reader Error", MessageBoxButton.OK, MessageBoxImage.Error);
            };
        }

        /// <summary>
        /// Get blf tag encoding encoded value
        /// </summary>
        /// <returns></returns>
        private int GetBLFTagEncodingEncodedValue()
        {
            int retValue = 0;
            try
            {
                Gen2.LinkFrequency linkFrequency = (Gen2.LinkFrequency)objReader.ParamGet("/reader/gen2/BLF");
                Gen2.TagEncoding tagEncoding = (Gen2.TagEncoding)objReader.ParamGet("/reader/gen2/tagEncoding");
                if (((Gen2.LinkFrequency.LINK640KHZ == linkFrequency)
                    || (Gen2.LinkFrequency.LINK250KHZ == linkFrequency))
                    && (Gen2.TagEncoding.FM0 == tagEncoding))
                {
                    retValue = 1;
                }
                if (Gen2.LinkFrequency.LINK250KHZ == linkFrequency)
                {
                    if (Gen2.TagEncoding.M2 == tagEncoding)
                    {
                        retValue = 2;
                    }
                    else if (Gen2.TagEncoding.M4 == tagEncoding)
                    {
                        retValue = 3;
                    }
                    else if (Gen2.TagEncoding.M8 == tagEncoding)
                    {
                        retValue = 4;
                    }
                }
            }
            catch (Exception ex)
            {
                throw ex;
            };
            return retValue;
        }

        #region Custom Gen2 Settings

        /// <summary>
        /// Get all the current gen2 settings and set the radio buttons accordingly
        /// </summary>
        private void LoadGen2Settings()
        {
            //Get all the current settings and set the radio buttons accordingly
            // Astra doesn't support blf parameter
            if (!(model.Equals("Astra")))
            {
                LINK250KHZ.IsEnabled = true;
            }
            else
            {
                LINK250KHZ.IsEnabled = false;
            }
            LINK640KHZ.IsEnabled = false;
            try
            {
                switch (model)
                {
                    case "Mercury6":
                    case "Astra-EX":
                    case "M6e":
                    case "M6e Micro":
                    case "M6e PRC":
                        LINK250KHZ.IsEnabled = true;
                        LINK640KHZ.IsEnabled = true;
                        break;
                }
            }
            catch (ArgumentException ex)
            {
                Onlog("ArgumentException : "+ex.Message);
                Onlog(ex.StackTrace);
            }

            try
            {
                // Astra doesn't support blf parameter
                if (!(model.Equals("Astra")))
                {
                    switch (OptimalReaderSettings["/reader/gen2/BLF"])
                    {
                        case "LINK250KHZ":
                            LINK250KHZ.IsChecked = true; break;
                        case "LINK640KHZ":
                            LINK640KHZ.IsChecked = true; break;
                    }
                }
            }
            catch (ArgumentException ex)
            {
                Onlog("ArgumentException : " + ex.Message);
                Onlog(ex.StackTrace);
            }

            grpbxtTari.IsEnabled = false;
            tari625.IsEnabled = false;
            tari125.IsEnabled = false;
            tari25.IsEnabled = false;
            try
            {
                // Astra doesn't support tari parameter
                if (!(model.Equals("Astra")))
                {
                    grpbxtTari.IsEnabled = true;
                    switch (model)
                    {
                        case "Mercury6":
                        case "Astra-EX":
                        case "M6e":
                        case "M6e Micro":
                        case "M6e PRC":
                            tari625.IsEnabled = true;
                            tari125.IsEnabled = true;
                            tari25.IsEnabled = true;
                            break;
                    }
                    switch (OptimalReaderSettings["/reader/gen2/tari"])
                    {
                        case "TARI_6_25US":
                            tari625.IsChecked = true; break;
                        case "TARI_12_5US":
                            tari125.IsChecked = true; break;
                        case "TARI_25US":
                            tari25.IsChecked = true; break;
                    }
                }
            }
            catch (ArgumentException ex)
            {
                Onlog("ArgumentException : " + ex.Message);
                Onlog(ex.StackTrace);
            }
            catch (ReaderCodeException ex)
            {
                Onlog("ReaderCodeException : " + ex.Message);
                Onlog(ex.StackTrace);
            }

            try
            {
                // Astra doesn't support tagencoding parameter
                if (!(model.Equals("Astra")))
                {
                    if ((model.Equals("Mercury6")) || (model.Equals("Astra-EX"))
                                        || (model.Equals("M6e")) || (model.Equals("M6e Micro"))
                                        || (model.Equals("M6e PRC")))
                    {
                        FM0.IsEnabled = true;
                    }
                    else
                    {
                        FM0.IsEnabled = false;
                    }
                    M2.IsEnabled = true;
                    M4.IsEnabled = true;
                    M8.IsEnabled = true;
                    switch (OptimalReaderSettings["/reader/gen2/tagEncoding"])
                    {
                        case "FM0":
                            FM0.IsChecked = true; break;
                        case "M2":
                            M2.IsChecked = true; break;
                        case "M4":
                            M4.IsChecked = true; break;
                        case "M8":
                            M8.IsChecked = true; break;
                    }
                }
                else
                {
                    FM0.IsEnabled = false;
                    M2.IsEnabled = false;
                    M4.IsEnabled = false;
                    M8.IsEnabled = false;
                }
            }
            catch (ArgumentException)
            {
                FM0.IsEnabled = false;
                M2.IsEnabled = false;
                M4.IsEnabled = false;
                M8.IsEnabled = false;
            }
            try
            {
                switch (OptimalReaderSettings["/reader/gen2/session"])
                {
                    case "S0":
                        S0.IsChecked = true; break;
                    case "S1":
                        S1.IsChecked = true; break;
                    case "S2":
                        S2.IsChecked = true; break;
                    case "S3":
                        S3.IsChecked = true; break;
                }
            }
            catch (ArgumentException)
            {
                S0.IsEnabled = false;
                S1.IsEnabled = false;
                S2.IsEnabled = false;
                S3.IsEnabled = false;
            }
            try
            {
                switch (OptimalReaderSettings["/reader/gen2/target"])
                {
                    case "A":
                        A.IsChecked = true; break;
                    case "B":
                        B.IsChecked = true; break;
                    case "AB":
                        AB.IsChecked = true; break;
                    case "BA":
                        BA.IsChecked = true; break;
                }
            }
            catch (FeatureNotSupportedException)
            {
                A.IsEnabled = false;
                B.IsEnabled = false;
                AB.IsEnabled = false;
                BA.IsEnabled = false;
            }
            try
            {
                 // Astra doesn't support tagencoding parameter
                if (!(model.Equals("Astra")))
                {
                    DynamicQ.IsEnabled = true;
                    StaticQ.IsEnabled = true;
                    if (OptimalReaderSettings["/reader/gen2/q"] == "DynamicQ")
                    {
                        DynamicQ.IsChecked = true;
                        Qvalue.SelectedIndex = -1;
                    }
                    else if (OptimalReaderSettings["/reader/gen2/q"] == "StaticQ")
                    {
                        StaticQ.IsChecked = true;
                        Qvalue.IsEnabled = true;
                        Qvalue.SelectedIndex = Convert.ToInt32(
                            OptimalReaderSettings["/application/performanceTuning/staticQValue"]);
                    }
                }
                else
                {
                    StaticQ.IsEnabled = false;
                    DynamicQ.IsEnabled = false;
                    Qvalue.IsEnabled = false;
                }
            }
            catch (FeatureNotSupportedException)
            {
                StaticQ.IsEnabled = false;
                DynamicQ.IsEnabled = false;
            }

            try
            {
                sldrReadPwr.Value = (double)(Convert.ToDouble(
                    objReader.ParamGet("/reader/radio/readPower").ToString()) / 100);
            }
            catch (Exception ex)
            {               
               Onlog("Read power exception : " + ex.Message);
            }

            try
            {
                chkbxAntennaDetection.IsChecked = (bool)objReader.ParamGet("/reader/antenna/checkPort");
                if (model.Equals("M6e Micro"))
                {
                    // M6e-micro doesn't support Antenna Detection. Users must always pick the
                    // connected antenna and start the read.
                    chkbxAntennaDetection.IsEnabled = false;
                }
                else
                {
                    chkbxAntennaDetection.IsEnabled = true;
                }
            }
            catch (FeatureNotSupportedException)
            {
                chkbxAntennaDetection.IsEnabled = false;
            }
            catch (ArgumentException)
            {
                chkbxAntennaDetection.IsEnabled = false;
            }           
        }

        /// <summary>
        /// Save the set gen2 settings. So that these can be set on the
        /// connected reader when read is performed
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnSaveGen2Settings_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (initialReaderSettingsLoaded && ((bool)chkCustomizeGen2Settings.IsChecked))
                {
                    Mouse.SetCursor(Cursors.Wait);
                    //LINK250KHZ_CheckedChanged
                    if ((bool)LINK250KHZ.IsChecked)
                    {
                        switch (model)
                        {
                            case "Mercury6":
                            case "Astra-EX":
                            case "M6e":
                            case "M6e PRC":
                            case "M6e Micro":
                                FM0.IsEnabled = true;
                                tari625.IsEnabled = true;
                                tari125.IsEnabled = true;
                                tari25.IsEnabled = true;
                                break;
                            default:
                                FM0.IsEnabled = false;
                                break;
                        }
                        //Work around for bug #2063 for Paine release
                        M2.IsEnabled = true;
                        M4.IsEnabled = true;
                        M8.IsEnabled = true;
                        OptimalReaderSettings["/reader/gen2/BLF"] = "LINK250KHZ";
                    }                
                    //LINK640KHZ_CheckedChanged
                    if ((bool)LINK640KHZ.IsChecked)
                    {
                        if ((bool)grpbxtTari.IsEnabled)
                        {
                            tari625.IsEnabled = true;
                            tari125.IsEnabled = false;
                            tari25.IsEnabled = false;
                            tari625.IsChecked = true;
                        }
                        //Work around for bug #2063 for Paine release
                        FM0.IsEnabled = true;
                        M2.IsEnabled = false;
                        M4.IsEnabled = false;
                        M8.IsEnabled = false;
                        FM0.IsChecked = true;
                        OptimalReaderSettings["/reader/gen2/BLF"] = "LINK640KHZ";
                    }

                    //tari625_CheckedChanged
                    if ((bool)tari625.IsChecked)
                    {
                        OptimalReaderSettings["/reader/gen2/tari"] = "TARI_6_25US";
                    }

                    //tari125_CheckedChanged
                    if ((bool)tari125.IsChecked)
                    {
                        OptimalReaderSettings["/reader/gen2/tari"] = "TARI_12_5US";
                    }

                    //tari25_CheckedChanged
                    if ((bool)tari25.IsChecked)
                    {
                        OptimalReaderSettings["/reader/gen2/tari"] = "TARI_25US";
                    }

                    //FM0_CheckedChanged
                    if ((bool)FM0.IsChecked)
                    {
                        OptimalReaderSettings["/reader/gen2/tagEncoding"] = "FM0";
                    }

                    //M2_CheckedChanged
                    if ((bool)M2.IsChecked)
                    {
                        OptimalReaderSettings["/reader/gen2/tagEncoding"] = "M2";
                    }

                    //M4_CheckedChanged
                    if ((bool)M4.IsChecked)
                    {
                        OptimalReaderSettings["/reader/gen2/tagEncoding"] = "M4";
                    }

                    //M8_CheckedChanged
                    if ((bool)M8.IsChecked)
                    {
                        OptimalReaderSettings["/reader/gen2/tagEncoding"] = "M8";
                    }

                    //S0_CheckedChanged
                    if ((bool)S0.IsChecked)
                    {
                        OptimalReaderSettings["/reader/gen2/session"] = "S0";
                    }

                    //S1_CheckedChanged
                    if ((bool)S1.IsChecked)
                    {
                        OptimalReaderSettings["/reader/gen2/session"] = "S1";
                    }

                    //S2_CheckedChanged
                    if ((bool)S2.IsChecked)
                    {
                        OptimalReaderSettings["/reader/gen2/session"] = "S2";
                    }

                    //S3_CheckedChanged
                    if ((bool)S3.IsChecked)
                    {
                        OptimalReaderSettings["/reader/gen2/session"] = "S3";
                    }

                    //A_CheckedChanged
                    if ((bool)A.IsChecked)
                    {
                        OptimalReaderSettings["/reader/gen2/target"] = "A";
                    }

                    //B_CheckedChanged
                    if ((bool)B.IsChecked)
                    {
                        OptimalReaderSettings["/reader/gen2/target"] = "B";
                    }

                    //AB_CheckedChanged
                    if ((bool)AB.IsChecked)
                    {
                        OptimalReaderSettings["/reader/gen2/target"] = "AB";
                    }

                    //BA_CheckedChanged
                    if ((bool)BA.IsChecked)
                    {
                        OptimalReaderSettings["/reader/gen2/target"] = "BA";
                    }

                    //DynamicQ_CheckedChanged
                    if ((bool)DynamicQ.IsChecked)
                    {
                        OptimalReaderSettings["/reader/gen2/q"] = "DynamicQ";
                        Qvalue.IsEnabled = false;
                    }

                    //StaticQ_CheckedChanged
                    if (((bool)StaticQ.IsChecked) && (Qvalue.SelectedIndex == -1))
                    {
                        //Enable Q value combo-box
                        Qvalue.IsEnabled = true;
                        OptimalReaderSettings["/reader/gen2/q"] = "StaticQ";
                        OptimalReaderSettings["/application/performanceTuning/staticQValue"] = "0";
                    }
                    else if (((bool)StaticQ.IsChecked == true) && (Qvalue.SelectedIndex != -1))
                    {
                        //Enable Q value combo-box
                        Qvalue.IsEnabled = true;
                        //Qvalue_SelectedIndexChanged
                        OptimalReaderSettings["/reader/gen2/q"] = "StaticQ";
                        OptimalReaderSettings["/application/performanceTuning/staticQValue"] = Qvalue.SelectedIndex.ToString();
                    }

                    ////WORD_ONLY_CheckedChanged
                    //if ((bool)WORD_ONLY.IsChecked)
                    //{
                    //    reader.ParamSet("/reader/gen2/writeMode", Gen2.WriteMode.WORD_ONLY);
                    //}

                    ////BLOCK_ONLY_CheckedChanged
                    //if ((bool)BLOCK_ONLY.IsChecked)
                    //{
                    //    reader.ParamSet("/reader/gen2/writeMode", Gen2.WriteMode.BLOCK_ONLY);
                    //}

                    ////BLOCK_FALLBACK_CheckedChanged
                    //if ((bool)BLOCK_FALLBACK.IsChecked)
                    //{
                    //    reader.ParamSet("/reader/gen2/writeMode", Gen2.WriteMode.BLOCK_FALLBACK);
                    //}

                    Mouse.SetCursor(Cursors.Arrow);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message 
                    + " Check supported protocol configurations in Reader's Hardware Guide.",
                    "Unsupported Reader Configuration", MessageBoxButton.OK, MessageBoxImage.Error);
                Mouse.SetCursor(Cursors.Arrow);
                LoadGen2Settings();
            }
        }

        #endregion

        /// <summary>
        /// Set reader read power slider
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void sldrReadPwr_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            try
            {
                if (null != objReader)
                {
                    Mouse.SetCursor(Cursors.Wait);
                    objReader.ParamSet("/reader/radio/readPower", Convert.ToInt32(100 * sldrReadPwr.Value));
                    Mouse.SetCursor(Cursors.Arrow);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message 
                    + " Check supported protocol configurations in Reader's Hardware Guide.",
                    "Unsupported Reader Configuration", MessageBoxButton.OK, MessageBoxImage.Error);
                Mouse.SetCursor(Cursors.Arrow);
                LoadGen2Settings();
            }
        }

        /// <summary>
        /// Populate the custom gen2 settings, save the settings set and disable the auto settings
        /// in performance tunning section
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkCustomizeGen2Settings_Checked(object sender, RoutedEventArgs e)
        {
            if ((bool)chkCustomizeGen2Settings.IsChecked)
            {
                gridPanleCustomizeGen2Settings.Visibility = System.Windows.Visibility.Visible;
                stackPanelCustomizeGen2settings.IsEnabled = true;                
                gridGen2PerformanceTuning1.IsEnabled = false;
                LoadGen2Settings();
                settingsScrollviewer.ScrollToVerticalOffset(2000);
            }
            else
            {
                gridPanleCustomizeGen2Settings.Visibility = System.Windows.Visibility.Collapsed;
                stackPanelCustomizeGen2settings.IsEnabled = false;
                gridGen2PerformanceTuning1.IsEnabled = true;
            }
        }

        /// <summary>
        /// Populate the custom gen2 settings
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void tglBtnDisplayGen2Settings_Checked(object sender, RoutedEventArgs e)
        {
            if ((bool)tglBtnDisplayGen2Settings.IsChecked)
            {                
                gridPanleCustomizeGen2Settings.Visibility = System.Windows.Visibility.Visible;
                LoadGen2Settings();
                settingsScrollviewer.ScrollToVerticalOffset(1500);
            }
            else
            {
                gridPanleCustomizeGen2Settings.Visibility = System.Windows.Visibility.Collapsed;
            }
        }

        #endregion Gen2PerformanceTuning

        #region ReaderDiagnostics
        /// <summary>
        /// Initialize reader diagnostics
        /// </summary>
        private void InitializeRdrDiagnostics()
        {
            try
            {
                // Create a FlowDocument to contain content for the RichTextBox.
                FlowDocument myFlowDoc = new FlowDocument();
                if (lblshowStatus.Content.ToString() == "Disconnected")
                {
                    //myFlowDoc.Blocks.Add(new Paragraph(new Run("RFID Engine: ")));
                    //myFlowDoc.Blocks.Add(new Paragraph(new Run("Firmware Version:")));
                    //myFlowDoc.Blocks.Add(new Paragraph(new Run("Hardware Version: ")));
                    myFlowDoc.Blocks.Add(new Paragraph(
                        new Run("Universal Reader Assistant Version: " 
                            + this.GetType().Assembly.GetName().Version.ToString())));
                    myFlowDoc.Blocks.Add(new Paragraph(
                        new Run("Mercury API Version: " + Assembly.GetAssembly(
                            typeof(ThingMagic.Reader)).GetName().Version.ToString())));
                }
                else
                {
                    // Add paragraphs to the FlowDocument.
                    //RFID Engine
                    myFlowDoc.Blocks.Add(new Paragraph(new Run("RFID Engine: " + model)));
                    //Get version information
                    string fwver = (string)objReader.ParamGet("/reader/version/software");
                    if (objReader is RqlReader)
                    {
                        int index = ((string)fwver).LastIndexOf("build");
                        myFlowDoc.Blocks.Add(new Paragraph(new Run("Firmware Version: " 
                            + fwver.Substring(0, index - 1))));
                    }
                    else if (objReader is SerialReader)
                    {
                        myFlowDoc.Blocks.Add(new Paragraph(new Run("Firmware Version: " 
                            + fwver.Substring(1, 10))));
                    }
                    else if (objReader is LlrpReader)
                    {
                        myFlowDoc.Blocks.Add(new Paragraph(new Run("Firmware Version: " + fwver)));
                    }

                    string[] hVersion;
                    if (!(model.Equals("Astra")))
                    {
                        hVersion = ((string)objReader.ParamGet("/reader/version/hardware")).Split('-');
                        myFlowDoc.Blocks.Add(new Paragraph(new Run("Hardware Version: " 
                            + hVersion[0].ToString())));
                    }
                    else
                    {
                        myFlowDoc.Blocks.Add(new Paragraph(new Run("Hardware Version: " + "-")));
                    }
                    myFlowDoc.Blocks.Add(new Paragraph(
                        new Run("Universal Reader Assistant Version: "
                            + this.GetType().Assembly.GetName().Version.ToString())));
                    myFlowDoc.Blocks.Add(new Paragraph(
                        new Run("Mercury API Version: " + Assembly.GetAssembly(
                            typeof(ThingMagic.Reader)).GetName().Version.ToString())));
                }

                RichTextBox myRichTextBox = new RichTextBox();

                // Add initial content to the RichTextBox.            
                richTxtBxRdrDiagnostics.Document = myFlowDoc;
            }
            catch (Exception ex)
            {
                throw ex;
            }
        }
        #endregion ReaderDiagnostics

        #region FirmwareUpdate

        //progress bar start flag
        bool prgStart = false;
        //progress bar stop flag
        bool prgStop = false;
        // Cache exception if any while upgrading the firmware for future use
        Exception exceptionCrc = null;
        private Thread progressStatus = null;
        // Open file dialog
        OpenFileDialog openFile = new OpenFileDialog();

        /// <summary>
        /// Open file dialog to select firmware for the connected reader
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnChooseFirmware_Click(object sender, RoutedEventArgs e)
        {
            openFile.ShowDialog();
            txtFirmwarePath.Text = openFile.FileName.ToString();
        }

        /// <summary>
        /// Update the firmware on the connected reader
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnUpdate_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (null != objReader)
                {
                    objReader.Destroy();
                    objReader = null;
                }
                try
                {
                    // Set progress bar style to indeterminate 
                    progressBar.IsIndeterminate = true;
                    // Based on the connection type, try connecting to the reader automatically
                    // if user has not connected initially
                    if ((bool)rdbtnLocalConnection.IsChecked)
                    {
                        if (!ValidatePortNumber(cmbReaderAddr.Text))
                        {
                            throw new IOException();
                        }
                        if (cmbReaderAddr.Text == "")
                        {
                            throw new IOException();
                        }
                        //Creates a Reader Object for operations on the Reader.
                        string readerUri = cmbReaderAddr.Text;
                        //Regular Expression to get the com port number from comport name .
                        //for Ex: If The Comport name is "USB Serial Port (COM19)" by using this 
                        // regular expression will get com port number as "COM19".
                        MatchCollection mc = Regex.Matches(readerUri, @"(?<=\().+?(?=\))");
                        foreach (Match m in mc)
                        {
                            readerUri = m.ToString();
                        }
                        objReader = Reader.Create(string.Concat("tmr:///", readerUri));
                    }
                    else
                    {
                        //Creates a Reader Object for operations on the Reader.
                        objReader = Reader.Create(string.Concat("tmr://", cmbFixedReaderAddr.Text));
                    }
                }
                catch (System.IO.IOException)
                {
                    if (!cmbReaderAddr.Text.Contains("COM"))
                    {
                        MessageBox.Show("Application needs a valid Reader Address of type COMx",
                            "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Error);
                    }
                    else
                    {
                        MessageBox.Show("Reader not connected on " + cmbReaderAddr.Text,
                            "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Error);
                    }
                    return;
                }
                try
                {
                    // connect to the reader
                    objReader.Connect();
                    // Set the region
                    if (Reader.Region.UNSPEC.Equals(objReader.ParamGet("/reader/region/id")))
                    {
                        objReader.ParamSet("/reader/region/id", Reader.Region.NA);
                    }
                }
                catch (FAULT_BL_INVALID_IMAGE_CRC_Exception ex)
                {
                    // Ignore INVALID_IMAGE here so we can reach FirmwareLoad --
                    // will be checked later in SendInitCommands, anyway
                    Onlog("FAULT_BL_INVALID_IMAGE_CRC_Exception : "+ex.Message);
                    Onlog(ex.StackTrace);
                }
                catch (Exception ex)
                {
                    Onlog(ex.Message);
                    MessageBox.Show(ex.Message, "Universal Reader Assistant Message",
                        MessageBoxButton.OK, MessageBoxImage.Error);
                    return;
                }
                if (txtFirmwarePath.Text == "")
                {
                    MessageBox.Show("Please select a firmware to load", 
                        "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Error);
                    return;
                }
                if ((bool)rdbtnLocalConnection.IsChecked)
                {
                    if (!txtFirmwarePath.Text.Contains(".sim"))
                    {
                        MessageBox.Show("Invalid File Extension", "Universal Reader Assistant Message",
                            MessageBoxButton.OK, MessageBoxImage.Error);
                        return;
                    }
                }
                if ((bool)rdbtnNetworkConnection.IsChecked)
                {
                    if (!txtFirmwarePath.Text.Contains(".tmfw"))
                    {
                        MessageBox.Show("Invalid File Extension", "Universal Reader Assistant Message",
                            MessageBoxButton.OK, MessageBoxImage.Error);
                        return;
                    }
                }
                if (null != objReader)
                {
                    // Disable the UI controls when ura is loading the firmware
                    btnUpdate.IsEnabled = false;
                    btnChooseFirmware.IsEnabled = false;
                    tcTagResults.IsEnabled = false;
                    lblMessage.Text = "";
                    prgStart = false;
                    prgStop = false;
                    // Cache the exception when thrown for future use
                    exceptionCrc = null;
                    // Set progress bar to initial stage
                    progressBar.Dispatcher.Invoke(new del1(updateProgressBar), new object[] { 0 });
                    System.IO.FileStream firmware = new System.IO.FileStream(txtFirmwarePath.Text,
                        System.IO.FileMode.Open, System.IO.FileAccess.Read);

                    Thread updateProgress = new Thread(delegate()
                    {
                        //Wait till updateTrd thread starts
                        while (!prgStart)
                        {
                            Thread.Sleep(10);
                        }
                        //After updateTrd thread starts, prgStart will be true
                        while (prgStart)
                        {
                            startProgress();
                        }
                        //If no exception received call stopProgress method
                        if (null == exceptionCrc)
                        {
                            stopProgress();
                        }
                    });

                    Thread updateTrd = new Thread(delegate()
                    {
                        try
                        {
                            prgStart = true;
                            Dispatcher.BeginInvoke(new ThreadStart(delegate()
                                {
                                    stackPanelFrmwrUpdatePrgss.Visibility = System.Windows.Visibility.Visible;
                                    //disable all the expanders when firmware Update is in progress
                                    //gridConnect.IsEnabled = false;
                                    //gridReadOptions.IsEnabled = false;
                                    //gridPerformanceMetrics.IsEnabled = false;
                                    //gridPerformanceTuning.IsEnabled = false;
                                    //gridDisplayOptions.IsEnabled = false;
                                    //gridRdrDiagnostics.IsEnabled = false;
                                    EnableDisableExpanderControl(false);
                                    btnRead.IsEnabled = false;
                                }));
                            // Load the firmware on to the connected reader
                            objReader.FirmwareLoad(firmware);
                            prgStart = false;
                            // Update the firmware Update status
                            lblMessage.Dispatcher.Invoke(new del2(updateStatus));
                        }
                        catch (Exception ex)
                        {
                            //Freeze the progress-bar status when an exception is caught 
                            prgStart = false;
                            // Cache the exception thrown
                            exceptionCrc = ex;
                            prgStop = true;
                            start = false;
                            progressStatus = null;
                            if (ex is FAULT_BL_INVALID_IMAGE_CRC_Exception)
                            {
                                //MessageBox.Show("Firmware Update failed : " + exceptionCrc.Message.ToString(), "Error!", MessageBoxButton.OK, MessageBoxImage.Error);
                                if (isFirmwareUpdateFailed)
                                {
                                    //Firmware Update failed, disconnect the reader.
                                    Dispatcher.BeginInvoke(new ThreadStart(delegate() {
                                        btnConnect_Click(this, new RoutedEventArgs()); }));
                                    stopProgress();
                                }
                                this.Dispatcher.BeginInvoke(new ThreadStart(delegate()
                                {
                                    progressBar.IsIndeterminate = false;
                                    progressBar.InvalidateVisual();
                                }));
                            }
                            else
                            {
                                MessageBox.Show("Error connecting to reader: " +
                                    exceptionCrc.Message.ToString(), "Universal Reader Assistant Message",
                                    MessageBoxButton.OK, MessageBoxImage.Error);
                                stopProgress();
                            }
                            Thread reportError = new Thread(delegate()
                            {
                                lblMessage.Dispatcher.Invoke(new del2(updateFailedStatus));

                            });
                            reportError.Start();
                        }
                        finally 
                        {
                            if (isFirmwareUpdateFailed)
                            {
                                MessageBox.Show("Firmware Update failed : " + exceptionCrc.Message.ToString(),
                                    "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Error);
                            }
                        }
                    });
                    updateProgress.Start();
                    updateTrd.Start();
                }
            }
            catch (System.IO.IOException ex)
            {
                MessageBox.Show(ex.Message.ToString(), "Universal Reader Assistant Message",
                    MessageBoxButton.OK, MessageBoxImage.Error);
                btnUpdate.IsEnabled = true;
                btnChooseFirmware.IsEnabled = true;
                tcTagResults.IsEnabled = true;
            }
            catch (ReaderCodeException ex)
            {
                MessageBox.Show("Error connecting to reader: " + ex.Message.ToString(),
                    "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Error);
                lblMessage.Text = "Update Failed";
                progressBar.Dispatcher.Invoke(new del1(updateProgressBar), new object[] { 0 });
                btnUpdate.IsEnabled = true;
                btnChooseFirmware.IsEnabled = true;
                tcTagResults.IsEnabled = true;
            }
            catch (System.UnauthorizedAccessException)
            {
                MessageBox.Show("Access to " + " denied. Please check if another program is "
                    + " accessing this port", "Universal Reader Assistant Message", MessageBoxButton.OK,
                    MessageBoxImage.Error);
                btnUpdate.IsEnabled = true;
                btnChooseFirmware.IsEnabled = true;
                tcTagResults.IsEnabled = true;
            }
        }

        bool start;
        /// <summary>
        /// Start the progress bar
        /// </summary>
        private void startProgress()
        {
            start = true;
            if (progressStatus == null)
            {
                this.progressStatus = new Thread(ProgressBarWork);
                this.progressStatus.IsBackground = true;
                this.progressStatus.Start();
            }
        }

        /// <summary>
        /// Stop the progress bar
        /// </summary>
        private void stopProgress()
        {
            start = false;
            if (progressStatus != null)
            {
                progressStatus.Join(100);
                progressStatus = null;
            }
        }

        /// <summary>
        /// Delegate with parameter
        /// </summary>
        /// <param name="x"></param>
        delegate void del1(int x);

        /// <summary>
        /// Delegate without parameter
        /// </summary>
        delegate void del2();

        /// <summary>
        /// Update progress bar
        /// </summary>
        /// <param name="y">value</param>
        void updateProgressBar(int y)
        {
            progressBar.Value = y;
        }

        /// <summary>
        /// Update the firmware Update status if successful
        /// </summary>
        void updateStatus()
        {
            //Update the status to Update successful
            lblMessage.Text = "Update Successful";
            lblMessage.FontWeight = FontWeights.Regular;
            lblMessage.Foreground = Brushes.Black;
            isFirmwareUpdateFailed = false;
            MessageBox.Show("Please restart Universal Reader Assistant",
                "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Information);
            Dispatcher.BeginInvoke(new ThreadStart(delegate()
                {
                    stackPanelFrmwrUpdatePrgss.Visibility = System.Windows.Visibility.Collapsed;
                    //enable all the expanders when firmware Update is completed
                    //gridConnect.IsEnabled = true;
                    //gridReadOptions.IsEnabled = true;
                    //gridPerformanceMetrics.IsEnabled = true;
                    //gridPerformanceTuning.IsEnabled = true;
                    //gridDisplayOptions.IsEnabled = true;
                    //gridRdrDiagnostics.IsEnabled = true;
                    EnableDisableExpanderControl(true);
                    btnRead.IsEnabled = true;
                }));
            progressBar.Dispatcher.Invoke(new del1(updateProgressBar), new object[] { 0 });
            lblMessage.Text = "";
            txtFirmwarePath.Text = "";
            btnUpdate.IsEnabled = true;
            btnChooseFirmware.IsEnabled = true;
            tcTagResults.IsEnabled = true;
            //Firmware Update failed, disconnect the reader.
            this.Dispatcher.BeginInvoke(new ThreadStart(delegate() {
                btnConnect.Content = "Disconnect";
                btnConnect_Click(this, new RoutedEventArgs());
                objReader = null;
            }));
        }

        /// <summary>
        /// Enable or disable expander ui controls
        /// </summary>
        /// <param name="flag"></param>
        private void EnableDisableExpanderControl(bool flag)
        {
            gridConnect.IsEnabled = flag;
            gridReadOptions.IsEnabled = flag;
            gridPerformanceMetrics.IsEnabled = flag;
            gridPerformanceTuning.IsEnabled = flag;
            gridDisplayOptions.IsEnabled = flag;
            gridRdrDiagnostics.IsEnabled = flag;
        }

        /// <summary>
        /// Update the firmware Update status if failed
        /// </summary>
        void updateFailedStatus()
        {
            //Update the status to Update failed
            progressBar.Dispatcher.Invoke(new del1(updateProgressBar), new object[] { 0 });
            lblMessage.Text = "Update Failed";
            lblMessage.FontWeight = FontWeights.Bold;
            lblMessage.Foreground = Brushes.Red;
            Dispatcher.BeginInvoke(new ThreadStart(delegate()
            {
                stackPanelFrmwrUpdatePrgss.Visibility = System.Windows.Visibility.Collapsed;
                //enable connect expander when firmware Update is failed. So that user can 
                // connect to another reader
                gridConnect.IsEnabled = true;
            }));
            isFirmwareUpdateFailed = true;
            btnUpdate.IsEnabled = true;
            btnChooseFirmware.IsEnabled = true;
            tcTagResults.IsEnabled = true;
            //if (null != objReader)
            //{
            //    objReader.Destroy();
            //    objReader = null;
            //}
            //Firmware Update failed, disconnect the reader.
            this.Dispatcher.BeginInvoke(new ThreadStart(delegate()
            {
                btnConnect.Content = "Disconnect";
                btnConnect_Click(this, new RoutedEventArgs());
                objReader = null;
            }));
        }

        /// <summary>
        /// Progress bar implementation
        /// </summary>
        private void ProgressBarWork()
        {
            Thread.Sleep(1000);
            int i = 0;
            while (i < 101)
            {
                progressBar.Dispatcher.Invoke(new del1(updateProgressBar), new object[] { i });
                //start = true, when firmware Update started.
                if (start)
                {
                    //If no exception caught, prgStop = false
                    if (!prgStop)
                    {
                        i++;
                    }
                }
                else
                {
                    if (prgStop)
                    {
                        //If exception caught, prgStop = true, come out of the while loop,
                        //firmware Update failed
                        break;
                    }
                    else
                    {
                        //start = false and prgStop = false, when firmware Update done
                        //successfully.
                        i = 102;
                        double maxvalue = 0;
                        progressBar.Dispatcher.Invoke(new ThreadStart(delegate() {
                            maxvalue = progressBar.Maximum; }));
                        progressBar.Dispatcher.Invoke(new del1(updateProgressBar),
                            new object[] {Convert.ToInt32( maxvalue) });
                    }
                }
                Thread.Sleep(350);
            }
        }

        #endregion

        /// <summary>
        /// Delegate for updating Read Tag ID text box
        /// </summary>
        /// <param name="data">String Data</param>
        delegate void OutputUpdateDelegate(TagReadData[] griddata);

        /// <summary>
        /// Class to sort com port serially
        /// </summary>
        class OrderNumeric : IComparer<string>
        {
            public int Compare(string strA, string strB)
            {
                int idA = int.Parse(strA.Substring(3)),
                idB = int.Parse(strB.Substring(3));

                return idA.CompareTo(idB);
            }
        }
        public uint startAddressToUC = 0;
        public int readLengthToUC = 0;
        public Gen2.Bank memBankToUC = Gen2.Bank.EPC;

        /// <summary>
        /// Cache reader settings such as start address, read length, gen2 memory bank
        /// </summary>
        private void CacheReadDataSettings()
        {

            if (!((bool)chkEmbeddedReadData.IsChecked))
            {
                startAddressToUC = 0;
                readLengthToUC = 0;
                memBankToUC = Gen2.Bank.EPC;
                lblhiddenembeddedReadvalue.Content = "";
            }
            else
            {
                startAddressToUC = Convert.ToUInt32( Utilities.CheckHexOrDecimal(txtembReadStartAddr.Text));
                readLengthToUC = Convert.ToInt32(Utilities.CheckHexOrDecimal(txtembReadLength.Text));
                switch (cbxReadDataBank.Text)
                {
                    case "EPC":
                        memBankToUC = Gen2.Bank.EPC;
                        lblhiddenembeddedReadvalue.Content = "";
                        break;
                    case "TID":
                        memBankToUC = Gen2.Bank.TID;
                        lblhiddenembeddedReadvalue.Content = "";
                        break;
                    case "User":
                        memBankToUC = Gen2.Bank.USER;
                        lblhiddenembeddedReadvalue.Content = "";
                        break;
                    case "Reserved":
                        lblhiddenembeddedReadvalue.Content = "Reserved";
                        break;
                };
            }
        }

        /// <summary>
        /// Context menu options i.e list of tabs
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void MenuItem_Click(object sender, RoutedEventArgs e)
        {
            TagReadRecord tagRead = (TagReadRecord)TagResults.dgTagResults.SelectedItem;            
            ctMenu.Visibility = System.Windows.Visibility.Collapsed;
            expdrReadOptions.IsEnabled = false;
            btnRead.IsEnabled = false;
            string menuItem = ((System.Windows.Controls.MenuItem)sender).Header.ToString();
            switch(menuItem)
            {
                case"Write EPC":            
                    tiWriteEPC.Focus();
                    tiWriteEPC.Visibility = System.Windows.Visibility.Visible;                
                    WriteEpc.Load(objReader, startAddressToUC, readLengthToUC, memBankToUC, tagRead);
                    break;

                case "Inspect Tag":
                    tiTagInspector.Focus();
                    tiTagInspector.Visibility = System.Windows.Visibility.Visible;
                    TagInspector.LoadTagInspector(objReader, startAddressToUC, memBankToUC, tagRead, model);
                    break;

                case "Edit User Memory":
                    tiUserMemory.Focus();
                    tiUserMemory.Visibility = System.Windows.Visibility.Visible;
                    UserMemory.LoadUserMemory(objReader, startAddressToUC, memBankToUC, tagRead, model);
                    break;

                case "Lock Tag":
                    tiLockTag.Focus();
                    tiLockTag.Visibility = System.Windows.Visibility.Visible;
                    LockTag.LoadReservedMemory(objReader, startAddressToUC, memBankToUC, tagRead, model);
                    break;
            }
        }

        /// <summary>
        /// Write EPC tab initialization
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void tiWriteEPC_GotFocus(object sender, RoutedEventArgs e)
        {
            WriteEpc.LoadEPC(objReader);
            if (lblshowStatus.Content.ToString() == "Reading")
            {
                tiTagResults.Focus();
                return;                
            }
            if(btnRead.Visibility == System.Windows.Visibility.Visible)
            {
                WriteEpc.spWriteEPC.IsEnabled = true;
            }
            btnRead.IsEnabled = false;

            //Disable clear tag results button and save on tool bar when tabs other then Tag Results is clicked
            btnClearTagReads.IsEnabled = false;
            saveData.IsEnabled = false;

            iso6bCheckBox.IsEnabled = false;
            ipx64CheckBox.IsEnabled = false;
            ipx256CheckBox.IsEnabled = false;
            ataCheckBox.IsEnabled = false;
            TagResults.dgTagResults.UnselectAll();
        }

        /// <summary>
        /// Lock tag tab initialization
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void tiLockTag_GotFocus(object sender, RoutedEventArgs e)
        {
            if (lblshowStatus.Content.ToString() == "Reading")
            {
                tiTagResults.Focus();
                return;
            }
            if (btnRead.Visibility == System.Windows.Visibility.Visible)
            {
                LockTag.spLockTag.IsEnabled = true;
            }
            btnRead.IsEnabled = false;
            //Disable clear tag results button on tool-bar when tabs other then Tag Results is clicked
            btnClearTagReads.IsEnabled = false;
            saveData.IsEnabled = false;
            TagResults.dgTagResults.UnselectAll();
            LockTag.LoadReservedMemory(objReader, model);
        }

        /// <summary>
        /// Tag results tab initialization
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void tiTagResults_GotFocus(object sender, RoutedEventArgs e)
        {
            expdrReadOptions.IsEnabled = true;
            // Don't enable the read button, when sync read or async read is in progress
            if (!(isSyncReadGoingOn || isAsyncReadGoingOn))
            {
                btnRead.IsEnabled = true;
            }
            //Disable clear tag results button on tool-bar when tabs other then Tag Results is clicked
            btnClearTagReads.IsEnabled = true;
            saveData.IsEnabled = true;
            iso6bCheckBox.IsEnabled = true;
            ipx64CheckBox.IsEnabled = true;
            ipx256CheckBox.IsEnabled = true;
            ataCheckBox.IsEnabled = true;
            
            //Set the default values of WriteEPC tab
            WriteEpc.ResetWriteEPCTab();

            //Set the default values of TagInspector tab
            TagInspector.ResetTagInspectorTab();

            //set the default values for UserMemory tab
            UserMemory.ResetUserMemoryTab();

            //set the default values for LockTag tab
            LockTag.ResetLockTagTab();
        }   

        /// <summary>
        /// Open connect expander
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void expdrConnect_Expanded(object sender, RoutedEventArgs e)
        {
            try
            {
                if (btnConnect.Content.ToString().Equals("Connect"))
                {
                    InitializeReaderUriBox();
                    btnConnectExpander.Content = btnConnect.Content.ToString() + "...";
                }
                else
                {
                    btnConnectExpander.Content = btnConnect.Content.ToString() + "...";
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        /// <summary>
        /// Optimize estimated number of tags
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void rdbtnOptmzExtmdNoTagsInField_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                //set q value
                if ((bool)rdbtnOptmzExtmdNoTagsInField.IsChecked)
                {
                    int tag_pop_adjust_num_tags = Convert.ToInt32(txtTagsNum.Text.ToString());
                    double num_tags_float = tag_pop_adjust_num_tags * 1.5;
                    int count = 0;
                    double num = 1.0;
                    for (; ; )
                    {
                        num = num * 2.0;
                        if (num > num_tags_float)
                            break;
                        count++;
                    }
                    if (num_tags_float > 32768)
                    {
                        count = 15;
                    }
                    OptimalReaderSettings["/reader/gen2/q"] = "StaticQ";
                    StaticQ.IsChecked = true;
                    OptimalReaderSettings["/application/performanceTuning/staticQValue"] = count.ToString();
                    Qvalue.SelectedIndex = count;
                    rdBtnSlBstChforPoplSize_Checked(sender, e);
                }
            }
        }

        /// <summary>
        /// Initialize unique by data ui controls
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkUniqueByData_Checked(object sender, RoutedEventArgs e)
        {
            if (null != objReader)
            {
                tagdb.chkbxUniqueByData = (bool)chkUniqueByData.IsChecked;
                tagdb.chkbxShowFailedDataReads = (bool)chkShowFailedDataReads.IsChecked;
                if (!((bool)chkUniqueByData.IsChecked))
                {
                    chkShowFailedDataReads.IsChecked = chkUniqueByData.IsChecked;
                }
            }
        }

        /// <summary>
        /// Allow only hex string in the textbox
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnPreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            int hexNumber;
            e.Handled = !int.TryParse(e.Text, NumberStyles.HexNumber, CultureInfo.CurrentCulture, out hexNumber);
        }

        /// <summary>
        /// Change filter start address based on the memory selected in the 
        /// filter memory bank combo-box selection 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cbxFilterMemBank_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (null != txtFilterStartAddr)
            {
                if (null != cbxFilterMemBank.SelectedItem)
                {
                    ComboBoxItem selectedItem = (ComboBoxItem)cbxFilterMemBank.SelectedItem;
                    if (selectedItem.Content.ToString() == "EPC ID")
                        txtFilterStartAddr.Text = "32";
                    else
                        txtFilterStartAddr.Text = "0";
                }
            }
        }

        /// <summary>
        /// Initialize tag inspector tab
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void tiTagInspector_GotFocus(object sender, RoutedEventArgs e)
        {
            if (lblshowStatus.Content.ToString() == "Reading")
            {
                tiTagResults.Focus();
                return;
            }
            if (btnRead.Visibility == System.Windows.Visibility.Visible)
            {
                TagInspector.spTagInspector.IsEnabled = true;
            }
            btnRead.IsEnabled = false;
            //Disable clear tag results button on toolbar when tabs other then Tag Results is clicked
            btnClearTagReads.IsEnabled = false;
            saveData.IsEnabled = true;
            TagResults.dgTagResults.UnselectAll();
            TagInspector.LoadTagInspector(objReader, model);
        }

        /// <summary>
        /// Sets the scroll bar in setting section when specific expander is clicked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void expdrPerformanceTuning_Expanded(object sender, RoutedEventArgs e)
        {            
            // Return the offset vector for the TextBlock object.
            Vector vector = VisualTreeHelper.GetOffset(((UIElement)sender));

            // Convert the vector to a point value.
            Point currentPoint = new Point(vector.X, vector.Y);
            settingsScrollviewer.ScrollToVerticalOffset(vector.Y);
        }

        /// <summary>
        /// Enable or disable epc format column in tag results tab
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cbxDisplayEPCAs_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (null != cbxDisplayEPCAs)
            {
                try
                {
                    string text = ((ComboBoxItem)cbxDisplayEPCAs.SelectedItem).Content.ToString();
                    switch (text)
                    {
                        case "ASCII":
                            TagResults.epcColumnInAscii.Visibility = System.Windows.Visibility.Visible;
                            TagResults.epcColumnInReverseBase36.Visibility = System.Windows.Visibility.Collapsed;
                            break;
                        case "Select":
                        case "Hex":
                            TagResults.epcColumnInAscii.Visibility = System.Windows.Visibility.Collapsed;
                            TagResults.epcColumnInReverseBase36.Visibility = System.Windows.Visibility.Collapsed;
                            break;
                        case "ReverseBase36":
                            TagResults.epcColumnInAscii.Visibility = System.Windows.Visibility.Collapsed;
                            TagResults.epcColumnInReverseBase36.Visibility = System.Windows.Visibility.Visible;
                            break;                        
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message, "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Information);
                    cbxDisplayEPCAs.SelectedIndex = 0;
                }
            }
        }

        /// <summary>
        /// Enable or disable embedded read data column in tag results tab
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cbxDisplayEmbRdDataAs_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (null != cbxDisplayEmbRdDataAs)
            {
                try
                {
                    string text = ((ComboBoxItem)cbxDisplayEmbRdDataAs.SelectedItem).Content.ToString();
                    switch (text)
                    {
                        case "ASCII":
                            TagResults.dataColumnInAscii.Visibility = System.Windows.Visibility.Visible;
                            //TagResults.dataColumnReverseBase36.Visibility = System.Windows.Visibility.Collapsed;
                            break;
                        case "Select":
                        case "Hex":
                            TagResults.dataColumnInAscii.Visibility = System.Windows.Visibility.Collapsed;
                            //TagResults.dataColumnReverseBase36.Visibility = System.Windows.Visibility.Collapsed;
                            break;
                        //case "ReverseBase36":
                        //    TagResults.dataColumnInAscii.Visibility = System.Windows.Visibility.Collapsed;
                        //    TagResults.dataColumnReverseBase36.Visibility = System.Windows.Visibility.Visible;
                        //    break;
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message, "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Information);
                    cbxDisplayEmbRdDataAs.SelectedIndex = 0;
                }
            }
        }

        /// <summary>
        /// Validate rf on time
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void txtRFOnTimeout_LostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            if (txtRFOnTimeout.Text == "")
            {
                MessageBox.Show("RF On (ms) can't be empty.", "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Error);
                txtRFOnTimeout.Text = "1000";
            }
        }

        /// <summary>
        /// Validate rf off time
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void txtRFOffTimeout_LostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            if (txtRFOffTimeout.Text == "")
            {
                MessageBox.Show("RF Off (ms) can't be empty.", "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Error);
                txtRFOffTimeout.Text = "0";
            }
        }

        /// <summary>
        /// Initialize user memory tab
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void tiUserMemory_GotFocus(object sender, RoutedEventArgs e)
        {
            if (lblshowStatus.Content.ToString() == "Reading")
            {
                tiTagResults.Focus();
                return;
            }
            if (btnRead.Visibility == System.Windows.Visibility.Visible)
            {
                UserMemory.spUserMemory.IsEnabled = true;
            }
            btnRead.IsEnabled = false;
            //Disable clear tag results button on tool-bar when tabs other then Tag Results is clicked
            btnClearTagReads.IsEnabled = false;
            saveData.IsEnabled = false;
            TagResults.dgTagResults.UnselectAll();
            UserMemory.LoadUserMemory(objReader, model);
        }

        /// <summary>
        /// Uncheck all check boxes present inside enable embedded read data group box
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkEmbeddedReadData_Unchecked(object sender, RoutedEventArgs e)
        {
            chkUniqueByData.IsChecked = chkEmbeddedReadData.IsChecked;

            // remove data column when enable embedded read data is unchecked
            cbxDisplayEmbRdDataAs.SelectedIndex = 0;
        }

        /// <summary>
        /// Validate filter data in filter section 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void txtFilterData_LostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            try
            {
                if (txtFilterData.Text == "")
                {
                    MessageBox.Show("Filter: Filter data can't be empty.", "Universal Reader Assistant Message", 
                        MessageBoxButton.OK, MessageBoxImage.Error);
                    txtFilterData.Text = "0";
                }
                if (txtFilterData.Text.EndsWith("0x") || txtFilterData.Text.EndsWith("0X"))
                {
                    throw new Exception("Not a valid hex number");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Filter: Filter data: " + ex.Message + " If hex, prefix with 0x", "Universal Reader Assistant Message",
                    MessageBoxButton.OK, MessageBoxImage.Error);
                txtFilterData.Text = "0";
            }
        }        

        /// <summary>
        /// Validate start address in filter section
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void txtFilterStartAddr_LostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            if (txtFilterStartAddr.Text == "")
            {
                MessageBox.Show("Filter: Starting BIT Address to apply Filter from can't be empty.", 
                    "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Error);
                if (null != cbxFilterMemBank.SelectedItem)
                {
                    ComboBoxItem selectedItem = (ComboBoxItem)cbxFilterMemBank.SelectedItem;
                    if (selectedItem.Content.ToString() == "EPC ID")
                        txtFilterStartAddr.Text = "32";
                    else
                        txtFilterStartAddr.Text = "0";
                }                
            }
            try
            {
                Utilities.CheckHexOrDecimal(txtFilterStartAddr.Text);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Filter: Starting BIT Address to apply Filter from: "
                    + ex.Message + " If hex, prefix with 0x", "Universal Reader Assistant Message",
                    MessageBoxButton.OK, MessageBoxImage.Error);
                if (null != cbxFilterMemBank.SelectedItem)
                {
                    ComboBoxItem selectedItem = (ComboBoxItem)cbxFilterMemBank.SelectedItem;
                    if (selectedItem.Content.ToString() == "EPC ID")
                        txtFilterStartAddr.Text = "32";
                    else
                        txtFilterStartAddr.Text = "0";
                }
            }
        }

        /// <summary>
        /// Validate starting address in embedded read data
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void txtembReadStartAddr_LostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            if (txtembReadStartAddr.Text == "")
            {
                MessageBox.Show("Embedded Read Data: Starting Word Address to read from can't be empty.", 
                    "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Error);
                txtembReadStartAddr.Text = "0";
            }
            try
            {
                uint valueHex = Utilities.CheckHexOrDecimal(txtembReadStartAddr.Text);
                if (!(objReader is SerialReader) && (valueHex > 0xFFFF))
                {
                    MessageBox.Show("Embedded Read Data: Starting Word Address can't be more then 0xFFFF",
                        "Universal Reader Assistant Message",
                        MessageBoxButton.OK, MessageBoxImage.Error);
                    txtembReadLength.Text = "0";
                    return;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Embedded Read Data: Starting Word Address, " + ex.Message + " If hex, prefix with 0x",
                    "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Error);
                txtembReadStartAddr.Text = "0";
            }
        }        

        /// <summary>
        /// Validate number of words to read in embedded read data
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void txtembReadLength_LostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            if (txtembReadLength.Text == "")
            {
                MessageBox.Show("Embedded Read Data: Number of Words to read can't be empty.", 
                    "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Error);
                txtembReadLength.Text = "0";
            }
            try
            {
                uint valueHex = Utilities.CheckHexOrDecimal(txtembReadLength.Text);
                if (valueHex > 100)
                {
                    MessageBox.Show("Embedded Read Data: Number of words can't be more then 0x64 words,it can be "
                        +"read by incrementing start address and length of target read data",
                        "Universal Reader Assistant Message",
                        MessageBoxButton.OK, MessageBoxImage.Error);
                    txtembReadLength.Text = "0";
                    return;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Embedded Read Data: Number of Words, " + ex.Message + " If hex, prefix with 0x", 
                    "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Error);
                txtembReadLength.Text = "0";
            }

            
        }

        /// <summary>
        /// Condition to check textbox to have only double values
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void txtbxValuedBm_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            e.Handled = !Utilities.DoubleCharChecker(e.Text);
            base.OnPreviewTextInput(e);
        }       

        /// <summary>
        /// Validate read power and throw exception if not a valid read power
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void txtbxValuedBm_PreviewLostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            try
            {
                if (txtbxValuedBm.Text != "")
                {
                    if ((Convert.ToDouble(txtbxValuedBm.Text) < sldrReadPwr.Minimum) 
                        || (Convert.ToDouble(txtbxValuedBm.Text) > sldrReadPwr.Maximum))
                    {
                        MessageBox.Show("Please enter read power within " + sldrReadPwr.Minimum 
                            + " and " + sldrReadPwr.Maximum + " dBm", "Universal Reader Assistant Message",
                            MessageBoxButton.OK, MessageBoxImage.Error);
                        txtbxValuedBm.Text = sldrReadPwr.Maximum.ToString();
                        return;
                    }
                }
                else
                {
                    MessageBox.Show("Readpower (dBm) can't be empty", "Universal Reader Assistant Message",
                        MessageBoxButton.OK, MessageBoxImage.Error);
                    txtbxValuedBm.Text = sldrReadPwr.Maximum.ToString();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Universal Reader Assistant Message",
                    MessageBoxButton.OK, MessageBoxImage.Error);
                txtbxValuedBm.Text = sldrReadPwr.Maximum.ToString();
            }
        }

        /// <summary>
        /// Validate hex string
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void txtFilterData_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            e.Handled = !Utilities.HexStringChecker(e.Text);
            base.OnPreviewTextInput(e);
        }

        #region Load/Save Profile

        // Remember if saved configurations is loaded by the user
        bool isLoadSavedConfigurations = false;

        /// <summary>
        /// Event handler to handle the load configurations when user presses Load button in
        /// Load/Save Profile section Used to load the configuration parameters from the user
        /// specified file
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnLoadConfig_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                OpenFileDialog openLoadSaveConfigFileDialog = new OpenFileDialog();
                openLoadSaveConfigFileDialog.Filter = "URA Configuration Files (.urac)|*.urac";
                openLoadSaveConfigFileDialog.Title = "Select a configuration file to load reader and UI configuration parameters";
                openLoadSaveConfigFileDialog.InitialDirectory = Directory.GetParent(
                    System.Reflection.Assembly.GetExecutingAssembly().Location).ToString();
                openLoadSaveConfigFileDialog.RestoreDirectory = true;
                if (true == openLoadSaveConfigFileDialog.ShowDialog())
                {
                    Mouse.SetCursor(Cursors.AppStarting);
                    isLoadSavedConfigurations = true;
                    loadSaveConfig.LoadConfigurations(openLoadSaveConfigFileDialog.FileName);
                    LoadConfigurations();
                    isLoadSavedConfigurations = false;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Load Profile: "+ex.Message, "Universal Reader Assistant Message",
                    MessageBoxButton.OK, MessageBoxImage.Exclamation);
                Mouse.SetCursor(Cursors.Arrow);
                isLoadSavedConfigurations = false;
            }
        }

        /// <summary>
        /// Displays message and logs the error message in the error log file 
        /// if the user enters value other then true or false
        /// </summary>
        /// <param name="parametername"></param>
        private void NotifyInvalidLoadConfigOption(string parametername)
        {
            MessageBox.Show("Saved parameter ["+parametername+"] parameter has invalid value ["+
                        loadSaveConfig.Properties[parametername] + "]. This parameter accepts only true or false."
                            + " URA sets this parameter to previous set value ","Universal Reader Assistant",
                            MessageBoxButton.OK, MessageBoxImage.Error);
            Onlog("Saved parameter [" + parametername + "] parameter has invalid value [" +
                        loadSaveConfig.Properties[parametername] + "]. This parameter accepts only true or false."
                            + " URA sets this parameter to previous set value ");
        }

        /// <summary>
        /// Load the configurations from the user specified configuration file
        /// </summary>
        private void LoadConfigurations()
        {
            if (lblshowStatus.Content.ToString() != "Connected")
            {
                if (loadSaveConfig.Properties["/application/connect/readerType"].Equals("SerialReader"))
                {
                    rdbtnLocalConnection.IsChecked = true;
                    cmbReaderAddr.Text = loadSaveConfig.Properties["/application/connect/readerURI"].ToUpper();
                    if (loadSaveConfig.Properties.ContainsKey("/reader/baudRate"))
                    {
                        if (loadSaveConfig.Properties["/reader/baudRate"] != "")
                        {
                            cbxBaudRate.SelectedIndex = GetIndexOf(cbxBaudRate,
                                loadSaveConfig.Properties["/reader/baudRate"], "Baudrate");
                        }
                        else
                        {
                            cbxBaudRate.SelectedIndex = 0;
                        }
                    }
                }
                else if (loadSaveConfig.Properties["/application/connect/readerType"].Equals("FixedReader"))
                {
                    rdbtnNetworkConnection.IsChecked = true;
                    cmbFixedReaderAddr.Text = loadSaveConfig.Properties["/application/connect/readerURI"];
                }
                else
                {
                    // Notify to the user if options are not valid.
                    throw new Exception("Saved reader type [/application/connect/readerType] parameter has invalid value [" +
                        loadSaveConfig.Properties["/application/connect/readerType"] 
                        + "]. URA will stop loading the remaining parameters."+
                        " Please provide valid reader type in configuration file.");
                    //NotifyLoadSaveConfigErrorMessage();
                    //rdbtnNetworkConnection.IsChecked = true;
                    //cmbFixedReaderAddr.Text = loadSaveConfig.Properties["/application/connect/readerURI"];
                }

                // Enable transport logging
                if (loadSaveConfig.Properties["/application/connect/enableTransportLogging"].ToLower().Equals("true"))
                {
                    chkEnableTransportLogging.IsChecked = true;
                }
                else if (loadSaveConfig.Properties["/application/connect/enableTransportLogging"].ToLower().Equals("false"))
                {
                    chkEnableTransportLogging.IsChecked = false;
                }
                else
                {
                    // Notify the error message to the user
                    NotifyInvalidLoadConfigOption("/application/connect/enableTransportLogging");
                }
                btnConnect_Click_Body(this, new RoutedEventArgs());
                if (lblshowStatus.Content.ToString() == "Connected")
                {
                    LoadAfterConnectConfigurations();
                    Mouse.SetCursor(Cursors.Arrow);
                    MessageBox.Show("Loaded reader and UI configuration parameters successfully", 
                        "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Information);
                }
            }
            else
            {
                LoadAfterConnectConfigurations();
                Mouse.SetCursor(Cursors.Arrow);
                MessageBox.Show("Loaded reader and UI configuration parameters successfully", 
                    "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Information);
            }
        }

        /// <summary>
        /// Display the error message to the user and log the message in error log file
        /// </summary>
        /// <param name="message">message to be displayed to the user</param>
        private void NotifyLoadSaveConfigErrorMessage(string message)
        {
            // display the message to the user
            MessageBox.Show(message, "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Error);
            // log the error message
            Onlog(message);
        }

        /// <summary>
        /// Load the after connect parameters on to the UI and reader, if the reader is already connected
        /// </summary>
        private void LoadAfterConnectConfigurations()
        {
            if (loadSaveConfig.Properties.ContainsKey("/application/connect/enableTransportLogging"))
            {
                if (loadSaveConfig.Properties["/application/connect/enableTransportLogging"].ToLower().Equals("true"))
                {
                    if (lblshowStatus.Content.ToString() == "Connected")
                    {
                        if ((bool)chkEnableTransportLogging.IsChecked)
                        {
                            // if already checked don't do anything
                        }
                        else
                        {
                            NotifyLoadSaveConfigErrorMessage("Saved enable transport logging "
                                +"[/application/connect/enableTransportLogging] value ["
                               + loadSaveConfig.Properties["/application/connect/enableTransportLogging"] 
                               + "] failed. Transport logging can be enabled only at the time of connecting "
                               +"to the reader and not when reader is connected. URA  skips this setting"
                                + " or change to the supported value and reload the configuration");
                        }
                    }
                }
            }
            // Set read behavior
            if(loadSaveConfig.Properties["/application/readwriteOption/ReadBehaviour"].Equals("ReadOnce"))
            {
                rdBtnReadOnce.IsChecked = true;
                string tempValueReadOnce = txtbxreadOnceTimeout.Text;
                if (Utilities.AreAllValidNumericChars(
                    loadSaveConfig.Properties["/application/readwriteOption/readOnceTimeout"]))
                {                    
                    try
                    {
                        txtbxreadOnceTimeout.Text = loadSaveConfig.Properties["/application/readwriteOption/readOnceTimeout"];
                        if ((Convert.ToInt32(txtbxreadOnceTimeout.Text) < 30) 
                            || ((Convert.ToInt32(txtbxreadOnceTimeout.Text) > 65535)))
                        {
                            NotifyLoadSaveConfigErrorMessage("Saved Timeout (ms) "
                                +"[/application/readwriteOption/readOnceTimeout] value ["
                           + loadSaveConfig.Properties["/application/readwriteOption/readOnceTimeout"] + "] failed. "
                           +"Please enter only positive number within 30 and 65535."
                           + "URA sets Timeout (ms) to previous set value [" + tempValueReadOnce + "] or change to "
                           +"the supported value and reload the configuration");
                            txtbxreadOnceTimeout.Text = tempValueReadOnce;
                        }
                    }
                    catch (Exception)
                    {
                        NotifyLoadSaveConfigErrorMessage("Saved Timeout (ms)[/application/readwriteOption/readOnceTimeout] value ["
                           + loadSaveConfig.Properties["/application/readwriteOption/readOnceTimeout"] + "] failed. "
                           +"Please enter only positive number within 30 and 65535. URA sets Timeout (ms) to previous set value ["
                           + tempValueReadOnce + "] or change to the supported value and reload the configuration");
                        txtbxreadOnceTimeout.Text = tempValueReadOnce;
                    }
                }
                else
                {                    
                    NotifyLoadSaveConfigErrorMessage("Saved Timeout (ms)[/application/readwriteOption/readOnceTimeout] value ["
                           + loadSaveConfig.Properties["/application/readwriteOption/readOnceTimeout"] + "] failed. Please enter "
                           +" only positive number within 30 and 65535. URA sets Timeout (ms) to previous set value ["
                           + tempValueReadOnce + "] or change to the supported value and reload the configuration");
                    txtbxreadOnceTimeout.Text = tempValueReadOnce;
                }
            }
            else if (loadSaveConfig.Properties["/application/readwriteOption/ReadBehaviour"].Equals("ReadContinuously"))
            {
                rdBtnReadContinuously.IsChecked = true;
                string tempValueRfOn = txtRFOnTimeout.Text;
                if (Utilities.AreAllValidNumericChars(loadSaveConfig.Properties["/reader/read/asyncOnTime"]))
                {
                    try
                    {
                        txtRFOnTimeout.Text = loadSaveConfig.Properties["/reader/read/asyncOnTime"];
                        if (Convert.ToInt32(txtRFOnTimeout.Text) < 0)
                        {
                            NotifyLoadSaveConfigErrorMessage("Saved RF On (ms) [/reader/read/asyncOnTime] value ["
                            + loadSaveConfig.Properties["/reader/read/asyncOnTime"] + "] failed. Please enter only "
                            +" positive number not more then 65535. URA sets RF On (ms) to previous set value  ["
                            + tempValueRfOn + "] or change to the supported value and reload the configuration");
                            txtRFOnTimeout.Text = tempValueRfOn;
                        }
                    }
                    catch (Exception)
                    {
                        NotifyLoadSaveConfigErrorMessage("Saved RF On (ms) [/reader/read/asyncOnTime] value ["
                           + loadSaveConfig.Properties["/reader/read/asyncOnTime"] + "] failed. Please enter only "
                           +"positive number not more then 65535. URA sets RF On (ms) to previous set value  ["
                           + tempValueRfOn + "] or change to the supported value and reload the configuration");
                        txtRFOnTimeout.Text = tempValueRfOn;
                    }
                }
                else
                {
                    
                    NotifyLoadSaveConfigErrorMessage("Saved RF On (ms) [/reader/read/asyncOnTime] value ["
                           + loadSaveConfig.Properties["/reader/read/asyncOnTime"] + "] failed. Please enter only "
                           +"positive number not more then 65535. URA sets RF On (ms) to previous set value  ["
                           + tempValueRfOn + "] or change to the supported value and reload the configuration");
                    txtRFOnTimeout.Text = tempValueRfOn;
                }
                string tempValueRfOff = txtRFOffTimeout.Text;
                if (Utilities.AreAllValidNumericChars(loadSaveConfig.Properties["/reader/read/asyncOffTime"]))
                {
                    try
                    {
                        txtRFOffTimeout.Text = loadSaveConfig.Properties["/reader/read/asyncOffTime"];
                        if (Convert.ToInt32(txtRFOffTimeout.Text) < 0)
                        {
                            NotifyLoadSaveConfigErrorMessage("Saved RF Off (ms)[/reader/read/asyncOffTime] value ["
                           + loadSaveConfig.Properties["/reader/read/asyncOffTime"] + "] failed. Please enter only "
                           +"positive number not more then 65535. URA sets RF Off (ms) to previous set value ["
                           + tempValueRfOff + "] or change to the supported value and reload the configuration");
                            txtRFOffTimeout.Text = tempValueRfOff;
                        }
                    }
                    catch (Exception)
                    {
                        NotifyLoadSaveConfigErrorMessage("Saved RF Off (ms)[/reader/read/asyncOffTime] value ["
                           + loadSaveConfig.Properties["/reader/read/asyncOffTime"] + "] failed. Please enter only "
                           +" number not more then 65535. URA sets RF Off (ms) to previous set value [" 
                           + tempValueRfOff + "] or change to the supported value and reload the configuration");
                        txtRFOffTimeout.Text = tempValueRfOff;
                    }
                }
                else
                {                    
                    NotifyLoadSaveConfigErrorMessage("Saved RF Off (ms)[/reader/read/asyncOffTime] value ["
                           + loadSaveConfig.Properties["/reader/read/asyncOffTime"] + "] failed. Please enter only "
                           +"positive number not more then 65535. URA sets RF Off (ms) to previous set value [" 
                           + tempValueRfOff + "] or change to the supported value and reload the configuration");                    
                    txtRFOffTimeout.Text = tempValueRfOff;
                }
            }
            else
            {
                NotifyLoadSaveConfigErrorMessage("Saved parameter [/application/readwriteOption/ReadBehaviour ] has "
                    +" invalid option [" + loadSaveConfig.Properties["/application/readwriteOption/ReadBehaviour"] 
                    + "]. URA skips the read behavior and timeout parameters given in the configuration file"
                    + ".URA sets to previous chosen read behavior and its relevant timeouts parameters.");
            }

            // Set region
            if (loadSaveConfig.Properties.ContainsKey("/reader/region/id"))
            {
                bool regionStatus = false;
                regioncombo.SelectedIndex = GetRegionIndexOf(regioncombo,
                    loadSaveConfig.Properties["/reader/region/id"], out regionStatus);
                if (!regionStatus)
                {
                    if (objReader is SerialReader)
                    {
                        NotifyLoadSaveConfigErrorMessage("Saved region[/reader/region/id] value ["
                               + loadSaveConfig.Properties["/reader/region/id"] + "] failed. Please enter a "
                               +"supported region. URA gets and applies region ["
                               + regionToSet.ToString() + "] to URA or change to the supported value and reload the configuration");
                    }
                    else
                    {
                        NotifyLoadSaveConfigErrorMessage("Saved region[/reader/region/id] value ["
                               + loadSaveConfig.Properties["/reader/region/id"] + "] failed. URA won't support to set this parameter."
                               + " URA gets the region from reader ["
                               + regionToSet.ToString() + "] and displays the value in UI.");
                    }
                    //set the region on module
                    regioncombo.SelectedItem = regioncombo.Items.GetItemAt(regioncombo.Items.IndexOf(regionToSet));
                }
            }

            // Set baud rate
            if (loadSaveConfig.Properties.ContainsKey("/reader/baudRate"))
            {
                    int tempValueBaudrate = cbxBaudRate.SelectedIndex;
                    if (loadSaveConfig.Properties["/reader/baudRate"] != "")
                    {
                        if (cbxBaudRate.Visibility == System.Windows.Visibility.Visible)
                        {
                            cbxBaudRate.SelectedIndex = GetIndexOf(cbxBaudRate,
                                loadSaveConfig.Properties["/reader/baudRate"], "Baudrate");
                        }
                        else
                        {
                            // Fast search is not supported by connected reader
                            NotifyLoadSaveConfigErrorMessage("Saved baudrate parameter [/reader/baudRate] "
                             + " is not supported by connected reader. URA skips this setting");
                        }
                    }
                    else
                    {
                        if (cbxBaudRate.Visibility == System.Windows.Visibility.Visible)
                        {
                            cbxBaudRate.SelectedIndex = tempValueBaudrate;
                            NotifyLoadSaveConfigErrorMessage("Saved baudrate[/reader/baudRate] value ["
                                  + loadSaveConfig.Properties["/reader/baudRate"] + "] failed. Please enter a supported baudrate."
                                  + " URA gets and sets baudrate ["
                                  + cbxBaudRate.Text + "] or change to the supported value and reload the configuration");
                        }
                    }
                }

            // Set fast search
            if(loadSaveConfig.Properties["/application/readwriteOption/enableFastSearch"].ToLower().Equals("true"))
            {
                if (chkEnableFastSearch.IsEnabled)
                {
                    chkEnableFastSearch.IsChecked = true;
                }
                else
                {
                    // Fast search is not supported by connected reader
                    NotifyLoadSaveConfigErrorMessage("Saved fast search parameter [/application/readwriteOption/enableFastSearch] "
                        +" is not supported by connected reader. URA skips this setting");
                    chkEnableFastSearch.IsChecked = false;
                }
            }
            else if (loadSaveConfig.Properties["/application/readwriteOption/enableFastSearch"].ToLower().Equals("false"))
            {
                chkEnableFastSearch.IsChecked = false;
            }
            else
            {
                // Notify the error message to the user
                NotifyInvalidLoadConfigOption("/application/readwriteOption/enableFastSearch");
            }

            // Antenna switching
            string temp = "/application/readwriteOption/switchingMethod";
            if (loadSaveConfig.Properties[temp].Equals("Equal"))
            {
                rdBtnEqualSwitching.IsChecked = true;
            }
            else if (loadSaveConfig.Properties[temp].Equals("Dynamic"))
            {
                rdBtnAutoSwitching.IsChecked = true;
            }
            else
            {
                NotifyLoadSaveConfigErrorMessage("Saved parameter [/application/readwriteOption/switchingMethod ]"
                    +" has invalid option ["
                    + loadSaveConfig.Properties["/application/readwriteOption/switchingMethod"] + "]"
                    + ".URA sets to previous chosen switching method.");
            }

            // Save protocol configurations
            if(loadSaveConfig.Properties.ContainsKey("/application/readwriteOption/Protocols"))
            {
                string [] protocolToSet = loadSaveConfig.Properties["/application/readwriteOption/Protocols"].Split(
                    new char[]{',',' '});
                List<string> protocollist = protocolToSet.ToList<string>();
                int protcolIndex = 0;
                foreach (CheckBox child in LogicalTreeHelper.GetChildren(stackpanel6))
                {
                    if(protocollist.Contains(child.Content.ToString()))
                    {
                        if (child.Visibility == System.Windows.Visibility.Visible)
                        {
                            child.IsChecked = true;
                        }
                        else
                        {
                            if (protcolIndex < protocollist.Count)
                            {
                                NotifyLoadSaveConfigErrorMessage("Saved [/application/readwriteOption/Protocols] protocol [" 
                                    + protocollist[protcolIndex].ToString() + "] "
                                    + "is not supported for connected reader. URA skips this protocol");
                            }
                            child.IsChecked = false;
                        }
                    }
                    else
                    {
                        if (protcolIndex < protocollist.Count)
                        {
                            NotifyLoadSaveConfigErrorMessage("Saved [/application/readwriteOption/Protocols] protocol ["
                                + protocollist[protcolIndex].ToString() + "] "
                                + "is not valid protocol option for connected reader. URA skips this protocol");
                        }
                        else
                        {
                            child.IsChecked = false;
                        }
                    }
                    protcolIndex++;
                }
            }

            // Antenna detection
            if (loadSaveConfig.Properties["/reader/antenna/checkPort"].ToLower().Equals("true"))
            {
                if (chkbxAntennaDetection.IsEnabled)
                {
                    chkbxAntennaDetection.IsChecked = true;
                    SetCheckPort(this, new EventArgs(), true);
                }
                else
                {
                    // Antenna detection is not supported by connected reader
                    NotifyLoadSaveConfigErrorMessage("Saved antenna detection parameter [/reader/antenna/checkPort] "
                        + " is not supported by connected reader. URA skips this setting");
                    chkbxAntennaDetection.IsChecked = false;
                    SetCheckPort(this, new EventArgs(), false);
                }
            }
            else if (loadSaveConfig.Properties["/reader/antenna/checkPort"].ToLower().Equals("false"))
            {
                chkbxAntennaDetection.IsChecked = false;
                SetCheckPort(this, new EventArgs(), false);
            }
            else
            {
                // Notify the error message to the user
                NotifyInvalidLoadConfigOption("/reader/antenna/checkPort");
            }

            // Save antenna configurations
            if(loadSaveConfig.Properties.ContainsKey("/application/readwriteOption/Antennas"))
            {
                if (!((bool)chkbxAntennaDetection.IsChecked))
                {
                    string[] antennasToSet = loadSaveConfig.Properties["/application/readwriteOption/Antennas"].Split(
                        new char[] { ',', ' ' });
                    // List of antennas to set on the connected reader
                    List<string> antennalist = antennasToSet.ToList<string>();
                    // List of available antenna on the connected reader
                    List<string> availableAntenna = new List<string>();
                    // Generate the list of available antenna ports on the 
                    // connected reader and set them to false.
                    foreach (CheckBox child in LogicalTreeHelper.GetChildren(stackPanel8))
                    {
                        availableAntenna.Add(child.Content.ToString());
                        child.IsChecked = false;
                    }
                    foreach (string antennaToConnect in antennasToSet)
                    {
                        // Check if the antenna to set is present in the list of available antennas
                        if (availableAntenna.Contains(antennaToConnect))
                        {
                            // Loop through all the available antenna
                            foreach (CheckBox child in LogicalTreeHelper.GetChildren(stackPanel8))
                            {
                                if (child.Content.ToString().Equals(antennaToConnect))
                                {
                                    if ((child.Visibility == System.Windows.Visibility.Visible)
                                        && (child.IsEnabled == true))
                                    {
                                        child.IsChecked = true;
                                    }
                                    else
                                    {
                                            NotifyLoadSaveConfigErrorMessage("Saved [/application/readwriteOption/Antennas] antenna ["
                                                + child.Content.ToString() + "] "
                                                + "is not supported. URA skips this antenna");
                                        child.IsChecked = false;
                                    }
                                }
                            }
                        }
                        else
                        {
                            // Not a valid antenna option
                            NotifyLoadSaveConfigErrorMessage("Saved [/application/readwriteOption/Antennas] antenna ["
                                + antennaToConnect+ "] "
                                + "is not valid antenna option.");
                        }
                    }
                }
                else
                {
                    // Antenna detection is enabled
                    NotifyLoadSaveConfigErrorMessage("Saved antenna detection parameter [/reader/antenna/checkPort] "
                        + " is enabled. URA skips manual antenna settings if provided any and gets the antenna connected from the reader.");
                }
            }

            // Embedded read data
            if (loadSaveConfig.Properties["/application/readwriteOption/enableEmbeddedReadData"].ToLower().Equals("true"))
            {
                if (ReadDataGroupBox.IsEnabled)
                {
                    chkEmbeddedReadData.IsChecked = true;
                    if (loadSaveConfig.Properties.ContainsKey("/application/readwriteOption/enableEmbeddedReadData/MemBank"))
                    {
                        cbxReadDataBank.SelectedIndex = GetIndexOf(cbxReadDataBank,
                            loadSaveConfig.Properties["/application/readwriteOption/enableEmbeddedReadData/MemBank"],
                            "EmbeddedReadData MemBank");
                    }
                    if (loadSaveConfig.Properties.ContainsKey("/application/readwriteOption/enableEmbeddedReadData/StartAddress"))
                    {
                        string tempValueEmbeddStartAdd = txtembReadStartAddr.Text;
                        try
                        {
                            Utilities.CheckHexOrDecimal(loadSaveConfig.Properties[
                                "/application/readwriteOption/enableEmbeddedReadData/StartAddress"]);
                            txtembReadStartAddr.Text = loadSaveConfig.Properties[
                                "/application/readwriteOption/enableEmbeddedReadData/StartAddress"];
                            txtembReadStartAddr.Focus();
                        }
                        catch (Exception)
                        {
                            NotifyLoadSaveConfigErrorMessage("Saved embedded read start address "
                                + " [/application/readwriteOption/enableEmbeddedReadData/StartAddress] value ["
                                       + loadSaveConfig.Properties["/application/readwriteOption/enableEmbeddedReadData/StartAddress"] + "]"
                                       + " invalid. Please enter valid dec or hex with prefix as 0x. URA sets embedded read start address to "
                                       + "previous set value [" + tempValueEmbeddStartAdd + "] or change to the supported value and reload "
                                       + "the configuration");
                            txtembReadStartAddr.Text = tempValueEmbeddStartAdd;
                        }
                    }
                    if (loadSaveConfig.Properties.ContainsKey("/application/readwriteOption/enableEmbeddedReadData/NoOfWordsToRead"))
                    {
                        string tempValueEmbeddedNoOfWrds = txtembReadLength.Text;
                        try
                        {
                            Utilities.CheckHexOrDecimal(loadSaveConfig.Properties[
                                "/application/readwriteOption/enableEmbeddedReadData/NoOfWordsToRead"]);
                            txtembReadLength.Text = loadSaveConfig.Properties[
                                "/application/readwriteOption/enableEmbeddedReadData/NoOfWordsToRead"];
                            txtembReadLength.Focus();
                        }
                        catch (Exception)
                        {
                            NotifyLoadSaveConfigErrorMessage("Saved embedded read, no of words to read "
                                + " [/application/readwriteOption/enableEmbeddedReadData/NoOfWordsToRead] value ["
                                      + loadSaveConfig.Properties["/application/readwriteOption/enableEmbeddedReadData/NoOfWordsToRead"] + "] "
                                      + "invalid. Please enter valid dec or hex with prefix as 0x. URA sets embedded read, no of words to read "
                                      + "to previous set value [" + tempValueEmbeddedNoOfWrds + "] or change to the supported value and reload"
                                      + " the configuration");
                            txtembReadLength.Text = tempValueEmbeddedNoOfWrds;
                        }
                    }
                }
                else
                {
                    // Embedded read data is not supported by connected reader
                    NotifyLoadSaveConfigErrorMessage("Saved enable embedded read data parameter "
                        +"[/application/readwriteOption/enableEmbeddedReadData] "
                        + " is not supported by connected reader. URA skips this setting and embedded read "
                        +"data relevant settings.");
                    chkEnableFastSearch.IsChecked = false;
                    ReadDataGroupBox.IsEnabled = false;
                }
            }
            else if (loadSaveConfig.Properties["/application/readwriteOption/enableEmbeddedReadData"].ToLower().Equals("false"))
            {
                chkEmbeddedReadData.IsChecked = false;
            }
            else
            {
                // Notify the error message to the user
                NotifyInvalidLoadConfigOption("/application/readwriteOption/enableEmbeddedReadData");
            }

            // UniqueByData
            if(loadSaveConfig.Properties["/reader/tagReadData/uniqueByData"].ToLower().Equals("true"))
            {
                chkUniqueByData.IsChecked = true;
            }
            else if (loadSaveConfig.Properties["/reader/tagReadData/uniqueByData"].ToLower().Equals("false"))
            {
                chkUniqueByData.IsChecked = false;
            }
            else
            {
                // Notify the error message to the user
                NotifyInvalidLoadConfigOption("/reader/tagReadData/uniqueByData");
            }

            // Show failed read data
            if(loadSaveConfig.Properties[
                "/application/readwriteOption/enableEmbeddedReadData/uniqueByData/ShowFailedDataRead"].ToLower().Equals("true"))
            {
                chkShowFailedDataReads.IsChecked = true;
            }
            else if (loadSaveConfig.Properties[
                "/application/readwriteOption/enableEmbeddedReadData/uniqueByData/ShowFailedDataRead"].ToLower().Equals("false"))
            {
                chkShowFailedDataReads.IsChecked = false;
            }
            else
            {
                // Notify the error message to the user
                NotifyInvalidLoadConfigOption(
                    "/application/readwriteOption/enableEmbeddedReadData/uniqueByData/ShowFailedDataRead");
            }

            // Apply filter
            if (loadSaveConfig.Properties["/application/readwriteOption/applyFilter"].ToLower().Equals("true"))
            {
                chkApplyFilter.IsChecked = true;
                if (loadSaveConfig.Properties.ContainsKey("/application/readwriteOption/applyFilter/FilterMemBank"))
                {
                    cbxFilterMemBank.SelectedIndex = GetIndexOf(cbxFilterMemBank,
                        loadSaveConfig.Properties["/application/readwriteOption/applyFilter/FilterMemBank"], "Filter MemBank");
                }
                if (loadSaveConfig.Properties.ContainsKey("/application/readwriteOption/applyFilter/FilterStartAddress"))
                {
                    string tempValueFilterStartAdd = txtFilterStartAddr.Text;
                    try
                    {
                        Utilities.CheckHexOrDecimal(loadSaveConfig.Properties[
                            "/application/readwriteOption/applyFilter/FilterStartAddress"]);
                        txtFilterStartAddr.Text = loadSaveConfig.Properties[
                            "/application/readwriteOption/applyFilter/FilterStartAddress"];
                        txtFilterStartAddr.Focus();
                    }
                    catch (Exception)
                    {
                        NotifyLoadSaveConfigErrorMessage("Saved filter start address "
                            +"[/application/readwriteOption/applyFilter/FilterStartAddress]"
                            +" value ["+ loadSaveConfig.Properties["/application/readwriteOption/applyFilter/FilterStartAddress"] + "] "
                            + "invalid. Please enter valid dec or hex with prefix as 0x. URA sets filter start address to previous set value "
                            +" [" + tempValueFilterStartAdd + "] or change to the supported value and reload the configuration");
                        txtFilterStartAddr.Text = tempValueFilterStartAdd;
                    }
                }
                if (ValidateFilterDataFromConfig(loadSaveConfig.Properties[
                    "/application/readwriteOption/applyFilter/FilterData"]))
                {
                    txtFilterData.Text = loadSaveConfig.Properties[
                        "/application/readwriteOption/applyFilter/FilterData"];
                    txtFilterData.Focus();
                }
                else
                {
                    string tempValueFilterData = txtFilterData.Text;
                    NotifyLoadSaveConfigErrorMessage("Saved Filter data [/application/readwriteOption/applyFilter/FilterData] value ["
                           + loadSaveConfig.Properties["/application/readwriteOption/applyFilter/FilterData"] + "] invalid. Please enter valid"
                           +" hex number. URA sets filter data to previous set value [" + tempValueFilterData + "] or change to the "
                           +" supported value and reload the configuration");
                    txtFilterData.Text = tempValueFilterData;
                }

                // Invert filter
                if (loadSaveConfig.Properties["/application/readwriteOption/applyFilter/InvertFilter"].ToLower().Equals("true"))
                {
                    chkFilterInvert.IsChecked = true;
                }
                else if (loadSaveConfig.Properties["/application/readwriteOption/applyFilter/InvertFilter"].ToLower().Equals("false"))
                {
                    chkFilterInvert.IsChecked = false;
                }
                else
                {
                    // Notify the error message to the user
                    NotifyInvalidLoadConfigOption("/application/readwriteOption/applyFilter/InvertFilter");
                }
            }
            else if (loadSaveConfig.Properties["/application/readwriteOption/applyFilter"].ToLower().Equals("false"))
            {
                chkApplyFilter.IsChecked = false;
            }
            else
            {
                // Notify the error message to the user
                NotifyInvalidLoadConfigOption("/application/readwriteOption/applyFilter");
            }

            // Performance tuning
            // Save Performance tuning settings
            
            // Set read power
            if(loadSaveConfig.Properties.ContainsKey("/reader/radio/readPower"))
            {
                if (loadSaveConfig.Properties["/reader/radio/readPower"] != "")
                {
                    double tempValue = 0;
                    double newValue = 0; 
                    try
                    {
                        tempValue = Convert.ToDouble(txtbxValuedBm.Text);
                        newValue = Convert.ToDouble(loadSaveConfig.Properties["/reader/radio/readPower"]) / 100;
                        if ((newValue >= sldrReadPwr.Minimum) && (newValue <= sldrReadPwr.Maximum))
                        {
                            txtbxValuedBm.Text = newValue.ToString();
                            sldrReadPwr.Value = newValue;
                        }
                        else
                        {
                            NotifyLoadSaveConfigErrorMessage("Saved read power[/reader/radio/readPower] value ["
                                + loadSaveConfig.Properties["/reader/radio/readPower"] + "] failed. Please enter within "
                                + sldrReadPwr.Minimum * 100 +
                                " and " + sldrReadPwr.Maximum * 100 + " cdBm in Load/Save profile. URA gets and sets read power ["
                                + (tempValue * 100).ToString() + "] cdBm or change to the supported value and reload the configuration");
                            txtbxValuedBm.Text = tempValue.ToString();
                            sldrReadPwr.Value = tempValue;
                        }
                    }
                    catch (FormatException)
                    {
                        NotifyLoadSaveConfigErrorMessage("Saved read power[/reader/radio/readPower] value ["
                                + loadSaveConfig.Properties["/reader/radio/readPower"] + "] failed. Entered value not in correct "
                                +" format. Please enter within " + sldrReadPwr.Minimum * 100 +
                                " and " + sldrReadPwr.Maximum * 100 + " cdBm in Load/Save profile. URA gets and sets read power ["
                                + (tempValue * 100).ToString() + "] cdBm or change to the supported value and reload the configuration");
                        txtbxValuedBm.Text = tempValue.ToString();
                        sldrReadPwr.Value = tempValue;
                    }
                }
                else
                {
                    // If read power is empty then keep the previous power. Don't set to max
                    sldrReadPwr.Value = Convert.ToDouble(txtbxValuedBm.Text);
                }
            }            
                //if (loadSaveConfig.Properties["/application/performanceTuning/Enable"].Equals("true"))
                //{
                    if (grpbxPerformanceTuning.IsEnabled)
                    {
                        if (loadSaveConfig.Properties["/application/performanceTuning/configureGen2SettingsType"].Equals("Manual"))
                        {
                            LoadManualGen2ConfigurationFromProfile();
                        }
                        else if (loadSaveConfig.Properties["/application/performanceTuning/configureGen2SettingsType"].Equals("Auto"))
                        {
                            LoadAutoGen2ConfigurationFromProfile();
                        }
                        else
                        {
                            // Notify the error message to the user
                            NotifyLoadSaveConfigErrorMessage("Saved [/application/performanceTuning/configureGen2SettingsType] value ["
                            + loadSaveConfig.Properties["/application/performanceTuning/configureGen2SettingsType"] + "] failed. "
                            +"Invalid option. Accepts only Auto or Manual. URA skips the performance tuning settings given in the "
                            +"configuration and sets to the previous set values ");
                        }
                    }
                    else
                    {
                        // Performance tuning is not supported by connected reader
                        NotifyLoadSaveConfigErrorMessage("Saved performance tuning parameter [/application/performanceTuning/Enable] "
                            + " is not supported by connected reader. URA skips this setting including configure Gen2 Settings");
                    }
                //}
                //else if (loadSaveConfig.Properties["/application/performanceTuning/Enable"].Equals("false"))
                //{
                //    if (grpbxPerformanceTuning.IsEnabled)
                //    {
                //        // Performance tuning is not supported by connected reader
                //        NotifyLoadSaveConfigErrorMessage("Saved performance tuning parameter [/application/performanceTuning/Enable] value is set to ["
                //               + loadSaveConfig.Properties["/application/performanceTuning/Enable"] + "]. "
                //               + "URA skips this [/application/performanceTuning/configureGen2SettingsType] "
                //               + "and related parameters and sets performance tuning parameters to previous set values or change to the supported "
                //               + "value and reload the configuration");
                //    }
                //}
                //else
                //{
                //    // Notify the error message to the user
                //    NotifyInvalidLoadConfigOption("/application/performanceTuning/Enable");
                //}

            // Save display options
            // Font size
            if (loadSaveConfig.Properties.ContainsKey("/application/displayOption/fontSize"))
            {
                if (Utilities.AreAllValidNumericChars(loadSaveConfig.Properties["/application/displayOption/fontSize"]))
                {
                    txtfontSize.Text = loadSaveConfig.Properties["/application/displayOption/fontSize"];
                    txtfontSize.Focus();
                }
                else
                {
                    string tempValueFontSize = txtfontSize.Text;
                    NotifyLoadSaveConfigErrorMessage("Saved Font [/application/displayOption/fontSize] value ["
                           + loadSaveConfig.Properties["/application/displayOption/fontSize"] + "] failed. Please enter only number."
                           + "URA sets Font to previous set value [" + tempValueFontSize + "] or change to the supported value and "
                           +" reload the configuration");
                    txtfontSize.Text = tempValueFontSize;
                }
            }
            else
            {
                txtfontSize.Text = "14";
            }

            // chkEnableTagAging
            if (loadSaveConfig.Properties["/application/displayOption/enableTagAging"].ToLower().Equals("true"))
            {
                chkEnableTagAging.IsChecked = true;
            }
            else if (loadSaveConfig.Properties["/application/displayOption/enableTagAging"].ToLower().Equals("false"))
            {
                chkEnableTagAging.IsChecked = false;
            }
            else
            {
                // Notify the error message to the user
                NotifyInvalidLoadConfigOption("/application/displayOption/enableTagAging");
            }

            // Refresh [TagResults] for every refresh rate interval set
            if (loadSaveConfig.Properties.ContainsKey("/application/displayOption/refreshRate"))
            {
                if (Utilities.AreAllValidNumericChars(loadSaveConfig.Properties["/application/displayOption/refreshRate"]))
                {
                    txtRefreshRate.Text = loadSaveConfig.Properties["/application/displayOption/refreshRate"];
                    txtRefreshRate.Focus();
                }
                else
                {
                    string tempValueRefreshRate = txtRefreshRate.Text;
                    NotifyLoadSaveConfigErrorMessage("Saved Refresh rate [/application/displayOption/refreshRate] value ["
                           + loadSaveConfig.Properties["/application/displayOption/refreshRate"] + "] failed. Please enter only number."
                           + "URA sets refresh rate to previous set value [" + tempValueRefreshRate + "] or change to the supported "
                           +"value and reload the configuration");
                    txtRefreshRate.Text = tempValueRefreshRate;
                }
            }
            else
            {
                txtRefreshRate.Text = "100";
            }

            // Select columns to be displayed on Tag Results
            if (loadSaveConfig.Properties.ContainsKey("/application/displayOption/tagResultColumnSelection"))
            {
                int columnInd = 0;
                foreach (ColumnSelectionForTagResult item in cbxcolumnSelection.Items)
                {
                    if (loadSaveConfig.Properties.ContainsKey("/application/displayOption/tagResultColumnSelection/enable"
                        + item.SelectedColumn))
                    {
                        if (loadSaveConfig.Properties["/application/displayOption/tagResultColumnSelection/enable"
                            +item.SelectedColumn].ToLower().Equals("true"))
                        {
                            selectedColumnList[columnInd].IsColumnChecked = true;
                        }
                        else if (loadSaveConfig.Properties["/application/displayOption/tagResultColumnSelection/enable"
                            +item.SelectedColumn].ToLower().Equals("false"))
                        {
                            selectedColumnList[columnInd].IsColumnChecked = false;
                        }
                        else
                        {
                            // Notify the error message to the user
                            MessageBox.Show("Saved parameter [/application/displayOption/tagResultColumnSelection/enable"
                                + item.SelectedColumn +"] parameter has invalid value [" +
                        loadSaveConfig.Properties["/application/displayOption/tagResultColumnSelection/enable"
                        +item.SelectedColumn] + "]. This parameter accepts only true or false."
                            + " URA sets this parameter to previous set value ", "Universal Reader Assistant",
                            MessageBoxButton.OK, MessageBoxImage.Error);
                            Onlog("Saved parameter [/application/displayOption/tagResultColumnSelection/enable" 
                                + item.SelectedColumn + "]  parameter has invalid value [" +
                                        loadSaveConfig.Properties["/application/displayOption/tagResultColumnSelection/enable"
                                        + item.SelectedColumn] + "]. This parameter accepts only true or false."
                                            + " URA sets this parameter to previous set value ");            
                        }
                    }
                    else
                    {
                        selectedColumnList[columnInd].IsColumnChecked = false;
                    }
                    ++columnInd;
                }
                cbxcolumnSelection.ItemsSource = null;
                cbxcolumnSelection.ItemsSource = selectedColumnList;
                AddColumnsToGrid(this, new RoutedEventArgs());
                cbxcolumnSelection.InvalidateVisual();
            }

            // Time stamp format
            if (loadSaveConfig.Properties.ContainsKey("/application/displayOption/tagResultColumn/timeStampFormat"))
            {
                if (loadSaveConfig.Properties["/application/displayOption/tagResultColumn/timeStampFormat"].Equals(""))
                {
                    cbxTimestampFormat.SelectedIndex = 0;
                }
                else
                {
                    cbxTimestampFormat.SelectedIndex = GetIndexOf(cbxTimestampFormat, 
                        loadSaveConfig.Properties["/application/displayOption/tagResultColumn/timeStampFormat"],
                        "TimeStamp Format");
                }
            }

            // BigNum Selection
            if (loadSaveConfig.Properties.ContainsKey("/application/displayOption/tagResult/bigNumSelection"))
            {
                if (loadSaveConfig.Properties["/application/displayOption/tagResult/bigNumSelection"].Equals(""))
                {
                    cbxBigNum.SelectedIndex = 0;
                }
                else
                {
                    cbxBigNum.SelectedIndex = GetIndexOf(cbxBigNum, 
                        loadSaveConfig.Properties["/application/displayOption/tagResult/bigNumSelection"],
                        "BigNum Selection");
                }
            }

            // Displays EPC in selected format
            if (loadSaveConfig.Properties.ContainsKey("/application/displayOption/tagResultColumn/displayEPCAs"))
            {
                if (loadSaveConfig.Properties["/application/displayOption/tagResultColumn/displayEPCAs"].Equals(""))
                {
                    cbxDisplayEPCAs.SelectedIndex = 0;
                }
                else
                {
                    cbxDisplayEPCAs.SelectedIndex = GetIndexOf(cbxDisplayEPCAs, 
                        loadSaveConfig.Properties["/application/displayOption/tagResultColumn/displayEPCAs"],
                        "Displays EPC in selected format");
                }
            }

            // Display embedded read data column format
            if ((bool)chkEmbeddedReadData.IsChecked)
            {
                if (loadSaveConfig.Properties.ContainsKey("/application/displayOption/tagResultColumn/displayEmbeddedReadDataAs"))
                {
                    if (loadSaveConfig.Properties["/application/displayOption/tagResultColumn/displayEmbeddedReadDataAs"].Equals(""))
                    {
                        cbxDisplayEmbRdDataAs.SelectedIndex = 0;
                    }
                    else
                    {
                        cbxDisplayEmbRdDataAs.SelectedIndex = GetIndexOf(cbxDisplayEmbRdDataAs,
                            loadSaveConfig.Properties["/application/displayOption/tagResultColumn/displayEmbeddedReadDataAs"],
                            "Displays embedded read data in selected format");
                    }
                }
            }
        }

        /// <summary>
        /// Load manual gen2 configurations from the profile and set the custom
        /// gen2 settings control in performance tunning sections
        /// </summary>
        private void LoadManualGen2ConfigurationFromProfile()
        {
            // log the parameter
            Onlog("/application/performanceTuning/configureGen2SettingsType" + "Value: " + "Manual");
            chkCustomizeGen2Settings.IsChecked = true;
            try
            {
                if ((model.Equals("Mercury6")) || (model.Equals("Astra-EX"))
                    || (model.Equals("M6e")) || (model.Equals("M6e Micro"))
                    || (model.Equals("M6e PRC")))
                {
                    switch (loadSaveConfig.Properties["/reader/gen2/BLF"])
                    {
                        case "LINK250KHZ":
                            LINK250KHZ.IsChecked = true; break;
                        case "LINK640KHZ":
                            LINK640KHZ.IsChecked = true; break;
                        default:
                            NotifyLoadSaveConfigErrorMessage("Saved BLF [/reader/gen2/BLF] has invalid BLF value ["
                            + loadSaveConfig.Properties["/reader/gen2/BLF"] + "]"
                            + " getting and setting the BLF value [" + OptimalReaderSettings["/reader/gen2/BLF"]
                            + "] from the reader or change to the supported value and reload the configuration");
                            LINK250KHZ.IsChecked = true;
                            switch (OptimalReaderSettings["/reader/gen2/BLF"])
                            {
                                case "LINK250KHZ":
                                    LINK250KHZ.IsChecked = true; break;
                                case "LINK640KHZ":
                                    LINK640KHZ.IsChecked = true; break;
                            }
                            break;
                    }
                }
                else
                {
                    if (model.Equals("Astra"))
                    {
                        NotifyLoadSaveConfigErrorMessage("Saved BLF [/reader/gen2/BLF] is not supported for connected reader. "
                               + "Connected reader doesn't support BLF. URA skips this settings");
                    }
                    else if (loadSaveConfig.Properties["/reader/gen2/BLF"].Equals("LINK250KHZ"))
                    {
                        LINK250KHZ.IsChecked = true;
                    }
                    else if (loadSaveConfig.Properties["/reader/gen2/BLF"].Equals("LINK640KHZ"))
                    {
                        NotifyLoadSaveConfigErrorMessage("Saved BLF [/reader/gen2/BLF] has invalid BLF value ["
                            + loadSaveConfig.Properties["/reader/gen2/BLF"] + "] for connected reader."
                            + " Getting and setting the BLF value [LINK250KHZ] from the reader or change to the supported value"
                            + " and reload the configuration");
                        LINK250KHZ.IsChecked = true;
                    }
                    else
                    {
                        NotifyLoadSaveConfigErrorMessage("Saved BLF [/reader/gen2/BLF] has invalid BLF value ["
                            + loadSaveConfig.Properties["/reader/gen2/BLF"] + "] for connected reader."
                            + " Getting and setting the BLF value [LINK250KHZ] from the reader or change to the supported value"
                            + " and reload the configuration");
                        LINK250KHZ.IsChecked = true;
                    }
                }
            }
            catch (ArgumentException ex)
            {
                Onlog("ArgumentException : " + ex.Message);
                Onlog(ex.StackTrace);
            }
            try
            {
                if ((model.Equals("Mercury6")) || (model.Equals("Astra-EX"))
                   || (model.Equals("M6e")) || (model.Equals("M6e Micro"))
                   || (model.Equals("M6e PRC")))
                {
                    switch (loadSaveConfig.Properties["/reader/gen2/tari"])
                    {
                        case "TARI_6_25US":
                            tari625.IsChecked = true; break;
                        case "TARI_12_5US":
                            tari125.IsChecked = true; break;
                        case "TARI_25US":
                            tari25.IsChecked = true; break;
                        default:
                            NotifyLoadSaveConfigErrorMessage("Saved Tari [/reader/gen2/tari] has invalid tari value ["
                            + loadSaveConfig.Properties["/reader/gen2/tari"] + "]."
                            + " Getting and setting the tari value [" + OptimalReaderSettings["/reader/gen2/tari"]
                            + "] from the reader or change to the supported value and reload the configuration");
                            switch (OptimalReaderSettings["/reader/gen2/tari"])
                            {
                                case "TARI_6_25US":
                                    tari625.IsChecked = true; break;
                                case "TARI_12_5US":
                                    tari125.IsChecked = true; break;
                                case "TARI_25US":
                                    tari25.IsChecked = true; break;
                            }
                            break;
                    }
                }
                else
                {
                    if (model.Equals("Astra"))
                    {
                        NotifyLoadSaveConfigErrorMessage("Saved tari [/reader/gen2/tari] is not supported for connected reader. "
                                + "Connected reader doesn't support tari. URA skips this settings");
                    }
                    else if (loadSaveConfig.Properties["/reader/gen2/tari"] != "")
                    {
                            NotifyLoadSaveConfigErrorMessage("Saved tari [/reader/gen2/tari] is not supported for connected reader. "
                                + "Connected reader doesn't support tari. URA skips this settings");
                    }
                    grpbxtTari.IsEnabled = false;
                }
            }
            catch (ArgumentException ex)
            {
                Onlog("ArgumentException : " + ex.Message);
                Onlog(ex.StackTrace);
            }
            catch (ReaderCodeException ex)
            {
                Onlog("ReaderCodeException : " + ex.Message);
                Onlog(ex.StackTrace);
            }

            try
            {
                if (model.Equals("Astra"))
                {
                    NotifyLoadSaveConfigErrorMessage("Saved tagEncoding [/reader/gen2/tagEncoding] is not supported for connected reader. "
                            + "Connected reader doesn't support tagEncoding. URA skips this settings");
                    FM0.IsEnabled = false;
                    M2.IsEnabled = false;
                    M4.IsEnabled = false;
                    M8.IsEnabled = false;
                }
                else
                {
                    switch (loadSaveConfig.Properties["/reader/gen2/tagEncoding"])
                    {
                        case "FM0":
                            if ((model.Equals("Mercury6")) || (model.Equals("Astra-EX"))
                                || (model.Equals("M6e")) || (model.Equals("M6e Micro"))
                                || (model.Equals("M6e PRC")))
                            {
                                FM0.IsChecked = true;
                            }
                            else
                            {
                                NotifyLoadSaveConfigErrorMessage("Saved tagEncoding [/reader/gen2/tagEncoding] has invalid tag encoding value ["
                                   + loadSaveConfig.Properties["/reader/gen2/tagEncoding"] + "] for connected reader."
                                   + " Getting and setting to the tag encoding value [" + OptimalReaderSettings["/reader/gen2/tagEncoding"]
                                   + "] or change to the supported value"
                                   + " and reload the configuration");
                                SetTagEncodingBasedOnReader();
                                FM0.IsEnabled = false;
                            }
                            break;
                        case "M2":
                            M2.IsChecked = true; break;
                        case "M4":
                            M4.IsChecked = true; break;
                        case "M8":
                            M8.IsChecked = true; break;
                        default:
                            NotifyLoadSaveConfigErrorMessage("Saved tagEncoding [/reader/gen2/tagEncoding] has invalid tag encoding value ["
                            + loadSaveConfig.Properties["/reader/gen2/tagEncoding"] + "]."
                            + " Getting and setting to the tag encoding value [" + OptimalReaderSettings["/reader/gen2/tagEncoding"]
                            + "] or change to the supported value"
                            + " and reload the configuration");
                            SetTagEncodingBasedOnReader();
                            break;
                    }
                }
            }
            catch (ArgumentException)
            {
                FM0.IsEnabled = false;
                M2.IsEnabled = false;
                M4.IsEnabled = false;
                M8.IsEnabled = false;
            }
            try
            {
                switch (loadSaveConfig.Properties["/reader/gen2/session"])
                {
                    case "S0":
                        S0.IsChecked = true; break;
                    case "S1":
                        S1.IsChecked = true; break;
                    case "S2":
                        S2.IsChecked = true; break;
                    case "S3":
                        S3.IsChecked = true; break;
                    default:
                        NotifyLoadSaveConfigErrorMessage("Saved session [/reader/gen2/session] has invalid session value ["
                            + loadSaveConfig.Properties["/reader/gen2/session"] + "]."
                            + " Getting and setting the session value [" + OptimalReaderSettings["/reader/gen2/session"] + "] from the reader"
                            + " or change to the supported value"
                            + " and reload the configuration");
                        switch (OptimalReaderSettings["/reader/gen2/session"])
                        {
                            case "S0":
                                S0.IsChecked = true; break;
                            case "S1":
                                S1.IsChecked = true; break;
                            case "S2":
                                S2.IsChecked = true; break;
                            case "S3":
                                S3.IsChecked = true; break;
                        }
                        break;
                }
            }
            catch (ArgumentException)
            {
                S0.IsEnabled = false;
                S1.IsEnabled = false;
                S2.IsEnabled = false;
                S3.IsEnabled = false;
            }
            try
            {
                switch (loadSaveConfig.Properties["/reader/gen2/target"])
                {
                    case "A":
                        A.IsChecked = true; break;
                    case "B":
                        B.IsChecked = true; break;
                    case "AB":
                        AB.IsChecked = true; break;
                    case "BA":
                        BA.IsChecked = true; break;
                    default:
                        NotifyLoadSaveConfigErrorMessage("Saved target [/reader/gen2/target] has invalid session value ["
                            + loadSaveConfig.Properties["/reader/gen2/target"] + "]."
                            + " Getting and setting the session value [" + OptimalReaderSettings["/reader/gen2/target"] + "] from the reader"
                            + " or change to the supported value"
                            + " and reload the configuration");
                        switch (OptimalReaderSettings["/reader/gen2/target"])
                        {
                            case "A":
                                A.IsChecked = true; break;
                            case "B":
                                B.IsChecked = true; break;
                            case "AB":
                                AB.IsChecked = true; break;
                            case "BA":
                                BA.IsChecked = true; break;
                        }
                        break;
                }
            }
            catch (FeatureNotSupportedException)
            {
                A.IsEnabled = false;
                B.IsEnabled = false;
                AB.IsEnabled = false;
                BA.IsEnabled = false;
            }
            try
            {
                if (model.Equals("Astra"))
                {
                    NotifyLoadSaveConfigErrorMessage("Saved q [/reader/gen2/q] is not supported for connected reader. "
                            + "Connected reader doesn't support q. URA skips this settings");
                    StaticQ.IsEnabled = false;
                    DynamicQ.IsEnabled = false;
                }
                else if (loadSaveConfig.Properties["/reader/gen2/q"] == "DynamicQ")
                {
                    DynamicQ.IsChecked = true;
                    Qvalue.SelectedIndex = -1;
                }
                else if (loadSaveConfig.Properties["/reader/gen2/q"] == "StaticQ")
                {
                    StaticQ.IsChecked = true;
                    Qvalue.IsEnabled = true;
                    try
                    {
                        if (Convert.ToInt32(loadSaveConfig.Properties["/application/performanceTuning/staticQValue"])
                            <= Qvalue.Items.Count)
                        {
                            Qvalue.SelectedIndex = Convert.ToInt32(loadSaveConfig.Properties[
                                "/application/performanceTuning/staticQValue"]);
                        }
                        else
                        {
                            NotifyLoadSaveConfigErrorMessage("Saved static q [/application/performanceTuning/staticQValue] has invalid StaticQ value ["
                            + loadSaveConfig.Properties["/application/performanceTuning/staticQValue"] + "]. Enter Q value within 0 to 15"
                            + " Getting and setting the q value [" + OptimalReaderSettings["/application/performanceTuning/staticQValue"]
                            + "] from the reader or change to the supported value and reload the configuration");
                            Qvalue.SelectedIndex = Convert.ToInt32(OptimalReaderSettings["/application/performanceTuning/staticQValue"]);
                        }
                    }
                    catch (Exception)
                    {
                        NotifyLoadSaveConfigErrorMessage("Saved static q [/application/performanceTuning/staticQValue] has invalid StaticQ value ["
                            + loadSaveConfig.Properties["/application/performanceTuning/staticQValue"] + "]."
                            + " Getting and setting the q value [" + OptimalReaderSettings["/application/performanceTuning/staticQValue"]
                            + "] from the reader or change to the supported value and reload the configuration");
                        Qvalue.SelectedIndex = Convert.ToInt32(OptimalReaderSettings["/application/performanceTuning/staticQValue"]);
                    }
                }
                else
                {
                    if (OptimalReaderSettings["/reader/gen2/q"] == "DynamicQ")
                    {
                        NotifyLoadSaveConfigErrorMessage("Saved q [/reader/gen2/q] has invalid q value ["
                            + loadSaveConfig.Properties["/reader/gen2/q"] + "]."
                            + " Getting and setting the q value [" + OptimalReaderSettings["/reader/gen2/q"] + "] from the reader"
                            + " or change to the supported value"
                            + " and reload the configuration");
                        DynamicQ.IsChecked = true;
                        Qvalue.SelectedIndex = -1;
                    }
                    else if (OptimalReaderSettings["/reader/gen2/q"] == "StaticQ")
                    {
                        NotifyLoadSaveConfigErrorMessage("Saved q [/reader/gen2/q] has invalid q value ["
                            + loadSaveConfig.Properties["/reader/gen2/q"] + "]."
                            + " Getting and setting the q value [" + OptimalReaderSettings["/reader/gen2/q"] + "] from the reader"
                            + " or change to the supported value"
                            + " and reload the configuration");
                        StaticQ.IsChecked = true;
                        Qvalue.IsEnabled = true;
                        Qvalue.SelectedIndex = Convert.ToInt32(OptimalReaderSettings["/application/performanceTuning/staticQValue"]);
                    }
                }
            }
            catch (FeatureNotSupportedException)
            {
                StaticQ.IsEnabled = false;
                DynamicQ.IsEnabled = false;
            }
        }

        /// <summary>
        /// Load auto gen2 configurations from the profile and set the
        /// controls in performance tunning section
        /// </summary>
        private void LoadAutoGen2ConfigurationFromProfile()
        {
            chkCustomizeGen2Settings.IsChecked = false;
            // Automatically adjust as population changes
                if (loadSaveConfig.Properties["/application/performanceTuning/automaticallyAdjustAsPopulationChanges"].ToLower().Equals("true"))
                {
                    if (rdbtnAutoAdjstAsPoplChngs.IsEnabled)
                    {
                        rdbtnAutoAdjstAsPoplChngs.IsChecked = true;
                        rdbtnOptmzExtmdNoTagsInField.IsChecked = false;
                    }                    
                    else
                    {
                        // Embedded read data is not supported by connected reader
                        NotifyLoadSaveConfigErrorMessage("Saved parameter [/application/performanceTuning/automaticallyAdjustAsPopulationChanges] "
                            + " is not supported by connected reader. URA skips this setting.");
                        rdbtnAutoAdjstAsPoplChngs.IsEnabled = false;
                    }
                }
                else if (loadSaveConfig.Properties["/application/performanceTuning/automaticallyAdjustAsPopulationChanges"].ToLower().Equals("false"))
                {
                    rdbtnAutoAdjstAsPoplChngs.IsChecked = false;
                    rdbtnOptmzExtmdNoTagsInField.IsChecked = true;
                }
                else
                {
                    // Notify the error message to the user
                    NotifyInvalidLoadConfigOption("/application/performanceTuning/automaticallyAdjustAsPopulationChanges");
                }            

            // Optimize for estimated number of tags in field
           
                if (loadSaveConfig.Properties["/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField"].ToLower().Equals("true"))
                {
                    if (rdbtnOptmzExtmdNoTagsInField.IsEnabled)
                    {
                        rdbtnOptmzExtmdNoTagsInField.IsChecked = true;
                        rdbtnAutoAdjstAsPoplChngs.IsChecked = false;
                        string tempValue = txtTagsNum.Text;
                        if (Utilities.AreAllValidNumericChars(
                            loadSaveConfig.Properties["/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField/tagsInTheField"]))
                        {
                            try
                            {
                                int tagcount = Convert.ToInt32(
                                    loadSaveConfig.Properties["/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField/tagsInTheField"]);
                                if ((tagcount >= 1) && (tagcount <= 99999))
                                {
                                    txtTagsNum.Text = loadSaveConfig.Properties[
                                        "/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField/tagsInTheField"];
                                }
                                else
                                {
                                    NotifyLoadSaveConfigErrorMessage("Saved estimated number of tags "
                                    + "in field [/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField/tagsInTheField] value ["
                                    + loadSaveConfig.Properties["/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField/tagsInTheField"]
                                    + "] failed. Please enter number within 1 to 99999. URA sets estimated number of tags in field to previous "
                                    + "set value [" + tempValue + "] or change to the supported value and reload the configuration");
                                    txtTagsNum.Text = tempValue;
                                }
                            }
                            catch (Exception)
                            {
                                NotifyLoadSaveConfigErrorMessage("Saved estimated number of tags "
                                    + "in field [/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField/tagsInTheField] value ["
                                    + loadSaveConfig.Properties["/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField/tagsInTheField"]
                                    + "] failed. Please enter number within 1 to 99999. URA sets estimated number of tags in field to previous "
                                    + "set value [" + tempValue + "] or change to the supported value and reload the configuration");
                                txtTagsNum.Text = tempValue;
                            }
                        }
                        else
                        {
                            NotifyLoadSaveConfigErrorMessage("Saved estimated number of tags "
                                + "in field [/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField/tagsInTheField] value ["
                                   + loadSaveConfig.Properties["/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField/tagsInTheField"]
                                   + "] failed. Please enter only positive number. URA sets estimated number of tags in field to previous "
                                   + "set value [" + tempValue + "] or change to the supported value and reload the configuration");
                            txtTagsNum.Text = tempValue;
                        }
                        // update static q count
                        UpDownCounterTextChange(this);
                    }
                    else
                    {
                        // Embedded read data is not supported by connected reader
                        NotifyLoadSaveConfigErrorMessage("Saved parameter [/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField] "
                            + " is not supported by connected reader. URA skips this setting.");
                        rdbtnOptmzExtmdNoTagsInField.IsEnabled = false;
                        stkpnlOptmzExtmdNoTagsInField.IsEnabled = false;
                        //stkpnlRdDistVsrdRate.IsEnabled = false;
                        //rdBtnSlBstChforPoplSize.IsEnabled = false;
                    }
                }
                else if (loadSaveConfig.Properties["/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField"].ToLower().Equals("false"))
                {
                    rdbtnOptmzExtmdNoTagsInField.IsChecked = false;
                    if (!(model.Equals("Astra")))
                    {
                        rdbtnAutoAdjstAsPoplChngs.IsChecked = true;
                    }
                    txtTagsNum.Text = "1";
                    // update static q count
                    UpDownCounterTextChange(this);
                }
                else
                {
                    // Notify the error message to the user
                    NotifyInvalidLoadConfigOption("/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField");
                }

            // Read Distance vs. Read Rate

                if (loadSaveConfig.Properties.ContainsKey("/application/performanceTuning/readDistancevsReadRate"))
                {
                    if (stkpnlRdDistVsrdRate.IsEnabled)
                    {
                        try
                        {
                            double readDistValue = 0.0;
                            readDistValue = Convert.ToDouble(loadSaveConfig.Properties["/application/performanceTuning/readDistancevsReadRate"]);
                            if ((readDistValue >= sldrRdDistVsrdRate.Minimum)
                                && (readDistValue <= sldrRdDistVsrdRate.Maximum))
                            {
                                sldrRdDistVsrdRate.Value = Convert.ToDouble(
                                    loadSaveConfig.Properties["/application/performanceTuning/readDistancevsReadRate"]);
                            }
                            else
                            {
                                NotifyLoadSaveConfigErrorMessage("Saved read distance vs. read rate "
                                + "[/application/performanceTuning/readDistancevsReadRate] value ["
                                   + loadSaveConfig.Properties["/application/performanceTuning/readDistancevsReadRate"]
                                   + "] failed. Please enter only positive number within " + sldrRdDistVsrdRate.Minimum
                                   + " and " + sldrRdDistVsrdRate.Maximum + "."
                                   + " URA sets to previous set value or change to the supported value and reload the configuration");
                            }
                        }
                        catch (Exception ex)
                        {
                            NotifyLoadSaveConfigErrorMessage("Load parameter [/application/performanceTuning/readDistancevsReadRate]: "
                                   + ex.Message + "Please enter only positive number within " + sldrRdDistVsrdRate.Minimum + " and "
                                   + sldrRdDistVsrdRate.Maximum + "."
                                   + " URA sets to previous set value or change to the supported value and reload the configuration");
                        }
                    }
                    else
                    {
                        // Embedded read data is not supported by connected reader
                        NotifyLoadSaveConfigErrorMessage("Saved parameter [/application/performanceTuning/readDistancevsReadRate] "
                            + " is not supported by connected reader. URA skips this setting.");
                        stkpnlRdDistVsrdRate.IsEnabled = false;
                        sldrRdDistVsrdRate.Value = 0;
                    }
                }
                else
                {
                    sldrRdDistVsrdRate.Value = 0;
                }


            // Select best choice for population size
            
                if (loadSaveConfig.Properties[
                    "/application/performanceTuning/selectBestChoiceForPopulationSize"].ToLower().Equals("true"))
                {
                    if (rdBtnSlBstChforPoplSize.IsEnabled)
                    {
                        rdBtnSlBstChforPoplSize.IsChecked = true;
                        rdBtnTagsRespondOption.IsChecked = false;
                    }
                    else
                    {
                        // Embedded read data is not supported by connected reader
                        NotifyLoadSaveConfigErrorMessage("Saved parameter [/application/performanceTuning/selectBestChoiceForPopulationSize] "
                            + " is not supported by connected reader. URA skips this setting.");
                        ////rdbtnOptmzExtmdNoTagsInField.IsEnabled = false;
                        ////stkpnlOptmzExtmdNoTagsInField.IsEnabled = false;
                        //stkpnlRdDistVsrdRate.IsEnabled = false;
                        rdBtnSlBstChforPoplSize.IsEnabled = false;
                    }
                }
                else if (loadSaveConfig.Properties["/application/performanceTuning/selectBestChoiceForPopulationSize"].ToLower().Equals("false"))
                {
                    rdBtnSlBstChforPoplSize.IsChecked = false;
                    rdBtnTagsRespondOption.IsChecked = true;
                }
                else
                {
                    // Notify the error message to the user
                    NotifyInvalidLoadConfigOption("/application/performanceTuning/selectBestChoiceForPopulationSize");
                }            

            // Customize tag response rate
            if (loadSaveConfig.Properties["/application/performanceTuning/customizeTagResponseRate"].ToLower().Equals("true"))
            {
                rdBtnTagsRespondOption.IsChecked = true;
                rdBtnSlBstChforPoplSize.IsChecked = false;
                try
                {
                    double tagResponseRate = 0.0;
                    tagResponseRate = Convert.ToDouble(loadSaveConfig.Properties[
                        "/application/performanceTuning/customizeTagResponseRate/TagResponseRate"]);
                    if ((tagResponseRate >= sldrTagrspLessVsTagrspMore.Minimum)
                        && (tagResponseRate <= sldrTagrspLessVsTagrspMore.Maximum))
                    {
                        sldrTagrspLessVsTagrspMore.Value = Convert.ToDouble(
                            loadSaveConfig.Properties["/application/performanceTuning/customizeTagResponseRate/TagResponseRate"]);
                    }
                    else
                    {
                        NotifyLoadSaveConfigErrorMessage("Saved Customize tag response rate "
                        + "[/application/performanceTuning/customizeTagResponseRate/TagResponseRate] value ["
                           + loadSaveConfig.Properties["/application/performanceTuning/customizeTagResponseRate/TagResponseRate"]
                           + "] failed. Please enter only positive number within " + sldrTagrspLessVsTagrspMore.Minimum + " and " 
                           + sldrTagrspLessVsTagrspMore.Maximum + "."
                           + " URA sets to previous set value or change to the supported value and reload the configuration");
                    }
                }
                catch (Exception ex)
                {
                    NotifyLoadSaveConfigErrorMessage("Load parameter [/application/performanceTuning/customizeTagResponseRate]: "
                        + ex.Message +" Please enter only positive number within " + sldrTagrspLessVsTagrspMore.Minimum + " and " 
                           + sldrTagrspLessVsTagrspMore.Maximum + "."
                           + " URA sets to previous set value or change to the supported value and reload the configuration");
                }
            }
            else if (loadSaveConfig.Properties["/application/performanceTuning/customizeTagResponseRate"].ToLower().Equals("false"))
            {
                if (model.Equals("Astra"))
                {
                    // If connected reader is astra and if /application/performanceTuning/selectBestChoiceForPopulationSize parameter
                    // is set to false when loading configurations of other reader. Set  /application/performanceTuning/customizeTagResponseRate
                    // parameter as true.
                    rdBtnTagsRespondOption.IsChecked = true;
                    sldrTagrspLessVsTagrspMore.Value = 0;
                }
                else
                {
                    rdBtnTagsRespondOption.IsChecked = false;
                    rdBtnSlBstChforPoplSize.IsChecked = true;
                    sldrTagrspLessVsTagrspMore.Value = 0;
                }
            }
            else
            {
                // Notify the error message to the user
                NotifyInvalidLoadConfigOption("/application/performanceTuning/customizeTagResponseRate");
            }
        }

        /// <summary>
        /// Method to set tag encoding based on reader
        /// </summary>
        private void SetTagEncodingBasedOnReader()
        {
            switch (OptimalReaderSettings["/reader/gen2/tagEncoding"])
            {
                case "FM0":
                    FM0.IsChecked = true;
                    break;
                case "M2":
                    M2.IsChecked = true; break;
                case "M4":
                    M4.IsChecked = true; break;
                case "M8":
                    M8.IsChecked = true; break;
            }
        }

        /// <summary>
        /// Get the index of specified element in the combo-box
        /// </summary>
        /// <param name="element">Combo-box control</param>
        /// <param name="FindIndexOf">string to be searched in the list of combo-box elements</param>
        /// <returns>index of the specified string</returns>
        private int GetIndexOf(Control element, string FindIndexOf, string cbxOptionName)
        {
            int index = -1;
            ComboBox comboBox = (ComboBox)element;
            bool iselementFound = false;
            foreach (ComboBoxItem cmbItem in comboBox.Items)
            {
                index++;
                if (cmbItem.Content.ToString() == FindIndexOf)
                {
                    iselementFound = true;
                    break; 
                }
            }
            if (!iselementFound)
            {
                // if element was not found return index of previous state
                index = comboBox.SelectedIndex;
                NotifyLoadSaveConfigErrorMessage("Saved option [" + FindIndexOf + "] not found in [" + cbxOptionName + "]."
                    + " Invalid option, URA sets to previous Chosen [" + comboBox.Text + "] as option for [" + cbxOptionName + "].");
                //MessageBox.Show("Saved option [" + FindIndexOf + "] not found in [" + cbxOptionName + "]."
                //    +" Invalid option, URA sets to previous Chosen [" + comboBox.Text + "] as option for [" + cbxOptionName + "].",
                //    "Universal Reader Assistant", MessageBoxButton.OK, MessageBoxImage.Exclamation);
                //Onlog("Saved option [" + FindIndexOf + "] not found in [" + cbxOptionName + "]."
                //    +" Invalid option, URA sets to previous Chosen [" + comboBox.Text + "] as option for [" + cbxOptionName + "].");
            }
            return index;
        }

        /// <summary>
        /// Get the index of the specified region element
        /// </summary>
        /// <param name="element">Combo-box control</param>
        /// <param name="FindIndexOf">region to be searched in combo-box</param>
        /// <returns>index of the specified region</returns>
        private int GetRegionIndexOf(Control element, string FindIndexOf, out bool status)
        {
            int index = -1;
            status = false;
            ComboBox comboBox = (ComboBox)element;
            bool isRegionFound = false;
            foreach (Reader.Region cmbItem in comboBox.Items)
            {
                index++;
                if (cmbItem.ToString() == FindIndexOf)
                {
                    isRegionFound = true;
                    status = true;
                    break;
                }
            }
            if (!isRegionFound)
            {
                // If no region found then return the index of the first element in the combo-box
                index = 0;
                status = false;
            }
            return index;
        }

        /// <summary>
        /// Get the parameters to be saved in the configuration file
        /// </summary>
        /// <returns>List of parameters to be saved int the configuration file</returns>
        private Dictionary<string, string> GetParametersToSave()
        {
            Dictionary<string, string> saveConfigurationList = new Dictionary<string, string>();

            // Save reader uri
            if ((bool)rdbtnLocalConnection.IsChecked)
            {
                saveConfigurationList.Add("/application/connect/readerType", "SerialReader");
                saveConfigurationList.Add("/application/connect/readerURI", uri);
            }
            else
            {
                saveConfigurationList.Add("/application/connect/readerType", "FixedReader");
                saveConfigurationList.Add("/application/connect/readerURI", uri);
            }

            // Baud rate
            if (cbxBaudRate.Visibility == System.Windows.Visibility.Visible)
            {
                if (cbxBaudRate.SelectedValue.ToString() != "Select")
                {
                    saveConfigurationList.Add("/reader/baudRate", cbxBaudRate.SelectedValue.ToString());
                }
                else
                {
                    saveConfigurationList.Add("/reader/baudRate", "");
                }
            }
            else
            {
                saveConfigurationList.Add("/reader/baudRate", "");
            }

            // Enable transport logging
            if ((bool)chkEnableTransportLogging.IsChecked)
            {
                saveConfigurationList.Add("/application/connect/enableTransportLogging",
                    ((bool)chkEnableTransportLogging.IsChecked).ToString().ToLower());
            }
            else
            {
                saveConfigurationList.Add("/application/connect/enableTransportLogging",
                    ((bool)chkEnableTransportLogging.IsChecked).ToString().ToLower());
            }

            // Set region
            if (regioncombo.IsEnabled)
            {
                saveConfigurationList.Add("/reader/region/id", regioncombo.SelectedValue.ToString());
            }
            else
            {
                saveConfigurationList.Add("/reader/region/id", regioncombo.SelectedValue.ToString());
            }

            // Set read behavior
            if ((bool)rdBtnReadOnce.IsChecked)
            {
                saveConfigurationList.Add("/application/readwriteOption/ReadBehaviour", "ReadOnce");
                saveConfigurationList.Add("/application/readwriteOption/readOnceTimeout", txtbxreadOnceTimeout.Text);
                saveConfigurationList.Add("/reader/read/asyncOnTime", txtRFOnTimeout.Text);
                saveConfigurationList.Add("/reader/read/asyncOffTime", txtRFOffTimeout.Text);
            }
            else
            {
                saveConfigurationList.Add("/application/readwriteOption/ReadBehaviour", "ReadContinuously");
                saveConfigurationList.Add("/reader/read/asyncOnTime", txtRFOnTimeout.Text);
                saveConfigurationList.Add("/reader/read/asyncOffTime", txtRFOffTimeout.Text);
                saveConfigurationList.Add("/application/readwriteOption/readOnceTimeout", txtbxreadOnceTimeout.Text);
            }

            // Set fast search
            if ((bool)chkEnableFastSearch.IsEnabled)
            {
                if ((bool)chkEnableFastSearch.IsChecked)
                {
                    saveConfigurationList.Add("/application/readwriteOption/enableFastSearch", "true");
                }
                else
                {
                    saveConfigurationList.Add("/application/readwriteOption/enableFastSearch", "false");
                }
            }
            else
            {
                // If fast search is not supported by connected reader, treat this as false
                saveConfigurationList.Add("/application/readwriteOption/enableFastSearch", "false");
            }

            // Antenna switching
            if ((bool)rdBtnEqualSwitching.IsChecked)
            {
                saveConfigurationList.Add("/application/readwriteOption/switchingMethod", "Equal");
            }
            else
            {
                saveConfigurationList.Add("/application/readwriteOption/switchingMethod", "Dynamic");
            }

            // Save protocol configurations
            StringBuilder protocols = new StringBuilder();
            foreach (CheckBox child in LogicalTreeHelper.GetChildren(stackpanel6))
            {
                if ((bool)child.IsChecked)
                {
                    protocols.Append(child.Content.ToString());
                    protocols.Append(",");
                }
            }
            saveConfigurationList.Add("/application/readwriteOption/Protocols", protocols.ToString().TrimEnd(','));

            // Save antenna configurations
            StringBuilder antennas = new StringBuilder();
            foreach (CheckBox child in LogicalTreeHelper.GetChildren(stackPanel8))
            {
                if ((bool)child.IsChecked)
                {
                    // Stack panel has antenna detection check-box as a child. Don't include this in antenna list.
                    if (!(child.Content.Equals("Antenna Detection")))
                    {
                        antennas.Append(child.Content.ToString());
                        antennas.Append(",");
                    }
                }
            }
            saveConfigurationList.Add("/application/readwriteOption/Antennas", antennas.ToString().TrimEnd(','));

            // Antenna detection
            if (chkbxAntennaDetection.IsEnabled)
            {
                if ((bool)chkbxAntennaDetection.IsChecked)
                {
                    saveConfigurationList.Add("/reader/antenna/checkPort", "true");
                }
                else
                {
                    saveConfigurationList.Add("/reader/antenna/checkPort", "false");
                }
            }
            else
            {
                // If the connected reader doesn't support antenna detection
                saveConfigurationList.Add("/reader/antenna/checkPort", "false");
            }

            // Embedded read data
            if (ReadDataGroupBox.IsEnabled)
            {
                if ((bool)chkEmbeddedReadData.IsChecked)
                {
                    saveConfigurationList.Add("/application/readwriteOption/enableEmbeddedReadData", "true");

                    // MemBank
                    saveConfigurationList.Add("/application/readwriteOption/enableEmbeddedReadData/MemBank",
                        cbxReadDataBank.Text);

                    // Start address
                    saveConfigurationList.Add("/application/readwriteOption/enableEmbeddedReadData/StartAddress",
                        txtembReadStartAddr.Text);

                    // Number of words to read
                    saveConfigurationList.Add("/application/readwriteOption/enableEmbeddedReadData/NoOfWordsToRead",
                        txtembReadLength.Text);
                }
                else
                {
                    saveConfigurationList.Add("/application/readwriteOption/enableEmbeddedReadData", "false");

                    // MemBank
                    saveConfigurationList.Add("/application/readwriteOption/enableEmbeddedReadData/MemBank",
                        cbxReadDataBank.Text);

                    // Start address
                    saveConfigurationList.Add("/application/readwriteOption/enableEmbeddedReadData/StartAddress",
                        txtembReadStartAddr.Text);

                    // Number of words to read
                    saveConfigurationList.Add("/application/readwriteOption/enableEmbeddedReadData/NoOfWordsToRead",
                        txtembReadLength.Text);
                }
            }
            else
            {
                // If connected reader doesn't support embedded read data
                saveConfigurationList.Add("/application/readwriteOption/enableEmbeddedReadData", "false");

                // MemBank
                saveConfigurationList.Add("/application/readwriteOption/enableEmbeddedReadData/MemBank",
                    cbxReadDataBank.Text);

                // Start address
                saveConfigurationList.Add("/application/readwriteOption/enableEmbeddedReadData/StartAddress",
                    txtembReadStartAddr.Text);

                // Number of words to read
                saveConfigurationList.Add("/application/readwriteOption/enableEmbeddedReadData/NoOfWordsToRead",
                    txtembReadLength.Text);
            }

            // UniqueByData
            if ((bool)chkUniqueByData.IsChecked)
            {
                saveConfigurationList.Add("/reader/tagReadData/uniqueByData", "true");
            }
            else
            {
                saveConfigurationList.Add("/reader/tagReadData/uniqueByData", "false");
            }

            // Show failed read data
            if ((bool)chkShowFailedDataReads.IsChecked)
            {
                saveConfigurationList.Add("/application/readwriteOption/enableEmbeddedReadData/uniqueByData/ShowFailedDataRead",
                    "true");
            }
            else
            {
                saveConfigurationList.Add("/application/readwriteOption/enableEmbeddedReadData/uniqueByData/ShowFailedDataRead",
                    "false");
            }

            // Apply filter
            if ((bool)chkApplyFilter.IsChecked)
            {
                saveConfigurationList.Add("/application/readwriteOption/applyFilter", "true");

                // MemBank
                saveConfigurationList.Add("/application/readwriteOption/applyFilter/FilterMemBank",
                    cbxFilterMemBank.Text);

                // Filter start address
                saveConfigurationList.Add("/application/readwriteOption/applyFilter/FilterStartAddress",
                    txtFilterStartAddr.Text);

                // Filter data
                saveConfigurationList.Add("/application/readwriteOption/applyFilter/FilterData",
                    txtFilterData.Text);
            }
            else
            {
                saveConfigurationList.Add("/application/readwriteOption/applyFilter", "false");
                // MemBank
                saveConfigurationList.Add("/application/readwriteOption/applyFilter/FilterMemBank",
                    cbxFilterMemBank.Text);

                // Filter start address
                saveConfigurationList.Add("/application/readwriteOption/applyFilter/FilterStartAddress",
                    txtFilterStartAddr.Text);

                // Filter data
                saveConfigurationList.Add("/application/readwriteOption/applyFilter/FilterData",
                    txtFilterData.Text);
            }

            if ((bool)chkFilterInvert.IsChecked)
            {
                saveConfigurationList.Add("/application/readwriteOption/applyFilter/InvertFilter", "true");
            }
            else
            {
                saveConfigurationList.Add("/application/readwriteOption/applyFilter/InvertFilter", "false");
            }

            // Save Performance tuning settings
            
            // Set read power
            saveConfigurationList.Add("/reader/radio/readPower", 
                (Convert.ToDouble(txtbxValuedBm.Text) * 100).ToString());
            if (grpbxPerformanceTuning.IsEnabled)
            {
                saveConfigurationList.Add("/application/performanceTuning/Enable", "true");
                if ((bool)chkCustomizeGen2Settings.IsChecked)
                {
                    saveConfigurationList.Add("/application/performanceTuning/configureGen2SettingsType", 
                        "Manual");
                    foreach (KeyValuePair<string, string> item in OptimalReaderSettings)
                    {
                        saveConfigurationList.Add(item.Key, item.Value);
                    }
                    if (model.Equals("Astra"))
                    {
                        // If connected model is astra, add default values for saving.
                        saveConfigurationList["/reader/gen2/BLF"] = "LINK250KHZ";
                        saveConfigurationList["/reader/gen2/tari"] = "TARI_6_25US";
                        saveConfigurationList["/reader/gen2/tagEncoding"] = "FM0";
                        saveConfigurationList["/reader/gen2/q"] = "DynamicQ";

                    }
                    // M5e variants not support tari value adding default value while saving
                    // configuration file
                    if (model.Equals("M5e PRC") || model.Equals("M5e Compact")||model.Equals("M5e"))
                    {
                        saveConfigurationList["/reader/gen2/tari"] = "TARI_6_25US";
                    }
                    if (saveConfigurationList["/reader/gen2/q"] == "DynamicQ")
                    {
                        // when ["/reader/gen2/q"] == "DynamicQ" then no need to save "/application/performanceTuning/staticQValue"
                        // in saveconfigurationList  
                        if (saveConfigurationList.ContainsKey("/application/performanceTuning/staticQValue"))
                        {
                            saveConfigurationList.Remove("/application/performanceTuning/staticQValue");
                        }
                    }
                    if (!(saveConfigurationList.ContainsKey("/application/performanceTuning/staticQValue")))
                    {
                        // Add [/application/performanceTuning/staticQValue] to saveconfigurationList
                        // only when ["/reader/gen2/q"] is StaticQ
                        if (saveConfigurationList.ContainsKey("/reader/gen2/q").Equals("StaticQ"))
                        saveConfigurationList.Add("/application/performanceTuning/staticQValue", "");
                    }
                    // Automatically adjust as population changes
                    saveConfigurationList.Add("/application/performanceTuning/automaticallyAdjustAsPopulationChanges",
                        "false");
                    // Optimize for estimated number of tags in field
                    saveConfigurationList.Add("/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField",
                        "false");
                    // Tags in the field
                    saveConfigurationList.Add("/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField/tagsInTheField",
                        txtTagsNum.Text);
                    // Read Distance vs. Read Rate
                    saveConfigurationList.Add("/application/performanceTuning/readDistancevsReadRate",
                        sldrRdDistVsrdRate.Value.ToString());

                    // Select best choice for population size
                    saveConfigurationList.Add("/application/performanceTuning/selectBestChoiceForPopulationSize", "false");
                    // Customize tag response rate
                    saveConfigurationList.Add("/application/performanceTuning/customizeTagResponseRate", "false");
                    // Tag response rate
                    saveConfigurationList.Add("/application/performanceTuning/customizeTagResponseRate/TagResponseRate",
                        sldrTagrspLessVsTagrspMore.Value.ToString());
                }
                else
                {
                    saveConfigurationList.Add("/application/performanceTuning/configureGen2SettingsType", "Auto");
                    foreach (KeyValuePair<string, string> item in OptimalReaderSettings)
                    {
                        saveConfigurationList.Add(item.Key, item.Value);
                    }
                    if (model.Equals("Astra"))
                    {
                        // If connected model is astra, add default values for saving.
                        saveConfigurationList["/reader/gen2/BLF"] = "LINK250KHZ";
                        saveConfigurationList["/reader/gen2/tari"] = "TARI_6_25US";
                        saveConfigurationList["/reader/gen2/tagEncoding"] = "FM0";
                        saveConfigurationList["/reader/gen2/q"] = "DynamicQ";
                    }
                    // M5e variants not support tari value adding default value while saving
                    // configuration file
                    if (model.Equals("M5e PRC") || model.Equals("M5e Compact")||model.Equals("M5e"))
                    {
                        saveConfigurationList["/reader/gen2/tari"] = "TARI_6_25US";
                    }
                    if (!(saveConfigurationList.ContainsKey("/application/performanceTuning/staticQValue")))
                    {
                        // Add [/application/performanceTuning/staticQValue] to saveconfigurationList
                        // only when ["/reader/gen2/q"] is StaticQ
                        if (saveConfigurationList.ContainsKey("/reader/gen2/q").Equals("StaticQ"))
                        saveConfigurationList.Add("/application/performanceTuning/staticQValue", "");
                    }
                    // Automatically adjust as population changes
                    if (rdbtnAutoAdjstAsPoplChngs.IsEnabled)
                    {
                        if ((bool)rdbtnAutoAdjstAsPoplChngs.IsChecked)
                        {
                            saveConfigurationList.Add("/application/performanceTuning/automaticallyAdjustAsPopulationChanges",
                                "true");
                        }
                        else
                        {
                            saveConfigurationList.Add("/application/performanceTuning/automaticallyAdjustAsPopulationChanges",
                                "false");
                        }
                    }
                    else
                    {
                        // If the connected reader doesn't support this parameter
                        saveConfigurationList.Add("/application/performanceTuning/automaticallyAdjustAsPopulationChanges",
                            "false");
                    }

                    // Optimize for estimated number of tags in field
                    if (rdbtnOptmzExtmdNoTagsInField.IsEnabled)
                    {
                        if ((bool)rdbtnOptmzExtmdNoTagsInField.IsChecked)
                        {
                            // Optimize for estimated number of tags in field
                            saveConfigurationList.Add("/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField",
                                "true");
                            // Tags in the field
                            saveConfigurationList.Add("/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField/tagsInTheField",
                                txtTagsNum.Text);
                        }
                        else
                        {
                            // Optimize for estimated number of tags in field
                            saveConfigurationList.Add("/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField", "false");
                            // Tags in the field
                            saveConfigurationList.Add("/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField/tagsInTheField",
                                txtTagsNum.Text);
                        }
                    }
                    else
                    {
                        // If the connected reader doesn't support this parameter
                        // Optimize for estimated number of tags in field
                        saveConfigurationList.Add("/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField", "false");
                        // Tags in the field
                        saveConfigurationList.Add("/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField/tagsInTheField",
                            txtTagsNum.Text);
                    }

                    // Read Distance vs. Read Rate
                    saveConfigurationList.Add("/application/performanceTuning/readDistancevsReadRate",
                        sldrRdDistVsrdRate.Value.ToString());

                    // Select best choice for population size
                    if (rdBtnSlBstChforPoplSize.IsEnabled)
                    {
                        if ((bool)rdBtnSlBstChforPoplSize.IsChecked)
                        {
                            saveConfigurationList.Add("/application/performanceTuning/selectBestChoiceForPopulationSize", "true");
                        }
                        else
                        {
                            saveConfigurationList.Add("/application/performanceTuning/selectBestChoiceForPopulationSize", "false");
                        }
                    }
                    else
                    {
                        // If the connected reader doesn't support this parameter
                        saveConfigurationList.Add("/application/performanceTuning/selectBestChoiceForPopulationSize", "false");
                    }

                    // Customize tag response rate
                    if ((bool)rdBtnTagsRespondOption.IsChecked)
                    {
                        saveConfigurationList.Add("/application/performanceTuning/customizeTagResponseRate", "true");
                        saveConfigurationList.Add("/application/performanceTuning/customizeTagResponseRate/TagResponseRate",
                            sldrTagrspLessVsTagrspMore.Value.ToString());
                    }
                    else
                    {
                        saveConfigurationList.Add("/application/performanceTuning/customizeTagResponseRate", "false");
                        saveConfigurationList.Add("/application/performanceTuning/customizeTagResponseRate/TagResponseRate",
                            sldrTagrspLessVsTagrspMore.Value.ToString());
                    }
                }
            }
            else
            {
                saveConfigurationList.Add("/application/performanceTuning/Enable", "false");
                // Save the present state of performance tuning in URA
                saveConfigurationList.Add("/application/performanceTuning/configureGen2SettingsType", "Auto");
                foreach (KeyValuePair<string, string> item in OptimalReaderSettings)
                {
                    saveConfigurationList.Add(item.Key,item.Value);
                }
                if (model.Equals("Astra"))
                {
                    // If connected model is astra, add default values for saving.
                    saveConfigurationList["/reader/gen2/BLF"] = "LINK250KHZ";
                    saveConfigurationList["/reader/gen2/tari"] = "TARI_6_25US";
                    saveConfigurationList["/reader/gen2/tagEncoding"] = "FM0";
                    saveConfigurationList["/reader/gen2/q"] = "DynamicQ";
                }
                // M5e variants not support tari value adding default value while saving
                // configuration file
                if (model.Equals("M5e PRC") || model.Equals("M5e Compact")||model.Equals("M5e"))
                {
                    saveConfigurationList["/reader/gen2/tari"] = "TARI_6_25US";
                }
                if (!(saveConfigurationList.ContainsKey("/application/performanceTuning/staticQValue")))
                {
                    // Add [/application/performanceTuning/staticQValue] to saveconfigurationList
                    // only when ["/reader/gen2/q"] is StaticQ
                    if (saveConfigurationList.ContainsKey("/reader/gen2/q").Equals("StaticQ"))
                    saveConfigurationList.Add("/application/performanceTuning/staticQValue", "");
                }
                // Automatically adjust as population changes
                if ((bool)rdbtnAutoAdjstAsPoplChngs.IsChecked)
                {
                    saveConfigurationList.Add("/application/performanceTuning/automaticallyAdjustAsPopulationChanges", "true");
                }
                else
                {
                    saveConfigurationList.Add("/application/performanceTuning/automaticallyAdjustAsPopulationChanges", "false");
                }

                // Optimize for estimated number of tags in field
                if ((bool)rdbtnOptmzExtmdNoTagsInField.IsChecked)
                {
                    // Optimize for estimated number of tags in field
                    saveConfigurationList.Add("/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField", "true");
                    // Tags in the field
                    saveConfigurationList.Add("/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField/tagsInTheField",
                        txtTagsNum.Text);
                }
                else
                {
                    // Optimize for estimated number of tags in field
                    saveConfigurationList.Add("/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField", "false");
                    // Tags in the field
                    saveConfigurationList.Add("/application/performanceTuning/optimizeForEstimatedNumberOfTagsInField/tagsInTheField",
                        txtTagsNum.Text);
                }

                // Read Distance vs. Read Rate
                saveConfigurationList.Add("/application/performanceTuning/readDistancevsReadRate",
                    sldrRdDistVsrdRate.Value.ToString());

                // Select best choice for population size
                if ((bool)rdBtnSlBstChforPoplSize.IsChecked)
                {
                    saveConfigurationList.Add("/application/performanceTuning/selectBestChoiceForPopulationSize", "true");
                }
                else
                {
                    saveConfigurationList.Add("/application/performanceTuning/selectBestChoiceForPopulationSize", "false");
                }

                // Customize tag response rate
                if ((bool)rdBtnTagsRespondOption.IsChecked)
                {
                    saveConfigurationList.Add("/application/performanceTuning/customizeTagResponseRate", "true");
                    saveConfigurationList.Add("/application/performanceTuning/customizeTagResponseRate/TagResponseRate",
                        sldrTagrspLessVsTagrspMore.Value.ToString());
                }
                else
                {
                    saveConfigurationList.Add("/application/performanceTuning/customizeTagResponseRate", "false");
                    saveConfigurationList.Add("/application/performanceTuning/customizeTagResponseRate/TagResponseRate",
                        sldrTagrspLessVsTagrspMore.Value.ToString());
                }
            }
            // Save display options
            // Font size
            saveConfigurationList.Add("/application/displayOption/fontSize", txtfontSize.Text);

            // chkEnableTagAging
            if ((bool)chkEnableTagAging.IsChecked)
            {
                saveConfigurationList.Add("/application/displayOption/enableTagAging", "true");
            }
            else
            {
                saveConfigurationList.Add("/application/displayOption/enableTagAging", "false");
            }

            // Refresh [TagResults] for every refresh rate interval set
            saveConfigurationList.Add("/application/displayOption/refreshRate", txtRefreshRate.Text);

            // Select columns to be displayed on Tag Results
            saveConfigurationList.Add("/application/displayOption/tagResultColumnSelection", "");
            foreach (ColumnSelectionForTagResult item in cbxcolumnSelection.Items)
            {
                if (item.IsColumnChecked)
                {
                    saveConfigurationList.Add("/application/displayOption/tagResultColumnSelection/enable"
                        + item.SelectedColumn, "true");
                }
                else
                {
                    saveConfigurationList.Add("/application/displayOption/tagResultColumnSelection/enable"
                        + item.SelectedColumn, "false");
                }
            }

            // Time stamp format
            // if the selected item is Select, keep it as null in config file
            if (cbxTimestampFormat.Text.Equals("Select"))
            {
                saveConfigurationList.Add("/application/displayOption/tagResultColumn/timeStampFormat", "");
            }
            else
            {
                saveConfigurationList.Add("/application/displayOption/tagResultColumn/timeStampFormat",
                    cbxTimestampFormat.Text);
            }

            // BigNum Selection
            // if the selected item is Select, keep it as null in config file
            if (cbxBigNum.Text.Equals("Select"))
            {
                saveConfigurationList.Add("/application/displayOption/tagResult/bigNumSelection", "");
            }
            else
            {
                saveConfigurationList.Add("/application/displayOption/tagResult/bigNumSelection",
                    cbxBigNum.Text);
            }

            // Displays EPC in selected format
            // if the selected item is Select, keep it as null in config file
            if (cbxDisplayEPCAs.Text.Equals("Select"))
            {
                saveConfigurationList.Add("/application/displayOption/tagResultColumn/displayEPCAs", "");
            }
            else
            {
                saveConfigurationList.Add("/application/displayOption/tagResultColumn/displayEPCAs",
                    cbxDisplayEPCAs.Text);
            }

            // BigNum Selection
            // if the selected item is Select, keep it as null in config file
            if (cbxDisplayEmbRdDataAs.Text.Equals("Select"))
            {
                saveConfigurationList.Add("/application/displayOption/tagResultColumn/displayEmbeddedReadDataAs", "");
            }
            else
            {
                saveConfigurationList.Add("/application/displayOption/tagResultColumn/displayEmbeddedReadDataAs",
                    cbxDisplayEmbRdDataAs.Text);
            }

            return saveConfigurationList;
        }

        /// <summary>
        /// Event handler to handle when the user press the Save button in Load/Save Profile.
        /// Used to save the configuration parameters in the .urac file.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnSaveConfig_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                SaveFileDialog saveLoadSaveConfigFileDialog = new SaveFileDialog();
                // Files to be displayed when save file dialog window is opened
                saveLoadSaveConfigFileDialog.Filter = "URA Configuration Files (.urac)|*.urac";
                // Title to be appeared on the save file dialog
                saveLoadSaveConfigFileDialog.Title = "Select a configuration file to save reader "
                    +"and UI configuration parameters";
                
                // Filename field will be populated with [readername].urac. The reader name should be:
                // for Network readers the current hostname, such as: “m6-21071f.urac”
                // for Serial readers: [module type-COM#], such as: “M5e-COM4.urac”
                string strDestinationFile = model + "_" + lblReaderUri.Content.ToString() + @".urac";
                saveLoadSaveConfigFileDialog.FileName = strDestinationFile;
                // Open the default directory to save the configuration file in the application
                // installed directory on the host machine
                saveLoadSaveConfigFileDialog.InitialDirectory = Directory.GetParent(
                    System.Reflection.Assembly.GetExecutingAssembly().Location).ToString();
                saveLoadSaveConfigFileDialog.RestoreDirectory = true;
                // Show the Dialog.
                if (saveLoadSaveConfigFileDialog.ShowDialog() == true)
                {
                    Mouse.SetCursor(Cursors.AppStarting);
                    loadSaveConfig.SaveConfigurations(saveLoadSaveConfigFileDialog.FileName,
                        GetParametersToSave());
                    Mouse.SetCursor(Cursors.Arrow);
                    // Set modify permission for configuration file. Since Windows OS doesn't 
                    // allow modifying any file present in ProgramFiles folder
                    Utilities.SetEditablePermissionOnConfigFile(saveLoadSaveConfigFileDialog.FileName);
                    MessageBox.Show("Saved reader and UI configuration parameters successfully",
                        "Universal Reader Assistant Message", 
                        MessageBoxButton.OK, MessageBoxImage.Information);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Save Profile: "+ex.Message,
                    "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Exclamation);
                Mouse.SetCursor(Cursors.Arrow);
            }
        }

        /// <summary>
        /// Validate load config filter data, strike out the 0x from the hex string.
        /// and checks whether the string is valid hex string
        /// </summary>
        /// <param name="hex">filter data</param>
        /// <returns>returns true, if valid hex string is found</returns>
        private bool ValidateFilterDataFromConfig(string hex)
        {
            string hexstring = string.Empty;
            int prelen = 0;
            // If string is prefixed with 0x
            if (hex.StartsWith("0x") || hex.StartsWith("0X"))
            {
                prelen = 2;
                // Strikeout the 0x and extract the remaining string
                hexstring = hex.Substring(prelen);
            }
            else
            {
                prelen = 0;
                hexstring = hex;
            }
            // Check whether the string is valid hex number
            //if (UInt32.TryParse(hexstring, NumberStyles.HexNumber, CultureInfo.CurrentCulture, out decValue))
            if (System.Text.RegularExpressions.Regex.IsMatch(hexstring, @"\A\b[0-9a-fA-F]+\b\Z"))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        #endregion Load/Save Profile

        /// <summary>
        /// Validate font size. Throw exception if the font size exceeds max limit
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void txtfontSize_LostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            try
            {
                if ((Convert.ToInt32(txtfontSize.Text) < 1) || (Convert.ToInt32(txtfontSize.Text) > 20))
                {
                    MessageBox.Show("Please input the font size between 1 and 20",
                        "Universal Reader Assistant", MessageBoxButton.OK, MessageBoxImage.Error);
                    txtfontSize.Text = "14";
                }
                else
                {
                    txtfontSize.Foreground = Brushes.Black;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Please input the font size between 1 and 20",
                        "Universal Reader Assistant", MessageBoxButton.OK, MessageBoxImage.Error);
                txtfontSize.Text = "14";
                Onlog("Time:" + DateTime.Now.ToString());
                Onlog(ex.ToString());
            }
        }

        /// <summary>
        /// TMR log header - Add Start Time, reader URI and original file name to tmrlog file
        /// </summary>
        private void tmrLogHeader(string readerUri)
        {
            log.WriteLine("*****************************************************************");
            log.WriteLine("Start Time: " + DateTime.Now.ToString("yyyy-MM-dd_HH:mm:ss:fff"));
            if (objReader is SerialReader)
            {
                // print reader uri in this format - Ex:tmr:///com31
                log.WriteLine("Reader: tmr:///" + readerUri);
            }
            else
            {
                // print reader uri in this format - Ex:tmr://172.16.16.111
                log.WriteLine("Reader: tmr://" + readerUri);
            }
            log.WriteLine("Original Filename: " + "tmrlog_" + tmrLogStartTime + ".txt");
            log.WriteLine("*****************************************************************");
            log.Flush();
        }

        /// <summary>
        /// Checks whether Data extensions is Expanded or not 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void expdrDataExtentions_Expanded(object sender, RoutedEventArgs e)
        {
            // Return the offset vector for the TextBlock object.
            Vector vector = VisualTreeHelper.GetOffset(((UIElement)sender));

            // Convert the vector to a point value.
            Point currentPoint = new Point(vector.X, vector.Y);
            settingsScrollviewer.ScrollToVerticalOffset(vector.Y);
        }

        /// <summary>
        /// Checks the checkbox EnableDataExtensions status
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void chkDataExtensions_Checked(object sender, RoutedEventArgs e)
        {
            if (!(bool)rdBtnStreamToTcpPort.IsChecked)
            {
                rdBtnStreamToTcpPort.IsEnabled = true;
                rdBtnStreamToTcpPort.IsChecked = true;     
            }
            else
            {
                imgTcpStreamStatus.Source = new BitmapImage(new Uri(@"..\Icons\broadcast-icon-off.png",
                    UriKind.RelativeOrAbsolute));
                lblTcpStreamStatus.Visibility = Visibility.Hidden;
                rdBtnStreamToTcpPort.IsEnabled=false;
                rdBtnStreamToTcpPort.IsChecked = false;
            }

        }

        /// <summary>
        /// Enables streaming to TCP when StreamtoTcpPort Radio button is checked 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void rdBtnStreamToTcpPort_Checked(object sender, RoutedEventArgs e)
        {
            if ((bool)rdBtnStreamToTcpPort.IsChecked)
            {
                IPAddress ip = IPAddress.Any;
                int port = int.Parse(txtTcpPort.Text);
                waitforclient = true;
                serverThread = new Thread(() => connectToTCPClient(ip,port));
                serverThread.Start();  
            }
            else
            {
                waitforclient = false;
                suspendChangedEvent.Set();
                listener.Stop();
                serverThread.Abort();
                clientConnected = false;
                suspendChangedEvent.Reset();   
            }
        }

        /// <summary>
        /// Method to Creates Tcp connection and waits for clients to connect 
        /// </summary>
        /// <param name="ip">server ip</param>
        /// <param name="port">server port</param>
        public void connectToTCPClient(IPAddress ip,int port)
        {
            try
            {
                ASCIIEncoding asen = new ASCIIEncoding();
                listener = new TcpListener(ip, port);
                // Start Listening at the specified port
                listener.Start();
                while (waitforclient)
                {
                    tagStreamSock = listener.AcceptSocket();
                    tagStreamSock.Send(asen.GetBytes("Automatic message: Connection Accepted!\n"));
                    if (isAsyncReadGoingOn)
                    {
                        imgTcpStreamStatus.Dispatcher.BeginInvoke((Action)(() =>
                        {
                            imgTcpStreamStatus.Source = new BitmapImage(new Uri(@"..\Icons\broadcast-icon-on.png",
                                UriKind.RelativeOrAbsolute));
                        }));

                        lblTcpStreamStatus.Dispatcher.BeginInvoke((Action)(() => {
                            lblTcpStreamStatus.Content = "Status:Data is streaming to port: " + txtTcpPort.Text;
                            lblTcpStreamStatus.Foreground = new SolidColorBrush(Colors.Green);
                            lblTcpStreamStatus.Visibility = Visibility.Visible; }));
                    }
                    clientConnected = true;
                }
                suspendChangedEvent.WaitOne();
                tagStreamSock.Close();
                listener.Stop();
            }
            catch{ };
        }

        /// Sending tags to TCP server Through Async read
        /// </summary>
        /// <param name="e"></param>
        void PrintTagReads(TagReadDataEventArgs e)
        {
           string data = StreamDataFormat(e.TagReadData);
            Dispatcher.BeginInvoke(DispatcherPriority.Background, new ThreadStart(delegate()
            {
                byte[] ba = Encoding.ASCII.GetBytes(data);
                tagStreamSock.Send(ba);
            }));
        }

        public string StreamDataFormat(TagReadData e)
        {
            return e.EpcString + "\t" + e.Time.ToString() + "\t" + e.Rssi.ToString() + "\t" + e.ReadCount.ToString()+"\n";
        }
        /// Sending tags to TCP server Through Sync read
        /// </summary>
        /// <param name="data"></param>
        public void sendTagsToTcp(TagReadData[] data)
        {
            Dispatcher.BeginInvoke(new ThreadStart(delegate()
           {
               for (int i = 0; i < data.Length; i++)
               {
                   string data1 = StreamDataFormat(data[i]);
                   byte[] ba = Encoding.ASCII.GetBytes(data1);
                   tagStreamSock.Send(ba);
               }
           }));
        }

        /// <summary>
        /// txtTcpPort textbox only accepts numericals 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void txtTcpPort_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            e.Handled = !Utilities.AreAllValidNumericChars(e.Text);
            base.OnPreviewTextInput(e);
        }

        /// <summary>
        /// Validating txtTcpPort, If value greater than 65535 show message.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void txtTcpPort_TextChanged(object sender, TextChangedEventArgs e)
        {
            try
            {
                if (txtTcpPort.Text != "")
                {
                    if (Convert.ToInt32(txtTcpPort.Text) > 65535)
                    {
                        MessageBox.Show("Please input TCP port value less then 65535",
                            "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                        txtTcpPort.Text = "9055";
                        return;
                    }
                    if (Convert.ToInt32(txtRFOnTimeout.Text) < 0)
                    {
                        txtTcpPort.Foreground = Brushes.Red;
                    }
                    else
                    {
                        txtTcpPort.Foreground = Brushes.Black;
                    }
                }
            }
            catch { }

        }

        /// <summary>
        /// Validating textbox txtTcpPort on keyboard lost focus. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void txtTcpPort_LostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            if (txtTcpPort.Text == "")
            {
                MessageBox.Show("Tcp port can't be empty.", "Universal Reader Assistant Message", MessageBoxButton.OK, MessageBoxImage.Error);
                txtTcpPort.Text = "9055";
            }
        }

        /// <summary>
       ///  WebService
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void rdBtnWebService_Checked(object sender, RoutedEventArgs e)
        {
            // REserved for future implementation

        }

        /// <summary>
        /// Stream to Serial port
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void rdBtnStreamToSerialPort_Checked(object sender, RoutedEventArgs e)
        {
            // Reserved for future implementation
        }

        /// <summary>
        /// Validating HIGGS3 TID filter data length and start address 
        /// </summary>
        private bool validateTidFilterData()
        {
            bool isTidDataLength = true;
            if (chkApplyFilter.IsChecked == true)
            {
                if (null != cbxFilterMemBank.SelectedItem)
                {
                    ComboBoxItem selectedItem = (ComboBoxItem)cbxFilterMemBank.SelectedItem;
                    if (selectedItem.Content.ToString() == "TID")
                    {
                        string txtModelID = string.Empty;
                        string txtVendorID = string.Empty;
                        string tidData = string.Empty;
                        tidData = Utilities.RemoveHexstringPrefix(txtFilterData.Text);
                        tidData = tidData.Replace(" ", string.Empty);
                        if (txtFilterStartAddr.Text != "0")
                        {
                            MessageBoxResult result = MessageBox.Show("Apply Filter:start address is not zero so we cannot determine tag type." +
                            " In case of Higgs3 if filter data length is greater than 12 bytes which falls into device configuration area of TID memory which is" +
                            " not fixed and filter may not be matched. Do you want to proceed ?",
                                   "Universal Reader Assistant Message", MessageBoxButton.YesNo, MessageBoxImage.Error);
                            if (result.Equals(MessageBoxResult.No))
                            {
                                txtFilterStartAddr.Focus();
                                isTidDataLength = false;
                            }
                        }
                        else
                        {
                            if (tidData.Length > 24)
                            {
                                txtVendorID = tidData.Substring(2, 3);
                                txtModelID = tidData.Substring(5, 3);
                                if (txtVendorID == "003" && txtModelID == "412")
                                {
                                    MessageBoxResult result = MessageBox.Show("tag  type identified is higgs3 and filter data length entered is greater than 12 bytes" +
                                    " which falls into device configuration area of TID memory which is not fixed and filter may not be matched. Do you want to proceed?",
                                        "Universal Reader Assistant Message", MessageBoxButton.YesNo, MessageBoxImage.Error);
                                    if (result.Equals(MessageBoxResult.No))
                                    {
                                        txtFilterData.Focus();
                                        isTidDataLength = false;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            return isTidDataLength;
        }
    }
}

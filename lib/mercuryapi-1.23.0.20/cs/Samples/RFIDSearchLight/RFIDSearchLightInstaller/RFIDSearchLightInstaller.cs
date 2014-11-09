using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Configuration.Install;
using System.Linq;
using System.Threading;

namespace RFIDSearchLightInstaller
{
    [RunInstaller(true)]
    public partial class RFIDSearchLightCABFile : Installer
    {
        public class InstallParams
        {
            private AutoResetEvent reset;
            private string strIniFile;

            public InstallParams(AutoResetEvent ev, string ini_file)
            {
                if (ev == null)
                    throw new Exception("Null reset event");
                reset = ev;
                strIniFile = ini_file;
            }

            public AutoResetEvent Reset
            {
                get
                {
                    return reset;
                }
            }

            public string IniFile
            {
                get
                {
                    return strIniFile;
                }
            }
        }

        private string appPath = null;

        public RFIDSearchLightCABFile()
        {
            InitializeComponent();
        }

        public override void Commit(System.Collections.IDictionary savedState)
        {
            // Call the Commit method of the base class
            base.Commit(savedState);

            AutoResetEvent ev;
            InstallParams ip;

            Microsoft.Win32.RegistryKey key = null;

            // Open the registry key containing the path to the Application Manager            
            key = Microsoft.Win32.Registry.LocalMachine.OpenSubKey("Software\\microsoft\\windows\\currentversion\\app paths\\ceappmgr.exe");

            // If the key is not null, then ActiveSync is installed on the user's desktop computer
            if (key != null)
            {
                // Get the path to the Application Manager from the registry value

                appPath = key.GetValue(null).ToString();

                if (appPath != null)
                {
                    //.Net 3.5
                    ev = new AutoResetEvent(false);
                    ip = new InstallParams(ev, "RFIDSearchLight.ini");
                    ThreadPool.QueueUserWorkItem(new WaitCallback(RunInstallerTask), ip);
                    ev.WaitOne();
                }
            }
            else
            {
                // No Active Sync - throw an error message
                throw new Exception("ActiveSync Not Installed");
            }
        }

        private void RunInstallerTask(Object iparam)
        {
            int time = 0;
            InstallParams iparams = iparam as InstallParams;
            AutoResetEvent are = (AutoResetEvent)iparams.Reset;
            // Get the target directory where the .ini file is installed.
            // This is sent from the Setup application                
            string strIniFilePath = "\"" + Context.Parameters["targetdir"] + iparams.IniFile;// +"\"";
            // Now launch the Application Manager - ActiveSync
            System.Diagnostics.Process process = new System.Diagnostics.Process();
            process.StartInfo.FileName = appPath;
            process.StartInfo.Arguments = strIniFilePath;
            process.Start();
            while (!process.HasExited)
            {
                time = 1000;
                Thread.Sleep(time);
            }
            are.Set();
        }
    }
}


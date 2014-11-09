using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Configuration.Install;
using System.Linq;
using System.Security.Permissions;
using Microsoft.Win32;
using System.IO;

namespace RFIDSearchLightInstaller
{
    [RunInstaller(true)]
    public partial class CustomInstaller : Installer
    {
        private const string CEAPPMGR_PATH =    @"SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\CEAPPMGR.EXE";
        private const string ACTIVESYNC_INSTALL_PATH =  @"SOFTWARE\Microsoft\Windows CE Services";
        private const string INSTALLED_DIR = "InstallDir";
        private const string CEAPPMGR_EXE_FILE = @"CEAPPMGR.EXE";
        private const string CEAPPMGR_INI_FILE = @"RFIDSearchLight.ini";
        private const string APP_SUBDIR = @"\RFIDSearchLight";
        private string TEMP_PATH = Environment.SystemDirectory + @"\TEMP\RFIDSearchLight";
        public CustomInstaller()
        {
            InitializeComponent();
            this.BeforeInstall += new InstallEventHandler(CustomInstaller_BeforeInstall);
            this.AfterInstall += new InstallEventHandler(CustomInstaller_AfterInstall);
            this.BeforeUninstall += new InstallEventHandler(CustomInstaller_BeforeUninstall);
        }

        void CustomInstaller_BeforeInstall(object sender, InstallEventArgs e)
        {
            // Find the location where the application will be installed
            string installPath = GetAppInstallDirectory();
            // Create the target directory
            Directory.CreateDirectory(installPath);
            // Copy your application files to the directory
            foreach (string installFile in Directory.GetFiles(TEMP_PATH))
            {
                File.Copy(installFile, Path.Combine(installPath,
                    Path.GetFileName(installFile)), true);
            }
            // Get the path to ceappmgr.exe
            RegistryKey keyAppMgr =
                    Registry.LocalMachine.OpenSubKey(CEAPPMGR_PATH);
            string appMgrPath = (string)keyAppMgr.GetValue(null);
            keyAppMgr.Close();
            // Run CeAppMgr.exe to install the files to the device
            System.Diagnostics.Process.Start(appMgrPath,
                "\"" + Path.Combine(installPath, CEAPPMGR_INI_FILE) + "\"");
        }

        void CustomInstaller_BeforeUninstall(object sender, InstallEventArgs e)
        {
            // Find where the application is installed
            string installPath = GetAppInstallDirectory();
            // Delete the files
            foreach (string appFile in Directory.GetFiles(installPath))
            {
                File.Delete(appFile);
            }
            // Delete the folder
            Directory.Delete(installPath);
            
        }

        void CustomInstaller_AfterInstall(object sender, InstallEventArgs e)
        {
            // Delete the temp files
            foreach (string tempFile in Directory.GetFiles(TEMP_PATH))
            {
                File.Delete(tempFile);
            }
        }

        private string GetAppInstallDirectory()
        {
            // Get the ActiveSync install directory
            RegistryKey keyActiveSync =
                    Registry.LocalMachine.OpenSubKey(ACTIVESYNC_INSTALL_PATH);
            if (keyActiveSync == null)
            {
                throw new Exception("ActiveSync is not installed.");
            }
            // Build the target directory path under the ActiveSync folder
            string activeSyncPath =
                    (string)keyActiveSync.GetValue(INSTALLED_DIR);
            string installPath = (activeSyncPath + APP_SUBDIR);
            keyActiveSync.Close();
            return installPath;
        }

        public override void Commit(System.Collections.IDictionary savedState)
        {
            // Call the Commit method of the base class
            base.Commit(savedState);

            // Open the registry key containing the path to the Application Manager
            Microsoft.Win32.RegistryKey key = null;
            key = Microsoft.Win32.Registry.LocalMachine.OpenSubKey("Software\\microsoft\\windows\\currentversion\\app paths\\ceappmgr.exe");

            // If the key is not null, then ActiveSync is installed on the user's desktop computer
            if (key != null)
            {
                // Get the path to the Application Manager from the registry value
                string appPath = null;
                appPath = key.GetValue(null).ToString();

                // Get the target directory where the .ini file is installed.
                // This is sent from the Setup application
                //string strIniFilePath = "\"" + Context.Parameters["InstallDir"] + "RFIDSearchLight.ini\"";
                string installPath = GetAppInstallDirectory();
                string strIniFilePath = installPath + @"\RFIDSearchLight.ini";
                if (appPath != null)
                {
                    // Now launch the Application Manager
                    System.Diagnostics.Process process = new System.Diagnostics.Process();
                    process.StartInfo.FileName = appPath;
                    process.StartInfo.Arguments = strIniFilePath;
                    process.Start();
                }
            }
            else
            {
                // No Active Sync - throw a message
            }
        }
    }
}

using System;
using System.Linq;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace ThingMagic.RFIDSearchLight
{
    public class NotifyIcon
    {
        //Declare click event
        public event System.EventHandler Click;

        private WindowSink windowSink;
        private int uID = 5000;

        //Constructor
        public NotifyIcon()
        {
            //Create instance of the MessageWindow subclass
            windowSink = new WindowSink(this);
            windowSink.uID = uID;
        }

        //Destructor
        ~NotifyIcon()
        {
            Remove();
        }


        public void Add(IntPtr hIcon)
        {
            TrayMessage(windowSink.Hwnd, NIM_ADD, (uint)uID, hIcon);
        }

        public void Remove()
        {

            TrayMessage(windowSink.Hwnd, NIM_DELETE, (uint)uID, IntPtr.Zero);
        }

        public void Modify(IntPtr hIcon)
        {

            TrayMessage(windowSink.Hwnd, NIM_MODIFY, (uint)uID, hIcon);

        }



        private void TrayMessage(IntPtr hwnd, int dwMessage, uint uID, IntPtr hIcon)
        {
            NOTIFYICONDATA notdata = new NOTIFYICONDATA();

            notdata.cbSize = 152;
            notdata.hIcon = hIcon;
            notdata.hWnd = hwnd;
            notdata.uCallbackMessage = WM_NOTIFY_TRAY;
            notdata.uFlags = NIF_MESSAGE | NIF_ICON;
            notdata.uID = uID;

            int ret = Shell_NotifyIcon(dwMessage, ref notdata);
        }

        #region API Declarations

        internal const int WM_LBUTTONDOWN = 0x0201;
        //User defined message
        internal const int WM_NOTIFY_TRAY = 0x0400 + 2001;

        internal const int NIM_ADD = 0x00000000;
        internal const int NIM_MODIFY = 0x00000001;
        internal const int NIM_DELETE = 0x00000002;

        const int NIF_MESSAGE = 0x00000001;
        const int NIF_ICON = 0x00000002;


        internal struct NOTIFYICONDATA
        {
            internal int cbSize;
            internal IntPtr hWnd;
            internal uint uID;
            internal uint uFlags;
            internal uint uCallbackMessage;
            internal IntPtr hIcon;
            //internal char[] szTip = new char[64]; 
            //internal IntPtr szTip;
        }

        [DllImport("coredll.dll")]
        internal static extern int Shell_NotifyIcon(
            int dwMessage, ref NOTIFYICONDATA pnid);

        [DllImport("coredll.dll")]
        internal static extern int SetForegroundWindow(IntPtr hWnd);

        [DllImport("coredll.dll")]
        internal static extern int ShowWindow(
            IntPtr hWnd,
            int nCmdShow);

        [DllImport("coredll.dll")]
        internal static extern IntPtr GetFocus();

        #endregion


        #region WindowSink

        internal class WindowSink : Microsoft.WindowsCE.Forms.MessageWindow
        {
            //Private members
            private int m_uID = 0;
            private NotifyIcon notifyIcon;

            //Constructor
            public WindowSink(NotifyIcon notIcon)
            {
                notifyIcon = notIcon;
            }

            public int uID
            {
                set
                {
                    m_uID = value;

                }
            }

            protected override void WndProc(ref Microsoft.WindowsCE.Forms.Message msg)
            {

                if (msg.Msg == WM_NOTIFY_TRAY)
                {
                    if ((int)msg.LParam == WM_LBUTTONDOWN)
                    {
                        if ((int)msg.WParam == m_uID)
                        {
                            //If somebody hooked, raise the event
                            if (notifyIcon.Click != null)
                                notifyIcon.Click(notifyIcon, null);

                        }
                    }
                }

            }
        }
        #endregion

    }
}

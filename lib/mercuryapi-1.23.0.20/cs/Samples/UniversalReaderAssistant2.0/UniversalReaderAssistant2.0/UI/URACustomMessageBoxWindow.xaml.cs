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
using System.Diagnostics;
using System.Drawing;
using System.ComponentModel;

namespace ThingMagic.URA2
{
    /// <summary>
    /// Interaction logic for URACustomMessageBoxWindow.xaml
    /// </summary>
    public partial class URACustomMessageBoxWindow : Window
    {

        private MessageBoxResult _messageBoxResult;
        private bool isDoNotAskMeAgainChecked = false;
        private bool isMessageBoxOpened = false;

        /// <summary>
        /// Status of messagebox
        /// </summary>
        public bool MessageBoxOpened
        {
            get
            {
                return isMessageBoxOpened;
            }
        }
        
        /// <summary>
        /// Result of messagebox
        /// </summary>
        public MessageBoxResult MessageBoxResult
        {
            get
            {
                return _messageBoxResult;
            }
        }

        /// <summary>
        /// Decides whether the messgaebox to displayed again and again or only once
        /// </summary>
        public bool DoNotAskMeAgainChecked
        {
            get 
            {
                return isDoNotAskMeAgainChecked;
            }
        }

        /// <summary>
        /// Initialize messagebox
        /// </summary>
        public URACustomMessageBoxWindow()
        {
            InitializeComponent();
            img.Source = SystemIcons.Information.ToImageSource();
        }

        /// <summary>
        /// Hyperlink
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void Hyperlink_RequestNavigate(object sender, RequestNavigateEventArgs e)
        {
            Process.Start(new ProcessStartInfo(e.Uri.AbsoluteUri));
            e.Handled = true;
        }

        /// <summary>
        /// Show message box
        /// </summary>
        public new MessageBoxResult Show()
        {
                if (!isDoNotAskMeAgainChecked)
                {
                    isMessageBoxOpened = true;
                    this.ShowDialog();
                    isMessageBoxOpened = false;
                }                                    
            return _messageBoxResult;
        }

        /// <summary>
        /// Messagebox ok button event handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void Button_Click(object sender, RoutedEventArgs e)
        {
            _messageBoxResult = MessageBoxResult.OK;
            this.Close();
        }

        protected override void OnSourceInitialized(EventArgs e)
        {
            // removes the application icon from the window top left corner
            // this is different than just hiding it
            WindowHelper.RemoveIcon(this);            

            // disable close button if needed and remove resize menu items from the window system menu
            var systemMenuHelper = new SystemMenuHelper(this);
            systemMenuHelper.DisableCloseButton = false;            
            systemMenuHelper.RemoveResizeMenu = true;
        }

        private void Window_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Escape)
            {
                e.Handled = true;
            }
        }

        protected override void OnClosing(CancelEventArgs e)
        {
            base.OnClosing(e);
            e.Cancel = true;
            this.Hide();
        }

        private void chkbxDoNotAsk_Checked(object sender, RoutedEventArgs e)
        {
            isDoNotAskMeAgainChecked = (bool)chkbxDoNotAsk.IsChecked;
        }
    }
}

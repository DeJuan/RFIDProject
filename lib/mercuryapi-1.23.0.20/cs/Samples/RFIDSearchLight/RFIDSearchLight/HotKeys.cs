#if PocketPC
using Microsoft.WindowsCE.Forms;
#else
using System.Windows.Forms;
#endif
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

        
// Based on GlobalHotKeys code example by Curtis Rutland
// http://www.dreamincode.net/forums/topic/180436-global-hotkeys/

// Adapted to Windows CE and repackaged by Harry Tsai <htsai@thingmagic.com>

namespace ThingMagic.RFIDSearchLight
{
    /// <summary>
    /// Manage HotKeys: register and unregister hot keys, fire event when a hot key is pressed
    /// </summary>
    class HotKeys
    {
#if PocketPC
        [DllImport("coredll.dll")]
#else
        [DllImport("user32.dll")]
#endif
        private static extern bool RegisterHotKey(IntPtr hWnd, int id, int fsModifiers, int vk);

#if PocketPC
        [DllImport("coredll.dll")]
#else
        [DllImport("user32.dll")]
#endif
        private static extern bool UnregisterHotKey(IntPtr hWnd, int id);

        // windows message id for hotkey
        public const int WM_HOTKEY_MSG_ID = 0x0312;

	    // Key modifiers
        public static class Mod
        {
            //modifiers
            public const int NOMOD = 0x0000;
            public const int ALT = 0x0001;
            public const int CTRL = 0x0002;
            public const int SHIFT = 0x0004;
            public const int WIN = 0x0008;
        }

	    // Platform-specific keys
        public static class Nomad
        {
            public const int VK_APP1 = 0xC1;
            public const int VK_APP2 = 0xC2;
            public const int VK_APP3 = 0xC3;
            public const int VK_APP4 = 0xC4;
            public const int VK_APP5 = 0xC5;
            public const int VK_APP6 = 0xC6;
            public const int BUTTON_ARROW = VK_APP1;
            public const int BUTTON_START_MENU = VK_APP2;
            public const int BUTTON_LEFT_SOFTKEY = VK_APP3;
            public const int BUTTON_RIGHT_SOFTKEY = VK_APP4;
            public const int BUTTON_OK = VK_APP5;
            public const int BUTTON_HEADSET_BUTTON = VK_APP6;
            public const int BUTTON_PISTOL_TRIGGER = VK_APP6;
        }

        /// <summary>
        ///  Specification of a hot key to be handled
        /// </summary>
        public class Spec
        {
            public int modifier;
            public int key;
            public IntPtr hWnd;
            public int id;

            public Spec(int modifier, int key, 
#if PocketPC
                MessageWindow msgwin
#else
                Form form
#endif
                )
            {
                this.modifier = modifier;
                this.key = key;
                this.hWnd =
#if PocketPC
                    msgwin.Hwnd
#else
                    form.Handle
#endif
                    ;
                id = this.GetHashCode();
            }

            public override int GetHashCode()
            {
                return modifier ^ key ^ hWnd.ToInt32();
            }
        }

        /// <summary>
        /// Manger object for a set of hotkeys
        /// </summary>
        public class Manager
        {
            public List<Spec> hotkeys = new List<Spec>();

            /// <summary>
            /// Change the set of hotkeys, but do not enable them
            /// </summary>
            /// <param name="newKeys">List of new hotkey specs</param>
            public void SetKeys(ICollection<Spec> newKeys)
            {
                // Clean up last set of hotkeys before instantiating a new one
                if (0 < this.hotkeys.Count)
                {
                    Disable();
                }
                // Make a copy of the new hotkeys
                hotkeys.Clear();
                hotkeys.AddRange(newKeys);
            }

            /// <summary>
            /// Change the set of hotkeys and enable them
            /// </summary>
            /// <param name="newKeys">List of new hotkey specs</param>
            public void Enable(ICollection<Spec> newKeys)
            {
                SetKeys(newKeys);
                // Enable new keys
                Enable();
            }

            /// <summary>
            /// Enable hotkeys
            /// </summary>
            public void Enable()
            {
                foreach (Spec hk in hotkeys)
                {
                    RegisterHotKey(hk.hWnd, hk.id, hk.modifier, hk.key);
                }
            }

            /// <summary>
            /// Disable hotkeys
            /// </summary>
            public void Disable()
            {
                foreach (Spec hk in hotkeys)
                {
                    UnregisterHotKey(hk.hWnd, hk.id);
                }
            }

            /// <summary>
            /// Automatically release hotkeys when Manager goes out of scope
            /// </summary>
            ~Manager()
            {
                Disable();
            }
        }
    }
}

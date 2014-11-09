/*
 * Copyright (c) 2009 ThingMagic, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
using System;
using System.Collections.Generic;
using System.Text;

namespace ThingMagic
{
    /// <summary>
    /// Gen2 protocol-specific constructs
    /// </summary>
    public static class Gen2
    {
        #region Nested Enums

        #region Bank

        /// <summary>
        /// Gen2 memory bank identifiers
        /// </summary>
        public enum Bank
        {
            /// <summary>
            /// Reserved memory contains kill and access passwords
            /// </summary>
            RESERVED = 0,
            /// <summary>
            /// EPC memory contains CRC, PC, EPC
            /// </summary>
            EPC = 1,
            /// <summary>
            /// TID memory contains tag implementation identifiers
            /// </summary>
            TID = 2,
            /// <summary>
            /// User memory is optional, but exists for user-defined data
            /// </summary>
            USER = 3,
            /// <summary>
            /// Used to enable the read of additional membanks - reserved mem bank
            /// </summary>
            GEN2BANKRESERVEDENABLED  = 0x4,
            /// <summary>
            /// Used to enable the read of additional membanks - epc mem bank
            /// </summary>
            GEN2BANKEPCENABLED  = 0x8,
            /// <summary>
            /// Used to enable the read of additional membanks - tid mem bank
            /// </summary>
            GEN2BANKTIDENABLED = 0x10,
            /// <summary>
            /// Used to enable the read of additional membanks - user mem bank
            /// </summary>
            GEN2BANKUSERENABLED = 0x20,
        }

        #endregion

        #region Target

        /// <summary>
        /// Gen2 target settings.
        /// Includes standard A(0) and B(1), as well as
        /// ThingMagic reader values of A-then-B, and B-then-A
        /// </summary>
        public enum Target
        {
            /// <summary>
            /// Search for tags in State A
            /// </summary>
            A,
            /// <summary>
            /// Search for tags in State B
            /// </summary>
            B,
            /// <summary>
            /// Search for tags in State A, then switch to B
            /// </summary>
            AB,
            /// <summary>
            /// Search for tags in State B, then switch to A
            /// </summary>
            BA
        }

        #endregion

        #region Session

        /// <summary>
        /// Gen2 Session settings. 
        /// </summary>
        public enum Session
        {
            /// <summary>
            /// Session 0.
            /// </summary>        
            S0 = 0,
            /// <summary>
            /// Session 1.
            /// </summary>
            S1 = 1,
            /// <summary>
            /// Session 2.
            /// </summary>
            S2 = 2,
            /// <summary>
            /// Session 3.
            /// </summary>
            S3 = 3,
        }

        #endregion

        #region TagEncoding

        /// <summary>
        /// Gen2 Tag Encoding.
        /// </summary>
        public enum TagEncoding
        {
            /// <summary>
            /// FM0.
            /// </summary>
            FM0 = 0,
            /// <summary>
            /// M = 2.
            /// </summary>
            M2 = 1,
            /// <summary>
            /// M = 4.
            /// </summary>
            M4 = 2,
            /// <summary>
            /// M = 8.
            /// </summary>
            M8 = 3,
        }

        #endregion

        #region DivideRatio

        /// <summary>
        /// Divide Ratio values
        /// </summary>
        public enum DivideRatio
        {
            /// <summary>
            /// Divide by 8
            /// </summary>
            DR8 = 0,
            /// <summary>
            /// Divide by 64/3
            /// </summary>
            DR64_3 = 1,
        }
        #endregion

        #region TrExt

        /// <summary>
        /// TRext: Include extended preamble in Tag-to-Reader response?
        /// </summary>
        public enum TrExt
        {
            /// <summary>
            /// No extension
            /// </summary>
            NOPILOTTONE = 0,
            /// <summary>
            /// Add pilot tone
            /// </summary>
            PILOTTONE = 1,
        }

        #endregion

        #region LockBits

        /// <summary>
        /// Gen2 lock bits, as used in Action and Mask fields of Gen2 Lock command.
        /// Not exposed to end user -- use friendlier Gen2LockActions instead.
        /// </summary>
        [Flags]
        private enum LockBits
        {
            /// <summary>
            /// No action (empty mask)
            /// </summary>
            NONE = 0,
            /// <summary>
            /// User memory permalock -- set to disallow changes to user memory lock bit
            /// </summary>
            USER_PERM = 1 << 0,
            /// <summary>
            /// User memory [write] lock -- set to disallow writes to user memory, clear to allow writes
            /// </summary>
            USER = 1 << 1,
            /// <summary>
            /// TID memory permalock -- set to disallow changes to TID memory lock bit
            /// </summary>
            TID_PERM = 1 << 2,
            /// <summary>
            /// TID memory [write] lock -- set to disallow writes to TID memory, clear to allow writes
            /// </summary>
            TID = 1 << 3,
            /// <summary>
            /// EPC memory permalock -- set to disallow changes to EPC memory lock bit
            /// </summary>
            EPC_PERM = 1 << 4,
            /// <summary>
            /// EPC memory [write] lock -- set to disallow writes to EPC memory, clear to allow writes
            /// </summary>
            EPC = 1 << 5,
            /// <summary>
            /// Access password memory permalock -- set to disallow changes to access password memory lock bit
            /// </summary>
            ACCESS_PERM = 1 << 6,
            /// <summary>
            /// Access password [read/write] lock -- set to disallow read and write of access password, clear to allow read and write
            /// </summary>
            ACCESS = 1 << 7,
            /// <summary>
            /// Kill password memory permalock -- set to disallow changes to kill password memory lock bit
            /// </summary>
            KILL_PERM = 1 << 8,
            /// <summary>
            /// Kill password [read/write] lock -- set to disallow read and write of kill password, clear to allow read and write
            /// </summary>
            KILL = 1 << 9,
        }

        #endregion

        #region LinkFrequency
        /// <summary>
        /// Gen2 LinkFrequency
        /// </summary>
        public enum LinkFrequency
        {
            /// <summary>
            ///LinkFrequency=250KHZ
            /// </summary>
            LINK250KHZ = 250,
            /// <summary>
            ///LinkFrequency=320KHZ
            /// </summary>
            LINK320KHZ = 320,
            /// <summary>
            /// LinkFrequency=640KHZ
            /// </summary>
            LINK640KHZ =640,
        }

        #endregion

        #region Tari
        /// <summary>
        /// Gen2 Tari Value
        /// </summary>
        public enum Tari
        {
            /// <summary>
            /// Tari = 25us
            /// </summary>
            TARI_25US = 0,
            /// <summary>
            /// Tari = 12.5us
            /// </summary>
            TARI_12_5US = 1,
            /// <summary>
            /// Tari = 6.25us
            /// </summary>
            TARI_6_25US = 2,
        }

        #endregion

        #region WriteMode
        /// <summary>
        /// The mode for write operation
        /// </summary>
        public enum WriteMode
        {
            /// <summary>
            /// use the standard write only
            /// </summary>
            WORD_ONLY,
            /// <summary>
            /// use BlockWrite only
            /// </summary>
            BLOCK_ONLY,
            /// <summary>
            /// use BlockWrite first, if fail, use standard write
            /// </summary>
            BLOCK_FALLBACK


        }

        #endregion

        #endregion

        #region Nested Classes

        #region Password

        /// <summary>
        /// Stores a 32-bit Gen2 password for use as an access or kill password.
        /// </summary>
        public class Password : TagAuthentication
        {
            #region Fields
            /// <summary>
            /// Raw 32-bit password value
            /// </summary>
            internal UInt32 _value;

            #endregion

            /// <summary>
            /// Get Gen2 native 32-bit password value (read-only)
            /// </summary>
            public UInt32 Value
            {
                get { return _value; }
            }

            #region Construction

            /// <summary>
            /// Create a new password object
            /// </summary>
            /// <param name="password">32-bit Gen2 password</param>
            public Password(UInt32 password)
            {
                _value = password;
            }

            #endregion

            #region ToString

            /// <summary>
            /// Human-readable representation
            /// </summary>
            /// <returns>Human-readable representation</returns>
            public override string ToString()
            {
                return Value.ToString("X8");
            }

            #endregion
        }

        #endregion

        #region Q

        /// <summary>
        /// Abstract Gen2 Q class.
        /// </summary>
        public class Q { }

        #endregion

        #region DynamicQ

        /// <summary>
        /// Gen2 Dynamic Q subclass.
        /// </summary>
        public class DynamicQ : Q
        {
            #region ToString

            /// <summary>
            /// Human-readable representation
            /// </summary>
            /// <returns>Human-readable representation</returns>
            public override string ToString()
            {
                return "DynamicQ";
            }

            #endregion
        }
        #endregion

        #region StaticQ

        /// <summary>
        /// Gen2 Static Q subclass.
        /// </summary>
        public class StaticQ : Q
        {
            #region Fields
            /// <summary>
            /// The Q value to use
            /// </summary>
            public byte InitialQ;

            #endregion

            #region Construction

            /// <summary>
            /// Create a static Q algorithim instance with a particular value.
            /// </summary>
            /// <param name="initQ">Q value</param>
            public StaticQ(byte initQ)
            {
                InitialQ = initQ;
            }

            #endregion

            #region ToString

            /// <summary>
            /// Human-readable representation
            /// </summary>
            /// <returns>Human-readable representation</returns>
            public override string ToString()
            {
                return String.Format("StaticQ({0:D})", InitialQ);
            }

            #endregion
        }

        #endregion

        #region TagData

        /// <summary>
        /// Gen2-specific version of TagData
        /// </summary>
        public class TagData : ThingMagic.TagData
        {
            #region Fields

            internal byte[] _pc;

            #endregion

            #region Properties

            /// <summary>
            /// Tag's RFID protocol
            /// </summary>
            public override TagProtocol Protocol
            {
                get { return TagProtocol.GEN2; }
            }

            /// <summary>
            /// PC (Protocol Control) bits
            /// </summary>
            public byte[] PcBytes
            {
                get { return (null != _pc) ? (byte[])_pc.Clone() : null; }
            }

            #endregion

            #region Construction

            /// <summary>
            /// Create TagData with blank CRC
            /// </summary>
            /// <param name="epcBytes">EPC value</param>
            public TagData(ICollection<byte> epcBytes) : base(epcBytes) { }

            /// <summary>
            /// Create TagData
            /// </summary>
            /// <param name="epcBytes">EPC value</param>
            /// <param name="crcBytes">CRC value</param>
            public TagData(ICollection<byte> epcBytes, ICollection<byte> crcBytes) : base(epcBytes, crcBytes) { }

            /// <summary>
            /// Create TagData
            /// </summary>
            /// <param name="epcBytes">EPC value</param>
            /// <param name="crcBytes">CRC value</param>
            /// <param name="pcBytes">PC value</param>
            public TagData(ICollection<byte> epcBytes, ICollection<byte> crcBytes, ICollection<byte> pcBytes)
                : base(epcBytes, crcBytes)
            {
                _pc = (null != pcBytes) ? CollUtil.ToArray(pcBytes) : null;
            }

            #endregion
        }

        #endregion

        #region Select

        /// <summary>
        /// Representation of a Gen2 Select operation
        /// </summary>
        public class Select : TagFilter
        {
            #region Fields

            /// <summary>
            /// Whether tags that meet the comparison are selected or deselected.
            /// false: Get matching tags.
            /// true: Drop matching tags.
            /// </summary>
            public bool Invert;

            /// <summary>
            /// The memory bank in which to compare the mask
            /// </summary>
            public Bank Bank;
            /// <summary>
            /// The location (in bits) at which to begin comparing the mask
            /// </summary>
            public UInt32 BitPointer;
            /// <summary>
            /// The length (in bits) of the mask
            /// </summary>
            public UInt16 BitLength;
            /// <summary>
            /// The mask value to compare with the specified region of tag
            /// memory, MSB first
            /// </summary>
            public byte[] Mask;

            #endregion

            #region Construction

            /// <summary>
            /// Create Gen2 Select
            /// </summary>
            /// <param name="invert"> false: Get matching tags.  true: Drop matching tags.</param>
            /// <param name="bank">The memory bank in which to compare the mask</param>
            /// <param name="bitPointer">The location (in bits) at which to begin comparing the mask</param>
            /// <param name="bitLength">The length (in bits) of the mask</param>
            /// <param name="mask">The mask value to compare with the specified region of tag memory, MSB first</param>
            public Select(bool invert, Bank bank, UInt32 bitPointer,
                          UInt16 bitLength, ICollection<byte> mask)
            {
                this.Invert = invert;
                if (bank == Bank.RESERVED)
                    throw new ArgumentException("Gen2.Select may not operate on reserved memory bank");
                this.Bank = bank;
                this.BitPointer = bitPointer;
                this.BitLength = bitLength;
                this.Mask = CollUtil.ToArray(mask);
            }

            #endregion

            #region Matches

            /// <summary>
            /// Test if a tag Matches this filter. Only applies to selects based
            /// on the EPC.
            /// </summary>
            /// <param name="t">tag data to screen</param>
            /// <returns>Return true to allow tag through the filter.
            /// Return false to reject tag.</returns>
            public bool Matches(ThingMagic.TagData t)
            {
                bool match = true;
                int i, bitAddr;

                if (Bank != Bank.EPC)
                    throw new NotSupportedException("Can't match against non-EPC memory");

                i = 0;
                bitAddr = (int)BitPointer;
                // Matching against the CRC and PC does not have defined
                // behavior; see section 6.3.2.11.1.1 of Gen2 version 1.2.0.
                // We choose to let it match, because that's simple.
                bitAddr -= 32;
                if (bitAddr < 0)
                {
                    i -= bitAddr;
                    bitAddr = 0;
                }

                for (; i < BitLength; i++, bitAddr++)
                {
                    if (bitAddr > (t.EpcBytes.Length * 8))
                    {
                        match = false;
                        break;
                    }
                    // Extract the relevant bit from both the EPC and the mask.
                    if (((t.EpcBytes[bitAddr / 8] >> (7 - (bitAddr & 7))) & 1) !=
                        ((Mask[i / 8] >> (7 - (i & 7))) & 1))
                    {
                        match = false;
                        break;
                    }
                }

                if (Invert)
                    match = match ? false : true;

                return match;
            }
            #endregion

            /// <summary>
            /// Returns a String that represents the current Object.
            /// </summary>
            /// <returns>A String that represents the current Object.</returns>
            public override string ToString()
            {
                return String.Format(
                    "Gen2.Select:[{0}{1},{2},{3},{4}]",
                    (Invert ? "Invert," : ""),
                    Bank, BitPointer, BitLength, ByteFormat.ToHex(Mask));
            }
        }

        #endregion

        #region LockAction

        /// <summary>
        /// Gen2 lock action specifier
        /// </summary>
        public class LockAction : TagLockAction
        {
            #region Static Fields

            private static readonly LockAction _KILL_LOCK = new LockAction(LockBits.KILL | LockBits.KILL_PERM, LockBits.KILL);
            private static readonly LockAction _KILL_UNLOCK = new LockAction(LockBits.KILL | LockBits.KILL_PERM, 0);
            private static readonly LockAction _KILL_PERMALOCK = new LockAction(LockBits.KILL | LockBits.KILL_PERM, LockBits.KILL | LockBits.KILL_PERM);
            private static readonly LockAction _KILL_PERMAUNLOCK = new LockAction(LockBits.KILL | LockBits.KILL_PERM, LockBits.KILL_PERM);
            private static readonly LockAction _ACCESS_LOCK = new LockAction(LockBits.ACCESS | LockBits.ACCESS_PERM, LockBits.ACCESS);
            private static readonly LockAction _ACCESS_UNLOCK = new LockAction(LockBits.ACCESS | LockBits.ACCESS_PERM, 0);
            private static readonly LockAction _ACCESS_PERMALOCK = new LockAction(LockBits.ACCESS | LockBits.ACCESS_PERM, LockBits.ACCESS | LockBits.ACCESS_PERM);
            private static readonly LockAction _ACCESS_PERMAUNLOCK = new LockAction(LockBits.ACCESS | LockBits.ACCESS_PERM, LockBits.ACCESS_PERM);
            private static readonly LockAction _EPC_LOCK = new LockAction(LockBits.EPC | LockBits.EPC_PERM, LockBits.EPC);
            private static readonly LockAction _EPC_UNLOCK = new LockAction(LockBits.EPC | LockBits.EPC_PERM, 0);
            private static readonly LockAction _EPC_PERMALOCK = new LockAction(LockBits.EPC | LockBits.EPC_PERM, LockBits.EPC | LockBits.EPC_PERM);
            private static readonly LockAction _EPC_PERMAUNLOCK = new LockAction(LockBits.EPC | LockBits.EPC_PERM, LockBits.EPC_PERM);
            private static readonly LockAction _TID_LOCK = new LockAction(LockBits.TID | LockBits.TID_PERM, LockBits.TID);
            private static readonly LockAction _TID_UNLOCK = new LockAction(LockBits.TID | LockBits.TID_PERM, 0);
            private static readonly LockAction _TID_PERMALOCK = new LockAction(LockBits.TID | LockBits.TID_PERM, LockBits.TID | LockBits.TID_PERM);
            private static readonly LockAction _TID_PERMAUNLOCK = new LockAction(LockBits.TID | LockBits.TID_PERM, LockBits.TID_PERM);
            private static readonly LockAction _USER_LOCK = new LockAction(LockBits.USER | LockBits.USER_PERM, LockBits.USER);
            private static readonly LockAction _USER_UNLOCK = new LockAction(LockBits.USER | LockBits.USER_PERM, 0);
            private static readonly LockAction _USER_PERMALOCK = new LockAction(LockBits.USER | LockBits.USER_PERM, LockBits.USER | LockBits.USER_PERM);
            private static readonly LockAction _USER_PERMAUNLOCK = new LockAction(LockBits.USER | LockBits.USER_PERM, LockBits.USER_PERM);

            private static readonly Dictionary<string, LockAction> _name2ladict = new Dictionary<string, LockAction>(StringComparer.OrdinalIgnoreCase);

            #endregion

            #region Fields

            private UInt16 _action;
            private UInt16 _mask;

            #endregion

            #region Static Properties

            /// <summary>
            /// Lock Kill Password
            /// </summary>
            public static LockAction KILL_LOCK { get { return _KILL_LOCK; } }

            /// <summary>
            /// Unlock Kill Password
            /// </summary>
            public static LockAction KILL_UNLOCK { get { return _KILL_UNLOCK; } }

            /// <summary>
            /// Permanently Lock Kill Password
            /// </summary>
            public static LockAction KILL_PERMALOCK { get { return _KILL_PERMALOCK; } }

            /// <summary>
            /// Permanently Unlock Kill Password
            /// </summary>
            public static LockAction KILL_PERMAUNLOCK { get { return _KILL_PERMAUNLOCK; } }

            /// <summary>
            /// Lock Access Password
            /// </summary>
            public static LockAction ACCESS_LOCK { get { return _ACCESS_LOCK; } }
            /// <summary>
            /// Unlock Access Password
            /// </summary>
            public static LockAction ACCESS_UNLOCK { get { return _ACCESS_UNLOCK; } }

            /// <summary>
            /// Permanently Lock Access Password
            /// </summary>
            public static LockAction ACCESS_PERMALOCK { get { return _ACCESS_PERMALOCK; } }

            /// <summary>
            /// Permanently Unlock Access Password
            /// </summary>
            public static LockAction ACCESS_PERMAUNLOCK { get { return _ACCESS_PERMAUNLOCK; } }

            /// <summary>
            /// Lock EPC Memory Bank
            /// </summary>
            public static LockAction EPC_LOCK { get { return _EPC_LOCK; } }

            /// <summary>
            /// Unlock EPC Memory Bank
            /// </summary>
            public static LockAction EPC_UNLOCK { get { return _EPC_UNLOCK; } }

            /// <summary>
            /// Permanently Lock EPC Memory Bank
            /// </summary>
            public static LockAction EPC_PERMALOCK { get { return _EPC_PERMALOCK; } }

            /// <summary>
            /// Permanently Unlock EPC Memory Bank
            /// </summary>
            public static LockAction EPC_PERMAUNLOCK { get { return _EPC_PERMAUNLOCK; } }

            /// <summary>
            /// Lock TID Memory Bank
            /// </summary>
            public static LockAction TID_LOCK { get { return _TID_LOCK; } }

            /// <summary>
            /// Unlock TID Memory Bank
            /// </summary>
            public static LockAction TID_UNLOCK { get { return _TID_UNLOCK; } }

            /// <summary>
            /// Permanently Lock TID Memory Bank
            /// </summary>
            public static LockAction TID_PERMALOCK { get { return _TID_PERMALOCK; } }

            /// <summary>
            /// Permanently Unlock TID Memory Bank
            /// </summary>
            public static LockAction TID_PERMAUNLOCK { get { return _TID_PERMAUNLOCK; } }

            /// <summary>
            /// Lock User Memory Bank
            /// </summary>
            public static LockAction USER_LOCK { get { return _USER_LOCK; } }

            /// <summary>
            /// Unlock User Memory Bank
            /// </summary>
            public static LockAction USER_UNLOCK { get { return _USER_UNLOCK; } }

            /// <summary>
            /// Permanently Lock User Memory Bank
            /// </summary>
            public static LockAction USER_PERMALOCK { get { return _USER_PERMALOCK; } }

            /// <summary>
            /// Permanently Unlock User Memory Bank
            /// </summary>
            public static LockAction USER_PERMAUNLOCK { get { return _USER_PERMAUNLOCK; } }

            #endregion

            #region Properties

            /// <summary>
            /// Action field for M5e Lock Tag command
            /// </summary>
            protected internal UInt16 Action
            {
                get { return _action; }
            }

            /// <summary>
            /// Mask field for M5e Lock Tag command
            /// </summary>
            protected internal UInt16 Mask
            {
                get { return _mask; }
            }

            #endregion

            #region Construction

            static LockAction()
            {
                _name2ladict.Add("KILL_LOCK", KILL_LOCK);
                _name2ladict.Add("KILL_UNLOCK", KILL_UNLOCK);
                _name2ladict.Add("KILL_PERMALOCK", KILL_PERMALOCK);
                _name2ladict.Add("KILL_PERMAUNLOCK", KILL_PERMAUNLOCK);
                _name2ladict.Add("ACCESS_LOCK", ACCESS_LOCK);
                _name2ladict.Add("ACCESS_UNLOCK", ACCESS_UNLOCK);
                _name2ladict.Add("ACCESS_PERMALOCK", ACCESS_PERMALOCK);
                _name2ladict.Add("ACCESS_PERMAUNLOCK", ACCESS_PERMAUNLOCK);
                _name2ladict.Add("EPC_LOCK", EPC_LOCK);
                _name2ladict.Add("EPC_UNLOCK", EPC_UNLOCK);
                _name2ladict.Add("EPC_PERMALOCK", EPC_PERMALOCK);
                _name2ladict.Add("EPC_PERMAUNLOCK", EPC_PERMAUNLOCK);
                _name2ladict.Add("TID_LOCK", TID_LOCK);
                _name2ladict.Add("TID_UNLOCK", TID_UNLOCK);
                _name2ladict.Add("TID_PERMALOCK", TID_PERMALOCK);
                _name2ladict.Add("TID_PERMAUNLOCK", TID_PERMAUNLOCK);
                _name2ladict.Add("USER_LOCK", USER_LOCK);
                _name2ladict.Add("USER_UNLOCK", USER_UNLOCK);
                _name2ladict.Add("USER_PERMALOCK", USER_PERMALOCK);
                _name2ladict.Add("USER_PERMAUNLOCK", USER_PERMAUNLOCK);
            }

            /// <summary>
            /// Create Gen2.LockAction out of raw mask and action bitmasks
            /// </summary>
            /// <param name="mask">Lock bits to act on</param>
            /// <param name="action">Lock bit values</param>
            public LockAction(UInt16 mask, UInt16 action)
            {
                _mask = mask;
                _action = action;
            }

            /// <summary>
            /// Create Gen2.LockAction out of enum-wrapped mask and action bitmasks
            /// </summary>
            /// <param name="mask">Lock bits to act on</param>
            /// <param name="action">Lock bit values</param>
            private LockAction(Gen2.LockBits mask, Gen2.LockBits action)
                : this((UInt16)mask, (UInt16)action) { }

            /// <summary>
            /// Create Gen2.LockAction out of other Gen2.LockActions
            /// </summary>
            /// <param name="actions">Lock actions to combine.
            /// If a data field is repeated, the last one takes precedence; e.g.,
            /// Gen2.LockAction.USER_LOCK, Gen2.LockAction.USER_UNLOCK
            /// turns into Gen2.LockAction.USER_UNLOCK.</param>
            public LockAction(params LockAction[] actions)
                : this((UInt16)0, (UInt16)0)
            {
                foreach (LockAction la in actions)
                {
                    // Union mask
                    _mask |= la.Mask;

                    // Overwrite action
                    _action &= (UInt16)(~(la.Mask));
                    _action |= (UInt16)(la.Action & la.Mask);
                }
            }

            #endregion

            #region Parse

            /// <summary>
            /// Convert the string representation into an equivalent object.
            /// </summary>
            /// <param name="value">A string containing the name to convert.
            /// May be the name of a predefined constant, or a comma-separated list of predefined constant names.
            /// </param>
            /// <returns>A LockAction whose value is represented by value.</returns>
            public static LockAction Parse(string value)
            {
                if (null == value)
                    throw new ArgumentNullException("value is null");

                List<Gen2.LockAction> actions = new List<Gen2.LockAction>();

                foreach (string name in value.Split(new char[] { ',' }))
                {
                    if (_name2ladict.ContainsKey(name))
                    {
                        LockAction act = _name2ladict[name];
                        actions.Add(act);
                    }
                    else
                        throw new ArgumentException("Unknown Gen2.LockAction " + value);
                }

                return new Gen2.LockAction(actions.ToArray());
            }

            #endregion

            #region ToString

            /// <summary>
            /// Convert the value of this instance to its equivalent string representation.
            /// </summary>
            /// <returns>A string that represents the current object</returns>
            public override string ToString()
            {
                List<string> names = new List<string>();

                foreach (KeyValuePair<string, LockAction> kv in _name2ladict)
                {
                    string name = kv.Key;
                    LockAction value = kv.Value;

                    // Extract relevant portion of action and mask
                    UInt16 maskpart = (UInt16)(Mask & value.Mask);
                    UInt16 actionpart = (UInt16)(Action & value.Mask);

                    // Compare to predefined constant
                    if ((value.Mask == maskpart) && (value.Action == actionpart))
                        names.Add(name);
                }

                return String.Join(",", names.ToArray());
            }

            #endregion
        }

        #endregion

        #region Tag Embedded Commands

        #region WriteData
        /// <summary>
        /// Embedded Tag Operation: Write Data
        /// </summary>
        public class WriteData : TagOp
        {
            #region Fields

            /// <summary>
            /// Gen2 memory bank to write to
            /// </summary>
            public Gen2.Bank Bank;
            /// <summary>
            /// Word address to start writing at
            /// </summary>
            public UInt32 WordAddress;
            /// <summary>
            /// Data to write
            /// </summary>
            public ushort[] Data;

            #endregion

            #region Construction
            /// <summary>
            /// Constructor to initialize the parameters of WriteData
            /// </summary>
            /// <param name="bank">The memory bank to write</param>
            /// <param name="wordAddress">Write starting address</param>
            /// <param name="data">The data to write</param>
            public WriteData(Gen2.Bank bank, UInt32 wordAddress, ushort[] data)
            {
                this.Bank = bank;
                this.WordAddress = wordAddress;
                this.Data = data;
            }

            #endregion

        }

        #endregion

        #region ReadData
        /// <summary>
        /// Embedded Tag Operation: Read Data
        /// </summary>
        public class ReadData : TagOp
        {
            #region Fields

            /// <summary>
            /// Gen2 memory bank to read from
            /// </summary>
            public Gen2.Bank Bank;
            /// <summary>
            /// Word address to start reading at
            /// </summary>
            public UInt32 WordAddress;
            /// <summary>
            /// Number of words to read
            /// </summary>
            public byte Len;

            #endregion

            #region Construction
            /// <summary>
            /// Constructor to initialize the parameters of ReadData
            /// </summary>
            /// <param name="bank">The memory bank to read</param>
            /// <param name="wordAddress">Read starting address</param>
            /// <param name="length">The length of data to read</param>
            public ReadData(Gen2.Bank bank, UInt32 wordAddress, byte length)
            {
                this.Bank = bank;
                this.WordAddress = wordAddress;
                this.Len = length;
            }

            #endregion

        }

        #endregion

        # region SecureTagType
        
        /// <summary>
        /// Enum SecureTagType Default, Alien Higgs 3 and Monza 4 QT  
        /// </summary>
        public enum SecureTagType
        {
            /// <summary>
            /// Default
            /// </summary>
            DEFAULT = 0x00,
            /// <summary>
            /// Alien Higgs 3 secure access
            /// </summary>
            HIGGS3 = 0x02,
            /// <summary>
            /// Monza 4 secure access.
            /// </summary>
            MONZA4 = 0x04,
        }
        #endregion

        #region SecureReadData
       /// <summary>
        /// Embedded Tag Operation: Secure Read Data
        /// </summary>
        public class SecureReadData : ReadData
        {
            #region Fields
            
            /// <summary>
            /// Enum SecureTagType Default, Alien Higgs 3 and Monza 4 QT
            /// </summary>
            public SecureTagType type;
            /// <summary>
            /// Password
            /// </summary>
            public TagAuthentication password;
            #endregion

            #region Construction
            /// <summary>
            /// Constructor to initialize the parameters of SecureReadData
            /// </summary>
            /// <param name="bank">The memory bank to read</param>
            /// <param name="wordAddress">Read starting address</param>
            /// <param name="length">The length of data to read</param>
            /// <param name="type">Secure tag type</param>
            /// <param name="password">Password</param>
            public SecureReadData(Gen2.Bank bank, UInt32 wordAddress, byte length, SecureTagType type, TagAuthentication password) : base (bank,wordAddress,length)
            {
                this.type = type;
                this.password = password;
            }

            #endregion
        }

        #endregion

        #region SecurePasswordLookup
        /// <summary>
        /// Secure Read Data : SecurePasswordLookup
        /// </summary>
        public class SecurePasswordLookup : TagAuthentication
        {
            #region Fields

            /// <summary>
            /// Number of bits used to address the AP list (MSB byte - byte 4) 
            /// </summary>
            public byte SecureAddressLength;
            /// <summary>
            /// EPC word offset (Next MSB byte - byte 3) 
            /// </summary>
            public byte SecureAddressOffset;
            /// <summary>
            /// User flash offset, starting from 0x0000 (LSB 2 bytes)
            /// </summary>
            public UInt16 SecureFlashOffset;

            #endregion

            #region Construction
            /// <summary>
            /// Constructor to initialize the parameters of SecurePasswordLookup
            /// </summary>
            /// <param name="addressLength">address the AP list</param>
            /// <param name="addressOffset">EPC word offset</param>
            /// <param name="flashOffset">User flash offset</param>
            public SecurePasswordLookup(byte addressLength, byte addressOffset, UInt16 flashOffset)
            {
                this.SecureAddressLength = addressLength;
                this.SecureAddressOffset = addressOffset;
                this.SecureFlashOffset = flashOffset;
            }

            #endregion
        }

        #endregion

        #region DenatranIAVWriteCredential
        /// <summary>
        /// Denatran IAV write credential
        /// </summary>
        public class DenatranIAVWriteCredential  : TagAuthentication
        {
            #region Fields

            /// <summary>
            /// Tag Identification
            /// </summary>
            byte [] tagId;
            /// <summary>
            /// Credentials written word
            /// </summary>
            byte[] dataBuf;

            #endregion

            #region Properties
            
            /// <summary>
            /// Tag Identification
            /// </summary>
            public byte[] TagId
            {
                get 
                {
                    return tagId;
                }
            }
            
            /// <summary>
            /// Credentials written word
            /// </summary>
            public byte[] DataBuf
            {
                get
                {
                    return dataBuf;
                }
            }
            #endregion
            #region Construction
            /// <summary>
            /// Constructor to initialize the parameters of DenatranIAVWriteCredential
            /// </summary>
            public DenatranIAVWriteCredential(byte [] TagId, byte [] DataBuf )
            {
                // Databuff length should be 16 bytes
                if (16 != DataBuf.Length)
                {
                    throw new ArgumentException(String.Format(
                        "Data buf value must be exactly 16 bytes long (got {0} bytes)",
                        DataBuf.Length));
                }
                dataBuf = DataBuf;

                // Tag ID length should be 8 bytes
                if (8 != TagId.Length)
                {
                    throw new ArgumentException(String.Format(
                        "Tag Identification value must be exactly 8 bytes long (got {0} bytes)",
                        TagId.Length));
                }
                tagId = TagId;
            }

            #endregion
        }

        #endregion

        #region DenatranIAVWriteSecCredential
        /// <summary>
        /// Denatran IAV writesec credential
        /// </summary>
        public class DenatranIAVWriteSecCredential : TagAuthentication
        {
            #region Fields

            /// <summary>
            /// Challenge
            /// </summary>
            byte[] data;
            /// <summary>
            /// Credentials written word
            /// </summary>
            byte[] credentials;

            #endregion

            #region Properties

            /// <summary>
            /// Data
            /// </summary>
            public byte[] Data
            {
                get
                {
                    return data;
                }
            }

            /// <summary>
            /// Credentials written word
            /// </summary>
            public byte[] Credentials
            {
                get
                {
                    return credentials;
                }
            }
            #endregion
            #region Construction
            /// <summary>
            /// Constructor to initialize the parameters of DenatranIAVWriteSecCredential
            /// </summary>
            public DenatranIAVWriteSecCredential(byte[] Data, byte[] Credentials)
            {
                // Credentials length should be 16 bytes
                if (16 != Credentials.Length)
                {
                    throw new ArgumentException(String.Format(
                        "Credentials value must be exactly 16 bytes long (got {0} bytes)",
                        Credentials.Length));
                }
                credentials = Credentials;

                // Challenge length should be 6 bytes
                if (6 != Data.Length)
                {
                    throw new ArgumentException(String.Format(
                        "Data value must be exactly 6 bytes long (got {0} bytes)",
                        Data.Length));
                }
                data = Data;
            }

            #endregion
        }

        #endregion

        #region Lock
        /// <summary>
        /// Embedded Tag Operation: Lock
        /// </summary>
        public class Lock : TagOp
        {
            #region Fields

            /// <summary>
            /// Access Password
            /// </summary>
            public UInt32 AccessPassword;

            /// <summary>
            /// Gen2 Lock Action
            /// </summary>
            public LockAction LockAction;

            #endregion

            #region Construction
            /// <summary>
            /// Constructor to initialize the parameters of Lock
            /// </summary>
            /// <param name="accessPassword">The access password</param>
            /// <param name="lockAction">The Gen2 Lock Action</param>
            
            public Lock(UInt32 accessPassword, LockAction lockAction)
            {
                this.AccessPassword = accessPassword;
                this.LockAction = lockAction;
            }

            #endregion

        }
        #endregion

        #region Kill
        /// <summary>
        ///Embedded Tag Operation: Kill
        /// </summary>
        public class Kill : TagOp
        {
            #region Fields

            /// <summary>
            /// Kill password to use to kill the tag
            /// </summary>
            public UInt32 KillPassword;

            #endregion

            #region Construction
            /// <summary>
            /// Constructor to initialize the parameters of Kill
            /// </summary>
            /// <param name="killPassword">Kill password to use to kill the tag</param>
            public Kill(UInt32 killPassword)
            {
                this.KillPassword = killPassword;
            }

            #endregion

        }
        #endregion

        #region BAPParameters
        /// <summary>
        /// BAP timing control parameters
        /// </summary>
        public sealed class BAPParameters
        {
            #region Fields

            // Use reader's current value (powerup default to 3000 us)
            private Int32 powerUpDelayUs = -1;
            // Use reader's current value (frequencyhopoff time default to 20000 us)
            private Int32 freqHopOfftimeUs = -1;

            #endregion

            #region Properties

            /// <summary>
            /// Time delay until transmission of first command after interrogator power-up
            /// For BAP tags to perform effective duty cycling, this value should be several
            /// msecs.  This value is specified in microseconds and defaults to 3000.
            /// </summary>
            public Int32 POWERUPDELAY
            {
                get
                {
                    return powerUpDelayUs;
                }
                set
                {
                    powerUpDelayUs = value;
                }
            }

            /// <summary>
            /// Time delay between dwells or time duration for RF field off while frequency 
            /// hopping . This value is specified in microseconds and defaults to 20000.
            /// </summary>
            public Int32 FREQUENCYHOPOFFTIME
            {
                get
                {
                    return freqHopOfftimeUs;
                }
                set
                {
                    freqHopOfftimeUs = value;
                }
            }

            #endregion

            #region ToString
            /// <summary>
            /// Human-readable representation
            /// </summary>
            /// <returns>Human-readable representation</returns>
            public override string ToString()
            {
                return String.Join(" ", new string[] {
                            "PowerUpDelay="+POWERUPDELAY.ToString(),
                            "FrequencyHopOfftime="+FREQUENCYHOPOFFTIME.ToString(),
                        });
            }
            #endregion ToString

        }
        #endregion BAPParameters

        #endregion
        
        #region TagCommands

        #region WriteTag
        /// <summary>
        /// Write a new ID to a tag.
        /// </summary>
        /// 
        public class WriteTag : TagOp
        {
            #region Fields

            /// <summary>
            /// the new tag ID to write
            /// </summary>
            public TagData Epc;

            #endregion

            #region Construction
            /// <summary>
            /// Constructor to initialize the parameters of WriteTag
            /// </summary>
            /// <param name="epc">the new tag ID to write</param>
            /// 
            public WriteTag(TagData epc)
            {
                this.Epc = epc;
            }

            #endregion

        }

        #endregion

        #region BlockWrite
        /// <summary>
        /// BlockWrite
        /// </summary>
        public class BlockWrite : TagOp
        {
            #region Fields

            /// <summary>
            /// the tag memory bank to write to
            /// </summary>
            public Gen2.Bank Bank;

            /// <summary>
            /// the word address to start writing to
            /// </summary>
            public uint WordPtr;

            /// <summary>
            /// the words to write
            /// </summary>
            public ushort[] Data;
            
            #endregion

            #region Construction
            /// <summary>
            /// Constructor to initialize the parameters of BlockWrite
            /// </summary>
            /// <param name="bank">Gen2 memory bank to write to</param>
            /// <param name="wordPtr">the word address to start writing to</param>
            /// <param name="data">the data to write</param>
            public BlockWrite(Gen2.Bank bank, uint wordPtr, ushort[] data)
            {
                this.Bank = bank;
                this.WordPtr=wordPtr;
                this.Data = data;
            }

            #endregion
        }

        #endregion

        #region BlockPermaLock
        /// <summary>
        /// BlockPermalock
        /// </summary>
        public class BlockPermaLock : TagOp
        {
            #region Fields

            /// <summary>
            /// Read or Lock?
            /// </summary>
            public byte ReadLock;

            /// <summary>
            /// the tag memory bank to lock
            /// </summary>
            public Gen2.Bank Bank;

            /// <summary>
            /// the staring word address to lock
            /// </summary>
            public uint BlockPtr;

            /// <summary>
            /// number of 16 blocks
            /// </summary>
            public byte BlockRange;

            /// <summary>
            /// the Mask
            /// </summary>
            public ushort[] Mask;

            #endregion

            #region Construction
            /// <summary>
            /// Constructor to initialize the parameters of BlockPermaLock
            /// </summary>
            /// <param name="readLock">Read or Lock?</param>
            /// <param name="bank">Gen2 Memory Bank to perform Lock</param>
            /// <param name="blockPtr">starting address of the blocks to operate</param>
            /// <param name="blockRange">number of 16 blocks</param>
            /// <param name="mask">mask</param>
         
            public BlockPermaLock(byte readLock,Gen2.Bank bank, uint blockPtr, byte blockRange, ushort[] mask)
            {
                this.ReadLock = readLock;
                this.Bank = bank;
                this.BlockPtr = blockPtr;
                this.BlockRange = blockRange;
                this.Mask = mask;

            }

            #endregion
        }

        #endregion

        #region BlockErase
        /// <summary>
        /// BlockErase
        /// </summary>
        public class BlockErase : TagOp
        {
            #region Fields

            /// <summary>
            /// the tag memory bank to erase
            /// </summary>
            public Gen2.Bank Bank;

            /// <summary>
            /// the word address to start erase to
            /// </summary>
            public UInt32 WordPtr;

            /// <summary>
            /// no of words to erase
            /// </summary>
            public byte WordCount;

            #endregion

            #region Construction

            /// <summary>
            /// Erase tag specific block
            /// </summary>
            /// <param name="bank">the tag memory bank to erase</param>
            /// <param name="wordPtr">the word address to start erase to</param>
            /// <param name="wordCount">no of words to erase</param>
            public BlockErase(Gen2.Bank bank, UInt32 wordPtr, byte wordCount)
            {
                this.Bank = bank;
                this.WordPtr = wordPtr;
                this.WordCount = wordCount;                
            }

            #endregion
        }

        #endregion

        #region CustomCommands
        /// <summary>
        /// Gen2CustomTagOp
        /// </summary>
        public class Gen2CustomTagOp : TagOp
        {
            /// <summary>
            /// Chip Type
            /// </summary>
            public byte ChipType = 0x00;
        }

        #region Alien

        /// <summary>
        /// Alien vendor type
        /// </summary>
        public class Alien : Gen2CustomTagOp
        {
            #region Higgs2
            /// <summary>
            /// Higgs2
            /// </summary>
            public class Higgs2 : Alien
            {
                /// <summary>
                /// Default constructor
                /// </summary>
                public Higgs2()
                {
                    ChipType = 0x03;
                }
            
                #region Higgs2PartialLoadImage
                /// <summary>
                /// Higgs2PartialLoadImage
                /// </summary>
                public class PartialLoadImage : Higgs2
                {
                    #region Fields
                    /// <summary>
                    /// The access password to write on the tag
                    /// </summary>
                    public UInt32 AccessPassword;

                    /// <summary>
                    /// The kill password to write on the tag
                    /// </summary>
                    public UInt32 KillPassword;

                    /// <summary>
                    /// The EPC to write to the tag. Maximum of 12 bytes (96 bits)
                    /// </summary>
                    public byte[] Epc;
                    #endregion

                    #region Construction
                    /// <summary>
                    /// Constructor to initialize the parameters of WriteTag
                    /// </summary>
                    /// <param name="accessPassword">the access password to write on the tag</param>
                    /// <param name="killPassword">the kill password to write on the tag</param>
                    /// <param name="epc">the EPC to write to the tag. Maximum of 12 bytes (96 bits)</param>
                    public PartialLoadImage(UInt32 accessPassword, UInt32 killPassword, byte[] epc)
                    {
                        this.AccessPassword = accessPassword;
                        this.KillPassword = killPassword;
                        this.Epc = epc;
                    }
                    #endregion
                }
                #endregion    
               
                #region Higgs2FullLoadImage

                /// <summary>
                /// Higgs2FullLoadImage
                /// </summary>
                public class FullLoadImage : Higgs2
                {
                    #region Fields
                    /// <summary>
                    /// The access password to write on the tag
                    /// </summary>
                    public UInt32 AccessPassword;

                    /// <summary>
                    /// The kill password to write on the tag
                    /// </summary>
                    public UInt32 KillPassword;

                    /// <summary>
                    /// The lock bits to write on the tag
                    /// </summary>
                    public UInt16 LockBits;

                    /// <summary>
                    /// The PC word to write on the tag
                    /// </summary>
                    public UInt16 PCWord;
                    /// <summary>
                    /// The EPC to write to the tag. Maximum of 12 bytes (96 bits)
                    /// </summary>
                    public byte[] Epc;

                    #endregion

                    #region Construction
                    /// <summary>
                    /// Constructor to initialize the parameters of FullLoadImage
                    /// </summary>
                    /// <param name="accessPassword">the access password to write on the tag</param>
                    /// <param name="killPassword">the kill password to write on the tag</param>
                    /// <param name="lockBits">the lock bits to write on the tag</param>
                    /// <param name="pcWord">the PC word to write on the tag</param>
                    /// <param name="epc">the EPC to write to the tag. Maximum of 12 bytes (96 bits)</param>
                    public FullLoadImage(UInt32 accessPassword, UInt32 killPassword, UInt16 lockBits, UInt16 pcWord, byte[] epc)
                    {
                        this.AccessPassword = accessPassword;
                        this.KillPassword = killPassword;
                        this.LockBits = lockBits;
                        this.PCWord = pcWord;
                        this.Epc = epc;
                    }
                    #endregion

                }

                #endregion
            }
            #endregion Higgs2

            #region Higgs3

            /// <summary>
            /// Higgs3
            /// </summary>
            public class Higgs3 : Alien
            { 
                /// <summary>
                /// Default constructor
                /// </summary>
                public Higgs3()
                {
                    ChipType = 0x05;
                }

                #region Higgs3FastLoadImage
            /// <summary>
            /// Higgs3FastLoadImage
            /// </summary>
            public class FastLoadImage : Higgs3
            {
                #region Fields

                /// <summary>
                /// The access password to use to write to the tag
                /// </summary>
                public UInt32 CurrentAccessPassword;

                /// <summary>
                /// The access password to write on the tag
                /// </summary>
                public UInt32 AccessPassword;

                /// <summary>
                /// The kill password to write on the tag
                /// </summary>
                public UInt32 KillPassword;

                /// <summary>
                /// The PC word to write on the tag
                /// </summary>
                public UInt16 PCWord;

                /// <summary>
                /// The EPC to write to the tag. Maximum of 12 bytes (96 bits)
                /// </summary>
                public byte[] Epc;

                #endregion

                #region Construction
                /// <summary>
                /// Constructor to initialize the parameters of FastLoadImage
                /// </summary>
                /// <param name="currentAccessPassword">the access password to use to write to the tag</param>
                /// <param name="accessPassword">the access password to write on the tag</param>
                /// <param name="killPassword">the kill password to write on the tag</param>
                /// <param name="pcWord">the PC word to write on the tag</param>
                /// <param name="epc">the EPC to write to the tag. Must be exactly 12 bytes (96 bits)</param>
                public FastLoadImage(UInt32 currentAccessPassword, UInt32 accessPassword, UInt32 killPassword, UInt16 pcWord, byte[] epc)
                {
                    this.CurrentAccessPassword = currentAccessPassword;
                    this.AccessPassword = accessPassword;
                    this.KillPassword = killPassword;
                    this.PCWord = pcWord;
                    this.Epc = epc;
                }
                #endregion

            }

            #endregion

            #region Higgs3LoadImage
            /// <summary>
            /// Higgs3FastLoadImage
            /// </summary>
            public class LoadImage : Higgs3
            {
                #region Fields

                /// <summary>
                /// The access password to use to write to the tag
                /// </summary>
                public UInt32 CurrentAccessPassword;

                /// <summary>
                /// The access password to write on the tag
                /// </summary>
                public UInt32 AccessPassword;

                /// <summary>
                /// The kill password to write on the tag
                /// </summary>
                public UInt32 KillPassword;

                /// <summary>
                /// The PC word to write on the tag
                /// </summary>
                public UInt16 PCWord;

                /// <summary>
                /// The EPC to write to the tag. Maximum of 12 bytes (96 bits)
                /// </summary>
                public byte[] EpcAndUserData;

                #endregion

                #region Construction
                /// <summary>
                /// Constructor to initialize the parameters of FastLoadImage
                /// </summary>
                /// <param name="currentAccessPassword">the access password to use to write to the tag</param>
                /// <param name="accessPassword">the access password to write on the tag</param>
                /// <param name="killPassword">the kill password to write on the tag</param>
                /// <param name="pcWord">the PC word to write on the tag</param>
                /// <param name="epcAndUserData">
                /// the EPC and user data to write to the
                /// tag. Must be exactly 76 bytes. The pcWord specifies which of this
                /// is EPC and which is user data.
                /// </param>
                public LoadImage(UInt32 currentAccessPassword, UInt32 accessPassword, UInt32 killPassword, UInt16 pcWord, byte[] epcAndUserData)
                {
                    this.CurrentAccessPassword = currentAccessPassword;
                    this.AccessPassword = accessPassword;
                    this.KillPassword = killPassword;
                    this.PCWord = pcWord;
                    this.EpcAndUserData = epcAndUserData;
                }
                #endregion
            }

            #endregion

            #region Higgs3BlockReadLock
            /// <summary>
            /// Higgs3BlockReadLock
            /// </summary>
            public class BlockReadLock : Higgs3
            {
                #region Fields

                /// <summary>
                /// The access password to use to write to the tag
                /// </summary>
                public UInt32 AccessPassword;

                /// <summary>
                /// A bitmask of bits to lock. Valid range 0-255
                /// </summary>
                public byte LockBits;

                #endregion

                #region Construction
                /// <summary>
                /// Constructor to initialize the parameters of Higgs3BlockReadLock
                /// </summary>
                /// <param name="accessPassword">the access password to use to write to the tag</param>
                /// <param name="lockBits">a bitmask of bits to lock. Valid range 0-255</param>
                public BlockReadLock(UInt32 accessPassword, byte lockBits)
                {
                    this.AccessPassword = accessPassword;
                    this.LockBits = lockBits;
                }
                #endregion

            }

            #endregion
        }
            #endregion Higgs3
        }

        #endregion Alien

        #region IDS
        /// <summary>
        /// IDS Microchip Custom Gen2 Tag Operations
        /// </summary>
        public class IDS : Gen2CustomTagOp
        {
            #region SL900A

            /// <summary>
            /// SL900A Sensor Logging Chip Protocol
            /// </summary>
            public class SL900A : IDS
            {
                #region Nested Enums

                /// <summary>
                /// SL900A sensor type values
                /// </summary>
                public enum Sensor
                {
                    /// <summary>
                    /// Reserved memory contains kill and access passwords
                    /// </summary>
                    TEMP = 0,
                    /// <summary>
                    /// EPC memory contains CRC, PC, EPC
                    /// </summary>
                    EXT1 = 1,
                    /// <summary>
                    /// TID memory contains tag implementation identifiers
                    /// </summary>
                    EXT2 = 2,
                    /// <summary>
                    /// User memory is optional, but exists for user-defined data
                    /// </summary>
                    BATTV = 3,
                }

                /// <summary>
                /// SL900A password access level values
                /// </summary>
                public enum Level
                {
                    /// <summary>
                    /// No access to any protected area
                    /// </summary>
                    NOT_ALLOWED = 0,
                    /// <summary>
                    /// System area access
                    /// </summary>
                    SYSTEM = 1,
                    /// <summary>
                    /// Application area access
                    /// </summary>
                    APPLICATION = 2,
                    /// <summary>
                    /// Measurement area access
                    /// </summary>
                    MEASUREMENT = 3,
                }

                /// <summary>
                /// Data log format selection
                /// </summary>
                public enum LoggingForm
                {
                    /// <summary>
                    /// Dense Logging
                    /// </summary>
                    DENSE = 0,
                    /// <summary>
                    /// Log values outside specified limits
                    /// </summary>
                    OUTOFLIMITS = 1,
                    /// <summary>
                    /// Log values at limit crossing points
                    /// </summary>
                    LIMITSCROSSING = 3,
                    /// <summary>
                    /// Trigger log on EXT1 input
                    /// </summary>
                    IRQ1 = 5,
                    /// <summary>
                    /// Trigger log on EXT2 input
                    /// </summary>
                    IRQ2 = 6,
                    /// <summary>
                    /// Trigger log on EXT1 and EXT2 input
                    /// </summary>
                    IRQ1IRQ2 = 7,
                }

                /// <summary>
                /// Logging memory-full behavior
                /// </summary>
                public enum StorageRule
                {
                    /// <summary>
                    /// Stop logging when memory fills
                    /// </summary>
                    NORMAL = 0,
                    /// <summary>
                    /// Roll around (circular buffer) when memory fills
                    /// </summary>
                    ROLLING = 1,
                }

                /// <summary>
                /// Data log memory-full behavior
                /// </summary>
                public enum DelayMode
                {
                    /// <summary>
                    /// Start logging after delay time
                    /// </summary>
                    TIMER = 0,
                    /// <summary>
                    /// Start logging on external input
                    /// </summary>
                    EXTSWITCH = 1,
                }

                /// <summary>
                /// Request the battery type re-check
                /// </summary>
                public enum BatteryType
                {
                    /// <summary>
                    /// Default
                    /// </summary>
                    CHECK = 0x00,
                    /// <summary>
                    /// Application requested for the re-check of battery type
                    /// </summary>
                    RECHECK = 0x01,
                }

                #endregion

                #region Fields

                /// <summary>
                /// Custom Command Code
                /// </summary>
                public byte CommandCode;

                /// <summary>
                /// Gen2 access password
                /// </summary>
                public UInt32 AccessPassword;

                /// <summary>
                /// IDS SL900A Password
                /// </summary>
                public UInt32 Password;

                /// <summary>
                /// Access level to which SL900A password applies
                /// </summary>
                public Level PasswordLevel;

                #endregion

                #region Construction
                /// <summary>
                /// Default constructor
                /// <param name="commandCode">Custom command code</param>
                /// </summary>
                public SL900A(byte commandCode)
                {
                    CommandCode = commandCode;
                    ChipType = 0x0A;
                    AccessPassword = 0;
                    Password = 0;
                    PasswordLevel = Level.NOT_ALLOWED;
                }

                /// <summary>
                /// Constructor to initialize commandcode, password and passwordlevel
                /// </summary>
                /// <param name="commandCode">Custom command code</param>
                /// <param name="passwordLevel">Passwordlevel</param>
                /// <param name="password">Password</param>                
                public SL900A(byte commandCode, Level passwordLevel, UInt32 password)
                {
                    CommandCode = commandCode;
                    ChipType = 0x0A;
                    AccessPassword = 0;
                    Password = password;
                    PasswordLevel = passwordLevel;
                 }
                #endregion Construction

                #region Data Structures

                /// <summary>
                /// Logging memory configuration
                /// </summary>
                public class ApplicationData
                {
                    /// <summary>
                    /// Create default ApplicationData
                    /// </summary>
                    public ApplicationData()
                    {
                        NumberOfWords = 0;
                    }

                    /// <summary>
                    /// Create ApplicationData
                    /// </summary>
                    /// <param name="reply">Raw reply message</param>
                    /// <param name="offset">Start index of raw value within reply</param>
                    public ApplicationData(byte[] reply, int offset)
                    {
                        Raw = ByteConv.ToU16(reply, offset);
                    }

                    /// <summary>
                    /// Raw 16-bit protocol value
                    /// </summary>
                    public UInt16 Raw = 0;

                    /// <summary>
                    /// Number of user bank memory words to reserve for non-logging purposes 
                    /// </summary>
                    public UInt16 NumberOfWords
                    {
                        get { return (UInt16)((Raw >> 7) & 0x1FF); }
                        set
                        {
                            if ((value & 0x1FF) != value)
                            {
                                throw new ArgumentOutOfRangeException("Number of words must fit in 9 bits");
                            }
                            UInt16 mask = 0x1FF;
                            Raw &= (UInt16)~mask;
                            Raw |= (UInt16)(value << 7);
                        }
                    }

                    /// <summary>
                    /// Broken word pointer
                    /// </summary>
                    public byte BrokenWordPointer
                    {
                        get { return (byte)(Raw & 0x7); }
                        set
                        {
                            if ((value & 0x7) != value)
                            {
                                throw new ArgumentOutOfRangeException("Broken word pointer must fit in 3 bits");
                            }
                            UInt16 mask = 0x7;
                            Raw &= (UInt16)~mask;
                            Raw |= (UInt16)(value);
                        }
                    }
                    
                    /// <summary>
                    /// Human-readable representation
                    /// </summary>
                    /// <returns>Human-readable representation</returns>
                    public override string ToString()
                    {
                        return String.Join(" ", new string[] {
                            "Raw=("+Raw+")",
                            "NumberOfWords=("+NumberOfWords+")",
                            "BrokenWordPointer=("+BrokenWordPointer+")",
                        });
                }
                }

                /// <summary>
                /// Calibration parameters
                /// </summary>
                public class CalibrationData
                {
                    /// <summary>
                    /// Raw 56-bit Calibration data value
                    /// </summary>
                    public UInt64 Raw;

                    /// <summary>
                    /// Create CalibrationData object from raw 2-byte reply
                    /// </summary>
                    /// <param name="reply">Raw reply message</param>
                    /// <param name="offset">Start index of raw value within reply</param>
                    public CalibrationData(byte[] reply, int offset)
                    {
                        // ToU64 requires 8 bytes of input, but CalibrationData is only 7 bytes long.
                        // Create a temporary array to provide the necessary padding.
                        byte[] tmp = new byte[] {0,0,0,0,0,0,0,0};
                        Array.Copy(reply, offset, tmp, 1, 7);
                        Raw = ByteConv.ToU64(tmp, 0);
                        Raw &= 0x00FFFFFFFFFFFFFF;
                    }

                    /// <summary>
                    /// Human-readable representation
                    /// </summary>
                    /// <returns>Human-readable representation</returns>
                    public override string ToString()
                    {
                        return String.Join(" ", new string[] {
                            "Ad1="+Ad1,
                            "Coarse1="+Coarse1,
                            "Ad2="+Ad2,
                            "Coarse2="+Coarse2,
                            "GndSwitch="+GndSwitch,
                            "Selp12="+Selp12,
                            "Adf="+Adf,
                            "Df="+Df,
                            "SwExtEn="+SwExtEn,
                            "Selp22="+Selp22,
                            "Irlev="+Irlev,
                            "RingCal="+RingCal,
                            "OffInt="+OffInt,
                            "Reftc="+Reftc,
                            "ExcRes="+ExcRes,
                        });
                    }

                    /// <summary>
                    /// AD1 lower voltage reference - fine - DO NOT MODIFY
                    /// </summary>
                    public byte Ad1
                    {
                        get { return (byte)ByteConv.GetBits(Raw, 51, 5); }
                    }

                    /// <summary>
                    /// AD1 lower voltage reference - coarse
                    /// </summary>
                    public byte Coarse1
                    {
                        get { return (byte)ByteConv.GetBits(Raw, 48, 3); }
                        set { ByteConv.SetBits(ref Raw, 48, 3, value, "Coarse1"); }
                    }

                    /// <summary>
                    /// AD2 lower voltage reference - fine - DO NOT MODIFY
                    /// </summary>
                    public byte Ad2
                    {
                        get { return (byte)ByteConv.GetBits(Raw, 43, 5); }
                    }

                    /// <summary>
                    /// AD2 lower voltage reference - coarse
                    /// </summary>
                    public byte Coarse2
                    {
                        get { return (byte)ByteConv.GetBits(Raw, 40, 3); }
                        set { ByteConv.SetBits(ref Raw, 40, 3, value, "Coarse2"); }
                    }

                    /// <summary>
                    /// Switches the lower AD voltage reference to ground
                    /// </summary>
                    public bool GndSwitch
                    {
                        get { return 0 != ByteConv.GetBits(Raw, 39, 1); }
                        set { ByteConv.SetBits(ref Raw, 39, 1, (UInt64)(value?1:0), "GndSwitch"); }
                    }

                    /// <summary>
                    /// POR voltage level for 1.5V system
                    /// </summary>
                    public byte Selp12
                    {
                        get { return (byte)ByteConv.GetBits(Raw, 37, 2); }
                        set { ByteConv.SetBits(ref Raw, 37, 2, value, "Selp12"); }
                    }

                    /// <summary>
                    /// Main reference voltage calibration -- DO NOT MODIFY
                    /// </summary>
                    public byte Adf
                    {
                        get { return (byte)ByteConv.GetBits(Raw, 32, 5); }
                    }

                    /// <summary>
                    /// RTC oscillator calibration
                    /// </summary>
                    public byte Df
                    {
                        get { return (byte)ByteConv.GetBits(Raw, 24, 8); }
                        set { ByteConv.SetBits(ref Raw, 24, 8, value, "Df"); }
                    }

                    /// <summary>
                    /// Controlled battery supply for external sensor - the battery voltage is connected to the EXC pin
                    /// </summary>
                    public bool SwExtEn
                    {
                        get { return 0 != ByteConv.GetBits(Raw, 23, 1); }
                        set { ByteConv.SetBits(ref Raw, 23, 1, (UInt64)(value ? 1 : 0), "SwExtEn"); }
                    }

                    /// <summary>
                    /// POR voltage level for 3V system
                    /// </summary>
                    public byte Selp22
                    {
                        get { return (byte)ByteConv.GetBits(Raw, 21, 2); }
                        set { ByteConv.SetBits(ref Raw, 21, 2, value, "Selp22"); }
                    }

                    /// <summary>
                    /// Voltage level interrupt level for external sensor -- ratiometric
                    /// </summary>
                    public byte Irlev
                    {
                        get { return (byte)ByteConv.GetBits(Raw, 19, 2); }
                        set { ByteConv.SetBits(ref Raw, 19, 2, value, "Irlev"); }
                    }

                    /// <summary>
                    /// Main system clock oscillator calibration -- DO NOT MODIFY
                    /// </summary>
                    public byte RingCal
                    {
                        get { return (byte)ByteConv.GetBits(Raw, 14, 5); }
                    }

                    /// <summary>
                    /// Temperature conversion offset calibration -- DO NOT MODIFY
                    /// </summary>
                    public byte OffInt
                    {
                        get { return (byte)ByteConv.GetBits(Raw, 7, 7); }
                    }

                    /// <summary>
                    /// Bandgap voltage temperature coefficient calibration -- DO NOT MODIFY
                    /// </summary>
                    public byte Reftc
                    {
                        get { return (byte)ByteConv.GetBits(Raw, 3, 4); }
                    }

                    /// <summary>
                    /// Excitate for resistive sensors without DC
                    /// </summary>
                    public bool ExcRes
                    {
                        get { return 0 != ByteConv.GetBits(Raw, 2, 1); }
                        set { ByteConv.SetBits(ref Raw, 2, 1, (UInt64)(value ? 1 : 0), "ExcRes"); }
                    }

                    /// <summary>
                    ///  Reserved for Future Use
                    /// </summary>
                    public byte RFU
                    {
                        get { return (byte)ByteConv.GetBits(Raw, 0, 2); }
                    }
                }

                /// <summary>
                /// Sensor Front End Parameters
                /// </summary>
                public class SfeParameters
                {
                    /// <summary>
                    /// Raw 16-bit SFE parameters value
                    /// </summary>
                     public UInt16 Raw;

                    /// <summary>
                    /// Create SFEParameters object from raw 2-byte reply
                    /// </summary>
                    /// <param name="reply">Raw reply message</param>
                    /// <param name="offset">Start index of raw value within reply</param>
                    public SfeParameters(byte[] reply, int offset)
                    {
                        Raw = ByteConv.ToU16(reply, offset);
                    }

                    /// <summary>
                    /// Human-readable representation
                    /// </summary>
                    /// <returns>Human-readable representation</returns>
                    public override string ToString()
                    {
                        return String.Join(" ", new string[] {
                            "Rang="+Rang,
                            "Seti="+Seti,
                            "Ext1="+Ext1,
                            "Ext2="+Ext2,
                            "AutorangeDisable="+AutorangeDisable,
                            "VerifySensorID="+VerifySensorID,
                        });
                    }

                    /// <summary>
                    /// External sensor 2 range
                    /// </summary>
                    public byte Rang
                    {
                        get { return (byte)ByteConv.GetBits(Raw, 11, 5); }
                        set { ByteConv.SetBits(ref Raw, 11, 5, value, "Rang"); }
                    }

                    /// <summary>
                    /// External sensor 1 range
                    /// </summary>
                    public byte Seti
                    {
                        get { return (byte)ByteConv.GetBits(Raw, 6, 5); }
                        set { ByteConv.SetBits(ref Raw, 6, 5, value, "Rang"); }
                    }

                    /// <summary>
                    /// External sensor 1 type
                    ///   00 -- linear resistive sensor
                    ///   01 -- high impedance input (voltage follower), bridge
                    ///   10 -- capacitive sensor with DC
                    ///   11 -- capacitive or resistive sensor without DC
                    /// </summary>
                    public byte Ext1
                    {
                        get { return (byte)ByteConv.GetBits(Raw, 4, 2); }
                        set { ByteConv.SetBits(ref Raw, 4, 2, value, "Rang"); }
                    }

                    /// <summary>
                    /// External sensor 2 type
                    ///   00 -- linear conductive sensor
                    ///   01 -- high impedance input (voltage follower), bridge
                    /// </summary>
                    public byte Ext2
                    {
                        get { return (byte)ByteConv.GetBits(Raw, 3, 1); }
                        set { ByteConv.SetBits(ref Raw, 3, 1, value, "Rang"); }
                    }

                    /// <summary>
                    /// Use preset range
                    /// </summary>
                    public bool AutorangeDisable
                    {
                        get { return 0 != ByteConv.GetBits(Raw, 2, 1); }
                        set { ByteConv.SetBits(ref Raw, 2, 1, (UInt16)(value ? 1 : 0), "AutorangeDisable"); }
                    }

                    /// <summary>
                    /// Sensor used in limit check
                    ///   00 - first selected sensor
                    ///   01 -- second selected sensor
                    ///   10 -- third selected sensor
                    ///   11 -- fourth selected sensor
                    /// </summary>
                    public byte VerifySensorID
                    {
                        get { return (byte)ByteConv.GetBits(Raw, 0, 2); }
                        set { ByteConv.SetBits(ref Raw, 0, 2, value, "Rang"); }
                    }
                }

                /// <summary>
                /// Combination Calibration Data / SFE Parameters object
                /// (as received from Get Calibration Data command)
                /// </summary>
                public class CalSfe
                {
                    /// <summary>
                    /// Calibration Data
                    /// </summary>
                    public CalibrationData Cal;
                    /// <summary>
                    /// Sensor Front End Parameters
                    /// </summary>
                    public SfeParameters Sfe;
                    /// <summary>
                    /// Create Calibration Data / SFE Parameter object from raw 72-bit reply
                    /// </summary>
                    /// <param name="reply"></param>
                    /// <param name="offset"></param>
                    public CalSfe(byte[] reply, int offset)
                    {
                        Cal = new CalibrationData(reply, offset + 0);
                        Sfe = new SfeParameters(reply, offset + 7);
                    }
                    /// <summary>
                    /// Human-readable representation
                    /// </summary>
                    /// <returns>Human-readable representation</returns>
                    public override string ToString()
                    {
                        return String.Join(" ", new string[] {
                            "Cal=("+Cal+")",
                            "Sfe=("+Sfe+")",
                        });
                    }
                }

                /// <summary>
                /// Delay Time structure for Initialize command
                /// </summary>
                public class Delay
                {
                    /// <summary>
                    /// Raw 16-bit protocol value
                    /// </summary>
                    public UInt16 Raw;

                    /// <summary>
                    /// Create default Delay setting
                    /// </summary>
                    public Delay()
                    {
                        Mode = DelayMode.TIMER;
                        Time = 0;
                        IrqTimerEnable = false;
                    }
                    
                    /// <summary>
                    /// Create Delay setting
                    /// </summary>
                    /// <param name="reply">Raw reply message</param>
                    /// <param name="offset">Start index of raw value within reply</param>
                    public Delay(byte [] reply, int offset)
                    {
                        Raw = ByteConv.ToU16(reply, offset);
                    }

                    /// <summary>
                    /// Logging start mode
                    /// </summary>
                    public DelayMode Mode
                    {
                        get
                        {
                            return (0 == ((Raw >> 1) & 0x1))
                            ? DelayMode.TIMER
                            : DelayMode.EXTSWITCH;
                        }
                        set
                        {
                            UInt16 mask = 0x1 << 1;
                            if (DelayMode.TIMER == value)
                            {
                                Raw &= (UInt16)~mask;
                            }
                            else
                            {
                                Raw |= mask;
                            }
                        }
                    }

                    /// <summary>
                    /// Logging timer delay value (units of 512 seconds)
                    /// </summary>
                    public UInt16 Time
                    {
                        get { return (UInt16)((Raw >> 4) & 0xFFF); }
                        set
                        {
                            if ((value & 0xFFF) != value)
                            {
                                throw new ArgumentOutOfRangeException("Delay Time must fit in 12 bits");
                            }
                            UInt16 mask = 0xFFF << 4;
                            Raw &= (UInt16)~mask;
                            Raw |= (UInt16)(value << 4);
                        }
                    }

                    /// <summary>
                    /// Trigger log on both timer and external interrupts?
                    /// </summary>
                    public bool IrqTimerEnable
                    {
                        get { return (0 != (Raw & 0x1)); }
                        set
                        {
                            UInt16 mask = 0x1;
                            if (value)
                            {
                                Raw |= mask;
                            }
                            else
                            {
                                Raw &= (UInt16)~mask;
                            }
                        }
                    }

                    /// <summary>
                    /// Human-readable representation
                    /// </summary>
                    /// <returns>Human-readable representation</returns>
                    public override string ToString()
                    {
                        return String.Join(" ", new string[] {
                            "Raw=("+Raw+")",
                            "DelayMode=("+Mode+")",
                            "Time=("+Time+")",
                            "IrqTimerEnable=("+IrqTimerEnable+")",
                        });
                }
                }

                /// <summary>
                /// Sensor limit excursion counters
                /// </summary>
                public class LimitCounter
                {
                    /// <summary>
                    /// Create LimitCounter reply object
                    /// </summary>
                    /// <param name="reply">Raw response containing 4-byte Limit Counter string</param>
                    /// <param name="offset">Index of byte where Limit Counter string starts</param>
                    public LimitCounter(byte[] reply, int offset)
                    {
                        _extremeLower = reply[offset + 0];
                        _lower = reply[offset + 1];
                        _upper = reply[offset + 2];
                        _extremeUpper = reply[offset + 3];
                    }
                    private byte _extremeLower;
                    private byte _lower;
                    private byte _upper;
                    private byte _extremeUpper;

                    /// <summary>
                    /// Number of times selected sensor has gone beyond extreme lower limit
                    /// </summary>
                    public byte ExtremeLower { get { return _extremeLower; } }
                    /// <summary>
                    /// Number of times selected sensor has gone beyond lower limit
                    /// </summary>
                    public byte Lower { get { return _lower; } }
                    /// <summary>
                    /// Number of times selected sensor has gone beyond upper limit
                    /// </summary>
                    public byte Upper { get { return _upper; } }
                    /// <summary>
                    /// Number of times selected sensor has gone beyond extreme upper limit
                    /// </summary>
                    public byte ExtremeUpper { get { return _extremeUpper; } }

                    /// <summary>
                    /// Human-readable representation
                    /// </summary>
                    /// <returns>Human-readable representation</returns>
                    public override string ToString()
                    {
                        return String.Join(" ", new string[] {
                            "ExtremeLower="+ExtremeLower.ToString(),
                            "Lower="+Lower.ToString(),
                            "Upper="+Upper.ToString(),
                            "ExtremeUpper="+ExtremeUpper.ToString(),
                        });
                    }
                }

                /// <summary>
                /// Get Log State reply
                /// </summary>
                public class LogState
                {
                    /// <summary>
                    /// Create Get Log state reply object
                    /// </summary>
                    /// <param name="reply">Raw 9 or 21-bit Get Log State reply</param>
                    public LogState(byte[] reply)
                    {
                        if (!((9 == reply.Length) || (20 == reply.Length)))
                        {
                            throw new ArgumentOutOfRangeException("GetLogState replies must be 9 or 20 bytes in length");
                        }
                        int offset = 0;
                        _LimitCount = new LimitCounter(reply, offset); offset += 4;
                        _SystemStat = new SystemStatus(reply, offset); offset += 4;
                        
                        if (20 == reply.Length)
                        {   
                            _SetShelLifeBlock0 = new ShelfLifeBlock0(reply, offset);
                            offset += 4;
                            _SetShelLifeBlock1 = new ShelfLifeBlock1(reply, offset);
                             offset += 4;
                             _remainingShelfLife = GetRemainingShelfLife(reply, offset);
                            offset += 3;
                        }

                        _StatFlags = new StatusFlags(reply[offset]); offset += 1;
                    }

                    private byte[] GetRemainingShelfLife(byte [] reply, int offset)
                    {
                        byte[] remainingShelfLife = new byte[3];
                        Array.Copy(reply, offset, remainingShelfLife, 0, 3);
                        return remainingShelfLife;
                    }
                    
                    private byte[] _remainingShelfLife = new byte[3];
                    /// <summary>
                    /// Remaining shelf life
                    /// </summary>
                    public byte[] RemainingShelfLife { get { return _remainingShelfLife; } }
                    private ShelfLifeBlock0 _SetShelLifeBlock0;
                    /// <summary>
                    /// ShelfLifeBlock0
                    /// </summary>
                    public ShelfLifeBlock0 ShelfLifeBlock0 { get { return _SetShelLifeBlock0; } }
                    private ShelfLifeBlock1 _SetShelLifeBlock1;
                    /// <summary>
                    /// ShelfLifeBlock1
                    /// </summary>
                    public ShelfLifeBlock1 ShelfLifeBlock1 { get { return _SetShelLifeBlock1; } }
                    private LimitCounter _LimitCount;
                    /// <summary>
                    /// Number of excursions beyond set limits
                    /// </summary>
                    public LimitCounter LimitCount { get { return _LimitCount; } }
                    private SystemStatus _SystemStat;
                    /// <summary>
                    /// Logging system status
                    /// </summary>
                    public SystemStatus SystemStat { get { return _SystemStat; } }
                    private StatusFlags _StatFlags;
                    /// <summary>
                    /// Logging status flags
                    /// </summary>
                    public StatusFlags StatFlags { get { return _StatFlags; } }

                    /// <summary>
                    /// Human-readable representation
                    /// </summary>
                    /// <returns>Human-readable representation</returns>
                    public override string ToString()
                    {
                        return String.Format(
                            "LimitCount({0}) SystemStat({1}) StatFlags({2})",
                            LimitCount.ToString(),
                            SystemStat.ToString(),
                            StatFlags.ToString());
                    }
                }

                /// <summary>
                /// IDS SL900A Get Sensor Value reply value
                /// </summary>
                public class SensorReading
                {
                    // Raw 16-bit response from GetSensorValue command
                    UInt16 _reply;

                    /// <summary>
                    /// Create Get Sensor Value reply object
                    /// </summary>
                    /// <param name="reply">Raw 16-bit Get Sensor Value response</param>
                    public SensorReading(UInt16 reply)
                    {
                        _reply = reply;
                    }

                    /// <summary>
                    /// Create Get Sensor Value reply object
                    /// </summary>
                    /// <param name="reply">Raw 16-bit Get Sensor Value response (as 2-byte, big-endian array)</param>
                    public SensorReading(byte[] reply)
                    {
                        if (2 != reply.Length)
                        {
                            throw new ArgumentException(String.Format(
                                "Sensor Reading value must be exactly 2 bytes long (got {0} bytes)",
                                reply.Length));
                        }
                        _reply = ByteConv.ToU16(reply, 0);
                    }

                    /// <summary>
                    /// Raw sensor reply
                    /// </summary>
                    public UInt16 Raw { get { return _reply; } }

                    /// <summary>
                    /// Did A/D conversion error occur?
                    /// </summary>
                    public bool ADError { get { return ((_reply >> 15) & 0x1) != 0; } }

                    /// <summary>
                    /// 5-bit Range/Limit value
                    /// </summary>
                    public byte RangeLimit { get { return (byte)((_reply >> 10) & 0x1F); } }

                    /// <summary>
                    /// 10-bit Sensor value
                    /// </summary>
                    public UInt16 Value { get { return (UInt16)((_reply >> 0 ) & 0x3FF); } }
                }

                /// <summary>
                /// IDS SL900A Get Battery Level reply value
                /// </summary>
                public class BatteryLevelReading
                {
                    // Raw 16-bit response from GetBattery Level command
                    UInt16 _reply;

                    /// <summary>
                    /// Create Get Battery Level reply object
                    /// </summary>
                    /// <param name="reply">Raw 16-bit Get Battery Level response</param>
                    public BatteryLevelReading(UInt16 reply)
                    {
                        _reply = reply;
                    }

                    /// <summary>
                    /// Create Get Battery Level reply object
                    /// </summary>
                    /// <param name="reply">Raw 16-bit Get Battery Level response (as 2-byte, big-endian array)</param>
                    public BatteryLevelReading(byte[] reply)
                    {
                        if (2 != reply.Length)
                        {
                            throw new ArgumentException(String.Format(
                                "Battery Level Reading value must be exactly 2 bytes long (got {0} bytes)",
                                reply.Length));
                        }
                        _reply = ByteConv.ToU16(reply, 0);
                    }

                    /// <summary>
                    /// Raw Battery level reply
                    /// </summary>
                    public UInt16 Raw { get { return _reply; } }

                    /// <summary>
                    /// Did A/D conversion error occur?
                    /// </summary>
                    public bool ADError { get { return ((_reply >> 15) & 0x1) != 0; } }

                    /// <summary>
                    /// 1-bit Battery Type
                    /// </summary>
                    public byte BatteryType { get { return (byte)((_reply >> 14) & 0x1); } }

                    /// <summary>
                    /// 10-bit Battery Level value
                    /// </summary>
                    public UInt16 Value { get { return (UInt16)((_reply >> 0) & 0x3FF); } }                 
                }

                /// <summary>
                /// Log Status Flags
                /// </summary>
                public class StatusFlags
                {
                    byte Raw;

                    /// <summary>
                    /// Crate StatusFlags object from raw 1-byte reply
                    /// </summary>
                    /// <param name="reply"></param>
                    public StatusFlags(byte reply)
                    {
                        Raw = reply;
                    }

                    /// <summary>
                    /// Logging active?
                    /// </summary>
                    public bool Active { get { return 0 != ((Raw >> 7) & 1); } }
                    /// <summary>
                    /// Measurement area full?
                    /// </summary>
                    public bool Full { get { return 0 != ((Raw >> 6) & 1); } }
                    /// <summary>
                    /// Measurement overwritten?
                    /// </summary>
                    public bool Overwritten { get { return 0 != ((Raw >> 5) & 1); } }
                    /// <summary>
                    /// A/D error occurred?
                    /// </summary>
                    public bool ADError { get { return 0 != ((Raw >> 4) & 1); } }
                    /// <summary>
                    /// Low battery?
                    /// </summary>
                    public bool LowBattery { get { return 0 != ((Raw >> 3) & 1); } }
                    /// <summary>
                    /// Shelf life low error?
                    /// </summary>
                    public bool ShelfLifeLow { get { return 0 != ((Raw >> 2) & 1); } }
                    /// <summary>
                    /// Shelf life high error?
                    /// </summary>
                    public bool ShelfLifeHigh { get { return 0 != ((Raw >> 1) & 1); } }
                    /// <summary>
                    /// Shelf life expired?
                    /// </summary>
                    public bool ShelfLifeExpired { get { return 0 != ((Raw >> 0) & 1); } }

                    /// <summary>
                    /// Human-readable representation
                    /// </summary>
                    /// <returns>Human-readable representation</returns>
                    public override string ToString()
                    {
                        return String.Join(" ", new string[] {
                            "Active=" + Active.ToString(),
                            "Full=" + Full.ToString(),
                            "Overwritten=" + Overwritten.ToString(),
                            "ADError=" + ADError.ToString(),
                            "LowBattery=" + LowBattery.ToString(),
                            "ShelfLifeLow=" + ShelfLifeLow.ToString(),
                            "ShelfLifeHigh=" + ShelfLifeHigh.ToString(),
                            "ShelfLifeExpired=" + ShelfLifeExpired.ToString(),
                        });
                    }
                }

                /// <summary>
                /// Logging System Status
                /// </summary>
                public class SystemStatus
                {
                    UInt32 Raw;
                    /// <summary>
                    /// Create SystemStatus reply object
                    /// </summary>
                    /// <param name="reply">Raw response containing 4-byte System Status string</param>
                    /// <param name="offset">Index of byte where System Status string starts</param>
                    public SystemStatus(byte[] reply, int offset)
                    {
                        Raw = ByteConv.ToU32(reply, offset);
                    }
                    /// <summary>
                    /// Measurement Address Pointer
                    /// </summary>
                    public UInt16 MeasurementAddressPointer { get { return (UInt16)((Raw >> 22) & 0x1FF); } }
                    /// <summary>
                    /// Number of memory replacements
                    /// </summary>
                    public byte NumMemReplacements { get { return (byte)((Raw >> 16) & 0x3F); } }
                    /// <summary>
                    /// Number of measurements
                    /// </summary>
                    public UInt16 NumMeasurements { get { return (UInt16)((Raw >> 1) & 0x7FFF); } }
                    /// <summary>
                    /// Active
                    /// </summary>
                    public bool Active { get { return 0 != (Raw & 0x1); } }

                    /// <summary>
                    /// Human-readable representation
                    /// </summary>
                    /// <returns>Human-readable representation</returns>
                    public override string ToString()
                    {
                        return String.Join(" ", new string[] {
                            "MeasurementAddressPointer="+MeasurementAddressPointer.ToString(),
                            "NumMemReplacements=" + NumMemReplacements.ToString(),
                            "NumMeasurements=" + NumMeasurements.ToString(),
                            "Active=" + Active,
                        });
                    }
                }

                /// <summary>
                /// Measurement Setup Data object
                ///(as received from Get Measurement Setup Data command)
                /// </summary>
                public class MeasurementSetupData
                {
                    // Raw 16 byte response from Measurement Setup Data command
                    byte[] Raw;
                    /// <summary>
                    /// Log Limits Data
                    /// </summary>
                    public LogLimit logLimits;
                    /// <summary>
                    /// Log Mode Data
                    /// </summary>
                    public LogModeData logModeData;
                    /// <summary>
                    /// Delay Data
                    /// </summary>
                    public Delay DelayData;
                    /// <summary>
                    /// Application Data
                    /// </summary>
                    public ApplicationData AppData;
                    /// <summary>
                    /// Time (seconds) between log readings
                    /// </summary>
                    UInt16 LogInterval;
                    /// <summary>
                    /// Start Time
                    /// </summary>
                    DateTime StartTime;

                    /// <summary>
                    /// Create Measurement Setup Data object from raw 16 byte reply
                    /// </summary>
                    /// <param name="reply"></param>
                    public MeasurementSetupData(byte[] reply)
                    {
                        Raw = reply;
                    }

                    /// <summary>
                    /// Create Measurement Setup Data object from raw 16 byte reply
                    /// </summary>
                    /// <param name="reply"></param>
                    /// <param name="offset"></param>
                    public MeasurementSetupData(byte[] reply, int offset)
                    {
                        if (16 != reply.Length)
                        {
                            throw new ArgumentException(String.Format(
                                "MeasurementSetupData value must be exactly 16 byte long (got {0} bytes)",
                                reply.Length));
                        }

                        //Start Time
                        StartTime = SerialReader.FromSL900aTime(ByteConv.ToU32(reply, offset));
                        offset += 4;

                        //Log Limits
                        logLimits = new LogLimit(reply, offset);
                        offset += 5;

                        //Log Mode
                        logModeData = new LogModeData(reply[offset]);
                        offset += 1;

                        //Log Interval
                        LogInterval = (UInt16)(((ByteConv.ToU16(reply, offset)) >> 1) & 0x0001);
                        offset += 2;

                        //Delay time
                        DelayData = new Delay(reply, offset);
                        offset += 2;

                        //Application Data
                        AppData = new ApplicationData(reply, offset);

                        Raw = reply;
                    }

                    /// <summary>
                    /// Human-readable representation
                    /// </summary>
                    /// <returns>Human-readable representation</returns>
                    public override string ToString()
                    {
                        return String.Join(" ", new string[] {
                            "StartTime=("+StartTime+")",
                            "LogLimits=("+logLimits+")",
                            "LogMode=("+logModeData+")",
                            "LogInterval=("+LogInterval+")",
                            "DelayTime=("+DelayData.ToString()+")",
                            "ApplicationData=("+AppData.ToString()+")",
                        });
                    }
                }

                /// <summary>
                /// Log limit parameters
                /// </summary>
                public class LogLimit
                {
                    /// <summary>
                    /// Raw 64 bit data
                    /// </summary>
                    public UInt64 Raw;

                    /// <summary>
                    /// Create LogLimit reply object
                    /// </summary>
                    public LogLimit() { }
                    
                    /// <summary>
                    /// Create LogLimit reply object
                    /// </summary>
                    /// <param name="reply">Raw reply message</param>
                    /// <param name="offset">Start index of raw value within reply</param>
                    public LogLimit(byte[] reply, int offset)
                    {
                        // ToU64 requires 8 bytes of input, but Log limit data is only 5 bytes long.
                        // Create a temporary array to provide the necessary padding.
                        byte[] data = new byte[] { 0, 0, 0, 0, 0, 0, 0, 0 };
                        Array.Copy(reply, offset, data, 3, 5);
                        Raw = ByteConv.ToU64 (data, 0);
                        Raw &= 0x000000FFFFFFFFFF;
                    }

                    /// <summary>
                    /// Number of times selected sensor has gone beyond extreme lower limit
                    /// </summary>
                    public ushort EXTREMELOWERLIMIT
                    {
                        get {return (ushort) ByteConv.GetBits(Raw, 30, 10);}
                        set {ByteConv.SetBits(ref Raw, 30, 10, value, "ExtremeLowerLimit");}
                    }

                    /// <summary>
                    /// Number of times selected sensor has gone beyond lower limit
                    /// </summary>
                    public ushort LOWERLIMIT
                    {
                        get {return (ushort) ByteConv.GetBits(Raw, 20, 10);}
                        set {ByteConv.SetBits(ref Raw, 20, 10, value, "LowerLimit");}
                    }

                    /// <summary>
                    /// Number of times selected sensor has gone beyond upper limit
                    /// </summary>
                    public ushort UPPERLIMIT
                    {
                        get {return (ushort) ByteConv.GetBits(Raw, 10, 10);}
                        set {ByteConv.SetBits(ref Raw, 10, 10, value, "UpperLimit");}
                    }

                    /// <summary>
                    /// Number of times selected sensor has gone beyond extreme upper limit
                    /// </summary>
                    public ushort EXTREMEUPPERLIMIT
                    {
                        get {return (ushort) ByteConv.GetBits(Raw, 0, 10);}
                        set {ByteConv.SetBits(ref Raw, 0, 10, value, "ExtremeUpperLimit");}
                    }

                    /// <summary>
                    /// Human-readable representation
                    /// </summary>
                    /// <returns>Human-readable representation</returns>
                    public override string ToString()
                    {
                        return String.Join(" ", new string[] {
                            "ExtremeLower="+EXTREMELOWERLIMIT.ToString("X"),
                            "Lower="+LOWERLIMIT.ToString("X"),
                            "Upper="+UPPERLIMIT.ToString("X"),
                            "ExtremeUpper="+EXTREMEUPPERLIMIT.ToString("X"),
                        });
                    }
                }

                #endregion Data Structures

                #region Tagops
                #region SL900AGetSensorValue
                /// <summary>
                /// SL900A Get Sensor Value Tagop
                /// </summary>
                public class GetSensorValue : SL900A
                {
                    #region Fields

                    /// <summary>
                    /// Which sensor to read
                    /// </summary>
                    public Sensor SensorType;

                    #endregion
                    #region Construction
                    /// <summary>
                    /// Constructor to initialize the parameters of GetSensorValue
                    /// </summary>
                    /// <param name="sensorType">Which sensor to read</param>
                    public GetSensorValue(Sensor sensorType) : base(0xAD)
                    {
                        this.SensorType = sensorType;
                    }
                    /// <summary>
                    /// Constructor to initialize the parameters of GetSensorValue
                    /// </summary>
                    /// <param name="sensorType">Which sensor to read</param>
                    /// <param name="passwordLevel">password level</param>
                    /// <param name="password">password</param>
                    public GetSensorValue(Sensor sensorType, Level passwordLevel, UInt32 password)
                        : base(0xAD, passwordLevel, password)
                    {
                        this.SensorType = sensorType;
                    }
                    #endregion
                }
                #endregion SL900AGetSensorValue
                #region SL900AAccessFIFO
                /// <summary>
                /// SL900A Access FIFO Tagop
                /// </summary>
                public abstract class AccessFifo : SL900A
                {
                    #region Enums
                    /// <summary>
                    /// AccessFifo subcommand values
                    /// </summary>
                    public enum SubcommandCode
                    {
                        /// <summary>
                        /// Read from FIFO
                        /// </summary>
                        READ = 0x80,
                        /// <summary>
                        /// Write to FIFO
                        /// </summary>
                        WRITE = 0xA0,
                        /// <summary>
                        /// Get FIFO status
                        /// </summary>
                        STATUS = 0xC0,
                    }
                    #endregion Enums
                    #region Fields
                    /// <summary>
                    /// AccessFifo subcommand code
                    /// </summary>
                    public SubcommandCode Subcommand;
                    #endregion Fields
                    #region Construction
                    /// <summary>
                    /// Create AccessFifo tagop
                    /// </summary>
                    public AccessFifo() : base(0xAF) {}
                    /// <summary>
                    /// Create AccessFifo tagop
                    /// </summary>
                    /// <param name="passwordLevel">password level</param>
                    /// <param name="password">password</param>
                    public AccessFifo(Level passwordLevel, UInt32 password) : base(0xAF, passwordLevel, password) { }
                    #endregion Construction
                }

                /// <summary>
                /// AccessFifo "Read Status" tagop
                /// </summary>
                public class AccessFifoStatus : AccessFifo
                {
                    /// <summary>
                    /// Create AccessFifo "Read Status" tagop
                    /// </summary>
                    public AccessFifoStatus()
                    {
                        Subcommand = SubcommandCode.STATUS;
                    }
                    
                    /// <summary>
                    /// Create AccessFifo "Read Status" tagop
                    /// </summary>
                    /// <param name="passwordLevel">password level</param>
                    /// <param name="password">password</param>
                    public AccessFifoStatus(Level passwordLevel, UInt32 password) : base(passwordLevel, password)
                    {
                        Subcommand = SubcommandCode.STATUS;
                }
                }

                /// <summary>
                /// AccessFifo "Read" tagop
                /// </summary>
                public class AccessFifoRead : AccessFifo
                {
                    /// <summary>
                    /// Number of bytes to read from FIFO
                    /// </summary>
                    public byte Length;
                    /// <summary>
                    /// Create AccessFifo "Read" tagop
                    /// </summary>
                    /// <param name="length">Number of bytes to read from FIFO</param>
                    public AccessFifoRead(byte length)
                    {
                        Subcommand = SubcommandCode.READ;

                        if (length != (length & 0xF))
                        {
                            throw new ArgumentOutOfRangeException("Invalid AccessFifo read length: " + length);
                        }
                        Length = length;
                    }
                    /// <summary>
                    /// Create AccessFifo "Read" tagop
                    /// </summary>
                    /// <param name="length">Number of bytes to read from FIFO</param>
                    /// <param name="passwordLevel">password level</param>
                    /// <param name="password">password</param>
                    public AccessFifoRead(byte length, Level passwordLevel, UInt32 password) : base(passwordLevel, password)
                    {
                        Subcommand = SubcommandCode.READ;

                        if (length != (length & 0xF))
                        {
                            throw new ArgumentOutOfRangeException("Invalid AccessFifo read length: " + length);
                }
                        Length = length;
                    }
                }

                /// <summary>
                /// Source of FIFO data
                /// </summary>
                public enum FifoSource
                {
                    /// <summary>
                    /// Data from SPI
                    /// </summary>
                    SPI = 0,
                    /// <summary>
                    /// Data from RFID
                    /// </summary>
                    RFID = 1,
                }

                /// <summary>
                /// AccessFifo "Write" tagop
                /// </summary>
                public class AccessFifoWrite : AccessFifo
                {
                    /// <summary>
                    /// Bytes to write to FIFO
                    /// </summary>
                    public byte[] Payload;
                    /// <summary>
                    /// Create AccessFifo "Write" tagop
                    /// </summary>
                    /// <param name="payload">Bytes to write to FIFO</param>
                    public AccessFifoWrite(byte[] payload)
                    {
                        Subcommand = SubcommandCode.WRITE;

                        if (payload.Length != (payload.Length & 0xF))
                        {
                            throw new ArgumentOutOfRangeException("Invalid AccessFifo write length: " + payload.Length);
                        }
                        Payload = payload;
                    }
                    /// <summary>
                    /// Create AccessFifo "Write" tagop
                    /// </summary>
                    /// <param name="payload">Bytes to write to FIFO</param>
                    /// <param name="passwordLevel">password level</param>
                    /// <param name="password">password</param>
                    public AccessFifoWrite(byte[] payload, Level passwordLevel, UInt32 password) : base(passwordLevel, password)
                    {
                        Subcommand = SubcommandCode.WRITE;

                        if (payload.Length != (payload.Length & 0xF))
                        {
                            throw new ArgumentOutOfRangeException("Invalid AccessFifo write length: " + payload.Length);
                }
                        Payload = payload;
                    }
                }

                /// <summary>
                /// FIFO Status return value
                /// </summary>
                public class FifoStatus
                {
                    // Raw 8-bit response from AccessFifo Status command
                    byte _reply;

                    /// <summary>
                    /// Create FifoStatus object from AccessFifo Status return value
                    /// </summary>
                    /// <param name="reply">8-bit reply from AccessFifo "Read Status" command</param>
                    public FifoStatus(byte reply)
                    {
                        _reply = reply;
                    }
                    /// <summary>
                    /// Create FIFO Status reply object
                    /// </summary>
                    /// <param name="reply">Raw 8-bit FIFO Status response (in byte array)</param>
                    public FifoStatus(byte[] reply)
                    {
                        if (1 != reply.Length)
                        {
                            throw new ArgumentException(String.Format(
                                "Fifo Status value must be exactly 1 byte long (got {0} bytes)",
                                reply.Length));
                        }
                        _reply = reply[0];
                    }

                    /// <summary>
                    /// Raw 8-bit response from AccessFifo Status command
                    /// </summary>
                    public byte Raw { get { return _reply; } }
                    /// <summary>
                    /// FIFO Busy bit
                    /// </summary>
                    public bool FifoBusy
                    {
                        get { return 0 != ((_reply >> 7) & 1); }
                    }
                    /// <summary>
                    /// Data Ready bit
                    /// </summary>
                    public bool DataReady
                    {
                        get { return 0 != ((_reply >> 6) & 1); }
                    }
                    /// <summary>
                    /// No Data bit
                    /// </summary>
                    public bool NoData
                    {
                        get { return 0 != ((_reply >> 5) & 1); }
                    }
                    /// <summary>
                    /// Data Source bit (SPI, RFID)
                    /// </summary>
                    public FifoSource Source
                    {
                        get { return (FifoSource)((_reply >> 4) & 1); }
                    }
                    /// <summary>
                    /// Number of valid bytes in FIFO register
                    /// </summary>
                    public byte NumValidBytes
                    {
                        get { return (byte)(_reply & 0xF); }
                    }

                    /// <summary>
                    /// Returns a string that represents the current object.
                    /// </summary>
                    /// <returns>A string that represents the current object.</returns>
                    public override string ToString()
                    {
                        return String.Join(" ", new string[] {
                            "Raw="+this.Raw.ToString("X4"),
                            "Busy="+this.FifoBusy,
                            "Ready="+this.DataReady,
                            "NoData="+this.NoData,
                            "Source="+this.Source,
                            "#bytes="+this.NumValidBytes,
                        });
                    }
                }

                #endregion SL900AAccessFIFO
                #region SL900ASetLogMode
                /// <summary>
                /// SL900A Set Log Mode Tagop
                /// </summary>
                public class SetLogMode : SL900A
                {
                    /// <summary>
                    /// Create Set Log Mode Tagop
                    /// </summary>
                    public SetLogMode() : base(0xA1) {}
                    /// <summary>
                    /// Create Set Log Mode Tagop
                    /// </summary>
                    /// <param name="passwordLevel">Password level</param> 
                    /// <param name="password">Password</param>
                    public SetLogMode( Level passwordLevel, UInt32 password) : base(0xA1, passwordLevel, password) { }
                    /// <summary>
                    /// Logging Format
                    /// </summary>
                    public LoggingForm Form = LoggingForm.DENSE;
                    /// <summary>
                    /// Log Memory-Full Behavior
                    /// </summary>
                    public StorageRule Storage = StorageRule.NORMAL;
                    /// <summary>
                    /// Enable log for EXT1 external sensor
                    /// </summary>
                    public bool Ext1Enable = false;
                    /// <summary>
                    /// Enable log for EXT2 external sensor
                    /// </summary>
                    public bool Ext2Enable = false;
                    /// <summary>
                    /// Enable log for temperature sensor
                    /// </summary>
                    public bool TempEnable = false;
                    /// <summary>
                    /// Enable log for battery sensor
                    /// </summary>
                    public bool BattEnable = false;
                    UInt16 _LogInterval = 1;
                    /// <summary>
                    /// Time (seconds) between log readings
                    /// </summary>
                    public UInt16 LogInterval
                    {
                        get { return _LogInterval; }
                        set
                        {
                            if ((value & 0x7FFF) != value)
                            {
                                throw new ArgumentOutOfRangeException("Log interval must fit in 15 bits");
                            }
                            _LogInterval = value;
                        }
                    }
                }
                #endregion SL900ASetLogMode
                #region SL900AInitialize
                /// <summary>
                /// SL900A Initialize Tagop
                /// </summary>
                public class Initialize : SL900A
                {
                    /// <summary>
                    ///  Create Initialize tagop
                    /// </summary>
                    public Initialize()
                        : base(0xAC)
                    {
                        DelayTime = new Delay();
                        AppData = new ApplicationData();
                    }

                    /// <summary>
                    ///  Create Initialize tagop
                    /// </summary>
                    /// <param name="passwordLevel">Password level</param> 
                    /// <param name="password">Password</param>
                    public Initialize(Level passwordLevel, UInt32 password)
                        : base(0xAC, passwordLevel, password)
                    {
                        DelayTime = new Delay();
                        AppData = new ApplicationData();
                    }

                    /// <summary>
                    /// Log start delay settings
                    /// </summary>
                    public Delay DelayTime;
                    /// <summary>
                    /// Log memory configuration
                    /// </summary>
                    public ApplicationData AppData;
                }
                #endregion SL900AInitialize
                #region SL900AStartLog
                /// <summary>
                /// SL900A Start Log tagop
                /// </summary>
                public class StartLog : SL900A
                {
                    /// <summary>
                    /// Create Start Log tagop
                    /// </summary>
                    /// <param name="startTime">Starting time</param>
                    public StartLog(DateTime startTime) : base(0xA7)
                    {
                        StartTime = startTime;
                    }

                    /// <summary>
                    /// Create Start Log tagop
                    /// </summary>
                    /// <param name="startTime">Starting time</param>
                    /// <param name="passwordLevel">Password level</param>
                    /// <param name="password">Password</param>                    
                    public StartLog(DateTime startTime, Level passwordLevel, UInt32 password)
                        : base(0xA7, passwordLevel, password)
                    {
                        StartTime = startTime;
                    }

                    /// <summary>
                    /// Time to initialize log timestamp counter with
                    /// </summary>
                    public DateTime StartTime;
                }
                #endregion SL900AStartLog
                #region SL900AEndLog
                /// <summary>
                /// SL900A End Log tagop
                /// </summary>
                public class EndLog : SL900A
                {
                    /// <summary>
                    /// Create EndLog tagop
                    /// </summary>
                    public EndLog() : base(0xA6) {}
                    /// <summary>
                    /// Create EndLog tagop
                    /// </summary>
                    /// <param name="passwordLevel">Password level</param>
                    /// <param name="password">Password</param>
                    public EndLog(Level passwordLevel, UInt32 password) : base(0xA6, passwordLevel, password) { }
                }
                #endregion SL900AEndLog
                #region SL900AGetLogState
                /// <summary>
                /// SL900A Get Log State tagop
                /// </summary>
                public class GetLogState : SL900A
                {
                    /// <summary>
                    /// Create GetLogState tagop
                    /// </summary>
                    public GetLogState() : base(0xA8) { }
                    /// <summary>
                    /// Create GetLogState tagop
                    /// </summary>
                    /// <param name="passwordLevel">Password level</param>
                    /// <param name="password">Password</param>
                    public GetLogState(Level passwordLevel, UInt32 password) : base(0xA8, passwordLevel, password) { }
                }
                #endregion SL900AGetLogState
                #region SL900AGetCalibrationData
                /// <summary>
                /// SL900A Get Calibration Data tagop
                /// </summary>
                public class GetCalibrationData : SL900A
                {
                    /// <summary>
                    /// Create GetCalibrationData tagop
                    /// </summary>
                    public GetCalibrationData() : base(0xA9) { }

                    /// <summary>
                    /// Create GetCalibrationData tagop
                    /// </summary>
                    /// <param name="passwordLevel">Password level</param>
                    /// <param name="password">Password</param>
                    public GetCalibrationData(Level passwordLevel, UInt32 password) : base(0xA9, passwordLevel, password) { }
                }
                #endregion SL900AGetCalibrationData
                #region SL900ASetCalibrationData
                /// <summary>
                /// SL900A Set Calibration Data tagop
                /// </summary>
                public class SetCalibrationData : SL900A
                {
                    /// <summary>
                    /// Calibration Data
                    /// </summary>
                    public CalibrationData Cal;

                    /// <summary>
                    /// Create SetCalibrationData tagop
                    /// </summary>
                    /// <param name="cal">Calibration data acquired from Get Calibration Data.
                    /// Must read data from tag first to avoid changing "DO NOT MODIFY" fields.</param>
                    public SetCalibrationData(CalibrationData cal)
                        : base(0xA5)
                    {
                        Cal = cal;
                    }

                    /// <summary>
                    /// Create SetCalibrationData tagop
                    /// </summary>
                    /// <param name="cal">Calibration data acquired from Get Calibration Data.
                    /// Must read data from tag first to avoid changing "DO NOT MODIFY" fields.</param>
                    /// <param name="passwordLevel">Password level</param>
                    /// <param name="password">Password</param>
                    public SetCalibrationData(CalibrationData cal, Level passwordLevel, UInt32 password)
                        : base(0xA5, passwordLevel, password)
                    {
                        Cal = cal;
                }
                }
                #endregion SL900ASetCalibrationData
                #region SL900ASetSfeParameters
                /// <summary>
                /// SL900A Set SFE Parameters tagop
                /// </summary>
                public class SetSfeParameters : SL900A
                {
                    /// <summary>
                    /// Calibration Data
                    /// </summary>
                    public SfeParameters Sfe;

                    /// <summary>
                    /// Create SetSfeParameters tagop
                    /// </summary>
                    /// <param name="sfe">SFE parameters acquired from Get Calibration Data.
                    /// Must read data from tag first to avoid changing "DO NOT MODIFY" fields.</param>
                    public SetSfeParameters(SfeParameters sfe)
                        : base(0xA4)
                    {
                        Sfe = sfe;
                    }

                    /// <summary>
                    /// Create SetSfeParameters tagop
                    /// </summary>
                    /// <param name="sfe">SFE parameters acquired from Get Calibration Data.
                    /// Must read data from tag first to avoid changing "DO NOT MODIFY" fields.</param>
                    /// <param name="passwordLevel">Password level</param>
                    /// <param name="password">Password</param>
                    public SetSfeParameters(SfeParameters sfe, Level passwordLevel, UInt32 password)
                        : base(0xA4, passwordLevel, password)
                    {
                        Sfe = sfe;
                }
                }
                #endregion SL900ASetSfeParameters
                #region SL900AGetBatteryLevel
                /// <summary>
                /// SL900A Get Battery Level Tagop
                /// </summary>
                public class GetBatteryLevel : SL900A
                {
                    #region Fields

                    /// <summary>
                    /// enum BatterType, re-check or default
                    /// </summary>
                    public BatteryType batteryType;

                    #endregion
                    #region Construction
                    /// <summary>
                    /// Constructor to initialize the parameters of GetBatteryLevel
                    /// </summary>
                    /// <param name="batteryType">enum</param>
                    public GetBatteryLevel(BatteryType batteryType)
                        : base(0xAA)
                    {
                        this.batteryType = batteryType;
                    }

                    /// <summary>
                    /// Constructor to initialize the parameters of GetBatteryLevel
                    /// </summary>
                    /// <param name="batteryType">enum</param>
                    /// <param name="passwordLevel">Password level</param>
                    /// <param name="password">Password</param>
                    public GetBatteryLevel(BatteryType batteryType, Level passwordLevel, UInt32 password)
                        : base(0xAA, passwordLevel, password)
                    {
                        this.batteryType = batteryType;
                    }
                    #endregion
                }
                #endregion SL900AGetBatteryLevel
                #region SL900AGetMeasurementSetup
                /// <summary>
                /// SL900A Get Measurement Setup Tagop
                /// </summary>
                public class GetMeasurementSetup : SL900A
                {
                    /// <summary>
                    /// Create GetMeasurementSetup tagop
                    /// </summary>
                    public GetMeasurementSetup() : base(0xA3) { }
                    /// <summary>
                    /// Create GetMeasurementSetup tagop
                    /// </summary>
                    /// <param name="passwordLevel">Password level</param>
                    /// <param name="password">Password</param>
                    public GetMeasurementSetup(Level passwordLevel, UInt32 password) : base(0xA3, passwordLevel, password) { }
                }
                #endregion SL900AGetMeasurementSetup
                #region SL900ALogModeData
                /// <summary>
                /// SL900A  Log Mode Data
                /// </summary>
                public class LogModeData 
                {
                    // Raw 8-bit response from Measurement Setup Data command
                    byte _reply;

                    /// <summary>
                    /// Create LogModeData object from Measurement Setup Data return value
                    /// </summary>
                    /// <param name="reply">8-bit reply from Measurement Setup Data command</param>
                    public LogModeData(byte reply)
                    {
                        _reply = reply;
                    }
                    /// <summary>
                    /// Create LogModeData reply object
                    /// </summary>
                    /// <param name="reply">Raw 8-bit LogModeData response (in byte array)</param>
                    public LogModeData(byte[] reply)
                    {
                        if (1 != reply.Length)
                        {
                            throw new ArgumentException(String.Format(
                                "LogModeData value must be exactly 1 byte long (got {0} bytes)",
                                reply.Length));
                        }
                        _reply = reply[0];
                    }

                    /// <summary>
                    /// Raw 8-bit response from Measurement Setup Data command
                    /// </summary>
                    public byte Raw { get { return _reply; } }


                    /// <summary>
                    /// Logging Format
                    /// </summary>
                    public LoggingForm Form 
                    {
                        get { return (LoggingForm)((_reply >> 5) & 7); }
                    }
                    
                    /// <summary>
                    /// Log Memory-Full Behavior
                    /// </summary>
                    public StorageRule Storage {
                        get { return (StorageRule)(((_reply >> 4) & 1)); }
                    }
                    
                    /// <summary>
                    /// Enable log for EXT1 external sensor
                    /// </summary>
                    public bool Ext1Enable {
                        get { return 0 != ((_reply >> 3) & 1); }
                    }
                    
                    /// <summary>
                    /// Enable log for EXT2 external sensor
                    /// </summary>
                    public bool Ext2Enable {
                        get { return 0 != ((_reply >> 2) & 1); }
                    }
                    
                    /// <summary>
                    /// Enable log for temperature sensor
                    /// </summary>
                    public bool TempEnable {
                        get { return 0 != ((_reply >> 1) & 1); }
                    }
                    
                    /// <summary>
                    /// Enable log for battery sensor
                    /// </summary>
                    public bool BattEnable {
                        get { return 0 != ((_reply >> 0) & 1); }
                    }

                    /// <summary>
                    /// Returns a string that represents the current object.
                    /// </summary>
                    /// <returns>A string that represents the current object.</returns>
                    public override string ToString()
                    {
                        return String.Join(" ", new string[] {
                            "Raw="+this.Raw.ToString("X4"),
                            "LoggingForm="+this.Form,
                            "StorageRule="+this.Storage,
                            "EXT1="+this.Ext1Enable,
                            "EXT2="+this.Ext2Enable,
                            "Temperature Sensor="+this.TempEnable,
                            "Battery Sensor="+this.BattEnable,
                        });
                    }
                }
                #endregion SL900ALogModeData
                #region SL900ASetPassword
                /// <summary>
                /// SL900A Set Password Tagop
                /// </summary>
                public class SetPassword : SL900A
                {
                    #region Fields
                    /// <summary>
                    /// IDS SL900A Password
                    /// </summary>
                    public UInt32 newPassword;

                    /// <summary>
                    /// Access level to which SL900A password applies
                    /// </summary>
                    public Level newPasswordLevel;
                    #endregion Fields

                    #region Constructor
                    /// <summary>
                    ///  Create Set password
                    /// </summary>
                    /// <param name="passwordLevel">New password level to be written on the tag</param>
                    /// <param name="password">New password to be written on the tag</param>
                    public SetPassword(Level passwordLevel, UInt32 password)
                        : base(0xA0)
                    {
                        newPassword = password;
                        newPasswordLevel = passwordLevel;
                    }
                    /// <summary>
                    ///  Create Set password
                    /// </summary>
                    /// <param name="currentPasswordLevel">Current password level on the tag</param>
                    /// <param name="currentPassword">Current password on the tag</param>
                    /// <param name="passwordLevel">New password level to be written on the tag</param>
                    /// <param name="password">New password to be written on the tag</param>
                    public SetPassword(Level currentPasswordLevel, UInt32 currentPassword, Level passwordLevel, UInt32 password)
                        : base(0xA0, currentPasswordLevel, currentPassword)
                    {
                        newPassword = password;
                        newPasswordLevel = passwordLevel;
                    }
                    #endregion Constructor
                }
                #endregion SL900ASetPassword
                #region SL900ASetLogLimit
                /// <summary>
                /// SL900A Set log limit tagop
                /// </summary>
                public class SetLogLimit : SL900A
                {
                    /// <summary>
                    /// Log limits data
                    /// </summary>
                    public LogLimit LogLimits;
                    /// <summary>
                    /// Create SetLogLimit tagop
                    /// </summary>
                    /// /// <param name="logLimits">Log limits data</param>
                    public SetLogLimit(LogLimit logLimits)
                        : base(0xA2)
                    {
                        LogLimits = logLimits;
                    }
                    /// <summary>
                    /// Create SetLogLimit tagop
                    /// </summary>
                    /// <param name="logLimits">Log limits data</param>
                    /// <param name="passwordLevel">Password level</param>
                    /// <param name="password">Password</param>
                    public SetLogLimit(LogLimit logLimits, Level passwordLevel, UInt32 password) : base(0xA2, passwordLevel, password)
                    {
                        LogLimits = logLimits;
                    }
                }
                #endregion SL900ASetLogLimit
                #region ShelfLifeBlock0
                /// <summary>
                /// ShelfLifeBlock0 values are intended as reference information purpose for the interrogator.
                /// </summary>
                public class ShelfLifeBlock0
                {
                    /// <summary>
                    /// 
                    /// </summary>
                    public UInt32 Raw;
                    /// <summary>
                    /// Default Constructor
                    /// </summary>
                    public ShelfLifeBlock0()
                    {
                    }
                    /// <summary>
                    /// Create ShelfLifeBlock0 reply object
                    /// </summary>
                    /// <param name="value">Raw response containing 4-byte ShelfLifeBlock0</param>
                    /// <param name="offset">Index of byte where ShelfLifeBlock0 starts</param>
                    public ShelfLifeBlock0(byte[] value, int offset)
                    {
                        Raw = ByteConv.ToU32(value, offset);
                    }

                    /// <summary>
                    /// Maximum temperature for the product
                    /// </summary>
                    public byte TMAX
                    {
                        get { return (byte) ByteConv.GetBits(Raw, 24, 8); }
                        set { ByteConv.SetBits(ref Raw, 24, 8, value, "Tmax"); }
                    }

                    /// <summary>
                    /// Minimum temperature for the product
                    /// </summary>
                    public byte TMIN
                    {
                        get { return (byte) ByteConv.GetBits(Raw, 16, 8); }
                        set { ByteConv.SetBits(ref Raw, 16, 8, value, "Tmin"); }
                    }

                    /// <summary>
                    /// Normal temperature
                    /// </summary>
                    public byte TSTD
                    {
                        get { return (byte) ByteConv.GetBits(Raw, 8, 8); }
                        set { ByteConv.SetBits(ref Raw, 8, 8, value, "Tstd"); }
                    }

                    /// <summary>
                    /// Activation energy
                    /// </summary>
                    public byte EA
                    {
                        get { return (byte) ByteConv.GetBits(Raw, 0, 8); }
                        set { ByteConv.SetBits(ref Raw, 0, 8, value, "Ea"); }
                    }
                }
                #endregion ShelfLifeBlock0
                #region ShelfLifeBlock1
                /// <summary>
                /// ShelfLifeBlock1 holds the information on the initial shelf life and initial temperature.
                /// </summary>
                public class ShelfLifeBlock1
                {
                    /// <summary>
                    /// 
                    /// </summary>
                    public UInt32 Raw;
                    /// <summary>
                    /// Default Constructor
                    /// </summary>
                    public ShelfLifeBlock1()
                    {
                        SetBits();
                    }
                    /// <summary>
                    /// Create ShelfLifeBlock1 reply object
                    /// </summary>
                    /// <param name="value">Raw response containing 4-byte ShelfLifeBlock1</param>
                    /// <param name="offset">Index of byte where ShelfLifeBlock1 starts</param>
                    public ShelfLifeBlock1(byte[] value, int offset)
                    {
                        Raw = ByteConv.ToU32(value, offset);
                        SetBits();
                    }

                    private void SetBits()
                    {
                        ByteConv.SetBits(ref Raw, 0, 2, 0, "RFU");
                    }

                    /// <summary>
                    /// Initial shelf life
                    /// </summary>
                    public byte SLINIT
                    {
                        get { return (byte) ByteConv.GetBits(Raw, 16, 16); }
                        set { ByteConv.SetBits(ref Raw, 16, 16, value, "Sinit"); }
                    }

                    /// <summary>
                    /// Initial temperature used in shelf life calculation
                    /// </summary>
                    public byte TINIT
                    {
                        get { return (byte) ByteConv.GetBits(Raw, 6, 10); }
                        set { ByteConv.SetBits(ref Raw, 6, 10, value, "Tinit"); }
                    }

                    /// <summary>
                    /// Sensor used for shelf life calculation
                    /// </summary>
                    public byte SENSORID
                    {
                        get { return (byte)ByteConv.GetBits(Raw, 4, 2); }
                        set { ByteConv.SetBits(ref Raw, 4, 2, value, "SensorID"); }
                    }

                    /// <summary>
                    /// Enables negative value for shelf life
                    /// </summary>
                    public bool ENABLENEGATIVE
                    {
                        get { return 0 != ByteConv.GetBits(Raw, 3, 1); }
                        set { ByteConv.SetBits(ref Raw, 3, 1, (UInt16)(value ? 1 : 0), "EnableNegative"); }
                    }

                    /// <summary>
                    /// Enables shelf life algorithm
                    /// </summary>
                    public bool ENABLEALGORITHM
                    {
                        get { return 0 != ByteConv.GetBits(Raw, 2, 1); }
                        set { ByteConv.SetBits(ref Raw, 2, 1, (UInt16)(value ? 1 : 0), "EnableAlgorithm"); }
                    }

                    /// <summary>
                    /// Reserved for future use
                    /// </summary>
                    protected bool RFU
                    {
                        set { ByteConv.SetBits(ref Raw, 0, 2, 0, "RFU"); }
                    }
                }
                #endregion ShelfLifeBlock1
                #region SL900ASetShelfLife
                /// <summary>
                /// SL900A Get Battery Level Tagop
                /// </summary>
                public class SetShelfLife : SL900A
                {
                    #region Fields
                    /// <summary>
                    /// 
                    /// </summary>
                    public ShelfLifeBlock0 shelfLifeBlock0;
                    /// <summary>
                    /// 
                    /// </summary>
                    public ShelfLifeBlock1 shelfLifeBlock1;
                    #endregion
                    #region Construction
                    /// <summary>
                    /// Constructor to initialize the parameters of ShelfLife
                    /// </summary>
                    /// <param name="slBlock0">ShelfLifeBlock0</param>
                    /// <param name="slBlock1">ShelfLifeBlock1</param>
                    public SetShelfLife(ShelfLifeBlock0 slBlock0, ShelfLifeBlock1 slBlock1)
                        : base(0xAB)
                    {
                        this.shelfLifeBlock0 = slBlock0;
                        this.shelfLifeBlock1 = slBlock1;
                    }
                    /// <summary>
                    /// Constructor to initialize the parameters of ShelfLife
                    /// </summary>
                    /// <param name="slBlock0">ShelfLifeBlock0</param>
                    /// <param name="slBlock1">ShelfLifeBlock1</param>
                    /// <param name="passwordLevel">PasswordLevel</param>
                    /// <param name="password">Password</param>                    
                    public SetShelfLife(ShelfLifeBlock0 slBlock0, ShelfLifeBlock1 slBlock1, Level passwordLevel, uint password)
                        : base(0xAB, passwordLevel, password)
                    {
                        this.shelfLifeBlock0 = slBlock0;
                        this.shelfLifeBlock1 = slBlock1;
                    }
                    #endregion
                }
                #endregion SL900ASetShelfLife
                #endregion Tagops
            }
            #endregion SL900A
        }
        #endregion IDS

        #region NXP

        /// <summary>
        /// NXP Gen2 Tag Operation
        /// </summary>
        public  class NxpGen2TagOp : Gen2CustomTagOp
        {
            
            #region Fields

             /// <summary>
             /// The access password to use to write to the tag
             /// </summary>
             public UInt32 AccessPassword;

             #endregion
             
            #region Construction
             /// <summary>
             /// Default constructor to initialize the parameters
             /// </summary>
             public NxpGen2TagOp()
             {
                 this.AccessPassword = 0;
             }

             /// <summary>
             /// Constructor to initialize the parameters
             /// </summary>
             /// <param name="accessPassword">the access password to use to write to the tag</param>
             public NxpGen2TagOp(UInt32 accessPassword)
             {
                 this.AccessPassword = accessPassword;
             }
 
             #endregion

            #region SetReadProtect

            /// <summary>
        /// NxpSetReadProtect
        /// </summary>
            public abstract class SetReadProtect : NxpGen2TagOp
        {

            #region Construction
            /// <summary>
            /// Constructor to initialize the parameters of NxpSetReadProtect
            /// </summary>
            /// <param name="accessPassword">the access password to use to write to the tag</param>            
            public SetReadProtect(UInt32 accessPassword): base(accessPassword)
            {
            }
            #endregion

        }

            #endregion

            #region ResetReadProtect

            /// <summary>
        /// NxpSetReadProtect
        /// </summary>
            public abstract class ResetReadProtect : NxpGen2TagOp
        {
            #region Construction
            /// <summary>
            /// Constructor to initialize the parameters of NxpResetReadProtect
            /// </summary>
            /// <param name="accessPassword">the access password to use to write to the tag</param>          
            public ResetReadProtect(UInt32 accessPassword) : base(accessPassword)
            {                
            }
            #endregion

        }

            #endregion

            #region ChangeEas

            /// <summary>
        /// NxpChangeEas
        /// </summary>
            public abstract class ChangeEas : NxpGen2TagOp
        {
            /// <summary>
            /// EAS
            /// </summary>
            public bool Reset;

            #region Construction
            /// <summary>
            /// Constructor to initialize the parameters of NxpResetReadProtect 9676398256
            /// </summary>
            /// <param name="accessPassword">the access password to use to write to the tag</param>
            /// <param name="reset">true to reset the EAS, false to set it</param>             
            public ChangeEas(UInt32 accessPassword, bool reset)
                : base(accessPassword)
            {                
                this.Reset = reset;
            }
            #endregion

        }

            #endregion

            #region EasAlarm

            /// <summary>
            /// NxpChangeEas
            /// </summary>
            public abstract class EasAlarm : NxpGen2TagOp
        {
            #region Fields

            /// <summary>
            /// Gen2 divide ratio
            /// </summary>
            public Gen2.DivideRatio DivideRatio;

            /// <summary>
            /// Gen2 TagEncoding 
            /// </summary>
            public Gen2.TagEncoding TagEncoding;

            /// <summary>
            /// Gen2 TrExt value
            /// </summary>
            public Gen2.TrExt TrExt;


            #endregion

            #region Construction
            /// <summary>
            /// Constructor to initialize the parameters of NxpEasAlarm
            /// </summary>
            /// <param name="dr">Gen2 divide ratio to use</param>
            /// <param name="m">Gen2 M parameter to use</param>
            /// <param name="trExt">Gen2 TrExt value to use</param>
            public EasAlarm(Gen2.DivideRatio dr, Gen2.TagEncoding m, Gen2.TrExt trExt)
            {
                this.DivideRatio = dr;
                this.TagEncoding = m;
                this.TrExt = trExt;
            }

            #endregion

        }

            #endregion

            #region Calibrate

            /// <summary>
        /// NxpCalibrate
        /// </summary>
            public abstract class Calibrate : NxpGen2TagOp
        {
            #region Construction
            /// <summary>
            /// Constructor to initialize the parameters of NxpCalibrate
            /// </summary>
            /// <param name="accessPassword">the access password</param>
            public Calibrate(UInt32 accessPassword) : base(accessPassword)
            {
            }
            #endregion

        }

            #endregion
        }

        /// <summary>
        /// NXP vendor type
        /// </summary>
        public class NXP : NxpGen2TagOp
        {

            #region G2X
            /// <summary>
            /// G2X class
            /// </summary>
            public class G2X 
            {
                /// <summary>
                /// Default constructor
                /// </summary>
                public G2X()
                {        
                }

                /// <summary>
                /// SetReadProtect
                /// </summary>
                public class SetReadProtect : NxpGen2TagOp.SetReadProtect
                {
                    #region Construction
                    /// <summary>
                    /// Constructor to initialize the parameters of NxpSetReadProtect
                    /// </summary>
                    /// <param name="accessPassword">the access password to use to write to the tag</param>            
                    public SetReadProtect(UInt32 accessPassword): base(accessPassword)
                    {
                        ChipType = 0x02;
                    }
                    #endregion
                }

                /// <summary>
                /// ResetReadProtect
                /// </summary>
                public  class ResetReadProtect : NxpGen2TagOp.ResetReadProtect
                {
                    #region Construction
                    /// <summary>
                    /// Constructor to initialize the parameters of NxpResetReadProtect
                    /// </summary>
                    /// <param name="accessPassword">the access password to use to write to the tag</param>
                    
                    public ResetReadProtect(UInt32 accessPassword) : base(accessPassword)
                    {
                        ChipType = 0x02;
                    }
                    #endregion
                }

                /// <summary>
                /// ChangeEas
                /// </summary>
                public  class ChangeEas : NxpGen2TagOp.ChangeEas
                {
                    #region Construction
                    /// <summary>
                    /// Constructor to initialize the parameters of NxpResetReadProtect 9676398256
                    /// </summary>
                    /// <param name="accessPassword">the access password to use to write to the tag</param>
                    /// <param name="reset">true to reset the EAS, false to set it</param>             
                    public ChangeEas(UInt32 accessPassword, bool reset)  : base(accessPassword,reset)
                    {
                        ChipType = 0x02;
                    }
                    #endregion

                }

                /// <summary>
                /// EasAlarm
                /// </summary>
                public  class EasAlarm : NxpGen2TagOp.EasAlarm
                {
                    #region Construction
                    /// <summary>
                    /// Constructor to initialize the parameters of NxpEasAlarm
                    /// </summary>
                    /// <param name="dr">Gen2 divide ratio to use</param>
                    /// <param name="m">Gen2 M parameter to use</param>
                    /// <param name="trExt">Gen2 TrExt value to use</param>
                    public EasAlarm(Gen2.DivideRatio dr, Gen2.TagEncoding m, Gen2.TrExt trExt): base(dr,m,trExt)
                    {
                        ChipType = 0x02;
                    }

                    #endregion
                }

                /// <summary>
                /// NxpCalibrate
                /// </summary>
                public  class Calibrate : NxpGen2TagOp.Calibrate
                {
                    #region Construction
                    /// <summary>
                    /// Constructor to initialize the parameters of NxpCalibrate
                    /// </summary>
                    /// <param name="accessPassword">the access password</param>
                    public Calibrate(UInt32 accessPassword)
                        : base(accessPassword)
                    {
                        ChipType = 0x02;
                    }
                    #endregion
                }
            }

            #endregion G2X

            #region G2I
            /// <summary>
            /// G2I class
            /// </summary>
            public class G2I 
            {
                /// <summary>
                /// Default constructor
                /// </summary>
                public G2I()
                {
                    
                }
                /// <summary>
                /// SetReadProtect
                /// </summary>
                public class SetReadProtect : NxpGen2TagOp.SetReadProtect
                {
                    #region Construction
                    /// <summary>
                    /// Constructor to initialize the parameters of NxpSetReadProtect
                    /// </summary>
                    /// <param name="accessPassword">the access password to use to write to the tag</param>            
                    public SetReadProtect(UInt32 accessPassword): base(accessPassword)
                    {
                        ChipType = 0x07;
                    }
                    #endregion
                }
                /// <summary>
                /// ResetReadProtect
                /// </summary>
                public  class ResetReadProtect : NxpGen2TagOp.ResetReadProtect
                {
                    #region Construction
                    /// <summary>
                    /// Constructor to initialize the parameters of NxpResetReadProtect
                    /// </summary>
                    /// <param name="accessPassword">the access password to use to write to the tag</param>
                    
                    public ResetReadProtect(UInt32 accessPassword) : base(accessPassword)
                    {
                        ChipType = 0x07;
                    }
                    #endregion
                }

                /// <summary>
                /// ChangeEas
                /// </summary>
                public  class ChangeEas : NxpGen2TagOp.ChangeEas
                {
                    #region Construction
                    /// <summary>
                    /// Constructor to initialize the parameters of NxpResetReadProtect 9676398256
                    /// </summary>
                    /// <param name="accessPassword">the access password to use to write to the tag</param>
                    /// <param name="reset">true to reset the EAS, false to set it</param>             
                    public ChangeEas(UInt32 accessPassword, bool reset)  : base(accessPassword,reset)
                    {
                        ChipType = 0x07;
                    }
                    #endregion

                }

                /// <summary>
                /// EasAlarm
                /// </summary>
                public  class EasAlarm : NxpGen2TagOp.EasAlarm
                {

                    #region Construction
                    /// <summary>
                    /// Constructor to initialize the parameters of NxpEasAlarm
                    /// </summary>
                    /// <param name="dr">Gen2 divide ratio to use</param>
                    /// <param name="m">Gen2 M parameter to use</param>
                    /// <param name="trExt">Gen2 TrExt value to use</param>
                    public EasAlarm(Gen2.DivideRatio dr, Gen2.TagEncoding m, Gen2.TrExt trExt): base(dr,m,trExt)
                    {
                        ChipType = 0x07;
                    }

                    #endregion
                }

                /// <summary>
                /// NxpCalibrate
                /// </summary>
                public  class Calibrate : NxpGen2TagOp.Calibrate
                {
                    #region Construction
                    /// <summary>
                    /// Constructor to initialize the parameters of NxpCalibrate
                    /// </summary>
                    /// <param name="accessPassword">the access password</param>
                    public Calibrate(UInt32 accessPassword): base(accessPassword)
                    {
                        ChipType = 0x07;
                    }
                    #endregion
                }

                /// <summary>
                /// NXP Config Change
                /// </summary>
                public class ChangeConfig : NxpGen2TagOp
                {

                    /// <summary>
                    /// configuration word
                    /// </summary>
                    public UInt16 ConfigWord;


                    /// <summary>
                    /// Constructor to initialize parameters of NxpConfigChange
                    /// </summary>
                    /// <param name="accessPassword">the access password</param>
                    /// <param name="configWord">Config Word</param>
                    public ChangeConfig(UInt32 accessPassword, ConfigWord configWord): base(accessPassword)
                    {
                        ChipType = 0x07;
                        SetConfigWord(configWord);
                    }

                    /// <summary>
                    /// Format the config word
                    /// </summary>
                    /// <param name="inputData">input data</param>
                    private void SetConfigWord(ConfigWord inputData)
                    {
                        // format config data
                        ConfigWord = 0x0000;
                        if (inputData.TamperAlarm)
                        {
                            ConfigWord |= (0x0001 << 15);
                        }
                        if (inputData.ExternalSupply)
                        {
                            ConfigWord |= (0x0001 << 14);
                        }
                        if (inputData.RFU2)
                        {
                            ConfigWord |= (0x0001 << 13);
                        }
                        if (inputData.RFU3)
                        {
                            ConfigWord |= (0x0001 << 12);
                        }
                        if (inputData.InvertDigitalOutput)
                        {
                            ConfigWord |= (0x0001 << 11);
                        }
                        if (inputData.TransparentMode)
                        {
                            ConfigWord |= (0x0001 << 10);
                        }
                        if (inputData.DataMode)
                        {
                            ConfigWord |= (0x0001 << 9);
                        }
                        if (inputData.ConditionalReadRangeReduction_onOff)
                        {
                            ConfigWord |= (0x0001 << 8);
                        }
                        if (inputData.ConditionalReadRangeReduction_openShort)
                        {
                            ConfigWord |= (0x0001 << 7);
                        }
                        if (inputData.MaxBackscatterStrength)
                        {
                            ConfigWord |= (0x0001 << 6);
                        }
                        if (inputData.DigitalOutput)
                        {
                            ConfigWord |= (0x0001 << 5);
                        }
                        if (inputData.PrivacyMode)
                        {
                            ConfigWord |= (0x0001 << 4);
                        }
                        if (inputData.ReadProtectUser)
                        {
                            ConfigWord |= (0x0001 << 3);
                        }
                        if (inputData.ReadProtectEPC)
                        {
                            ConfigWord |= (0x0001 << 2);
                        }
                        if (inputData.ReadProtectTID)
                        {
                            ConfigWord |= (0x0001 << 1);
                        }
                        if (inputData.PsfAlarm)
                        {
                            ConfigWord |= 0x0001;
                        }
                    }
                }

                /// <summary>
                /// ConfigWord
                /// </summary>
                public  class ConfigWord
                {
                    // Down - MSB to LSB
                    /// <summary>PSF alarm flag (Permanently stored in tag memory)/// </summary>
                    public bool PsfAlarm;
                     /// <summary>Read protect TID bank (permanently stored in tag memory)/// </summary>
                    public bool ReadProtectTID;
                    /// <summary>Read protect EPC bank (Permanently stored in tag memory)/// </summary>
                    public bool ReadProtectEPC;
                    /// <summary>Read protect User memory bank (permanently stored in tag memory)/// </summary>
                    public bool ReadProtectUser;
                    /// <summary>Read range reduction on/off (permanently stored in tag memory)/// </summary>
                    public bool PrivacyMode;
                    /// <summary>Digital output (permanently stored in tag memory)/// </summary>
                    public bool DigitalOutput;
                    /// <summary>Maximum backscatter strength (permanently stored in tag memory)/// </summary>
                    public bool MaxBackscatterStrength;
                    /// <summary>Conditional Read Range Reduction open/short (permanently stored in tag memory)/// </summary>
                    public bool ConditionalReadRangeReduction_openShort;
                    /// <summary>Conditional Read Range Reduction on/off (permanently stored in tag memory)/// </summary>
                    public bool ConditionalReadRangeReduction_onOff;
                    /// <summary>Transparent mode data/raw (reset at power up)/// </summary>
                    public bool DataMode;
                    /// <summary>Transparent mode on/off (reset at power up)/// </summary>
                    public bool TransparentMode;
                    /// <summary>Invert digital output (reset at power up)/// </summary>
                    public bool InvertDigitalOutput;
                    /// <summary>RFU 3/// </summary>
                    public bool RFU3;
                    /// <summary>RFU 2/// </summary>
                    public bool RFU2;
                    /// <summary>External supply flag digital input (read only)/// </summary>
                    public bool ExternalSupply;
                    /// <summary>Tamper alarm flag (Read only)/// </summary>
                    public bool TamperAlarm;

                    /// <summary>
                    /// Constructor
                    /// </summary>
                    public  ConfigWord()
                    {               
                    }

                    /// <summary>
                    /// Retrieve configuration word
                    /// </summary>
                    /// <param name="inputData"></param>
                    /// <returns></returns>
                    public ConfigWord GetConfigWord(UInt16 inputData)
                    {
                        ConfigWord cfgWord = new ConfigWord();
                        /* format config data*/
                        if ((inputData & 0x8000) != 0)
                        {
                            cfgWord.TamperAlarm = true;
                        }
                        if ((inputData & 0x4000) != 0)
                        {
                            cfgWord.ExternalSupply = true;
                        }
                        if ((inputData & 0x2000) != 0)
                        {
                            cfgWord.RFU2 = true;
                        }
                        if ((inputData & 0x1000) != 0)
                        {
                            cfgWord.RFU3 = true;
                        }
                        if ((inputData & 0x0800) != 0)
                        {
                            cfgWord.InvertDigitalOutput = true;
                        }
                        if ((inputData & 0x0400) != 0)
                        {
                            cfgWord.TransparentMode = true;
                        }
                        if ((inputData & 0x0200) != 0)
                        {
                            cfgWord.DataMode = true;
                        }
                        if ((inputData & 0x0100) != 0)
                        {
                            cfgWord.ConditionalReadRangeReduction_onOff = true;
                        }
                        if ((inputData & 0x0080) != 0)
                        {
                            cfgWord.ConditionalReadRangeReduction_openShort = true;
                        }
                        if ((inputData & 0x0040) != 0)
                        {
                            cfgWord.MaxBackscatterStrength = true;
                        }
                        if ((inputData & 0x0020) != 0)
                        {
                            cfgWord.DigitalOutput = true;
                        }
                        if ((inputData & 0x0010) != 0)
                        {
                            cfgWord.PrivacyMode = true;
                        }
                        if ((inputData & 0x0008) != 0)
                        {
                            cfgWord.ReadProtectUser = true;
                        }
                        if ((inputData & 0x0004) != 0)
                        {
                            cfgWord.ReadProtectEPC = true;
                        }
                        if ((inputData & 0x0002) != 0)
                        {
                            cfgWord.ReadProtectTID = true;
                        }
                        if ((inputData & 0x0001) != 0)
                        {
                            cfgWord.PsfAlarm = true;
                        }
                        return cfgWord;
                    }

                }

            }
            #endregion G2I
        }

        #endregion NXP

        #region ImpinjMonza4

        /// <summary>
        /// 
        /// </summary>
        public class Impinj : Gen2CustomTagOp
        {
            /// <summary>
            /// 
            /// </summary>
            public class Monza4
            {
                Monza4()
                {

                }


                /// <summary>
                /// Monza4 QTReadWrite
                /// </summary>
                public class QTReadWrite : Gen2CustomTagOp
                {
                    #region Fields

                    /// <summary>
                    /// The access password to use to write to the tag
                    /// </summary>
                    public UInt32 AccessPassword;

                    /// <summary>
                    /// control byte 
                    /// </summary>
                    public int ControlByte;

                    /// <summary>
                    /// Payload word
                    /// </summary>
                    public int PayloadWord;

                    #endregion

                    #region Construction


                    /// <summary>
                    /// Constructor to initialize the parameters QTReadWrite
                    /// </summary>
                    /// <param name="accessPassword">the access password</param>
                    /// <param name="payLoad">comprises of qtSR - Bit 15 (MSB) is first transmitted bit of the
                    /// payload field and qtMEM - Tag uses Public/Private Memory Map
                    /// </param>
                    /// <param name="controlByte">comprises of qtReadWrite - The Read/Write field indicates whether the tag
                    /// reads or writes QT control data and
                    /// </param>
                    public QTReadWrite(UInt32 accessPassword, QTPayload payLoad, QTControlByte controlByte)
                    {
                        ChipType = 0x08;
                        this.AccessPassword = accessPassword;
                        SetPayload(payLoad);
                        SetControlByte(controlByte);
                    }
                    #endregion

                    private void SetPayload(QTPayload payload)
                    {
                        //payload
                        PayloadWord = 0x0000;
                        if (payload.QTSR)
                        {
                            PayloadWord |= 0x8000;
                        }
                        if (payload.QTMEM)
                        {
                            PayloadWord |= 0x4000;
                        }
                        if (payload.RFU13)
                        {
                            PayloadWord |= 0x2000;
                        }
                        if (payload.RFU12)
                        {
                            PayloadWord |= 0x1000;
                        }
                        if (payload.RFU11)
                        {
                            PayloadWord |= 0x0800;
                        }
                        if (payload.RFU10)
                        {
                            PayloadWord |= 0x0400;
                        }
                        if (payload.RFU9)
                        {
                            PayloadWord |= 0x0200;
                        }
                        if (payload.RFU8)
                        {
                            PayloadWord |= 0x0100;
                        }
                        if (payload.RFU7)
                        {
                            PayloadWord |= 0x0080;
                        }
                        if (payload.RFU6)
                        {
                            PayloadWord |= 0x0040;
                        }
                        if (payload.RFU5)
                        {
                            PayloadWord |= 0x0020;
                        }
                        if (payload.RFU4)
                        {
                            PayloadWord |= 0x0010;
                        }
                        if (payload.RFU3)
                        {
                            PayloadWord |= 0x0008;
                        }
                        if (payload.RFU2)
                        {
                            PayloadWord |= 0x0004;
                        }
                        if (payload.RFU1)
                        {
                            PayloadWord |= 0x0002;
                        }
                        if (payload.RFU0)
                        {
                            PayloadWord |= 0x0001;
                        }
                    }

                    private void SetControlByte(QTControlByte ctrlByte)
                    {
                        //control byte
                        ControlByte = 0x00;
                        if (ctrlByte.QTReadWrite)
                        {
                            ControlByte |= 0x80;
                        }
                        if (ctrlByte.Persistence)
                        {
                            ControlByte |= 0x40;
                        }
                    }

                }

                /// <summary>
                ///  Monza4 QT Control Byte
                /// </summary>
                public class QTControlByte
                {
                    /// <summary>
                    ///  The following bits are Reserved for Future Use. And will be ignored.
                    /// RFU_TM's are ThingMagic specific RFU bits
                    /// </summary>
                    public bool RFU_TM0;
                    /// <summary>
                    ///  The following bits are Reserved for Future Use. And will be ignored.
                    /// RFU_TM's are ThingMagic specific RFU bits
                    /// </summary>
                    public bool RFU_TM1;
                    /// <summary>
                    ///  The following bits are Reserved for Future Use. And will be ignored.
                    /// RFU_TM's are ThingMagic specific RFU bits
                    /// </summary>
                    public bool RFU_TM2;
                    /// <summary>
                    ///  The following bits are Reserved for Future Use. And will be ignored.
                    /// RFU_TM's are ThingMagic specific RFU bits
                    /// </summary>
                    public bool RFU_TM3;
                    /// <summary>
                    ///  RFU_Impinj bits are as per the Monza4 specification.
                    /// </summary>
                    public bool RFU_Impinj4;
                    /// <summary>
                    ///  RFU_Impinj bits are as per the Monza4 specification.
                    /// </summary>
                    public bool RFU_Impinj5;
                    /// <summary>
                    /// If Read/Write=1, the Persistence field indicates whether the QT control is
                    /// written to nonvolatile (NVM) or volatile memory.
                    /// Persistence=0 means write to volatile memory. Persistence=1 means write to NVM memory
                    /// </summary>
                    public bool Persistence;
                    /// <summary>
                    /// The Read/Write field indicates whether the tag reads or writes QT control data.
                    /// Read/Write=0 means read the QT control bits in cache.
                    /// Read/Write=1 means write the QT control bits
                    /// </summary>
                    public bool QTReadWrite;
                }
                /// <summary>
                /// Monza QT Payload
                /// </summary>
                public class QTPayload
                {
                    /// <summary>
                    /// Bit 15 (MSB) is first transmitted bit of the payload field.
                    /// Bit # Name Description
                    /// 1: Tag reduces range if in or about to be in OPEN or SECURED state
                    /// 0: Tag does not reduce range
                    /// </summary>
                    public bool QTSR;

                    /// <summary>
                    /// Bit 14 (MSB) is first transmitted bit of the payload field.
                    /// 1:Tag uses Public Memory Map (see Table 2-10)
                    /// 0: Tag uses Private Memory Map (see Table 2-9)
                    /// </summary>
                    public bool QTMEM;


                    /// <summary>
                    /// Reserved for future use. Tag will return these bits as zero.
                    /// </summary>
                    public bool RFU13;
                    /// <summary>
                    /// Reserved for future use. Tag will return these bits as zero.
                    /// </summary>
                    public bool RFU12;
                    /// <summary>
                    /// Reserved for future use. Tag will return these bits as zero.
                    /// </summary>
                    public bool RFU11;
                    /// <summary>
                    /// Reserved for future use. Tag will return these bits as zero.
                    /// </summary>
                    public bool RFU10;
                    /// <summary>
                    /// Reserved for future use. Tag will return these bits as zero.
                    /// </summary>
                    public bool RFU9;
                    /// <summary>
                    /// Reserved for future use. Tag will return these bits as zero.
                    /// </summary>
                    public bool RFU8;
                    /// <summary>
                    /// Reserved for future use. Tag will return these bits as zero.
                    /// </summary>
                    public bool RFU7;
                    /// <summary>
                    /// Reserved for future use. Tag will return these bits as zero.
                    /// </summary>
                    public bool RFU6;
                    /// <summary>
                    /// Reserved for future use. Tag will return these bits as zero.
                    /// </summary>
                    public bool RFU5;
                    /// <summary>
                    /// Reserved for future use. Tag will return these bits as zero.
                    /// </summary>
                    public bool RFU4;
                    /// <summary>
                    /// Reserved for future use. Tag will return these bits as zero.
                    /// </summary>
                    public bool RFU3;
                    /// <summary>
                    /// Reserved for future use. Tag will return these bits as zero.
                    /// </summary>
                    public bool RFU2;
                    /// <summary>
                    /// Reserved for future use. Tag will return these bits as zero.
                    /// </summary>
                    public bool RFU1;
                    /// <summary>
                    /// Reserved for future use. Tag will return these bits as zero.
                    /// </summary>
                    public bool RFU0;
                }
            }
        }

        #endregion ImpinjMonza4

        #region IAVDenatran

        /// <summary>
        /// IAVDenatran secure tag operations.
        /// </summary>
        public class Denatran : Gen2CustomTagOp
        {
            /// <summary>
            /// Protocol
            /// </summary>
            public class IAV : Denatran
            {
                #region Nested Enum
                /// <summary>
                /// Enum IAVDenatran secure tag operation 
                /// </summary>
                public enum SecureTagOpType
                {
					/// <summary>
                    /// ActivateSecureMode - PA Protocol
                    /// </summary>
                    ACTIVATESECUREMODE = 0x00,

                    /// <summary>
                    /// AuthenticateOBU - PA Protocol
                    /// </summary>
                    AUTHENTICATEOBU = 0x01,

                    /// <summary>
                    /// ACTIVATESINIAVMODE - G0 Protocol
                    /// </summary>
                    ACTIVATESINIAVMODE = 0x02,

                    /// <summary>
                    /// OBUAUTHID - G0 Protocol
                    /// </summary>
                    OBUAUTHID = 0x03,

                    /// <summary>
                    /// AUTHENTICATEOBUFULLPASS1 - G0 Protocol
                    /// </summary>
                    AUTHENTICATEOBUFULLPASS1 = 0x04,

                    /// <summary>
                    /// AUTHENTICATEOBUFULLPASS2 - G0 Protocol
                    /// </summary>
                    AUTHENTICATEOBUFULLPASS2 = 0x05,

                    /// <summary>
                    /// OBUREADFROMMEMMAP - G0 Protocol
                    /// </summary>
                    OBUREADFROMMEMMAP = 0x06,

                    /// <summary>
                    /// OBUWRITETOMEMMAP - G0 Protocol
                    /// </summary>
                    OBUWRITETOMEMMAP = 0x07,

                    /// <summary>
                    /// AUTHENTICATEOBUFULLPASS - G0 Protocol
                    /// </summary>
                    AUTHENTICATEOBUFULLPASS = 0x08,

                    /// <summary>
                    /// GETTOKENID - G0 Protocol
                    /// </summary>
                    GETTOKENID = 0x09,
                    
                    /// <summary>
                    /// READSEC - IP63 Protocol
                    /// </summary>
                    READSEC=0x0A,
                    
                    /// <summary>
                    /// WRITESEC - IP63 Protocol
                    /// </summary>
                    WRITESEC=0x0B
                }
                #endregion

                #region Field

                /// <summary>
                /// IAVDenatran secure tagop type
                /// </summary>
                public SecureTagOpType Mode;

                /// <summary>
                /// PayLoad byte
                /// </summary>
                public byte Payload;

                /// <summary>
                /// WordAddress pointer to the USER data
                /// </summary>
                public UInt16 WritePtr = 0;
                
                /// <summary>
                /// User data to be written
                /// </summary>
                public UInt16 WordData = 0;
                
                /// <summary>
                /// Data credentials written word
                /// </summary>
                public DenatranIAVWriteCredential WriteCredentials;

                /// <summary>
                /// Data credentials written word
                /// </summary>
                public DenatranIAVWriteSecCredential WriteSecCredentials;

                #endregion

                /// <summary>
                /// Default constructor
                /// </summary>
                public IAV() 
                {
                    ChipType = 0x0B;
                }

                #region ActivateSiniavMode - G0 Protocol
                /// <summary>
                /// Activate Siniav mode - G0 Protocol
                /// </summary>
                public class ActivateSiniavMode : IAV
                {
                    #region Fields

                    /// <summary>
                    /// 64 bits of token number to activate the tag
                    /// </summary>
                    public byte[] Token = null;
                    
                    #endregion
               
                    #region Construction
                    /// <summary>
                    /// Constructor
                    /// <param name="payLoadbyte">payload 1byte->[TC(Transmission Count) 1bit + RFFU(Reserved For Furture Use) 7bits]</param>
                    /// </summary>
                    public ActivateSiniavMode(byte payLoadbyte)
                    {
                        Mode = Denatran.IAV.SecureTagOpType.ACTIVATESINIAVMODE;
                        Payload = payLoadbyte;
                    }
                    /// <summary>
                    /// Constructor
                    /// </summary>
                    /// <param name="payLoadbyte">payload 1byte->[TC(Transmission Count) 1bit + RFFU(Reserved For Furture Use) 7bits]</param>
                    /// <param name="token">64 bits of token number to activate the tag</param>
                    public ActivateSiniavMode(byte payLoadbyte, byte [] token)
                    {
                        // Token length should be 8 bytes
                        if (8 != token.Length)
                        {
                            throw new ArgumentException(String.Format(
                                "Token value must be exactly 8 bytes long (got {0} bytes)",
                                token.Length));
                        }
                        
                        // Currently last two bits of the payload is used as TokenDesc (Token Descriptor): 2 bits
                        // parameter indicating the presence and format of Token
                        // 00 : No Token. 
                        // 01 : Token of 64 bits.
                        if (0x01 == (0x03 & payLoadbyte))
                        {
                            Token = token;
                            Mode = Denatran.IAV.SecureTagOpType.ACTIVATESINIAVMODE;
                            Payload = payLoadbyte;
                        }
                        else
                        {
                            Token = null;
                            Mode = Denatran.IAV.SecureTagOpType.ACTIVATESINIAVMODE;
                            Payload = payLoadbyte;
                        }
                    }
                    #endregion
                }
                #endregion ActivateSiniavMode - G0 Protocol

                #region OBUAuthID - G0 Protocol
                /// <summary>
                /// G0 Protocol - Unilateral OBU authentication and identification.
                /// </summary>
                public class OBUAuthID : IAV
                {
                    #region Construction
                    /// <summary>
                    /// Constructor:
                    /// <param name="payLoadbyte">1byte->[TC(Transmission Count) 1bit + RFFU(Reserved For Furture Use) 7bits].</param>
                    /// </summary>
                    public OBUAuthID(byte payLoadbyte)
                    {
                        Mode = Denatran.IAV.SecureTagOpType.OBUAUTHID;
                        Payload = payLoadbyte;
                    }
                    #endregion
                }

                #endregion OBUAuthID - G0 Protocol

                #region OBUAuthFullPass1 - G0 Protocol
                /// <summary>
                /// G0 Protocol - Unilateral OBU Authentication with full read of page 1 of USER memory, pass 1.
                /// </summary>
                public class OBUAuthFullPass1 : IAV
                {
                    #region Construction
                    /// <summary>
                    /// Constructor:
                    /// <param name="payLoadbyte">1byte->[TC(Transmission Count) 1bit + RFFU(Reserved For Furture Use) 7bits].</param>
                    /// </summary>
                    public OBUAuthFullPass1(byte payLoadbyte)
                    {
                        Mode = Denatran.IAV.SecureTagOpType.AUTHENTICATEOBUFULLPASS1;
                        Payload = payLoadbyte;
                    }
                    #endregion
                }
                #endregion OBUAuthFullPass1 - G0 Protocol

                #region OBUAuthFullPass2 - G0 Protocol
                /// <summary>
                /// G0 Protocol - Unilateral OBU Authentication with full read of page 1 of USER memory, pass 2.
                /// </summary>
                public class OBUAuthFullPass2 : IAV
                {
                    #region Construction
                    /// <summary>
                    /// Constructor:
                    /// <param name="payLoadbyte">1byte->[TC(Transmission Count) 1bit + RFFU(Reserved For Furture Use) 7bits].</param>
                    /// </summary>
                    public OBUAuthFullPass2(byte payLoadbyte)
                    {
                        Mode = Denatran.IAV.SecureTagOpType.AUTHENTICATEOBUFULLPASS2;
                        Payload = payLoadbyte;
                    }
                    #endregion
                }

                #endregion OBUAuthFullPass2 - G0 Protocol

                #region OBUReadFromMemMap - G0 Protocol
                /// <summary>
                /// G0 Protocol - OBUReadFromMemMap
                /// </summary>
                public class OBUReadFromMemMap : IAV
                {
                    #region Construction
                    /// <summary>
                    /// Constructor:
                    /// </summary>
                    /// <param name="payLoadbyte">1byte->[TC(Transmission Count) 1bit + RFFU(Reserved For Furture Use) 7bits] + 2bytes->[MMWordPtr (Word pointer) 16 bits].</param>
                    /// <param name="wordAddress">WordAddress pointer to the USER data</param>
                    public OBUReadFromMemMap(byte payLoadbyte, ushort wordAddress)
                    {
                        Mode = Denatran.IAV.SecureTagOpType.OBUREADFROMMEMMAP;
                        Payload = payLoadbyte;
                        WritePtr = wordAddress;
                    }
                    #endregion
                }

                #endregion OBUReadFromMemMap - G0 Protocol

                #region OBUWriteToMemMap - G0 Protocol
                /// <summary>
                /// G0 Protocol - OBUWriteToMemMap
                /// </summary>
                public class OBUWriteToMemMap : IAV
                {
                    #region Construction
                    /// <summary>
                    /// Constructor:
                    /// </summary>
                    /// <param name="payLoadbyte">1byte->[TC(Transmission Count) 1bit + RFFU(Reserved For Furture Use) 7bits] + 2bytes->[MMWordPtr (Word pointer) 16 bits] + 2bytes->[Word (Word to be write) 16 bits] + 16bytes->[Write-Credentials 128 bits].</param>
                    /// <param name="wordAddress">WordAddress pointer to the USER data</param>
                    /// <param name="word">User data to be written</param>
                    /// <param name="writeCredential">Data credentials written word</param>
                    public OBUWriteToMemMap(byte payLoadbyte, ushort wordAddress, ushort word, DenatranIAVWriteCredential writeCredential)
                    {
                        Mode = Denatran.IAV.SecureTagOpType.OBUWRITETOMEMMAP;
                        Payload = payLoadbyte;
                        WritePtr = wordAddress;
                        WordData = word;
                        WriteCredentials = writeCredential;
                    }
                    #endregion
                }

                #endregion OBUWriteToMemMap - G0 Protocol

                #region ActivateSecureMode PA Protocol
                /// <summary>
                /// PA Protocol - Activate secure mode
                /// </summary>
                public class ActivateSecureMode : IAV
                {
                    #region Construction
                    /// <summary>
                    /// Constructor:
                    /// <param name="payLoadbyte">1byte->[TC(Transmission Count) 1bit + RFFU(Reserved For Furture Use) 7bits]. </param>
                    /// </summary>
                    public ActivateSecureMode(byte payLoadbyte)
                    {
                        Mode = Denatran.IAV.SecureTagOpType.ACTIVATESECUREMODE;
                        Payload = payLoadbyte;
                    }
                    #endregion
                }
                #endregion ActivateSecureMode PA Protocol

                #region AuthenticateOBU PA Protocol
                /// <summary>
                /// PA Protocol - Authenticate OBU
                /// </summary>
                public class AuthenticateOBU : IAV
                {
                    #region Construction
                    /// <summary>
                    /// Constructor:
                    /// <param name="payLoadbyte">1byte->[TC(Transmission Count) 1bit + RFFU(Reserved For Furture Use) 7bits].</param>
                    /// </summary>
                    public AuthenticateOBU(byte payLoadbyte)
                    {
                        Mode = Denatran.IAV.SecureTagOpType.AUTHENTICATEOBU;
                        Payload = payLoadbyte;
                    }
                    #endregion
                }
                #endregion AuthenticateOBU PA Protocol

                #region OBUAuthFullPass - G0 Protocol
                /// <summary>
                /// G0 Protocol - Unilateral OBU Authentication with full read of page 1 of USER memory, full pass.
                /// </summary>
                public class OBUAuthFullPass : IAV
                {
                    #region Construction
                    /// <summary>
                    /// Constructor:
                    /// <param name="payLoadbyte">1byte->[TC(Transmission Count) 1bit + RFFU(Reserved For Furture Use) 7bits].</param>
                    /// </summary>
                    public OBUAuthFullPass(byte payLoadbyte)
                    {
                        Mode = Denatran.IAV.SecureTagOpType.AUTHENTICATEOBUFULLPASS;
                        Payload = payLoadbyte;
                    }
                    #endregion
                }
                #endregion OBUAuthFullPass - G0 Protocol

                #region GetTokenId - G0 Protocol
                /// <summary>
                /// G0 Protocol - Get TokedId.
                /// </summary>
                public class GetTokenId : IAV
                {
                    #region Construction
                    /// <summary>
                    /// Constructor:
                    /// </summary>
                    public GetTokenId()
                    {
                        Mode = Denatran.IAV.SecureTagOpType.GETTOKENID;
                    }
                    #endregion
                }

                #endregion GetTokenId - G0 Protocol

                #region ReadSec - IP63 Protocol
                /// <summary>
                /// IP63 Protocol - ReadSec
                /// </summary>
                public class ReadSec : IAV
                {
                    #region Construction
                    /// <summary>
                    /// Constructor:
                    /// </summary>
                    /// <param name="payLoadbyte">1byte->[TC(Transmission Count) 1bit + RFFU(Reserved For Furture Use) 7bits].</param>
                    /// <param name="wordAddress">WordAddress pointer to the USER data</param>
                    public ReadSec(byte payLoadbyte, ushort wordAddress)
                    {
                        Mode = Denatran.IAV.SecureTagOpType.READSEC;
                        Payload = payLoadbyte;
                        WritePtr = wordAddress;
                    }
                    #endregion
                }

                #endregion ReadSec - IP63 Protocol

                #region WriteSec - IP63 Protocol
                /// <summary>
                ///  IP63 Protocol - WriteSec
                /// </summary>
                public class WriteSec : IAV
                {
                    #region Construction
                    /// <summary>
                    /// Constructor:
                    /// </summary>
                    /// <param name="payLoadbyte">1byte->[TC(Transmission Count) 1bit + RFFU(Reserved For Furture Use) 7bits].</param>
                    /// <param name="writeSecCredential">Data credentials written word</param>
                    public WriteSec(byte payLoadbyte, DenatranIAVWriteSecCredential writeSecCredential)
                    {
                        Mode = Denatran.IAV.SecureTagOpType.WRITESEC;
                        Payload = payLoadbyte;
                        WriteSecCredentials = writeSecCredential;
                    }
                    #endregion
                }

                #endregion WriteSec - IP63 Protocol
            }
        }

        #endregion IAVDenatran

        #endregion CustomCommands

        #endregion TagCommands

        #endregion

    }
}
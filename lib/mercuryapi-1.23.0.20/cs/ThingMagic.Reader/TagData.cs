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

namespace ThingMagic
{
    /// <summary>
    /// Data relating to the identity and content of a tag.
    /// </summary>
    public class TagData : TagFilter
    {
        #region Fields

        internal byte[] _epc = null;
        internal byte[] _crc = null;

        #endregion

        #region Properties

        /// <summary>
        /// Tag's EPC
        /// </summary>
        public byte[] EpcBytes
        {
            get { return (null != _epc) ? (byte[]) _epc.Clone() : null; }
        }

        /// <summary>
        /// Internally-stored tag EPC
        /// </summary>
        /// <summary>
        /// Tag's EPC, as a human-readable hex string with no prefix or separators; e.g., 0123456789ABCDEF
        /// </summary>
        public String EpcString
        {
            get { return ByteFormat.ToHex(this.EpcBytes, "", ""); }
        }
        
        /// <summary>
        /// Tag's CRC
        /// </summary>
        public byte[] CrcBytes
        {
            get { return (null != _crc) ? (byte[]) _crc.Clone() : null; }
        }
        
        /// <summary>
        /// Tag's RFID protocol
        /// </summary>
        public virtual TagProtocol Protocol
        {
            get { return TagProtocol.NONE; }
        }

        #endregion

        #region Construction

        /// <summary>
        /// Create TagData with blank CRC
        /// </summary>
        /// <param name="epcBytes">EPC value</param>
        public TagData(ICollection<byte> epcBytes) : this(epcBytes, null) { }

        /// <summary>
        /// Create TagData
        /// </summary>
        /// <param name="epcBytes">EPC value</param>
        /// <param name="crcBytes">CRC value</param>
        public TagData(ICollection<byte> epcBytes, ICollection<byte> crcBytes)
        {
            _epc = (null != epcBytes) ? CollUtil.ToArray(epcBytes) : null;
            _crc = (null != crcBytes) ? CollUtil.ToArray(crcBytes) : null;
        }
        /// <summary>
        /// Create TagData from human-readable EPC
        /// </summary>
        /// <param name="epcHexString">EPC value, as hex string; e.g., "1234567890ABCDEF"</param>
        public TagData(string epcHexString) : this(ByteFormat.FromHex(epcHexString)) { }

        #endregion

        #region ToString

        /// <summary>
        /// Human-readable representation
        /// </summary>
        /// <returns>A string representing the current object</returns>
        public override string ToString()
        {
            return String.Join("", new string[] {
                 "", "EPC:", this.EpcString,
            });
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
        public bool Matches(TagData t)
        {
            for (int i = 0 ; i < EpcBytes.Length && i < t.EpcBytes.Length ; i++)
                if (EpcBytes[i] != t.EpcBytes[i])
                    return false;
            return true;
        }

        #endregion
    }
}

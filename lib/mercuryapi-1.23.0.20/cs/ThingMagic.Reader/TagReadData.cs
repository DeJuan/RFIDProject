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

namespace ThingMagic
{
    /// <summary>
    /// Data representing the reading of an RFID tag
    /// </summary>
    public class TagReadData
    {
        #region Static Fields

        private static byte[] _noData = new byte[0];

        #endregion

        #region Fields

        internal TagData _tagData = null;
        internal int _antenna = 0;
        internal int _lqi = 0;
        internal int _frequency = 0;
        internal int _phase = 0;
        internal GpioPin[] _GPIO = null;
        internal Reader reader = null;
        internal bool isAsyncRead;

        /// <summary>
        /// Number of times the Tag was read.
        /// </summary>
        public int ReadCount = 0;

        /// <summary>
        /// Time that search started
        /// </summary>
        internal DateTime _baseTime;
        /// <summary>
        /// Time tag was read, in milliseconds since start of search
        /// </summary>
        internal int _readOffset = 0;
        internal byte[] _data = _noData;
        internal byte[] _dataEPCMem = _noData;
        internal byte[] _dataReservedMem = _noData;
        internal byte[] _dataTidMem = _noData;
        internal byte[] _dataUserMem = _noData;
        #endregion

        #region Properties

        /// <summary>
        /// Read Data Bytes
        /// </summary>
        public byte[] Data
        {
            get { return (byte[])_data.Clone(); }
        }
        
        /// <summary>
        /// Read EPC Bank Data Bytes
        /// </summary>
        public byte[] EPCMemData
        {
            get { return (byte[])_dataEPCMem; }
        }

        /// <summary>
        /// Read Tid Bank Data Bytes
        /// </summary>
        public byte[] TIDMemData
        {
            get { return (byte[])_dataTidMem; }
        }

        /// <summary>
        /// Read User Bank Data Bytes
        /// </summary>
        public byte[] USERMemData
        {
            get { return (byte[])_dataUserMem; }
        }

        /// <summary>
        /// Read Reserved Bank Data Bytes
        /// </summary>
        public byte[] RESERVEDMemData
        {
            get { return (byte[])_dataReservedMem; }
        }

        /// <summary>
        /// [1-based] numeric identifier of antenna that tag was read on
        /// </summary>
        public int Antenna
        {
            get { return _antenna; }
        }

        /// <summary>
        /// RSSI units
        /// </summary>
        public int Rssi
        {
            get { return _lqi; }
            set { _lqi = value; }
        }

        /// <summary>
        /// EPC of tag
        /// </summary>
        public byte[] Epc
        {
            get { return _tagData.EpcBytes; }
        }

        /// <summary>
        /// EPC of tag, as human-readable string
        /// </summary>
        public string EpcString
        {
            get { return _tagData.EpcString; }
        }

        /// <summary>
        /// Tag that was read
        /// </summary>
        public TagData Tag
        {
            get { return _tagData; }
        }

        /// <summary>
        /// Frequency at which the tag was read.
        /// </summary>
        public int Frequency
        {
            get { return _frequency; }
        }

        /// <summary>
        /// Time when tag was read
        /// </summary>
        public DateTime Time
        {
            get 
            {
                if (isAsyncRead)
                {                    
                    return this._baseTime;
        }
                else
                {                    
                    return this._baseTime.AddMilliseconds(this._readOffset);
                };
            }
        }

        /// <summary>
        /// Phase when tag was read
        /// </summary>
        public int Phase
        {
            get { return _phase; }
        }

        /// <summary>
        /// GPIO value when tag was read
        /// </summary>
        public GpioPin[] GPIO
        {
            get { return _GPIO; }
        }

        /// <summary>
        /// Reader when tag was read
        /// </summary>
        public Reader Reader
        {
            get { return reader; }
            set { reader = value; }
        }

        #endregion

        #region ToString

        /// <summary>
        /// Human-readable representation
        /// </summary>
        /// <returns>A string representing the current object</returns>
        public override string ToString()
        {
            return String.Join("", new string[] {
                 "", "EPC:", (null != _tagData) ? _tagData.EpcString : "null",
                " ", "ant:", this.Antenna.ToString(),
                " ", "count:", this.ReadCount.ToString(),
                " ", "time:", this.Time.ToString("yyyy-MM-dd'T'HH:mm:ss.fffK"),
            });
        }

        #endregion
    }
}
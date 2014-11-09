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
    /// Firmware load options
    /// </summary>
    public class FixedReaderFirmwareLoadOptions : FirmwareLoadOptions 
    {
        #region Fields
        private bool eraseContents;
        private bool revertDefaultSettings;
        #endregion Fields

        #region Construction
        /// <summary>
        /// Default constructor
        /// </summary>
        public FixedReaderFirmwareLoadOptions()
        {
        }

        /// <summary>
        /// Constructor with arguments
        /// </summary>
        /// <param name="eContents">true/false</param>
        /// <param name="rdSettings">true/false</param>
        public FixedReaderFirmwareLoadOptions(bool eContents, bool rdSettings)
        {
            eraseContents = eContents;
            revertDefaultSettings = rdSettings;
        }
        #endregion Construction

        #region Properties
        /// <summary>
        /// Sets the EraseContents
        /// </summary>
        public bool EraseContents
        {
            get { return eraseContents; }
            set { eraseContents = value; }
        }
        /// <summary>
        /// Sets the RevertDefaultSettings
        /// </summary>
        public bool RevertDefaultSettings
        {
            get { return revertDefaultSettings; }
            set { revertDefaultSettings = value; }
        }
        #endregion Properties

        #region ToString
        /// <summary>
        /// Human-readable representation
        /// </summary>
        /// <returns>Human-readable representation</returns>
        public override string ToString()
        {
            return "FixedReaderFirmwareLoadOptions(" + eraseContents.ToString() + "," + revertDefaultSettings.ToString() + ")";
        }
        #endregion ToString
    }
}
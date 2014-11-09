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
    /// This class represents a version number for a component of the module.    
    /// </summary>
    public sealed class VersionNumber
    {
        #region Fields

        /// <summary>
        /// Part number for the version field
        /// </summary>
        public byte Part1, Part2, Part3, Part4;
        /// <summary>
        /// Composite Version Info number
        /// </summary>
        public UInt32 CompositeVersion;

        #endregion

        #region Construction

        /// <summary>
        /// Construct a new VersionNumber object given the individual components.
        /// Note that all version number components are discussed and
        /// presented in hexadecimal format, that is, in the version number
        /// "9.5.12.0", the 12 is 0x12 and should be passed to this
        /// constructor as such.
        /// </summary>
        /// <param name="part1">the first part of the version number</param>
        /// <param name="part2">the second part of the version number</param>
        /// <param name="part3">the third part of the version number</param>
        /// <param name="part4">the fourth part of the version number</param>
        public VersionNumber(byte part1, byte part2, byte part3, byte part4)
        {
            this.Part1 = part1;
            this.Part2 = part2;
            this.Part3 = part3;
            this.Part4 = part4;

            CompositeVersion =
              (UInt32) (part1 << 24) |
              (UInt32) (part2 << 16) |
              (UInt32) (part3 << 8)  |
              (UInt32) (part4 << 0);
        }

        #endregion

        #region CompareTo

        /// <summary>
        /// Compares two versions of the Version Number
        /// </summary>
        /// <param name="v">Version Number to compare</param>
        /// <returns>Relative order of objects being compared</returns>
        public int CompareTo(VersionNumber v)
        {
            return (int) ((long) this.CompositeVersion - (long) v.CompositeVersion);
        }

        #endregion

        #region ToString

        /// <summary>
        /// Return a string representation of the version number, as a
        /// sequence of four two-digit hexadecimal numbers separated by
        /// dots, for example "09.05.12.0".
        /// </summary>
        /// <returns>String representation of the VersionNumber</returns>
        public override string ToString()
        {
            return string.Format("{0:X2}.{1:X2}.{2:X2}.{3:X2}", Part1, Part2, Part3, Part4);
        }

        #endregion

        #region Equals

        /// <summary>
        /// Returns true if the object Matches the CompositeVersion of VersionNumber
        /// </summary>
        /// <param name="obj">Object to perform the equal function</param>
        /// <returns>Bool value of the Equals function</returns>
        public override bool Equals(object obj)
        {
            if (!(obj is VersionNumber))
                return false;
            VersionNumber vn = (VersionNumber) obj;
            return CompositeVersion == vn.CompositeVersion;
        }

        #endregion

        #region GetHashCode

        /// <summary>
        /// Returns int representation of VersionInfo
        /// </summary>
        /// <returns>int representation of VersionInfo</returns>
        public override int GetHashCode()
        {
            return (int) CompositeVersion;
        }

        #endregion
    }
}
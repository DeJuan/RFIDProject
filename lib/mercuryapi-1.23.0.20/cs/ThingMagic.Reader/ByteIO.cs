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
using System.IO;

namespace ThingMagic
{
    /// <summary>
    /// Read numeric fields from binary streams
    /// </summary>
    public static class ByteIO
    {
        #region ReadU16

        /// <summary>
        /// Read big-endian Uint16 (e.g., firmware file header) from binary _stream
        /// </summary>
        /// <param name="src">Binary input _stream</param>
        /// <returns>Unsigned 16-bit integer</returns>
        public static UInt16 ReadU16( BinaryReader src )
        {
            UInt16 value = 0;

            for (int i = 0 ; i < 2 ; i++)
            {
                value <<= 8;
                value |= src.ReadByte();
            }

            return value;
        }

        #endregion

        #region ReadU32
        /// <summary>
        /// Read big-endian Uint32 (e.g., firmware file header) from binary _stream
        /// </summary>
        /// <param name="src">Binary input _stream</param>
        /// <returns>Unsigned 32-bit integer</returns>
        public static UInt32 ReadU32( BinaryReader src )
        {
            UInt32 value = 0;

            for (int i = 0 ; i < 4 ; i++)
            {
                value <<= 8;
                value |= src.ReadByte();
            }

            return value;
        } 
        #endregion
    }
}
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
    /// Convert byte arrays to and from human-readable strings
    /// </summary>
    public static class ByteFormat
    {
        #region FromHex

        /// <summary>
        /// Convert human-readable hex string to byte array;
        /// e.g., 123456 or 0x123456 -> {0x12, 0x34, 0x56};
        /// </summary>
        /// <param name="hex">Human-readable hex string to convert</param>
        /// <returns>Byte array</returns>
        public static byte[] FromHex(string hex)
        {
            int prelen = 0;

            if (hex.StartsWith("0x") || hex.StartsWith("0X"))
                prelen = 2;

            byte[] bytes = new byte[(hex.Length - prelen) / 2];

            for (int i = 0 ; i < bytes.Length ; i++)
            {
                string bytestring = hex.Substring(prelen + (2 * i), 2);
                bytes[i] = byte.Parse(bytestring, System.Globalization.NumberStyles.HexNumber);
            }

            return bytes;
        }

        #endregion

        #region ToHex

        /// <summary>
        /// Convert byte array to human-readable hex string; e.g., {0x12, 0x34, 0x56} -> 123456
        /// </summary>
        /// <param name="bytes">Byte array to convert</param>
        /// <returns>Human-readable hex string</returns>
        public static string ToHex(byte[] bytes)
        {
            return ToHex(bytes, "0x", "");
        }
     
        /// <summary>
        /// Convert byte array to human-readable hex string; e.g., {0x12, 0x34, 0x56} -> 123456
        /// </summary>
        /// <param name="bytes">Byte array to convert</param>
        /// <param name="prefix">String to place before byte strings</param>
        /// <param name="separator">String to place between byte strings</param>
        /// <returns>Human-readable hex string</returns>
        public static string ToHex(byte[] bytes, string prefix, string separator)
        {
            if (null == bytes)
                return "null";

            List<string> bytestrings = new List<string>();

            foreach (byte b in bytes)
                bytestrings.Add(b.ToString("X2"));

            return prefix + String.Join(separator, bytestrings.ToArray());
        } 

        /// <summary>
        /// Convert u16 array to human-readable hex string; e.g., {0x1234, 0x5678} -> 12345678
        /// </summary>
        /// <param name="words">u16 array to convert</param>
        /// <returns>Human-readable hex string</returns>
        public static string ToHex(UInt16[] words)
        {
            StringBuilder sb = new StringBuilder(4 * words.Length);

            foreach (UInt16 word in words)
                sb.Append(word.ToString("X4"));

            return sb.ToString();
        }

        #endregion
    }
}
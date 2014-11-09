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
    /// Convert numeric fields within M5e messages
    /// </summary>
    public static class ByteConv
    {
        #region BytesPerBits

        /// <summary>
        /// Calculate number of bytes required to hold number of bits
        /// </summary>
        /// <param name="bitCount">Number of bits to be held</param>
        /// <returns>Number of bytes required</returns>
        public static int BytesPerBits(int bitCount)
        {
            return BigsPerSmalls(bitCount, 8);
        }

        #endregion

        #region WordsPerBytes

        /// <summary>
        /// Calculate number of 16-bit words required to contain number of bytes
        /// </summary>
        /// <param name="byteCount">Number of bytes to be contained</param>
        /// <returns>Number of words required</returns>
        public static int WordsPerBytes(int byteCount)
        {
            return BigsPerSmalls(byteCount, 2);
        }

        #endregion 
        
        #region BigsPerSmalls

        /// <summary>
        /// Calculate number of larger units required to contain number of smaller units.
        /// For example, a byte contains 8 bits.  1 byte holds 0-8 bits, 2 bytes hold 9-16 bits, ...
        /// </summary>
        /// <param name="smallCount">Number of smaller units to be contained.</param>
        /// <param name="smallsPerBig">Number of smaller units contained within one larger unit; e.g., 8 bits per byte</param>
        /// <returns>Number of larger units required to contain smallCount smaller units.</returns>
        private static int BigsPerSmalls(int smallCount, int smallsPerBig)
        {
            if (smallCount <= 0)
                return 0;

            return ((smallCount - 1) / smallsPerBig) + 1;
        }

        #endregion

        #region ToAscii

        /// <summary>
        /// Extract ASCII string from byte string
        /// </summary>
        /// <param name="bytes">Source byte string</param>
        /// <param name="offset">Place to start extracting</param>
        /// <param name="length">Number of bytes to extract</param>
        /// <returns>ASCII string from byte string</returns>
        public static string ToAscii(byte[] bytes, int offset, int length)
        {
            return new ASCIIEncoding().GetString(bytes, offset, length);
        }

        #endregion

        #region EncodeU16

        /// <summary>
        /// Create byte array that represents unsigned 16-bit integer
        /// </summary>
        /// <param name="value">16-bit integer to encode</param>
        /// <returns>2-byte encoding of 16-bit integer</returns>
        public static byte[] EncodeU16(UInt16 value)
        {
            byte[] bytes = new byte[2];
            FromU16(bytes, 0, value);
            return bytes;
        }

        #endregion
        #region EncodeS16

        /// <summary>
        /// Create byte array that represents signed 16-bit integer
        /// </summary>
        /// <param name="value">16-bit integer to encode</param>
        /// <returns>2-byte encoding of 16-bit integer</returns>
        public static byte[] EncodeS16(Int16 value)
        {
            byte[] bytes = new byte[2];
            FromS16(bytes, 0, value);
            return bytes;
        }

        #endregion

        #region FromU16

        /// <summary>
        /// Insert unsigned 16-bit integer into big-endian byte string
        /// </summary>
        /// <param name="bytes">Target big-endian byte string</param>
        /// <param name="offset">Location to insert into</param>
        /// <param name="value">16-bit integer to insert</param>
        /// <returns>Number of bytes inserted</returns>
        public static int FromU16(byte[] bytes, int offset, UInt16 value)
        {
            int end = offset;
            bytes[end++] = (byte) ((value >> 8) & 0xFF);
            bytes[end++] = (byte) ((value >> 0) & 0xFF);
            return end - offset;
        }

        #endregion

        #region FromS16

        /// <summary>
        /// Insert signed 16-bit integer into big-endian byte string
        /// </summary>
        /// <param name="bytes">Target big-endian byte string</param>
        /// <param name="offset">Location to insert into</param>
        /// <param name="value">16-bit integer to insert</param>
        /// <returns>Number of bytes inserted</returns>
        public static int FromS16(byte[] bytes, int offset, Int16 value)
        {
            int end = offset;
            bytes[end++] = (byte)((value >> 8) & 0xFF);
            bytes[end++] = (byte)((value >> 0) & 0xFF);
            return end - offset;
        }

        #endregion

        #region ToU16

        /// <summary>
        /// Extract unsigned 16-bit integer from big-endian byte string
        /// </summary>
        /// <param name="bytes">Source big-endian byte string</param>
        /// <param name="offset">Location to extract from.  Will be updated to post-decode offset.</param>
        /// <returns>Unsigned 16-bit integer</returns>
        public static UInt16 ToU16(byte[] bytes, ref int offset)
        {
            if (null == bytes) return default(byte);
            int hi = (UInt16) (bytes[offset++]) << 8;
            int lo = (UInt16) (bytes[offset++]);
            return (UInt16) (hi | lo);
        }

        /// <summary>
        /// Extract unsigned 16-bit integer from big-endian byte string
        /// </summary>
        /// <param name="bytes">Source big-endian byte string</param>
        /// <param name="offset">Location to extract from</param>
        /// <returns>Unsigned 16-bit integer</returns>
        public static UInt16 ToU16(byte[] bytes, int offset)
        {
            // TODO: Deprecate in favor of version that auto-increments offset
            return ToU16(bytes, ref offset);
        }

        #endregion

        #region ToU16s

        /// <summary>
        /// Convert byte array to u16 array
        /// </summary>
        /// <param name="bytes">Input bytes</param>
        /// <returns>Output words</returns>
        public static UInt16[] ToU16s(ICollection<byte> bytes)
        {
            if (0 != (bytes.Count % 1))
                throw new ArgumentException("Byte array length must be even");

            byte[] byteArray = CollUtil.ToArray(bytes);
            UInt16[] words = new UInt16[byteArray.Length / 2];
            for (int i = 0 ; i < words.Length ; i++)
            {
                words[i] = (UInt16) byteArray[2 * i];
                words[i] <<= 8;
                words[i] |= byteArray[2 * i + 1];
            }

            return words;
        }

        #endregion

        #region EncodeU32

        /// <summary>
        /// Create byte array that represents unsigned 32-bit integer
        /// </summary>
        /// <param name="value">32-bit integer to encode</param>
        /// <returns>4-byte encoding of 32-bit integer</returns>
        public static byte[] EncodeU32(UInt32 value)
        {
            byte[] bytes = new byte[4];
            FromU32(bytes, 0, value);
            return bytes;
        }

        #endregion

        #region FromU32

        /// <summary>
        /// Insert unsigned 32-bit integer into big-endian byte string
        /// </summary>
        /// <param name="bytes">Target big-endian byte string</param>
        /// <param name="offset">Location to insert into</param>
        /// <param name="value">32-bit integer to insert</param>
        /// <returns>Number of bytes inserted</returns>
        public static int FromU32(byte[] bytes, int offset, UInt32 value)
        {
            int end = offset;
            bytes[end++] = (byte) ((value >> 24) & 0xFF);
            bytes[end++] = (byte) ((value >> 16) & 0xFF);
            bytes[end++] = (byte) ((value >> 8) & 0xFF);
            bytes[end++] = (byte) ((value >> 0) & 0xFF);
            return end - offset;
        }

        #endregion

        #region ToU32

        /// <summary>
        /// Extract unsigned 32-bit integer from big-endian byte string
        /// </summary>
        /// <param name="bytes">Source big-endian byte string</param>
        /// <param name="offset">Location to extract from</param>
        /// <returns>Unsigned 32-bit integer</returns>
        public static UInt32 ToU32(byte[] bytes, int offset)
        {
            return (UInt32)(0
                | ((UInt32)(bytes[offset + 0]) << 24)
                | ((UInt32)(bytes[offset + 1]) << 16)
                | ((UInt32)(bytes[offset + 2]) << 8)
                | ((UInt32)(bytes[offset + 3]) << 0)
                );
        }

        #endregion

        #region EncodeU64

        /// <summary>
        /// Create byte array that represents unsigned 64-bit integer
        /// </summary>
        /// <param name="value">64-bit integer to encode</param>
        /// <returns>4-byte encoding of 64-bit integer</returns>
        public static byte[] EncodeU64(UInt64 value)
        {
            byte[] bytes = new byte[8];
            FromU64(bytes, 0, value);
            return bytes;
        }

        #endregion

        #region FromU64

        /// <summary>
        /// Insert unsigned 64-bit integer into big-endian byte string
        /// </summary>
        /// <param name="bytes">Target big-endian byte string</param>
        /// <param name="offset">Location to insert into</param>
        /// <param name="value">64-bit integer to insert</param>
        /// <returns>Number of bytes inserted</returns>
        public static int FromU64(byte[] bytes, int offset, UInt64 value)
        {
            int end = offset;
            bytes[end++] = (byte)((value >> 56) & 0xFF);
            bytes[end++] = (byte)((value >> 48) & 0xFF);
            bytes[end++] = (byte)((value >> 40) & 0xFF);
            bytes[end++] = (byte)((value >> 32) & 0xFF);
            bytes[end++] = (byte)((value >> 24) & 0xFF);
            bytes[end++] = (byte)((value >> 16) & 0xFF);
            bytes[end++] = (byte)((value >> 8) & 0xFF);
            bytes[end++] = (byte)((value >> 0) & 0xFF);
            return end - offset;
        }

        #endregion

        #region ToU64

        /// <summary>
        /// Extract unsigned 64-bit integer from big-endian byte string
        /// </summary>
        /// <param name="bytes">Source big-endian byte string</param>
        /// <param name="offset">Location to extract from</param>
        /// <returns>Unsigned 64-bit integer</returns>
        public static UInt64 ToU64(byte[] bytes, int offset)
        {
            return (UInt64)(0
                | ((UInt64)(bytes[offset + 0]) << 56)
                | ((UInt64)(bytes[offset + 1]) << 48)
                | ((UInt64)(bytes[offset + 2]) << 40)
                | ((UInt64)(bytes[offset + 3]) << 32)
                | ((UInt64)(bytes[offset + 4]) << 24)
                | ((UInt64)(bytes[offset + 5]) << 16)
                | ((UInt64)(bytes[offset + 6]) << 8)
                | ((UInt64)(bytes[offset + 7]) << 0)
                );
        }

        #endregion

        /// <summary>
        /// Extract integer from big-endian byte string
        /// </summary>
        /// <param name="data">Source big-endian byte string</param>
        /// <param name="offset">Location to extract from</param>
        /// <returns>integer</returns>
        public static int GetU8(byte[] data,int offset)
        {
            return (int)data[offset] & 0xff;//(data[offset] << 0);
        }

        /// <summary>
        /// Extract integer from big-endian byte string
        /// </summary>
        /// <param name="data">Source big-endian byte string</param>
        /// <param name="offset">Location to extract from</param>
        /// <returns>integer</returns>
        public static int GetU24(byte[] data, int offset)
        {
            return (int)((data[offset] & 0xff) << 16)
              | (int)((data[offset + 1] & 0xff) << 8)
              | (int)((data[offset + 2] & 0xff) << 0);
        }

        /// <summary>
        /// convert a ushort array to byte array
        /// </summary>
        /// <param name="array">the input ushort array</param>
        /// <returns>the converted byte array</returns>
        public static byte[] ConvertFromUshortArray(ushort [] array)
        {
            List<byte> byteList = new List<byte>();
            foreach (ushort x in array)
            {
                byteList.AddRange(EncodeU16(x));
            }
            return byteList.ToArray();
        }

        /// <summary>
        /// Create bitmask with specified position and size; e.g., MakeBitMask(3, 2) returns 0x18
        /// </summary>
        /// <param name="offset">Offset of least-significant bit in mask</param>
        /// <param name="length">Number of bits in mask</param>
        /// <returns>Bitmask with 1's in specified positions</returns>
        public static UInt64 MakeBitMask(int offset, int length)
        {
            UInt64 mask;

            mask = 0;
            mask = ~mask;
            mask <<= length;
            mask = ~mask;

            mask <<= offset;

            return mask;
    }

        /// <summary>
        /// Get bits within an integer;  e.g., GetBits(0x1234, 4, 4) returns 0x3
        /// </summary>
        /// <param name="raw">Integer to extract bits from</param>
        /// <param name="offset">Offset of least-significant bit to modify</param>
        /// <param name="length">Number of bits to modify</param>
        /// <returns>Extracted bits</returns>
        public static UInt64 GetBits(UInt64 raw, int offset, int length)
        {
            UInt64 mask = MakeBitMask(offset, length);
            return (raw & mask) >> offset;
        }

        /// <summary>
        /// Set bits within an integer.
        /// </summary>
        /// <param name="raw">Integer to modify</param>
        /// <param name="offset">Offset of least-significant bit to modify</param>
        /// <param name="length">Number of bits to modify</param>
        /// <param name="value">Value to assign to target bits</param>
        /// <param name="desc">Human-readable description of bits (for error messages)</param>
        public static void SetBits(ref UInt64 raw, int offset, int length, UInt64 value, string desc)
        {
            UInt64 valueMask = MakeBitMask(0, length);
            if ((value & valueMask) != value)
            {
                throw new ArgumentOutOfRangeException(String.Format(
                    "{0} value does not fit within {1} bits: {2}",
                    desc, length, "0x" + value.ToString("X")
                    ));
            }
            UInt64 mask = MakeBitMask(offset, length);
            raw &= ~mask;
            raw |= value << offset;
        }

        /// <summary>
        /// Set bits within an integer.
        /// </summary>
        /// <param name="raw">Integer to modify</param>
        /// <param name="offset">Offset of least-significant bit to modify</param>
        /// <param name="length">Number of bits to modify</param>
        /// <param name="value">Value to assign to target bits</param>
        /// <param name="desc">Human-readable description of bits (for error messages)</param>
        public static void SetBits(ref UInt32 raw, int offset, int length, UInt32 value, string desc)
        {
            UInt64 raw64 = raw;
            SetBits(ref raw64, offset, length, value, desc);
            raw = (UInt32)raw64;
        }

        /// <summary>
        /// Set bits within an integer.
        /// </summary>
        /// <param name="raw">Integer to modify</param>
        /// <param name="offset">Offset of least-significant bit to modify</param>
        /// <param name="length">Number of bits to modify</param>
        /// <param name="value">Value to assign to target bits</param>
        /// <param name="desc">Human-readable description of bits (for error messages)</param>
        public static void SetBits(ref UInt16 raw, int offset, int length, UInt16 value, string desc)
        {
            UInt64 raw64 = raw;
            SetBits(ref raw64, offset, length, value, desc);
            raw = (UInt16)raw64;
        }
    }
}
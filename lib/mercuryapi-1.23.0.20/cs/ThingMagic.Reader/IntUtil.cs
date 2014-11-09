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
using System.Globalization;

namespace ThingMagic
{
    /// <summary>
    /// Integer conversion routines
    /// </summary>
    public static class IntUtil
    {
        #region StrToLong

        /// <summary>
        /// Parse string to integer, like C strtol function.
        /// </summary>
        /// <param name="str">Integer-representing string; e.g.,
        /// 256 (decimal),
        /// 0x400 (hexadecimal)</param>
        /// <returns>Integer represented by string</returns>

        public static long StrToLong(string str)
        {
            if (str.ToLower().StartsWith("0x"))
                return int.Parse(str.Substring(2), NumberStyles.AllowHexSpecifier);

            return int.Parse(str);
        }

        #endregion

    }
}

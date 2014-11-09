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
    /// Reader-related exceptions
    /// </summary>
    public class ReaderException : Exception
    {
        #region Construction

        /// <summary>
        /// Create reader-related exception
        /// </summary>
        /// <param name="message">Message explaining exception</param>
        public ReaderException(string message) : base(message) { }

        /// <summary>
        /// Create reader-related exception
        /// </summary>
        /// <param name="message">Message explaining exception</param>
        /// <param name="innerException">Underlying exception that triggered this exception</param>
        public ReaderException(string message, Exception innerException) : base(message, innerException) { }

        private TagReadData[] tagReads;

        /// <summary>
        /// Tag data
        /// </summary>
        public TagReadData[] TagReads
        {
            get { return tagReads; }
            set { tagReads = value; }
        }

        #endregion
    }
}

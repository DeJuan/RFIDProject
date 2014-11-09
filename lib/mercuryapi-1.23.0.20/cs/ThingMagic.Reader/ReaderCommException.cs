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

namespace ThingMagic
{
    /// <summary>
    /// Reader communication exception
    /// </summary>
    public class ReaderCommException : ReaderException
    {
        #region Fields
        
        private byte[] _readerMessage; 
        
        #endregion

        #region Properties

        /// <summary>
        /// Copy of raw message that triggered the error
        /// </summary>
        public byte[] ReaderMessage { get { return (byte[]) _readerMessage.Clone(); } } 

        #endregion

        #region Construction

        /// <summary>
        /// Create ReaderCommException with no underlying reader message
        /// </summary>
        /// <param name="message">Human-readable description of error</param>
        public ReaderCommException(string message)
            : this(message, new byte[0]) { }

        /// <summary>
        /// Create ReaderCommException
        /// </summary>
        /// <param name="message">Human-readable description of error</param>
        /// <param name="readerMessage">Copy of raw message that triggered the error</param>
        public ReaderCommException(string message, byte[] readerMessage)
            : base(message)
        {
            _readerMessage = (byte[]) readerMessage.Clone();
        }
        
        #endregion
    }
}
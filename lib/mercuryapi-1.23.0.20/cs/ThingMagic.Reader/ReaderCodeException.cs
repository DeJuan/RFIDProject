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
    /// Thrown when an error code is recieved from a reader device
    /// </summary>
    public class ReaderCodeException : ReaderException
    {
        #region Fields

        private int _code; 

        #endregion

        #region Properties

        /// <summary>
        /// ThingMagic fault code
        /// </summary>
        public int Code { get { return _code; } }
        
        #endregion

        #region Construction

        /// <summary>
        /// Create ReaderCodeException
        /// </summary>
        /// <param name="code">Numeric error code (ThingMagic fault code)</param>
        /// <param name="message">Human-readable description of error</param>
        public ReaderCodeException(string message, int code)
            : base(message)
        {
            _code = code;
        }

        #endregion        
    }
}
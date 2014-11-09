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
    // Transport-Layer Logging
    /// <summary>
    /// Listener gets notification of all messages going across the transport layer.
    /// </summary>
    public class TransportListenerEventArgs : EventArgs
    {
        #region Fields

        private bool   _tx      = false;
        private byte[] _data    = null;
        private int    _timeout = 0;

        #endregion

        #region Properties

        /// <summary>
        /// Message direction: True=host to reader, False=reader to host
        /// </summary>    
        public bool Tx
        {
            get { return _tx; }
        }

        /// <summary>
        /// Message contents, including framing and checksum bytes
        /// </summary>                
        public byte[] Data
        {
            get { return _data; }
        }
        /// <summary>
        /// Transport timeout setting (milliseconds) when message was sent or received
        /// </summary>            
        public int Timeout
        {
            get { return _timeout; }
        }

        #endregion

        #region Construction

        /// <summary>
        /// TransportListenerEventArgs constructor
        /// </summary>
        /// <param name="tx">if the listener is for tx or rx(true for tx)</param>
        /// <param name="data">the event data</param>
        /// <param name="timeout">the listen timeout</param>
        public TransportListenerEventArgs(bool tx, byte[] data, int timeout)
        {
            _tx      = tx;
            _data    = data;
            _timeout = timeout;            
        }

        #endregion
    }
}

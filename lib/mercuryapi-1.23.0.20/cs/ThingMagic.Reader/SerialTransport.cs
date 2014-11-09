/*
 * Copyright (c) 2013 ThingMagic.
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
    /// Serial transport
    /// </summary>
    public abstract class SerialTransport
    {
        /// <summary>
        /// Causes the communication interface to be opened but does not transmit any serial-layer data.
        /// This should perform actions such as opening a serial port device or establishing a network
        /// connection within a wrapper protocol.
        /// </summary>
        public abstract void Open();
        
        /// <summary>
        /// Send bytes down the serial transport layer. No interpretation or modification occurs.
        /// </summary>
        /// <param name="length">array containing the bytes to be sent</param>
        /// <param name="message">number of bytes to send</param>
        /// <param name="offset">position in array to send from</param>        
        public abstract void SendBytes(int length, byte[] message, int offset);    
        
        /// <summary>
        /// Receive a number of bytes on the serial transport. 
        /// </summary>
        /// <param name="length">number of bytes to receive</param>
        /// <param name="messageSpace">byte array to store the message in, or null to have one allocated</param>
        /// <param name="offset">location in messageSpace to store bytes</param>
        /// <returns>the byte array with the number of bytes added</returns>
        public abstract int ReceiveBytes(int length, byte[] messageSpace, int offset);
        
        /// <summary>
        /// Get/Set the current baud rate of the communication channel.
        /// </summary>
        public abstract int BaudRate
        {  get;  set;  }

        /// <summary>
        /// Get current status of the communication channel. True, if Open
        /// </summary>
        public abstract bool IsOpen
        { get; }

        /// <summary>
        /// Gets or sets the number of milliseconds before a time-out occurs when a read operation
        /// does not finish
        /// </summary>
        public abstract Int32 ReadTimeout
        { get;  set;  }

        /// <summary>
        /// Gets or sets the number of milliseconds before a time-out occurs when a write operation does not finish. 
        /// </summary>
        public abstract Int32 WriteTimeout
        { get;  set;  }

        /// <summary>
        /// Gets or sets the port for communications, including but not limited to all available COM ports.
        /// </summary>
        public abstract string PortName
        { get;  set;  }
        
        /// <summary>
        /// Take any actions necessary (possibly none) to remove unsent data from the output path.
        /// </summary>
        public abstract void Flush();
        
        /// <summary>
        /// Close the communication channel.
        /// </summary>
        public abstract void Shutdown();

    }
}

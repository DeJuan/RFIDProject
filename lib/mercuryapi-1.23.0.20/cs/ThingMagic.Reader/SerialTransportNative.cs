using System;
using System.Collections.Generic;
using System.Text;
using System.IO.Ports;

namespace ThingMagic
{
    /// <summary>
    /// Serial transport native framework calls
    /// </summary>
    public class SerialTransportNative : SerialTransport
    {
        private SerialPort serialPort = null;
        /// <summary>
        /// Serial Reader factory function
        /// </summary>
        /// <param name="uriString">URI-style path to serial device; e.g., /COM1</param>
	    public static SerialReader CreateSerialReader(String uriString)
	    {
	        SerialReader rdr = new SerialReader(uriString,
                new SerialTransportNative());
            return rdr;
	    }

        /// <summary>
        /// Constructor
        /// Initialize native serial port
        /// </summary>
        public SerialTransportNative()
        {
            serialPort = new SerialPort();
            // Enable the Data Terminal Ready (DTR) signal during
            // serial communication.
            serialPort.DtrEnable = true;
        }

        /// <summary>
        /// Set/Get serial baudrate
        /// </summary>
        public override int BaudRate
        {
            get
            {
                return serialPort.BaudRate;
            }
            set
            {
                serialPort.BaudRate = value;
            }
        }

        /// <summary>
        /// Discards data from the serial driver's receive buffer.
        /// </summary>
        public override void Flush()
        {
            serialPort.DiscardInBuffer();
        }

        /// <summary>
        /// Gets a value indicating the open or closed status of the SerialPort object.
        /// </summary>
        public override bool IsOpen
        {
            get
            {
                return serialPort.IsOpen;
            }
        }

        /// <summary>
        /// Opens a new serial port connection.
        /// </summary>
        public override void Open()
        {
            serialPort.Open();
        }

        /// <summary>
        /// Set/Get the port for communications, including but not limited to all available
        /// COM ports.
        /// </summary>
        public override string PortName
        {
            get
            {
                return serialPort.PortName;
            }
            set
            {
                serialPort.PortName = value;
            }
        }

        /// <summary>
        /// Gets or sets the number of milliseconds before a time-out occurs when
        /// a read operation does not finish.
        /// </summary>
        public override int ReadTimeout
        {
            get
            {
                return serialPort.ReadTimeout;
            }
            set
            {
                serialPort.ReadTimeout = value;
            }
        }

        /// <summary>
        /// Reads a number of bytes from the SerialPort input buffer and writes those 
        /// bytes into a byte array at the specified offset.
        /// </summary>
        /// <param name="length">The number of bytes to read.</param>
        /// <param name="messageSpace">The byte array to write the input to.</param>
        /// <param name="offset">The offset in the buffer array to begin writing.</param>
        /// <returns>The number of bytes read</returns>
        public override int ReceiveBytes(int length, byte[] messageSpace, int offset)
        {
            return serialPort.Read(messageSpace, offset, length);
        }

        /// <summary>
        /// Writes a specified number of bytes to the serial port using data from a buffer.
        /// </summary>
        /// <param name="length">The number of bytes to write.</param>
        /// <param name="message">The byte array that contains the data to write to the port.</param>
        /// <param name="offset">The zero-based byte offset in the buffer parameter at which to begin copying bytes to the port.</param>
        public override void SendBytes(int length, byte[] message, int offset)
        {
            serialPort.Write(message, offset, length);
        }

        /// <summary>
        /// Closes the port connection,
        /// </summary>
        public override void Shutdown()
        {
            serialPort.Close();
        }

        /// <summary>
        /// Gets or sets the number of milliseconds before a time-out occurs when
        /// a write operation does not finish.
        /// </summary>
        public override int WriteTimeout
        {
            get
            {
                return serialPort.WriteTimeout;
            }
            set
            {
                serialPort.WriteTimeout = value;
            }
        }
    }
}

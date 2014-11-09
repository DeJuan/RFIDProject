using System;
using System.Collections.Generic;
using System.Text;
using System.Net;
using System.Net.Sockets;

namespace ThingMagic
{
    /// <summary>
    /// Serial transport TCP framework calls
    /// </summary>
    public class SerialTransportTCP : SerialTransport
    {
        private TcpClient clientSocket = null;
        NetworkStream serverStream = null;
        /// <summary>
        /// host name,readerUri
        /// </summary>
        public string hostName = string.Empty, readerUri = string.Empty;
        /// <summary>
        ///Tcp port,Baudrate
        /// </summary>
        public int port, Baudrate;
        /// <summary>
        /// Serial Reader factory function
        /// </summary>
        /// <param name="uriString">URI-style path to serial device; e.g., /COM1</param>
	    public static SerialReader CreateSerialReader(String uriString)
	    {
	        SerialReader rdr = new SerialReader(uriString,
						    new SerialTransportTCP());
            return rdr;
	    }

        /// <summary>
        /// Constructor
        /// Initialize native serial Tcp port
        /// </summary>
        public SerialTransportTCP()
        {
            clientSocket = new TcpClient();
        }
        /// <summary>
        /// Set/Get TCP baudrate
        /// </summary>
        public override int BaudRate
        {
            get
            {
                return 0 ;
            }
            set
            {
                Baudrate = 0;
            }
        }

        /// <summary>
        /// Discards data from the serverstream
        /// </summary>
        public override void Flush()
        {   
            serverStream.Flush();
        }
        /// <summary>
        /// Gets a value indicating the open or closed status of the TCP Port object.
        /// </summary>
        public override bool IsOpen
        {
#if !WindowsCE
            get
            {
                return clientSocket.Connected;

            }
#endif
#if WindowsCE
            get
            {
                return false;
            }
#endif
        }

        /// <summary>
        /// Set/Get the port for communications, including but not limited to all available
        /// TCP ports.
        /// </summary>
        public override string PortName
        {
            get
            {
                return readerUri;
            }
            set
            {
               readerUri=value;
               Uri objUri = new Uri(readerUri);
               clientSocket.Connect(objUri.Host, objUri.Port);
               serverStream = clientSocket.GetStream();
            }
        }
        /// <summary>
        /// Opens a TCP connection.
        /// </summary>
        public override void Open()
        {
#if !WindowsCE
            if (!clientSocket.Connected)
#endif
            {
                 clientSocket.Connect(hostName,port);
                 serverStream = clientSocket.GetStream();
            }
        }
        /// <summary>
        /// Reads a number of bytes from the Tcp SerialPort input buffer and writes those 
        /// bytes into a byte array at the specified offset.
        /// </summary>
        /// <param name="length">The number of bytes to read.</param>
        /// <param name="messageSpace">The byte array to write the input to.</param>
        /// <param name="offset">The offset in the buffer array to begin writing.</param>
        /// <returns>The number of bytes read</returns>
         public override int ReceiveBytes(int length, byte[] messageSpace, int offset)
        {
            return serverStream.Read(messageSpace, offset, length);
        }
        /// <summary>
        /// Writes a specified number of bytes to the tcp serial port using data from a buffer.
        /// </summary>
        /// <param name="length">The number of bytes to write.</param>
        /// <param name="message">The byte array that contains the data to write to the port.</param>
        /// <param name="offset">The zero-based byte offset in the buffer parameter at which to begin copying bytes to the port.</param>
         public override void SendBytes(int length, byte[] message, int offset)
        {
            serverStream.Write(message, offset, length);
        }
        /// <summary>
        /// Closes the port connection,
        /// </summary>
         public override void Shutdown()
        {
            serverStream.Close();
            clientSocket.Close();
        }
        /// <summary>
        /// Gets or sets the number of milliseconds before a time-out occurs when
        /// a write operation does not finish.
        /// </summary>
        public override int WriteTimeout
        {
            get
            {
                return serverStream.WriteTimeout;
            }
            set
            {
                serverStream.WriteTimeout = value;
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
                return serverStream.ReadTimeout;
            }
            set
            {
                serverStream.ReadTimeout = value;
            }
        }
    }
}

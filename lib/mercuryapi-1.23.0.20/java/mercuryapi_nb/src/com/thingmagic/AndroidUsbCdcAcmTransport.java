/*
 * Copyright (c) 2014 ThingMagic, Inc.
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
package com.thingmagic;



import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * 
 */
public class AndroidUsbCdcAcmTransport implements SerialTransport {

    private AndroidUsbCdcAcmTransport.DeviceType mType;

    private static enum DeviceType {

        TYPE_AM, TYPE_2232C, TYPE_R, TYPE_2232H, TYPE_4232H;
    }
    private AndroidUsbReflection androidUSBReflection = null;
    private static final int DEFAULT_WRITE_BUFFER_SIZE = 256;
    private static final int DEFAULT_READ_BUFFER_SIZE = 256;
    private static final int USB_RECIP_DEVICE = 0x00;
    private static final int USB_ENDPOINT_OUT = 0x00;
    private static final int FTDI_DEVICE_OUT_REQTYPE = 64 | USB_RECIP_DEVICE | USB_ENDPOINT_OUT;
    private static final int SIO_SET_BAUD_RATE_REQUEST = 3;
    private static final int SIO_RESET_REQUEST = 0;
    private static final int SIO_RESET_SIO = 0;
    private static final int USB_WRITE_TIMEOUT_MILLIS = 5000;
    protected  Object mReadBufferLock;
    protected  Object mWriteBufferLock;
    private static Object readEndPoint;
    private static Object writeEndPoint;
    private int readEndPointAddress = 1;
    private int writeEndPointAddress = 0;
    private static Object usbDeviceConnection;
    private boolean opened = false;
    private byte opCode = (byte)0x03;
    static int countLine = 0;
    private boolean isDataExist = false;
    private int readBuffTail = 0;
    private int readBuffHead = 0;
    private byte[] readBuffer;
    private boolean isAsyncRead = false;
    byte[] mWriteBuffer;
    byte[] mReadBuffer;

  
    AndroidUsbCdcAcmTransport() 
    {
        androidUSBReflection = new AndroidUsbReflection();
        readBuffer = new byte[256*2];
        mWriteBuffer = new byte[DEFAULT_WRITE_BUFFER_SIZE];
        mReadBuffer = new byte[DEFAULT_READ_BUFFER_SIZE];
        mReadBufferLock = new Object();
        mWriteBufferLock = new Object();
    }

    public void open() throws ReaderException 
    {
        Object usbInterface = null;
        try 
        {
            if (!opened) 
             {
                usbDeviceConnection = androidUSBReflection.openDevice();
                //Get the usb interface to claim the device
                int count = androidUSBReflection.getInterfaceCount();
                for (int i = 0; i < count; i++)
                {
                    usbInterface = androidUSBReflection.getInterface(i);
                    boolean claim = androidUSBReflection.claimInterface(usbDeviceConnection, usbInterface);
                    if (!claim) 
                    {
                        throw new Exception("Unable to claim the device : ");
                    }
                }

                int result = androidUSBReflection.controlTransfer(usbDeviceConnection, FTDI_DEVICE_OUT_REQTYPE, SIO_RESET_REQUEST,
                        SIO_RESET_SIO, 0 /* index */, null, 0, USB_WRITE_TIMEOUT_MILLIS);//Resetting(flush) the usb communication channel
                if (result != 0)
                {
                    throw new IOException("Unable to reset : " + result);
                }
                
               
                //Read and write end points for opened port
                readEndPoint = androidUSBReflection.getReadEndPoint(usbInterface, readEndPointAddress);
                writeEndPoint = androidUSBReflection.getWriteEndPoint(usbInterface, writeEndPointAddress);
                opened = true;
            }
        }
        catch (Exception ex) 
        { 
            shutdown();
            throw new ReaderCommException("Couldn't open device");
        } 
        finally 
        {
            if (!opened) 
            {                
                shutdown();
            }
        }
    }

    public void sendBytes(int length, byte[] message, int offset, int timeoutMs) throws ReaderException
    {
        try
        {
           
            opCode =  message[2];
          
            final int writeLength;
            final int amtWritten;
            synchronized (mWriteBufferLock) 
            {
                final byte[] writeBuffer;
                 
                writeLength = Math.min(message.length - offset, mWriteBuffer.length);
                if (offset == 0)
                {
                    writeBuffer = message;
                } 
                else
                {
                    // bulkTransfer does not support offsets, make a copy.
                    System.arraycopy(message, 0, mWriteBuffer, 0, writeLength);
                    writeBuffer = mWriteBuffer;
                }
                // Performs a bulk transaction on the writeEndPoint
                amtWritten = androidUSBReflection.bulkTransfer(usbDeviceConnection, writeEndPoint, writeBuffer, length, timeoutMs);
                if (amtWritten <= 0)
                {
                    throw new IOException("Error writing " + writeLength
                                    + " bytes at offset " + offset + " length=" + message.length);
                }
                offset += amtWritten;
            }
            if(opCode == (byte)0x2f && message[5] == (byte)0x01)
            {
               isAsyncRead = true;
               readBuffTail = 0;
            }
        }
        catch (Exception ex) 
        {
            shutdown();
            throw new ReaderException("Operation timed out");
        }
    }

    public byte[] receiveBytes(int length, byte[] messageSpace, int offset, int timeoutMillis) throws ReaderException
    {
        try
        {
            if (messageSpace == null)
            {
                messageSpace = new byte[DEFAULT_READ_BUFFER_SIZE];
            }
            int numBytesRead = 0;
            if (isDataExist) 
            {                   
                if (readBuffTail >= length)
                {
                     messageSpace = copyRequestedData(length, messageSpace, offset);
                }
                else
                {
                    numBytesRead = performRead(mReadBuffer, length, offset, timeoutMillis);
                    if (numBytesRead > 0)
                    {
                        messageSpace = copyRequestedData(length, messageSpace, offset);
                    }
                }
            } 
            else
            {
                numBytesRead = performRead(mReadBuffer, length, offset, timeoutMillis);
                if (numBytesRead > 0) 
                {
                    messageSpace = copyRequestedData(length, messageSpace, offset);
                }
            }
            if (messageSpace[1] == length && (messageSpace[2]== (byte)0x2f && messageSpace[5]== (byte)0x02))
            {
                isAsyncRead = false;
                readBuffTail = 0;
            }
        }
        catch (Exception ex) 
        {
            if(ex.getMessage().equalsIgnoreCase("Timeout") && isAsyncRead)
             {
                 isAsyncRead = false;
                 shutdown();
                 throw new ReaderCommException("Connection lost");
             }
             else
             {
                 throw new ReaderCommException(ex.getMessage()); 
             }
        }
        return messageSpace;
    }

    public int performRead(byte[] mReadBuffer, int length, int offset, int timeoutMillis) throws ReaderCommException
    {
        int numBytesRead = 0;
        try 
        {
            int count = 0;
            final int readAmt = mReadBuffer.length;
            boolean isWaitForResponse = true;
            int internalTimeout = 0;
          
            while (isWaitForResponse) 
            {
              
                synchronized (mReadBufferLock) 
                {
                    // Performs a bulk transaction on the readEndPoint
                    numBytesRead = androidUSBReflection.bulkTransfer(usbDeviceConnection, readEndPoint, mReadBuffer, readAmt,
                            timeoutMillis);
                }  
                                  
               
                 // Wait for the response until given timeout or for data
                if (numBytesRead > 0 || internalTimeout >= timeoutMillis) 
                {
                    if(numBytesRead > 0)
                    {
                        System.arraycopy(mReadBuffer, 0, readBuffer, readBuffTail, numBytesRead);
                        readBuffTail += numBytesRead;
                        if(readBuffTail >= length)
                        {
                           isWaitForResponse = false;
                           break;
                        }
                    }
                    if(readBuffTail < length && internalTimeout >= timeoutMillis)
                    {
                        throw new ReaderCommException("Timeout");
                    }
                }
                else if(internalTimeout < timeoutMillis)
                {                    
                    Thread.sleep(1);
                    internalTimeout++;
                }
            }
        }        
        catch (Exception ex)
        {
            if(ex.getMessage().equalsIgnoreCase("Timeout"))
             {
                 throw new ReaderCommException("Timeout"); 
             }
             else
             {
                 throw new ReaderCommException(ex.getMessage()); 
             }
        }
        return numBytesRead;
    }

  

    public byte[] copyRequestedData(int length, byte[] messageSpace, int offset) 
    {
        try 
        {
            if (readBuffTail < length)
            {
                while (readBuffTail < length)
                {
                    performRead(mReadBuffer, length, offset, 1000);
                }
            }
            if (isAsyncRead) 
            {
                //Assuming that data is corrupted and eliminate the corrupted data
                 messageSpace = checkForCorruptedData(length, offset, messageSpace);
            } 
            else 
            {
                System.arraycopy(readBuffer, readBuffHead, messageSpace, offset, length);
            }
            readBuffTail -= length;

            if (readBuffTail > 0)
            {
                readBuffHead += length;
                isDataExist = true;
                System.arraycopy(readBuffer, readBuffHead, readBuffer, 0, readBuffTail);
                readBuffHead = 0;
            }
            else 
            {
                isDataExist = false;
                readBuffHead = 0;
                readBuffTail = 0;
            }
        }
        catch (Exception ex) 
        {
           Logger.getLogger(AndroidUsbCdcAcmTransport.class.getName()).log(Level.SEVERE, null, ex);
        }
        return messageSpace;
    }
    
    public byte[] checkForCorruptedData(int length, int offset, byte[] messageSpace)
    {
        try 
        {
            int corruptedBytes = 0;
            if (offset == 0)
            {
                int responseLen = readBuffer[1];
                if (responseLen == 0) 
                {
                    responseLen = length;
                }
                if (readBuffTail < responseLen) 
                {
                    performRead(mReadBuffer, length, offset, 1000);
                }
                for (byte b : readBuffer) 
                {
                    String data = String.format("%02x", b);
                    String data1 = String.format("%02x", readBuffer[corruptedBytes + 2]);
                    if (corruptedBytes == (responseLen + length))
                    {
                        System.arraycopy(readBuffer, readBuffHead, messageSpace, offset, length);
                        break;
                    }

                    if (corruptedBytes > 0 && (data.equalsIgnoreCase("ff") && data1.equalsIgnoreCase("22"))) 
                    {
                        readBuffTail -= corruptedBytes;
                        if(readBuffTail > 0)
                        {
                           System.arraycopy(readBuffer, corruptedBytes, readBuffer, readBuffHead, readBuffTail);
                           checkForCorruptedData(length, offset, messageSpace);
                        }
                        else
                        {
                          readBuffTail = 0;  
                        }
                        if (readBuffTail < length)
                        {
                            performRead(mReadBuffer, length, offset, 1000);
                            System.arraycopy(readBuffer, readBuffHead, messageSpace, 0, length + offset);
                            length += offset;
                        } 
                        else
                        {
                            System.arraycopy(readBuffer, readBuffHead, messageSpace, 0, length + offset);
                            length += offset;
                        }
                        break;
                    }

                    corruptedBytes++;
                }
            } 
            else 
            {
                System.arraycopy(readBuffer, readBuffHead, messageSpace, offset, length);
            }

        } 
        catch (Exception ex) 
        {
           Logger.getLogger(AndroidUsbCdcAcmTransport.class.getName()).log(Level.SEVERE, null, ex);
        }
        return messageSpace;
    }
    int baud = 115200;

    public int getBaudRate() throws ReaderException 
    {
        return baud;
    }

    private long[] convertBaudrate(int baudrate)
    {

        int divisor = 24000000 / baudrate;
        int bestDivisor = 0;
        int bestBaud = 0;
        int bestBaudDiff = 0;
        int fracCode[] = {
            0, 3, 2, 4, 1, 5, 6, 7
        };

        for (int i = 0; i < 2; i++) 
        {
            int tryDivisor = divisor + i;
            int baudEstimate;
            int baudDiff;

            if (tryDivisor <= 8)
            {
                // Round up to minimum supported divisor
                tryDivisor = 8;
            } 
            else if (mType != AndroidUsbCdcAcmTransport.DeviceType.TYPE_AM && tryDivisor < 12) 
            {
                // BM doesn't support divisors 9 through 11 inclusive
                tryDivisor = 12;
            } 
            else if (divisor < 16) 
            {
                // AM doesn't support divisors 9 through 15 inclusive
                tryDivisor = 16;
            } 
            else 
            {
                if (mType == AndroidUsbCdcAcmTransport.DeviceType.TYPE_AM) 
                {
                    // TODO
                } 
                else
                {
                    if (tryDivisor > 0x1FFFF) 
                    {
                        // Round down to maximum supported divisor value (for
                        // BM)
                        tryDivisor = 0x1FFFF;
                    }
                }
            }

            // Get estimated baud rate (to nearest integer)
            baudEstimate = (24000000 + (tryDivisor / 2)) / tryDivisor;

            // Get absolute difference from requested baud rate
            if (baudEstimate < baudrate) {
                baudDiff = baudrate - baudEstimate;
            } else {
                baudDiff = baudEstimate - baudrate;
            }

            if (i == 0 || baudDiff < bestBaudDiff)
            {
                // Closest to requested baud rate so far
                bestDivisor = tryDivisor;
                bestBaud = baudEstimate;
                bestBaudDiff = baudDiff;
                if (baudDiff == 0) 
                {
                    // Spot on! No point trying
                    break;
                }
            }
        }

        // Encode the best divisor value
        long encodedDivisor = (bestDivisor >> 3) | (fracCode[bestDivisor & 7] << 14);
        // Deal with special cases for encoded value
        if (encodedDivisor == 1) 
        {
            encodedDivisor = 0; // 3000000 baud
        } 
        else if (encodedDivisor == 0x4001)
        {
            encodedDivisor = 1; // 2000000 baud (BM only)
        }

        // Split into "value" and "index" values
        long value = encodedDivisor & 0xFFFF;
        long index;
        if (mType == AndroidUsbCdcAcmTransport.DeviceType.TYPE_2232C || mType == AndroidUsbCdcAcmTransport.DeviceType.TYPE_2232H
                || mType == AndroidUsbCdcAcmTransport.DeviceType.TYPE_4232H) 
        {
            index = (encodedDivisor >> 8) & 0xffff;
            index &= 0xFF00;
            index |= 0 /* TODO mIndex */;
        } 
        else 
        {
            index = (encodedDivisor >> 16) & 0xffff;
        }

        // Return the nearest baud rate
        return new long[]{
                    bestBaud, index, value
                };
    }

    public void setBaudRate(int baudRate) throws ReaderException 
    {
        baudRate = 115200;
        mType = AndroidUsbCdcAcmTransport.DeviceType.TYPE_R;
        long[] vals = convertBaudrate(baudRate);
        long index = vals[1];
        long value = vals[2];
        try 
        {
            int result = androidUSBReflection.controlTransfer(usbDeviceConnection, FTDI_DEVICE_OUT_REQTYPE,
                    SIO_SET_BAUD_RATE_REQUEST, (int) value, (int) index,
                    null, 0, USB_WRITE_TIMEOUT_MILLIS);
            if (result != 0) 
            {
                throw new IOException("Setting baudrate failed: result=" + result);
            }
        } 
        catch (IOException ex) 
        {
            Logger.getLogger(AndroidUSBTransport.class.getName()).log(Level.SEVERE, null, ex);
        }

    }

    public void flush() throws ReaderException 
    {
        try 
        {
            int result = androidUSBReflection.controlTransfer(usbDeviceConnection, FTDI_DEVICE_OUT_REQTYPE, SIO_RESET_REQUEST,
                    SIO_RESET_SIO, 0 /* index */, null, 0, USB_WRITE_TIMEOUT_MILLIS);
            if (result != 0) 
            {
                throw new IOException("Unable to reset : " + result);
            }
        } 
        catch (Exception ex) 
        {
        }
    }

    public void shutdown() throws ReaderException 
    {
        try
        {
            if(usbDeviceConnection != null)
            {
                androidUSBReflection.closeConnection(usbDeviceConnection);
            }
        }
        catch (Exception ex) 
        {
        }
    }
    
    public SerialReader createSerialReader(String uri) throws ReaderException 
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }
}

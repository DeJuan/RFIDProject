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
import java.io.PipedInputStream;
import java.io.PipedOutputStream;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * 
 */
public class AndroidUSBTransport implements SerialTransport 
{

    private AndroidUsbReflection androidUSBReflection = null;
    private static final int DEFAULT_READ_BUFFER_SIZE = 256;
    private static final int DEFAULT_WRITE_BUFFER_SIZE = 256;
    protected  Object mWriteBufferLock;
    private byte[] readBuffer;
    private byte[] writeBuffer;
    private static PipedInputStream iStream = null;
    private static PipedOutputStream oStream = null;
    private static final int    READ_WAIT_MS = 10;
    private boolean readThreadStop = true;
    private boolean isContinuous = false;
    private byte opCode = (byte)0x03;
    ConcurrentLinkedQueue< byte[]> queue = null;

   
    AndroidUSBTransport() 
    {
       //default constructor
        androidUSBReflection = new AndroidUsbReflection();
        readBuffer = new byte[DEFAULT_READ_BUFFER_SIZE ];
        writeBuffer = new byte[DEFAULT_WRITE_BUFFER_SIZE];
        mWriteBufferLock = new Object();
    }

    public void open() throws ReaderException {

        try 
        {
           boolean isOpen = androidUSBReflection.isOpen();
           if(!isOpen)
           {
               throw new ReaderException("Couldn't open device");
           }
           if(!readThreadStop)
           {
               stopRead();
           }
           flush();
        }
        catch (Exception ex)
        {
            shutdown();
            throw new ReaderException("Couldn't open device");
        }        
    }

    public void sendBytes(int length, byte[] message, int offset, int timeoutMs) throws ReaderException 
    {
        try
        {
            int writeSize;
            int writtenSize;  
            opCode = message[2];

            while (offset < length) 
            {
                writeSize = length;

                if (offset + writeSize > length) 
                {
                    writeSize = length - offset;
                }
                System.arraycopy(message, offset, writeBuffer, 0, writeSize);
                synchronized (mWriteBufferLock)
                {
                    writtenSize = androidUSBReflection.write(writeBuffer, writeSize);
                }
                
                if (writtenSize < 0) 
                {
                   
                    throw new IOException("Error writing " + length
                            + " bytes at offset " + offset + " length=" + message.length);
                }
                offset += writtenSize;
            }
            if(opCode == (byte)0x2f && message[5] == (byte)0x01)
            {
                readThreadStop = true;
                queue = new ConcurrentLinkedQueue<byte []>();
                oStream = new PipedOutputStream();
                iStream = new PipedInputStream(oStream);
                startRead();
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

            boolean isWaitForResponse = true;
            int internalTimeout = timeoutMillis;
            if (!isContinuous)
            {
                while (isWaitForResponse)
                {
                    int len = androidUSBReflection.getstatusQ();
                    if (len >=length || internalTimeout <= 0) 
                    {
                        if(len > 0)
                        {
                            androidUSBReflection.read(readBuffer, length, timeoutMillis);
                            System.arraycopy(readBuffer, 0, messageSpace, offset, length);
                        }
                        if(internalTimeout <= 0 && len < length)
                        {
                            throw new ReaderCommException("Timeout");
                        }                       
                        break;
                    } 
                    else 
                    {
                        Thread.sleep(25);
                        internalTimeout -= 25;
                    }
                }
            } 
            else
            {
               messageSpace = readContinuous(length, messageSpace, offset, timeoutMillis);
            }
            if (messageSpace[1] == length && ( messageSpace[2] == (byte)0x2f
                    &&  messageSpace[5] == (byte)0x02)) 
            {               
                stopRead();
            }
        } 
        catch (Exception ex)
        {            
           shutdown();
           if(ex.getMessage().equalsIgnoreCase("Timeout") && isContinuous)
            {             
                stopRead();
                throw new ReaderCommException(String.format("Connection lost"));
            }
            else
            {
                throw new ReaderCommException(ex.getMessage());
            }
        }
       
        return messageSpace;
    }

    public byte[] readContinuous(int length, byte[] messageSpace, int offset, int timeout) throws ReaderCommException
    {
       try
        {
            boolean isWaitForResponse = true;
            int internalTimeout = 0;
            while (isWaitForResponse)
            {
                if (iStream.available() >= length || internalTimeout >= timeout) 
                {
                    if (iStream.available() >= length)
                    {
                      iStream.read(messageSpace, offset, length);
                    }
                    else if(iStream.available() < length && internalTimeout >= timeout)
                    {
                       throw new ReaderCommException("Timeout");
                    }                    
                    break;
                }
                else if(internalTimeout < timeout)
                {
                   Thread.sleep(1); 
                   internalTimeout++;
                }                           
            }   
        }
        catch(Exception ex)
        {
           throw new ReaderCommException("Timeout"); 
        }
        return messageSpace;
    }
  
    private void stopRead() 
    {
        readThreadStop = true;  
        isContinuous = false;   
       
    }


    private void startRead() 
    {
        if(readThreadStop) 
        {
            readThreadStop = false; 
            isContinuous = true;
            new Thread(readThread).start();
            new Thread(dataThread).start();
        }
    }
   
    private Runnable readThread = new Runnable() {
        @Override
        public void run() 
        {
            int len = 0;
            byte[] readBuff;

            for (;;) 
            {// this is the main loop for transferring
                len = androidUSBReflection.getstatusQ();
                if (len > 0) 
                {
                    readBuff = new byte[len];
                    try 
                    {
                        len = androidUSBReflection.read(readBuff, len, READ_WAIT_MS); // You might want to set wait ms.
                    } catch (Exception ex)
                    {
                    }
                    queue.add(readBuff);
                }
                if (readThreadStop)
                {
                    return;
                }
            }
        } // end of run()
    }; // end of runnable
   
    private Runnable dataThread = new Runnable()
    {
        @Override
        public void run() {
            for (;;) 
            {
                try 
                {
                    if (queue.size() > 0)
                    {
                        byte[] buff = queue.poll();
                        oStream.write(buff);
                    }
                    else
                    {
                        Thread.sleep(100);
                    }
                } 
                catch (Exception ex)
                {
                }
                if (readThreadStop)
                {
                    return;
                }
            }
        }//end of run
    };// end of runnable
    
    int baud = 115200;

    public int getBaudRate() throws ReaderException 
    {
        return baud;
    }

    public void setBaudRate(int baudRate) throws ReaderException 
    {
        boolean isBaudSet = false;  
        try 
        {
            androidUSBReflection.setBaudRate(115200);
            if(isBaudSet)
            {
               baud =  baudRate;
            }
        }
        catch (Exception ex)
        {
           Logger.getLogger(AndroidUSBTransport.class.getName()).log(Level.SEVERE, null, ex); 
        }
    }

    public void flush() throws ReaderException
    {
        try 
        {
            androidUSBReflection.reSet();
        } 
        catch (Exception ex) 
        {
            Logger.getLogger(AndroidUSBTransport.class.getName()).log(Level.SEVERE, null, ex); 
        }
    }

    public void shutdown() throws ReaderException 
    {
        try 
        {
            androidUSBReflection.close();

        } 
        catch (Exception ex)
        {
            Logger.getLogger(AndroidUSBTransport.class.getName()).log(Level.SEVERE, null, ex); 
        }
    }
    
    public SerialReader createSerialReader(String uri) throws ReaderException 
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }
}

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

import java.io.*;
import java.net.*;
import java.util.logging.Level;
import java.util.logging.Logger;
/**
 *
 * 
 */
public class SerialTransportTCP implements SerialTransport
{

   Socket tcpSocket = null;
   OutputStream outputStream = null;
   InputStream inputStream = null;
   static String tcpURI;
   
  public SerialTransportTCP()
   {
      //default constructor  
   }
    
    public void open() throws ReaderException 
    {
        try
        {
            if(tcpSocket == null)
            {
                URI uri = new URI(tcpURI);
                tcpSocket = new Socket(uri.getHost(),uri.getPort());
                outputStream = tcpSocket.getOutputStream();
                inputStream = tcpSocket.getInputStream();
            }
        }
        catch(Exception ex)
        {
            throw new ReaderCommException(ex.getMessage());
        }
    }

    public void sendBytes(int length, byte[] message, int offset, int timeoutMs) throws ReaderException {
       try
        {
            if(outputStream == null)
            {
               throw new ReaderException("TCP Connection lost");
            }
            outputStream.write(message, offset, length);
        }
        catch(Exception ex)
        {
          throw new ReaderCommException(ex.getMessage());
        }
    }

    public byte[] receiveBytes(int length, byte[] messageSpace, int offset, int timeoutMillis) throws ReaderException {
       
        try
        {
            if(inputStream == null)
            {
               throw new IOException("TCP Connection lost");
            }
            int responseWaitTime = 0;
            while (inputStream.available() < length && responseWaitTime <timeoutMillis)
            {
                Thread.sleep(10);
                // Repeat the loop for every 10 milli sec untill we receive required
                // data
                responseWaitTime+=10;
            }
            if (inputStream.available() <= 0)
            {
                throw new IOException("Timeout");
            }
            inputStream.read(messageSpace, offset, length);
        }
        catch(Exception ex)
        {
          throw new ReaderCommException(ex.getMessage());
        }
        return messageSpace;
    }

    public int getBaudRate() throws ReaderException
    {
        return 0;
    }

    public void setBaudRate(int rate) throws ReaderException
    {
       //TCP socket does not support baudrates, ignore setBaudRate.
    }

    public void flush() throws ReaderException
    {
        try
        {
            outputStream.flush();
        }
        catch(Exception ex)
        {
           throw new ReaderCommException(ex.getMessage());
        }
    }

    public void shutdown() throws ReaderException {
        try
        {
            tcpSocket.close();
        }
        catch (IOException ex) 
        {
            Logger.getLogger(SerialTransportTCP.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    static public class Factory implements ReaderFactory
    {
        public SerialReader createReader(String uri) throws ReaderException 
        {
            tcpURI = uri;
            return new SerialReader(uri, new SerialTransportTCP());
        }        
    }
}

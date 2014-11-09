/*
 * Copyright (c) 2013 ThingMagic, Inc.
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
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;

/**
 *
 * @author qvantel
 */
public class BluetoothTransportBluecove implements SerialTransport{

    private static OutputStream btOutputStream = null;
    private static InputStream btInputStream = null;
    private boolean opened = false;
    private String deviceName;
    
 /** Creates a new instance of SerialPort through bluetooth */
    public BluetoothTransportBluecove(String deviceName)
    {
        if (deviceName.startsWith("/")) {
            this.deviceName = deviceName.substring(1);
        } else {
            this.deviceName = deviceName;
        }
    }
    public void open()
    {
        try
        {
            if(!opened)
            {
                String serverUrl="btspp://"+deviceName+":1;authenticate=false;encrypt=false;master=true";
                System.out.println("serverUrl :"+serverUrl);
                Class connectorClass;
                Class outputConnectorClass;
                Class inputConnectorClass;
                try
                {
                    connectorClass = Class.forName("javax.microedition.io.Connector");
                    outputConnectorClass = Class.forName("javax.microedition.io.OutputConnection");
                    inputConnectorClass = Class.forName("javax.microedition.io.InputConnection");                    
                }
                catch (ClassNotFoundException cnf)
                {
                    throw new Exception("Bluecove jar is not available in classpath");
                }

                Object connectorObj = BluecoveBluetoothReflection.connectToBluetoothSocket(connectorClass, serverUrl);
                btOutputStream=BluecoveBluetoothReflection.getOutputStream(outputConnectorClass, connectorObj);
                btInputStream=BluecoveBluetoothReflection.getInputStream(inputConnectorClass, connectorObj);
                opened = true;
            }

        } catch (Exception ex) {
            ex.printStackTrace();
              //  throw new Exception(ex.getMessage());
        }
    }

    public void shutdown() {
        try
        {
            btOutputStream.close();
            btInputStream.close();
            //conn.close();
            opened = false;
        }
        catch (IOException Ioe)
        {
            Ioe.printStackTrace();
        }
    }

    public void flush()
    {
        try
        {
            btOutputStream.flush();
        }
        catch (IOException Ioe)
        {
            Ioe.printStackTrace();
        }
    }

    int rate = 9600;

    public void setBaudRate(int rate) {
        // nativeSetBaudRate(rate);
    }

    public int getBaudRate() {
        return rate;
    }

    public void sendBytes(int length, byte[] message, int offset, int timeoutMs)
                    throws ReaderException
    {
        try
        {
            btOutputStream.write(message, offset, length);
        } catch (IOException Ioe) {
            throw new ReaderException(Ioe.getMessage());
        }
    }

    public byte[] receiveBytes(int length, byte[] messageSpace, int offset,
                    int timeoutMs) throws ReaderException
    {
        if (messageSpace == null)
        {
            messageSpace = new byte[length + offset];
        }
        try
        {
            int responseWaitTime=0;
            while(btInputStream.available()<=0 && responseWaitTime < 2000 )
            {
                 Thread.sleep(1);
                // Repeat the loop for every 1 sec untill we received response.
                responseWaitTime++;
            }
            
            if(btInputStream.available()<=0)
            {
                shutdown();
                throw new Exception("No Response from reader.");
            }
            while( btInputStream.available() < length )
            {
                Thread.sleep(1);
                // Repeat the loop for every 1 sec untill we received required data
            }
            btInputStream.read(messageSpace, offset, length);
         

        } catch (Exception ex) {
            ex.printStackTrace();;
            throw new ReaderException(ex.getMessage());
        }
        return messageSpace;
    }
    
   public SerialReader createSerialReader(String uri) throws ReaderException 
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }
}

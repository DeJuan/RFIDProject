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

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.lang.reflect.Method;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.Enumeration;
import java.util.jar.JarEntry;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

public class BluetoothTransportNative implements SerialTransport {

    private String deviceName;
    static Object classInstance = null;
    static Class<?> cls = null;
    Method method = null;

    static
    {
        load();
    }

    private static void load()
    {
        try
        {
            String javaRunTime = System.getProperty("java.runtime.name");
            String filePath=null;
            if(javaRunTime.equalsIgnoreCase("Android Runtime"))
            {
                filePath=BluetoothTransportNative.class.getResource("BluetoothAndroid.jar").getPath();
                String apkPath=filePath.replace("file:", "").split("!")[0];
                filePath=ExtarctBluetoothJarToLocal(apkPath);
            }
            else
            {
                filePath=BluetoothTransportNative.class.getResource("BluetoothWindows.jar").getPath();
            }
            File file = new File(filePath);
            if(!file.isFile())
            {
                throw new ReaderException("Bluetooth supporting jars not found");
            }
            URL url = file.toURI().toURL();
            URL[] urls = new URL[]{url};
            ClassLoader cl = new URLClassLoader(urls, BluetoothTransportNative.class.getClassLoader());
            cls = cl.loadClass("Bluetooth.BluetoothTransportImpl");
            classInstance = cls.newInstance();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

     private static String ExtarctBluetoothJarToLocal(String apkPath) throws Exception
     {
        String tempJarPath="";
        try
        {
            ZipFile apk = new ZipFile(apkPath);
            Enumeration apkEnum = apk.entries();
            ZipEntry ze = null;
            while (apkEnum.hasMoreElements())
            {
               ze = (ZipEntry) apkEnum.nextElement();
               if (ze.getName().equals("com/thingmagic/BluetoothAndroid.jar"))
               {
                   JarEntry orgJar = new JarEntry(ze);
                   File tempJar = File.createTempFile("BluetoothAndroid",".jar");
                   tempJar.deleteOnExit();
                   tempJarPath=tempJar.getPath();

                   InputStream is = apk.getInputStream(orgJar); // get the input stream
                   FileOutputStream fos = new FileOutputStream(tempJar);

                   while (is.available() > 0)
                   {  // write contents of 'is' to 'fos'
                        fos.write(is.read());
                   }
                   fos.close();
                   is.close();
               }
            }
 
        } catch (Exception ex) {
            ex.printStackTrace();
            throw ex;
        }
        return tempJarPath;
    }
     
    
    /** Creates a new instance of SerialPort through bluetooth */
    public BluetoothTransportNative(String deviceName)
    {
        if (deviceName.startsWith("/")) {
            this.deviceName = deviceName.substring(1);
        } else {
            this.deviceName = deviceName;
        }
    }

    public void open() throws ReaderException
    {
        try
        {
            method = cls.getDeclaredMethod("open", String.class);
            method.invoke(classInstance, deviceName);
        } catch (Exception ex) {
            ex.printStackTrace();
            shutdown();
            throw new ReaderException("Failed to connect " + ex.getMessage());
        }
    }

    /**
     * Disconnects from the connected Bluetooth device.
     */
    public void shutdown()
    {
        try {
            method = cls.getDeclaredMethod("shutdown", null);
            method.invoke(classInstance, null);
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    public void flush()
    {
        try {
            method = cls.getDeclaredMethod("flush", null);
            method.invoke(classInstance, null);
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }
    int rate = 9600;

    public void setBaudRate(int rate) {
      // Baudrate was set to bluetooth dongle
    }

    public int getBaudRate() {
        //return Default baudrate
        return rate;
    }

    public void sendBytes(int length, byte[] message, int offset, int timeoutMs)
            throws ReaderException
    {
        try
        {
            Object[] args = new Object[4];
            args[0] = length;
            args[1] = message;
            args[2] = offset;
            args[3] = timeoutMs;
            Class[] classes = null;
            if (args != null) {
                classes = new Class[args.length];
                for (int i = 0; i < args.length; ++i)
                {
                    classes[i] = args[i].getClass();
                }
            }

            method = cls.getDeclaredMethod("sendBytes", classes);
            method.invoke(classInstance, args);
        } catch (Exception ex) {
            throw new ReaderException(ex.getMessage());
        }
    }

    public byte[] receiveBytes(int length, byte[] messageSpace, int offset,
            int timeoutMs) throws ReaderException
    {
        try
        {
            Object[] args = new Object[4];
            args[0] = length;
            args[1] = messageSpace;
            args[2] = offset;
            args[3] = timeoutMs;
            Class[] classes = null;
            if (args != null) {
                classes = new Class[args.length];
                for (int i = 0; i < args.length; ++i)
                {
                    classes[i] = args[i].getClass();
                }
            }

            method = cls.getDeclaredMethod("receiveBytes", classes);
            messageSpace = (byte[]) method.invoke(classInstance, args);

        } catch (Exception ex) {
            ex.printStackTrace();
            throw new ReaderException(ex.getMessage());
        }
        return messageSpace;
    }
    
    public SerialReader createSerialReader(String uri) throws ReaderException 
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }
}

/*
 * Copyright (c) 2008 ThingMagic, Inc.
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

/*
 * For the load() method, which is derived from sqlitejdbc-v056:
 * 
 * Copyright (c) 2007 David Crawshaw <david@zentus.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

package com.thingmagic;

import java.io.File;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.URI;


class SerialTransportNative implements SerialTransport
{

  private static native int nativeInit();
  private native int nativeCreate(String port);
  private native int nativeOpen();
  private native int nativeSendBytes(int length, byte[] message, int offset,
                                     int timeoutMs);
  private native int nativeReceiveBytes(int length, byte[] message, int offset,
                                        int timeoutMs);
  private native int nativeSetBaudRate(int rate);
  private native int nativeGetBaudRate();
  private native int nativeShutdown();
  private native int nativeFlush();

  static
  {
    load();
   nativeInit();
  }
  
  SerialTransportNative() 
  {
     //default constructor
  }

  private static void load()
  {
   
    String libpath = System.getProperty("com.thingmagic.seriallib.path");
    String libname = System.getProperty("com.thingmagic.seriallib.name");
   
    if (libname == null) {
                libname = System.mapLibraryName("SerialTransportNative");
        }
    // guess what a bundled library would be called
    String osname = System.getProperty("os.name").toLowerCase();
    String osarch = System.getProperty("os.arch");
    if (osname.startsWith("mac os"))
    {
      osname = "mac";
      osarch = "universal";
    }
    if (osname.startsWith("windows")) 
    {
      osname = "win";
      if (osarch.startsWith("a") && osarch.endsWith("64"))
      {
        osarch = "x64";
      }
    }
    if (osarch.startsWith("i") && osarch.endsWith("86"))
    {
      osarch = "x86";
    }
    libname = osname + '-' + osarch + ".lib";
    // try a bundled library
    try
    {
      InputStream in = SerialTransportNative.class.getResourceAsStream(libname);
      if (in == null)
      {
        throw new RuntimeException("libname: "+libname+" not found");
      }
    File tmplib = File.createTempFile("libtmserialport", ".lib");
      tmplib.deleteOnExit();
   
      OutputStream out = new FileOutputStream(tmplib);
      byte[] buf = new byte[1024];
      for (int len; (len = in.read(buf)) != -1;)
      {
         
        out.write(buf, 0, len);
      }
      in.close();
      out.close();     

     System.load(tmplib.getAbsolutePath());
    }
    catch (IOException e)
    {
       
      throw new RuntimeException("Error loading " + libname);
    }
  }

    
  /* Used by the native code to point to its state object. Don't touch. */
  public long transportPtr; 

  private boolean opened=false;
  private String deviceName;

  /** Creates a new instance of SerialPort */
  public SerialTransportNative(String deviceName) 
  {
    nativeCreate(sanitizeComPortName(deviceName));
  }
    
  /** The Win32 API is inconsistent about serial port naming.
   * Technically, they should all be of the form \\.\COMn,
   * but for single-digit numbers, it lets you get away with just COMn.
   * Cover up the difference by forcing \\.\ prefix on all COM port names.
   *
   * @param portName Serial port name
   * @return If portName starts with COM or /COM, returns modified name of \\.\COM..., else returns portName unchanged.
   */
  private String sanitizeComPortName(String portName) {
    if (portName.toUpperCase().startsWith("COM"))
    {
      portName = "\\\\.\\" + portName;
    } else if (portName.toUpperCase().startsWith("/COM"))
    {
      portName = "\\\\.\\" + portName.substring(1);
    }
    return portName;
  }

  public void open()
    throws ReaderException
  {
    if (0 != nativeOpen())
    {
      throw new ReaderCommException("Couldn't open device");
    }
  }
     

  public void shutdown()
  {
    nativeShutdown();
  }

  public void flush() {
    if (!opened)
    {
      return;
    }

    nativeFlush();
  }

  int rate;
  public void setBaudRate(int rate)
  {
    nativeSetBaudRate(rate);
  }

  public int getBaudRate()
  {
    return nativeGetBaudRate();
  }

  public void sendBytes(int length, byte[] message, int offset, int timeoutMs)
    throws ReaderException
  {
    int ret;

    ret = nativeSendBytes(length, message, offset, timeoutMs);
    if (0 != ret)
    {
      throw new ReaderCommException("Serial error");
    }
  }

    
  public byte[] receiveBytes(int length, byte[] messageSpace, int offset, int timeoutMs)
    throws ReaderException
  {
    int ret;

    if (messageSpace == null)
    {
      messageSpace = new byte[length + offset];
    }

    ret = nativeReceiveBytes(length, messageSpace, offset, timeoutMs);

       return messageSpace;
  }

  static class Factory implements ReaderFactory
  { 

      public SerialReader createReader(String uriString) throws ReaderException
      {
          String readerUri = null;
          try 
          {
              URI uri = new URI(uriString);
              readerUri = uri.getPath();
          } 
          catch (Exception ex) 
          {
              
          }
          return new SerialReader(readerUri, new SerialTransportNative(readerUri));
      }
    }


}

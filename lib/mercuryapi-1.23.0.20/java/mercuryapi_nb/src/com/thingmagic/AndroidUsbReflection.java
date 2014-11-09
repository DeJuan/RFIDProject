/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.thingmagic;

import java.io.IOException;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Provides access to Android USB classes via Java reflection.
 * To simplify the build and reduce the size of our distribution by not making a hard link to Android.
 */

public class AndroidUsbReflection 
{

    private static Class usbRequest;
    private static Object usbManager;
    private static Object usbDevice;
    private static Constructor conStructor;
    private static Object instance;
    public static Map deviceList;
    public static int deviceClass;
    public static Object ftDev;
    

    public AndroidUsbReflection(Object usbmanager,Object ftdev, Object usbdevice, int deviceClass)
    {
        this.usbManager = usbmanager;
        this.usbDevice = usbdevice;
        this.deviceClass = deviceClass;
        this.ftDev = ftdev;
    }

    public AndroidUsbReflection(Map devices) 
    {
       deviceList = devices;
    }
    public static int getDeviceClass()
    {
         return deviceClass;
    }
  
    AndroidUsbReflection()
    {
        //Default Constructor
    }

    public HashMap<String, Object> getUSBDeviceList() throws NoSuchFieldException 
    {
        HashMap<String, Object> deviceList = new HashMap<String, Object>();

        try 
        {

            deviceList = (HashMap<String, Object>) invokeMethodThrowsIOException(
                    getMethod(usbManager.getClass(), "getDeviceList"),
                    usbManager);

        }
        catch (Exception ex)
        {
           Logger.getLogger(AndroidUsbReflection.class.getName()).log(Level.SEVERE, null, ex);
        }
        return deviceList;
    }
    //FTDI device driver calls http://www.ftdichip.com/Drivers/D2XX.htm
     /**
     * Invokes the method
     * {@link  com.ftdi.j2xx.FT_Device #getQueueStatus}.
     *
     * @return the number of bytes available to read from the driver Rx buffer.
     *
     */
    public static int getstatusQ()
    {
          return (Integer) invokeMethod(getMethod(ftDev.getClass(), "getQueueStatus"), ftDev);
    }
    
     /**
     * Invokes the method
     * {@link  com.ftdi.j2xx.FT_Device #read}.
     *
     * @ returns the number of bytes successfully read from the device.
     *
     */
    public static int read(byte[] buf, int len, long timeout) throws IOException
    {   
        int readLen =0;
        try{
            readLen = (Integer)invokeMethod(getMethod(ftDev.getClass(), "read", new Class[]{byte[].class, int.class, long.class}),
                ftDev, buf, Integer.valueOf(len), Long.valueOf(timeout));
            
        }
        catch(Exception ex)
        {
           Logger.getLogger(AndroidUsbReflection.class.getName()).log(Level.SEVERE, null, ex);
        }        
        return readLen;
    }
    
     /**
     * Invokes the method
     * {@link  com.ftdi.j2xx.FT_Device #write}.
     *
     * @writes data to the device from the application buffer.
     *
     */
    public static int write(byte[] buf, int len) throws IOException
    {
    
        return(Integer) invokeMethodThrowsIOException(getMethod(ftDev.getClass(), "write", new Class[]{ byte[].class, int.class}),
                ftDev,  buf, Integer.valueOf(len));
        
    }
    
     /**
     * Invokes the method
     * {@link  com.ftdi.j2xx.FT_Device #close}.
     *
     * @closes the device
     *
     */
    public static void close() 
    { 
        try
        {
            if(ftDev != null)
            {
                synchronized (ftDev) 
                {
                    invokeMethod(getMethod(ftDev.getClass(), "close"), ftDev);
                }
            }
        } 
        catch (Exception ex)
        {
          Logger.getLogger(AndroidUsbReflection.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
    
     /**
     * Invokes the method
     * {@link  com.ftdi.j2xx.FT_Device #resetDevice}.
     *
     * @sends a vendor command to the device to cause a reset and flush any data from the device buffers.
     *
     */
    public static void reSet()
    {
        
        invokeMethod(getMethod(ftDev.getClass(), "resetDevice"),
                ftDev);
    }
    
     /**
     * Invokes the method
     * {@link  com.ftdi.j2xx.FT_Device #isOpen}.
     *
     * @return the open status of the device
     *
     */
    public static boolean isOpen()
    {
        
        return(Boolean) invokeMethod(getMethod(ftDev.getClass(), "isOpen"),
                ftDev);
    }
    
    
     /**
     * Invokes the method
     * {@link  com.ftdi.j2xx.FT_Device #getQueueStatus}.
     *
     * @sends a vendor command to the device to change the baud rate generator value.
     *
     */
    public static boolean setBaudRate(int baudRate)
    {
       return(Boolean) invokeMethod(getMethod(ftDev.getClass(), "setBaudRate", new Class[]{int.class}),
                ftDev, baudRate);
    }

    /**
     * Invokes the method
     * {@link  com.ftdi.j2xx.FT_Device #readBufferFull}.
     *
     * @return true if Rx buffer is full.
     *
     */  
    public static boolean isBufferFull()
    {
          return (Boolean) invokeMethod(getMethod(ftDev.getClass(), "readBufferFull"), ftDev);
    }
    
    //Android standard SDK calls
    /**
     * Invokes the method
     * {@link android.hardware.usb.UsbManager#hasAccessPermission}.
     *
     * @param usbDevice a {@link android.hardware.usb.UsbDevice} object
     * @return true if the caller has permission to access the device.
     *
     */
    public static boolean hasAccessPermission() 
    {
        return (Boolean) invokeMethod(getMethod(usbManager.getClass(), "hasPermission", usbDevice.getClass()),
                usbManager, usbDevice);
    }

    public static boolean requestPermission() 
    {
        return (Boolean) invokeMethod(getMethod(usbManager.getClass(), "hasPermission", usbDevice.getClass()),
                usbManager, usbDevice);
    }
    public static void reSet(Object usbConnection)
    {
        invokeMethod(getMethod(usbConnection.getClass(), "controlTransfer", new Class[]{}),
                usbConnection, usbDevice);
    }

    /**
     * Invokes the method {@link  android.hardware.usb.UsbManager#openDevice}.
     *
     * @param usbDevice a {@link android.hardware.usb.UsbDevice} object
     * @return a {@link android.hardware.usb.UsbRequest} object, or null if
     * failed to open
     *
     */
    public static Object openDevice()
    {
        return (Object) invokeMethod(
                getMethod(usbManager.getClass(), "openDevice", usbDevice.getClass()),
                usbManager, usbDevice);
    }

    /**
     * Invokes the method
     * {@link  android.hardware.usb.UsbDevice#getInterfaceCount}.
     *
     * @return the number of UsbInterfaces this device contains.
     *
     */
    public static int getInterfaceCount() 
    {
        return (Integer) invokeMethod(getMethod(usbDevice.getClass(), "getInterfaceCount"), usbDevice);
    }

    /**
     * Invokes the method
     * {@link  android.hardware.usb.UsbDeviceConnection#claimInterface}.
     *
     * @param usbInterface a {@link android.hardware.usb.UsbInterface} object
     * and boolean flag (true to disconnect kernel driver if necessary)
     * @return true if the interface was successfully claimed
     *
     */
    public static boolean claimInterface(Object usbConnection, Object usbInterface)
            throws IllegalArgumentException
    {
        boolean flag = true;
        return (Boolean) invokeMethodThrowsIllegalArgumentException(
                getMethod(usbConnection.getClass(), "claimInterface", new Class[]{usbInterface.getClass(), boolean.class}),
                usbConnection, usbInterface, Boolean.valueOf(flag));
    }

    /**
     * Invokes the method {@link android.hardware.usb.UsbDevice#getInterface}.
     *
     * @param index a integer
     * @return usbInterface {@link android.hardware.usb.UsbInterface} a Object
     *
     */
    public static Object getInterface(int index) 
    {
        return invokeMethodThrowsIllegalArgumentException(
                getMethod(usbDevice.getClass(), "getInterface", new Class[]{int.class}),
                usbDevice, Integer.valueOf(index));
    }

    /**
     * Invokes the method {@link android.hardware.usb.UsbInterface#getEndpoint}.
     *
     * @param index a integer(0 for read endpoint)
     * @return usbEndpoint {@link android.hardware.usb.UsbEndpoint} a Object
     *
     */
    public static Object getReadEndPoint(Object usbInterFace, int readPointAddr) throws IOException 
    {
        return (Object) invokeMethodThrowsIOException(
                getMethod(usbInterFace.getClass(), "getEndpoint", new Class[]{int.class}),
                usbInterFace, Integer.valueOf(readPointAddr));
    }

    /**
     * Invokes the method {@link android.hardware.usb.UsbEndpoint#getDirection}.
     *
     * @return USB_DIR_OUT(Constant Value:0) if the direction is host to device,
     * and USB_DIR_IN(Constant Value:128) if the direction is device to host.
     *
     */
    public static int getEndPointDirection(Object usbEndpoint) throws IOException
    {
        return (Integer) invokeMethodThrowsIOException(
                getMethod(usbEndpoint.getClass(), "getDirection"),
                usbEndpoint);
    }

    /**
     * Invokes the method {@link android.hardware.usb.UsbInterface#getEndpoint}.
     *
     * @param index a integer(1 for write endpoint)
     * @return usbEndpoint {@link android.hardware.usb.UsbEndpoint} a Object
     *
     */
    public static Object getWriteEndPoint(Object usbInterFace, int writeEndPointAddr) throws IOException 
    {
        return (Object) invokeMethodThrowsIOException(
                getMethod(usbInterFace.getClass(), "getEndpoint", new Class[]{int.class}),
                usbInterFace, Integer.valueOf(writeEndPointAddr));
    }

    /**
     * Invokes the method
     * {@link android.hardware.usb.UsbDeviceConnection#bulkTransfer}.
     *
     * @param usbendPoint {@link android.hardware.usb.UsbEndpoint} a Object
     * @param buffer a byte[] for data to send or receive
     * @param length a integer,the length of the data to send or receive
     * @param timeout a integer
     *
     * @return length of data transferred (or zero) for success, or negative
     * value for failure.
     *
     */
    public static int bulkTransfer(Object usbConnection, Object endPoint, byte[] data, int length, int timeout) throws IOException 
    {
        return (Integer) invokeMethodThrowsIOException(getMethod(usbConnection.getClass(), "bulkTransfer", new Class[]{endPoint.getClass(), byte[].class, int.class, int.class}),
                usbConnection, endPoint, data, Integer.valueOf(length), Integer.valueOf(timeout));
    }

    /**
     * Invokes the method
     * {@link android.hardware.usb.UsbDeviceConnection#bulkTransfer}.
     *
     * @param usbendPoint {@link android.hardware.usb.UsbEndpoint} a Object
     * @param buffer a byte[] for data to send or receive
     * @param offset a integer, the index of the first byte in the buffer to
     * send or receive
     * @param length a integer,the length of the data to send or receive
     * @param timeout a integer
     *
     * @return length of data transferred (or zero) for success, or negative
     * value for failure.
     *
     */
    public static int bulkTransfer(Object usbConnection, Object endPoint, byte[] data, int offset, int length, int timeout) throws IOException 
    {

        return (Integer) invokeMethodThrowsIOException(getMethod(usbConnection.getClass(), "bulkTransfer", new Class[]{endPoint.getClass(), byte[].class, int.class, int.class, int.class}),
                usbConnection, endPoint, data, Integer.valueOf(offset), Integer.valueOf(length), Integer.valueOf(timeout));
    }

    /**
     * Invokes the method
     * {@link android.hardware.usb.UsbDeviceConnection#controlTransfer}.
     *
     * @param requestType a integer, request type for this transaction
     * @param requestID a integer, request ID for this transaction
     * @param value a integer, value field for this transaction
     * @param buffer a byte[], buffer for data portion of transaction, or null
     * if no data needs to be sent or received
     * @param timeout a integer
     *
     * @return length of data transferred (or zero) for success, or negative
     * value for failure.
     *
     */
    public static int controlTransfer(Object usbConnection, int FTDI_DEVICE_OUT_REQTYPE, int SIO_SET_BAUD_RATE_REQUEST, int value, int index, byte[] data, int length, int USB_WRITE_TIMEOUT_MILLIS) throws IOException 
    {
        return (Integer) invokeMethodThrowsIOException(getMethod(usbConnection.getClass(), "controlTransfer", new Class[]{int.class, int.class, int.class, int.class, byte[].class, int.class, int.class}),
                usbConnection, Integer.valueOf(FTDI_DEVICE_OUT_REQTYPE), Integer.valueOf(SIO_SET_BAUD_RATE_REQUEST), Integer.valueOf(value),
                Integer.valueOf(index), data, Integer.valueOf(length), Integer.valueOf(USB_WRITE_TIMEOUT_MILLIS));
    }

    /**
     *
     * Invokes the method {@link  android.hardware.usb.UsbRequest#initialize}.
     *
     * @param usbConnection {@link android.hardware.usb.UsbDeviceConnection} a object
     * @param usbEndpoint a{@link android.hardware.usb.UsbEndpoint} object
     *
     * @return true if the request was successfully opened.
     *
     * Initializes the request so it can read or write data on the given
     * endpoint.
     */
    public static boolean initialize(Object usbConnection, Object readEndPoint) 
    {
        try 
        {
            usbRequest = Class.forName("android.hardware.usb.UsbRequest");
            conStructor = usbRequest.getConstructor();
            instance = conStructor.newInstance();
        } 
        catch (Exception e) 
        {

            return false;
        }
        return (Boolean) invokeMethodThrowsIllegalArgumentException(
                getMethod(usbRequest, "initialize", new Class[]{usbConnection.getClass(), readEndPoint.getClass()}),
                instance, usbConnection, readEndPoint);
    }

    /**
     * Invokes the method {@link  android.hardware.usb.UsbRequest#queue}.
     *
     * @param buffer a ByteBuffer, containing the bytes to write, or location to
     * store the results of a read
     * @param length a integer, number of bytes to read or write
     *
     * @return true if the queueing operation succeeded
     *
     */
    public static ByteBuffer readBufferqueue(ByteBuffer buffer , int length) throws IOException 
    {
        invokeMethodThrowsIOException(
                getMethod(usbRequest, "queue", new Class[]{ByteBuffer.class, int.class}),
                instance, buffer, Integer.valueOf(length));

        return buffer;
    }

    public static Object requestWait(Object usbConnection) throws IOException
    {
        Object obj = invokeMethodThrowsIOException(
                getMethod(usbConnection.getClass(), "requestWait"),
                usbConnection);
        return obj;

    }
    
     public static Object setClientData() throws IOException 
     {
        Object obj = invokeMethodThrowsIOException(
                getMethod(usbRequest, "setClientData", new Class[]{Object.class}),
                instance, null);
        return obj;
     }

    /**
     * Invokes the method
     * {@link android.hardware.usb.UsbDeviceConnection#close}.
     *
     * Releases all system resources related to the device.
     */
    public static void closeConnection(Object usbConnection)
    {
        invokeMethod(getMethod(usbConnection.getClass(), "close"),
                usbConnection);
    }

    // Reflection helper methods
    private static Method getMethod(Class clazz, String name)
    {
        try 
        {
            return clazz.getMethod(name, new Class[0]);
        }
        catch (NoSuchMethodException e)
        {
            throw new RuntimeException(e);
        }
    }

    private static Method getMethod(Class clazz, String name, Class<?>... parameterTypes) 
    {
        try 
        {
            return clazz.getMethod(name, parameterTypes);
        }
        catch (NoSuchMethodException e)
        {
            throw new RuntimeException(e);
        }
    }

    private static Object invokeStaticMethod(Method method) 
    {
        try
        {
            return method.invoke(null);
        } 
        catch (IllegalAccessException e)
        {
            throw new RuntimeException(e);
        }
        catch (InvocationTargetException e) 
        {
            Throwable cause = e.getCause();
            cause.printStackTrace();
            if (cause instanceof RuntimeException)
            {
                throw (RuntimeException) cause;
            } 
            else 
            {
                throw new RuntimeException(cause);
            }
        }
    }

    private static Object invokeMethod(Method method, Object thisObject, Object... args)
    {
        try 
        {
            return method.invoke(thisObject, args);
        } 
        catch (IllegalAccessException e) 
        {
            throw new RuntimeException(e);
        } 
        catch (InvocationTargetException e)
        {
            Throwable cause = e.getCause();
            cause.printStackTrace();
            if (cause instanceof RuntimeException)
            {
                throw (RuntimeException) cause;
            } 
            else 
            {
                throw new RuntimeException(cause);
            }
        }
    }

    private static Object invokeMethodThrowsIllegalArgumentException(Method method,
            Object thisObject, Object... args) throws IllegalArgumentException 
    {
        try 
        {
            return method.invoke(thisObject, args);
        } 
        catch (IllegalAccessException e)
        {
            throw new RuntimeException(e);
        }
        catch (InvocationTargetException e) 
        {
            Throwable cause = e.getCause();
            cause.printStackTrace();
            if (cause instanceof IllegalArgumentException)
            {
                throw (IllegalArgumentException) cause;
            } 
            else if (cause instanceof RuntimeException)
            {
                throw (RuntimeException) cause;
            } 
            else 
            {
                throw new RuntimeException(e);
            }
        }
    }

    private static Object invokeMethodThrowsIOException(Method method, Object thisObject,
            Object... args) throws IOException 
    {
        try 
        {
            return method.invoke(thisObject, args);
        } 
        catch (IllegalAccessException e)
        {
            throw new RuntimeException(e);
        } 
        catch (InvocationTargetException e) 
        {
            Throwable cause = e.getCause();
            cause.printStackTrace();
            if (cause instanceof IOException)
            {
                throw (IOException) cause;
            }
            else if (cause instanceof RuntimeException)
            {
                throw (RuntimeException) cause;
            } 
            else
            {
                throw new RuntimeException(e);
            }
        }
    }
}

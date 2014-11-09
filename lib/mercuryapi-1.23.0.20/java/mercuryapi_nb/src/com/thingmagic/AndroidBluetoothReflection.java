package com.thingmagic;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/**
 * Provides access to Android Bluetooth classes via Java reflection.
 *
 */
public class AndroidBluetoothReflection
{
  private static final int BOND_BONDED = 0xC;
  public AndroidBluetoothReflection()
  {
      //Default Constructor
  }

  /**
   * Invokes the method
   * {@link android.bluetooth.BluetoothAdapter#getDefaultAdapter}.
   *
   * @return a {@link android.bluetooth.BluetoothAdapter} object, or null if
   *         Bluetooth is not available
   */
  public static Object getBluetoothAdapter() {
    // return BluetoothAdapter.getDefaultAdapter();
    Class bluetoothAdapterClass;
    try {
      bluetoothAdapterClass = Class.forName("android.bluetooth.BluetoothAdapter");
    } catch (ClassNotFoundException e) {
      // Bluetooth is not available on this Android device.
      return null;
    }
    return invokeStaticMethod(getMethod(bluetoothAdapterClass, "getDefaultAdapter"));
  }

  /**
   * Invokes the method
   * {@link android.bluetooth.BluetoothAdapter#isEnabled}.
   *
   * @param bluetoothAdapter a {@link android.bluetooth.BluetoothAdapter} object
   * @return true if Bluetooth is enabled, false otherwise
   */
  public static boolean isBluetoothEnabled(Object bluetoothAdapter) {
    // boolean enabled = bluetoothAdapter.isEnabled();
    return (Boolean) invokeMethod(getMethod(bluetoothAdapter.getClass(), "isEnabled"),
        bluetoothAdapter);
  }
  
  /**
   * Invokes the method
   * {@link android.bluetooth.BluetoothAdapter#checkBluetoothAddress}.
   *
   * @param bluetoothAdapter a {@link android.bluetooth.BluetoothAdapter} object
   * @param address a string that might be a bluetooth MAC address
   * @return true if the address is valid, false otherwise
   */
  public static boolean checkBluetoothAddress(Object bluetoothAdapter, String address) {
    // return bluetoothAdapter.checkBluetoothAddress(address);
    return (Boolean) invokeMethod(
        getMethod(bluetoothAdapter.getClass(), "checkBluetoothAddress", String.class),
        bluetoothAdapter, address);
  }

  /**
   * Invokes the method
   * {@link android.bluetooth.BluetoothDevice#getBondState}.
   *
   * @param bluetoothDevice a {@link android.bluetooth.BluetoothDevice} object
   * @return the bond state of the given {@link android.bluetooth.BluetoothDevice}
   */
  public static boolean isBonded(Object bluetoothDevice) {
    // return bluetoothDevice.getBondState() == BOND_BONDED;
    int bondState = (Integer) invokeMethod(getMethod(bluetoothDevice.getClass(), "getBondState"),
        bluetoothDevice);
    return bondState == BOND_BONDED;
  }
  
  /**
   * Invokes the method
   * {@link android.bluetooth.BluetoothAdapter#getRemoteDevice}.
   *
   * @param bluetoothAdapter a {@link android.bluetooth.BluetoothAdapter} object
   * @param address the bluetooth MAC address of the device
   * @return a {@link android.bluetooth.BluetoothDevice} object
   */
  public static Object getRemoteDevice(Object bluetoothAdapter, String address)
      throws IllegalArgumentException {
    // return bluetoothAdapter.getRemoteDevice(address);
    return invokeMethodThrowsIllegalArgumentException(
        getMethod(bluetoothAdapter.getClass(), "getRemoteDevice", String.class),
        bluetoothAdapter, address);
  }

  /**
   * Invokes the method
   * {@link android.bluetooth.BluetoothDevice#createRfcommSocketToServiceRecord}.
   *
   * @param bluetoothDevice a {@link android.bluetooth.BluetoothDevice} object
   * @param uuid the service record uuid
   * @return a {@link android.bluetooth.BluetoothSocket} object
   */
  public static Object createBluetoothSocket(Object bluetoothDevice)
      throws IOException {
    // return bluetoothDevice.createRfcommSocketToServiceRecord(uuid);
      return invokeMethodThrowsIOException(
               getMethod(bluetoothDevice.getClass(), "createRfcommSocket", new Class[] { int.class }),
        bluetoothDevice, Integer.valueOf(1));
    }

// BluetoothSocket methods

  /**
   * Invokes the method
   * {@link android.bluetooth.BluetoothSocket#connect}.
   *
   * @param bluetoothSocket a {@link android.bluetooth.BluetoothSocket} object
   */
  public static void connectToBluetoothSocket(Object bluetoothSocket) throws IOException {
    // bluetoothSocket.connect();
    invokeMethodThrowsIOException(getMethod(bluetoothSocket.getClass(), "connect"),
        bluetoothSocket);
  }

  /**
   * Invokes the method
   * {@link android.bluetooth.BluetoothSocket#getInputStream}.
   *
   * @param bluetoothSocket a {@link android.bluetooth.BluetoothSocket} object
   * @return the {@link InputStream}
   */
  public static InputStream getInputStream(Object bluetoothSocket) throws IOException {
    // return bluetoothSocket.getInputStream();
    return (InputStream) invokeMethodThrowsIOException(
        getMethod(bluetoothSocket.getClass(), "getInputStream"),
        bluetoothSocket);
  }

  /**
   * Invokes the method
   * {@link android.bluetooth.BluetoothSocket#getOutputStream}.
   *
   * @param bluetoothSocket a {@link android.bluetooth.BluetoothSocket} object
   * @return the {@link OutputStream}
   */
  public static OutputStream getOutputStream(Object bluetoothSocket) throws IOException {
    // return bluetoothSocket.getOutputStream();
    return (OutputStream) invokeMethodThrowsIOException(
        getMethod(bluetoothSocket.getClass(), "getOutputStream"),
        bluetoothSocket);
  }

  /**
   * Invokes the method
   * {@link android.bluetooth.BluetoothSocket#close}.
   *
   * @param bluetoothSocket a {@link android.bluetooth.BluetoothSocket} object
   */
  public static void closeBluetoothSocket(Object bluetoothSocket) throws IOException {
    // bluetoothSocket.close();
    invokeMethodThrowsIOException(getMethod(bluetoothSocket.getClass(), "close"),
        bluetoothSocket);
  }

  
  
  // Reflection helper methods

  private static Method getMethod(Class clazz, String name) {
    try {
      return clazz.getMethod(name, new Class[0]);
    } catch (NoSuchMethodException e) {
      throw new RuntimeException(e);
    }
  }

   private static Method getMethod(Class clazz, String name, Class<?>... parameterTypes) {
    try {
      return clazz.getMethod(name, parameterTypes);
    } catch (NoSuchMethodException e) {
      throw new RuntimeException(e);
    }
  }
   
  private static Object invokeStaticMethod(Method method) {
    try {
      return method.invoke(null);
    } catch (IllegalAccessException e) {
      throw new RuntimeException(e);
    } catch (InvocationTargetException e) {
      Throwable cause = e.getCause();
      cause.printStackTrace();
      if (cause instanceof RuntimeException) {
        throw (RuntimeException) cause;
      } else {
        throw new RuntimeException(cause);
      }
    }
  }

   private static Object invokeMethod(Method method, Object thisObject, Object... args) {
    try {
      return method.invoke(thisObject, args);
    } catch (IllegalAccessException e) {
      throw new RuntimeException(e);
    } catch (InvocationTargetException e) {
      Throwable cause = e.getCause();
      cause.printStackTrace();
      if (cause instanceof RuntimeException) {
        throw (RuntimeException) cause;
      } else {
        throw new RuntimeException(cause);
      }
    }
  }

    private static Object invokeMethodThrowsIllegalArgumentException(Method method,
      Object thisObject, Object... args) throws IllegalArgumentException {
    try {
      return method.invoke(thisObject, args);
    } catch (IllegalAccessException e) {
      throw new RuntimeException(e);
    } catch (InvocationTargetException e) {
      Throwable cause = e.getCause();
      cause.printStackTrace();
      if (cause instanceof IllegalArgumentException) {
        throw (IllegalArgumentException) cause;
      } else if (cause instanceof RuntimeException) {
        throw (RuntimeException) cause;
      } else {
        throw new RuntimeException(e);
      }
    }
  }

  private static Object invokeMethodThrowsIOException(Method method, Object thisObject,
      Object... args) throws IOException {
    try {
      return method.invoke(thisObject, args);
    } catch (IllegalAccessException e) {
      throw new RuntimeException(e);
    } catch (InvocationTargetException e) {
      Throwable cause = e.getCause();
      cause.printStackTrace();
      if (cause instanceof IOException) {
        throw (IOException) cause;
      } else if (cause instanceof RuntimeException) {
        throw (RuntimeException) cause;
      } else {
        throw new RuntimeException(e);
      }
    }
  }
}

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package com.thingmagic;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.URL;
import java.net.URLClassLoader;

public class BluecoveBluetoothReflection {

  /**
   * Invokes the method
   * {@link javax.microedition.io.Connector#open}.
   *
   * @param bluetoothConnector a {@link javax.microedition.io.Connector} object
   *  @param serviceUrl a {@link java.Land.String} object
   */
  public static Object connectToBluetoothSocket(Class connectorClass,String serviceUrl) throws Exception
  {
    //Connector.open(serviceUrl);
          Constructor connectorClassConstructor = connectorClass.getDeclaredConstructor(null);
          connectorClassConstructor.setAccessible(true);
    return invokeMethodThrowsIOException(getMethod(connectorClass, "open",String.class),
        connectorClassConstructor,serviceUrl);
  }

   /**
   * Invokes the method
   * {@link javax.microedition.io.InputConnection#openInputStream}.
   *
   * @param inputConnectorClass a {@link javax.microedition.io.InputConnection} class
   * @param bluetoothConnection a {@link javax.microedition.io.Connector} object
   * @return the {@link InputStream}
   */
  public static InputStream getInputStream(Class inputConnectorClass ,Object bluetoothConnection) throws IOException {
    // return InputConnection.openInputStream();
    return (InputStream) invokeMethodThrowsIOException(
        getMethod(inputConnectorClass, "openInputStream"),
        bluetoothConnection);
  }

  /**
   * Invokes the method
   * {@link javax.microedition.io.OutputConnection#openOutputStream}.
   *
   * @param outputConnectorClass a {@link javax.microedition.io.OutputConnection} class
   * @param bluetoothConnection a {@link javax.microedition.io.Connector} object
   * @return the {@link OutputStream}
   */
  public static OutputStream getOutputStream(Class outputConnectorClass,Object bluetoothConnection) throws IOException {
    // return OutputConnection.openOutputStream();
    return (OutputStream) invokeMethodThrowsIOException(
        getMethod(outputConnectorClass, "openOutputStream"),
        bluetoothConnection);
  }
  
// Reflection helper methods

  private static Method getMethod(Class clazz, String name) {
    try {
      return clazz.getMethod(name, new Class[0]);
    } catch (NoSuchMethodException e) {
      throw new RuntimeException(e);
    }
  }

   public static Method getMethod(Class clazz, String name, Class<?>... parameterTypes) {
    try {
      return clazz.getMethod(name, parameterTypes);
    } catch (NoSuchMethodException e) {
      throw new RuntimeException(e);
    }
  }

  public static Object invokeStaticMethod(Method method) {
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

   public static Object invokeMethod(Method method, Object thisObject, Object... args) {
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

    public static Object invokeMethodThrowsIllegalArgumentException(Method method,
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

  public static Object invokeMethodThrowsIOException(Method method, Object thisObject,
      Object... args) throws IOException {
    try {
      return method.invoke(thisObject, args);
    } catch (IllegalAccessException e) {
      throw new RuntimeException(e);
    } catch (InvocationTargetException e) {
      Throwable cause = e.getCause();
      System.out.println(" InvocationTargetException ");
      if (cause instanceof IOException) {
           System.out.println(" IOException ");
        throw (IOException) cause;
      } else if (cause instanceof RuntimeException) {
        throw (RuntimeException) cause;
      } else {
        throw new RuntimeException(e);
      }
    }
  }
}

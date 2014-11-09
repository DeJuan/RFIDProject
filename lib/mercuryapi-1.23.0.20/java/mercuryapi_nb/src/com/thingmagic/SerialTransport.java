/*
 * Copyright (c) 2009 ThingMagic, Inc.
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

public interface SerialTransport
{

  /**
   * Causes the communication interface to be opened but
   * does not transmit any serial-layer data. This should perform
   * actions such as opening a serial port device or establishing a
   * network connection within a wrapper protocol.
   */
  void open()
    throws ReaderException;

  /**
   * Send bytes down the serial transport layer. No interpretation or
   * modification occurs.
   *
   * @param message array containing the bytes to be sent
   * @param length number of bytes to send
   * @param offset position in array to send from
   * @param timeoutMs The duration to wait for the operation to complete.
   */
  void sendBytes(int length, byte[] message, int offset, int timeoutMs)
    throws ReaderException;

  /**
   * Receive a number of bytes on the serial transport. 
   *
   * @param length number of bytes to receive
   * @param messageSpace byte array to store the message in, or null to have one allocated
   * @param offset location in messageSpace to store bytes
   * @param timeoutMillis maximum duration to wait for a message
   * @return the byte array with the number of bytes added
   * @throws TimeoutException if timeoutMillis pass without a message being received
   */
  byte[] receiveBytes(int length, byte[] messageSpace, int offset, int timeoutMillis)
    throws ReaderException;

  /**
   * Get the current baud rate of the communication channel.
   *
   * @return the baud rate
   */ 
  int getBaudRate()
    throws ReaderException;

  /**
   * Set the current baud rate of the communication channel.
   *
   * @param rate the baud rate to set
   * @throws IllegalArgumentException if the channel does not support the rate
   */ 
  void setBaudRate(int rate)
    throws ReaderException;

  /**
   * Take any actions necessary (possibly none) to remove unsent data
   * from the output path.
   */
  void flush()
    throws ReaderException;
  /**
   * Close the communication channel.
   */
  void shutdown()
    throws ReaderException;  
 
}

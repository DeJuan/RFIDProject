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

package com.thingmagic;

/**
 * The listener interface for receiving low-level packets sent to
 * and recieved from the device. The class that is interested in
 * observing packets implements this interface, and the object
 * created with that class is registered with
 * addTransportListener(). When data is sent to the device, the message
 * method is invoked with tx set to true; when data is recieved from
 * the device, the message method is invoked with tx set to false.
 */
public interface TransportListener
{
  /**
   * Invoked when a data packet is sent or recieved
   *
   * @param data the packet, as bytes
   * @param tx whether the packet was sent to the device or recieved
   * from the device
   * @param timeout the timeout specified for this transaction
   */
  public void message(boolean tx, byte[] data, int timeout);
}

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

import java.util.Arrays;

/**
 * A class to represent RFID tags. Instances of this class are immutable.
 */
public class TagData implements TagFilter
{
  final byte epc[];
  final byte crc[];
  final int hash;

  // Non-public empty constructor
  TagData()
  {
    epc = null;
    crc = null;
    hash = 0;
  }

  /**
   * Check the length of the EPC.
   * Intended to be overridden by subclasses.
   *
   * @param epcbytes epc length in bytes
   * @return whether the length is valid
   */

  boolean checkLen(int epcbytes)
  {
    return true;
  }

  /**
   * Construct a tag object representing the specified EPC
   *
   * @param epc the bytes representing the EPC
   */
  public TagData(byte[] epc)
  {
    this(epc, null);
  }

  /**
   * Construct a tag object representing the specified EPC
   *
   * @param epc the bytes representing the EPC
   * @param crc the bytes representing the CRC 
   */
  public TagData(byte[] epc, byte[] crc)
  {

//    if (checkLen(epc.length) == false)
//    {
//        throw new IllegalArgumentException(
//          String.format("Invalid EPC length %d bytes for protocol %s",
//                        epc.length, getProtocol().toString()) + " epc : " + ReaderUtil.byteArrayToHexString(epc));
//    }
    this.epc = epc.clone();
    this.crc = (crc == null) ? null : crc.clone();
    hash = Arrays.hashCode(epc); // + protocol?
  }

  /**
   * Construct a tag object representing the specified EPC and CRC
   *
   * @param epc a hexadecimal string representing the EPC
   * @param crc a hexadecimal string representing the CRC
   */
  public TagData(String epc, String crc)
  {
    int len, start;

    len = epc.length();

    if ((len % 2) != 0)
    {
      throw new IllegalArgumentException("Hex string is not an integer number of bytes");
    }

    start = 0;
    if (epc.regionMatches(true, 0, "0x", 0, 2))
    {
      start = 2;
    }

//    if (checkLen(len/2) == false)
//    {
//        throw new IllegalArgumentException(
//          String.format("Invalid EPC length %d bytes for protocol %s",
//                        len/2, getProtocol().toString()) + " epc : " + epc);
//    }

    this.epc = hexStringToBytes(epc.substring(start));
    this.crc = hexStringToBytes(crc);
    hash = Arrays.hashCode(this.epc); // + protocol?
  }

  /**
   * Construct a tag object representing the specified EPC (no CRC)
   *
   * @param epc a hexadecimal string representing the EPC
   */
  public TagData(String epc)
  {
    this(epc, null);
  }


  static byte[] hexStringToBytes(String s)
  {
    byte[] ret;
    int len;

    if (s == null)
    {
      return null;
    }

    len = s.length();
    ret = new byte[len / 2];
    for (int i = 0; i < len; i+=2)
    {
      ret[i/2] = (byte)Integer.parseInt(s.substring(i, i + 2), 16);
    }

    return ret;
  }

  /**
   * Returns a hexadecimal string version of this tag's EPC.
   *
   * @return a string representation of the EPC.
   */
  public String epcString()
  {
    StringBuilder sb = new StringBuilder(epc.length * 2);

    for (int i = 0; i < epc.length; i++)
    {
      sb.append(String.format("%02X", (epc[i] & 0xff)));
    }
    return new String(sb);
  }

  /**
   * Returns this tag's EPC.
   *
   * @return an array containing the bytes of the EPC
   */
  public byte[] epcBytes()
  {
    return epc.clone();
  }

  /**
   * Returns this tag's CRC.
   *
   * @return an array containing the bytes of the CRC
   */
  public byte[] crcBytes()
  {
    return crc.clone();
  }

  public String toString()
  {
    return String.format("EPC:%s", epcString());
  }

  /**
   * Returns the protocol of this tag.
   */
  public TagProtocol getProtocol()
  {
    return TagProtocol.NONE;
  }

  /**
   * Comapres the specified Object with this TagData for
   * equality. Returns true if and only if the specified Object is
   * also a TagData with the same protocol and EPC.
   *
   * @param obj the Object to be compared for equality with this TagData
   * @return true if the specified Object is equal to this TagData
   */
  public boolean equals(Object obj)
  {
    TagData t;

    if (obj == this)
    {
      return true;
    }

    if (!(obj instanceof TagData))
    {
      return false;
    }

    t = (TagData)obj;

    return (hash == t.hash
            && Arrays.equals(t.epc, epc)
            && getProtocol() == t.getProtocol());
  }

  /**
   * Returns the hash code value for this TagData.
   *
   * @return the hash code value for this TagData.
   */
  public int hashCode()
  {
    return hash;
  }

  public boolean matches(TagData t)
  {
    for (int i = 0; i < epc.length && i < t.epc.length; i++)
    {
      if (epc[i] != t.epc[i])
      {
        return false;
      }
    }
    return true;
  }

}

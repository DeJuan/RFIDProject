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

/**
 *
 * 
 */
public class Iso180006bUcode 
{
    
    Iso180006bUcode()
    {
        
    }
    
    /**
 * This class extends {@link TagData} to represent the details of an 
 * ISO 18000-6BUCODE RFID tag.
 */
  public static class TagData extends com.thingmagic.TagData
  {

    @Override
    boolean checkLen(int epcbytes)
    {
      if (epcbytes != 8)
      {
        return false;
      }
      return true;
    }

    public TagProtocol getProtocol()
    {
      return TagProtocol.ISO180006B_UCODE;
    }

    /**
     * Construct an ISO 18000-6BUCODE tag data from a byte array.
     *
     * @param bEPC EPC bytes. Must be 8 bytes
     */
    public TagData(byte[] bEPC)
    {
      super(bEPC);
    }

    /**
     * Construct an ISO 18000-6BUCODE tag data from a byte array.
     *
     * @param bEPC EPC bytes. Must be 8 bytes
     * @param crc CRC bytes
     */
    public TagData(byte[] bEPC, byte[] crc)
    {
      super(bEPC, crc);
    }

    /**
     * Construct an ISO 18000-6BUCODE tag data from a hexadecimal string.
     *
     * @param sEPC Hex string. Must be 8 bytes (16 hex digits)
     */
    public TagData(String sEPC)
    {
      super(sEPC);
    }

    /**
     * Construct an ISO 18000-6BUCODE tag data from a hexadecimal string.
     *
     * @param sEPC Hex string. Must be 8 bytes (16 hex digits)
     * @param sCRC Hex string. Must be 2 bytes (4 hex digits)
     */
    public TagData(String sEPC, String sCRC)
    {
      super(sEPC, sCRC);
    }

  }   
}
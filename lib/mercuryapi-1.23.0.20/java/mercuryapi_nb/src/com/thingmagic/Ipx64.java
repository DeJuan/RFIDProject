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
 * This class is a namespace for iPx-specific subclasses of generic
 * Mercury API classes, data structures, constants, and convenience
 * functions.
 */
public class Ipx64 {

    // non-public, this class is never instantiated
    Ipx64() {
    }

    /**
     * This class extends {@link TagData} to represent the details of an
     * iPx RFID tag.
     */
    public static class TagData extends com.thingmagic.TagData {

        final byte[] pc;

        public TagProtocol getProtocol() {
            return TagProtocol.IPX64;
        }

        /**
         * Construct an iPx tag data from a hexadecimal string.
         *
         * @param sEpc Hex string.
         */
        public TagData(String sEpc) {
            super(sEpc);

            pc = new byte[2];
            pc[0] = (byte) ((epc.length) << 3);
            pc[1] = 0;
        }

        /**
         * Construct an iPx tag data from a byte array.
         *
         * @param bEpc Bytes of EPC.
         */
        public TagData(byte[] bEpc) {
            super(bEpc);

            pc = new byte[2];
            pc[0] = (byte) ((epc.length) << 3);
            pc[1] = 0;
        }


        public TagData(byte[] bEPC, byte[] newPC) {
            super(bEPC);

            pc = newPC.clone();
        }

        /**
         * Construct an IPX64 tag data from a byte array.
         *
         * @param bEPC EPC bytes. Must be 8 bytes
         * @param crc CRC bytes
         */
        public TagData(byte[] bEPC, byte[] crc,  byte[] newPC) {
            super(bEPC, crc);
            pc = newPC.clone();
        }

        /**
         * Construct an ISO 18000-6B tag data from a hexadecimal string.
         *
         * @param sEPC Hex string. Must be 8 bytes (16 hex digits)
         * @param sCRC Hex string. Must be 2 bytes (4 hex digits)
         */
        public TagData(String sEPC, String sCRC) {
            super(sEPC, sCRC);

            pc = new byte[2];
            pc[0] = (byte) ((epc.length) << 3);
            pc[1] = 0;
        }

        public byte[] pcBytes() {
            return pc.clone();
        }

        public String toString() {
            return String.format("IPX64:%s", epcString());
        }        
    }
}

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
using System;
using System.Collections.Generic;

namespace ThingMagic
{
    /// <summary>
    /// Iso tag-specific constructs
    /// </summary>
    public static class Iso180006b
    {
        #region Nested Enums

	#region LinkFrequency

        /// <summary>
        /// enum to define link frequency options
        /// </summary>
	public enum LinkFrequency
        {
            /// <summary>
            /// 160kHz link frequency
            /// </summary>
            LINK160KHZ = 160,
            /// <summary>
            /// 40kHz link frequency
            /// </summary>
            LINK40KHZ = 40,
        }

        #endregion

        #region ModulationDepth
        /// <summary>
        /// Iso180006b Modulation Depth Value
        /// </summary>
        public enum ModulationDepth
        {
            /// <summary>
            /// Depth = 99%
            /// </summary>
            DEPTH99PERCENT = 0,
            /// <summary>
            /// Depth = 11%
            /// </summary>
            DEPTH11PERCENT = 1,
        }

        #endregion

        #region Delimiter
        /// <summary>
        /// Iso180006b Delimiter Value
        /// </summary>
        public enum Delimiter
        {
            /// <summary>
            /// Delimiter = 1
            /// </summary>
            DELIMITER1 = 1,
            /// <summary>
            /// Delimiter = 4
            /// </summary>
            DELIMITER4 = 4,
        }

        #endregion

        #region SelectOp

        /// <summary>
        /// enum to define select options
        /// </summary>
        public enum SelectOp
        {
            /// <summary>
            /// Select Tags that match the select criteria
            /// </summary>            
            EQUALS,
            /// <summary>
            /// Select Tags that do not match the select criteria
            /// </summary>
            NOTEQUALS,
            /// <summary>
            /// Select Tags that match select data that are less less than the select criteria
            /// </summary>
            LESSTHAN,
            /// <summary>
            /// Select Tags that match select data that are greated than the select criteria
            /// </summary>
            GREATERTHAN,
        }
 
        #endregion

        #endregion

        #region Nested Classes

        #region TagData

        /// <summary>
        /// ISO specific version of TagData.
        /// </summary>
        public class TagData : ThingMagic.TagData
        {
            #region Properties

            /// <summary>
            /// Tag's RFID protocol
            /// </summary>
            public override TagProtocol Protocol
            {
                get
                {
                    return TagProtocol.ISO180006B;
                }
            }

            #endregion

            #region Construction

            /// <summary>
            /// Create TagData with blank CRC
            /// </summary>
            /// <param name="epcBytes">EPC value</param>
            public TagData(ICollection<byte> epcBytes) : base(epcBytes) { }

            /// <summary>
            /// Create TagData
            /// </summary>
            /// <param name="epcBytes">EPC value</param>
            /// <param name="crcBytes">CRC value</param>
            public TagData(ICollection<byte> epcBytes, ICollection<byte> crcBytes) : base(epcBytes, crcBytes) { } 

            #endregion
        }

        #endregion

        #region LockAction
        /// <summary>
        /// Iso180006B lockAction
        /// </summary>
        public class LockAction : TagLockAction
        {
            #region Fields

            /// <summary>
            /// The address of the byte to lock.
            /// </summary>
            public byte Address;

            #endregion

            #region Construction

            /// <summary>
            /// Create a lock action for the given address
            /// </summary>
            /// <param name="address">Memory address to lock</param>
            public LockAction(byte address)
            {
                Address = address;
            }

	    #endregion

	}
        #endregion

        #region Select
        /// <summary>
        /// Representation of a ISO Select Operation.
        /// </summary>
        public class Select : TagFilter
        {
            #region Fields

            /// <summary>
            /// Whether to invert the selection (deselect tags that meet the comparison and vice versa).
            /// </summary>
            public bool Invert;

            /// <summary>
            /// The select option to provide
            /// </summary>
            public SelectOp Op;

            /// <summary>
            /// The select address
            /// </summary>
            public byte Address;

            /// <summary>
            /// A bitmask of which of the eight provided bytes to compare
            /// </summary>
            public byte Mask;

            /// <summary>
            /// The data to compare. Exactly eight bytes.
            /// </summary>
            public byte[] Data; 

            #endregion

            #region Construction

            /// <summary>
            /// Select constructor.
            /// </summary>
            /// <param name="invert">invert the selection</param>
            /// <param name="op">select options</param>
            /// <param name="address">starting mask address</param>
            /// <param name="mask">the select mask</param>
            /// <param name="data">the data for comparison</param>
            public Select(bool invert, SelectOp op, byte address, byte mask, ICollection<byte> data)
            {
                Invert = invert;
                Op = op;
                Address = address;
                Mask = mask;
                Data = CollUtil.ToArray(data);

                if (Data.Length != 8)
                    throw new ArgumentException("ISO180006B select data must be 8 bytes");
            }

            #endregion

            #region Matches

            /// <summary>
            /// Predicate for post-processing filters.
            /// </summary>
            /// <param name="t">tag data to screen</param>
            /// <returns>Return true to allow tag through the filter.
            /// Return false to reject tag.</returns>
            public bool Matches(ThingMagic.TagData t)
            {
                throw new NotSupportedException();
            }

            #endregion

            /// <summary>
            /// Returns a String that represents the current Object.
            /// </summary>
            /// <returns>A String that represents the current Object.</returns>
            public override string ToString()
            {
                return String.Format(
                    "Iso180006b.Select:[{0}{1},{2},0x{3:X2},{4}]",
                    (Invert ? "Invert," : ""),
                    Op, Address, Mask, ByteFormat.ToHex(Data));

            }
        }

        #endregion

        #region LockTag
        /// <summary>
        /// Perform a lock or unlock operation on a tag. The first tag seen
        /// is operated on - the singulation parameter may be used to control
        /// this. Note that a tag without an access password set may not
        /// accept a lock operation or remain locked.
        /// </summary>
        public class LockTag : TagOp
        {
            #region Fields

            /// <summary>
            /// the address to lock tag
            /// </summary>
            public byte Address;

            #endregion

            #region Construction
            /// <summary>
            /// Constructor to initialize the parameters of LockTag
            /// </summary>
            /// <param name="address">the address to lock tag</param>
            public LockTag(byte address)
            {
                this.Address = address;
            }

            #endregion

        }

        #endregion

        #region WriteData
        /// <summary>
        /// Write data to the memory bank of a tag.
        /// </summary>
        /// 
        public class WriteData : TagOp
        {
            #region Fields

            /// <summary>
            /// the byte address to start writing to
            /// </summary>
            public byte Address;

            /// <summary>
            /// the bytes to write
            /// </summary>
            public ICollection<byte> Data;

            #endregion

            #region Construction
            /// <summary>
            /// Constructor to initialize the parameters of WriteData
            /// </summary>
            /// <param name="address">the byte address to start writing to</param>
            /// <param name="data">the bytes to write</param>
            /// 
            public WriteData(byte address, ICollection<byte> data)
            {
                this.Address = address;
                this.Data = data;
            }

            #endregion

        }

        #endregion

        #region ReadData
        /// <summary>
        /// Read data from the memory bank of a tag.
        /// </summary>    
        public class ReadData : TagOp
        {
            #region Fields

            /// <summary>
            /// the byte address to start reading at
            /// </summary>
            public byte byteAddress;

            /// <summary>
            /// the number of bytes to read
            /// </summary>
            public byte length;

            #endregion

            #region Construction
            /// <summary>
            /// Constructor to initialize the parameters of ReadData
            /// </summary>
            /// <param name="byteAddress">the byte address to start reading at</param>
            /// <param name="length">the number of bytes to read</param>
            public ReadData(byte byteAddress, byte length)
            {
                this.byteAddress = byteAddress;
                this.length = length;
            }

            #endregion

        }

        #endregion

        

        #endregion

    }
}

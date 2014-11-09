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
    /// ReadPlan consisting of list of antennas, tag protocol, filter and weight
    /// </summary>
    public class SimpleReadPlan : ReadPlan
    {
        #region Static Fields

        /// <summary>
        /// Default for protocol
        /// </summary>
        protected static readonly TagProtocol _defaultProtocol = TagProtocol.GEN2;

        /// <summary>
        /// Default for antennaList
        /// </summary>
        protected static readonly int[] _defaultAntennas = null;

        /// <summary>
        /// Default for filter
        /// </summary>
        protected static readonly TagFilter _defaultFilter = null;

        /// <summary>
        /// Default for TagOp
        /// </summary>
        protected static readonly TagOp _defaultTagOp = null;

        #endregion

        #region Fields

        /// <summary>
        /// List of protocols
        /// </summary>
        public TagProtocol Protocol;

        /// <summary>
        /// List of antennas.
        /// May be null, which means "use auto-detected antennas".
        /// </summary>

        public int[] Antennas;

        /// <summary>
        /// Tag filter
        /// </summary>
        public TagFilter Filter;

        /// <summary>
        /// Tag Operation
        /// </summary>
        public TagOp Op = null;

        /// <summary>
        /// Fast search
        /// </summary>
        public bool UseFastSearch = false;

        #endregion

        #region Construction

        /// <summary>
        ///  Create a SimpleReadPlan
        /// </summary>
        public SimpleReadPlan()
            : this(_defaultAntennas, _defaultProtocol) { }

        /// <summary>
        ///  Create a SimpleReadPlan
        /// </summary>
        /// <param name="antennaList">List of antenna numbers</param>
        /// <param name="protocol">Protocol identifier</param>
        public SimpleReadPlan(ICollection<int> antennaList, TagProtocol protocol)
            : this(antennaList, protocol, _defaultFilter,_defaultTagOp, DEFAULT_WEIGHT) { }

        /// <summary>
        ///  Create a SimpleReadPlan
        /// </summary>
        /// <param name="antennaList">List of antenna numbers.  May be null.</param>
        /// <param name="protocol">Protocol identifier</param>
        /// <param name="filter">Tag filter.  May be null.</param>
        /// <param name="weight">Relative scheduling weight</param>
        public SimpleReadPlan(ICollection<int> antennaList, TagProtocol protocol, TagFilter filter, int weight)
            : this(antennaList, protocol, filter, _defaultTagOp, weight)
        {
        }

        /// <summary>
        ///  Create a SimpleReadPlan
        /// </summary>
        /// <param name="antennaList">List of antenna numbers.  May be null.</param>
        /// <param name="protocol">Protocol identifier</param>
        /// <param name="filter">Tag filter.  May be null.</param>
        /// <param name="op">Operation mode</param>
        /// <param name="weight">Relative scheduling weight</param>
        public SimpleReadPlan(ICollection<int> antennaList, TagProtocol protocol, TagFilter filter, TagOp op, int weight)
            : base(weight)
        {
            Antennas = (antennaList != null) ? CollUtil.ToArray(antennaList) : null;
            Protocol = protocol;
            Filter = filter;
            Op = op;
        }

        /// <summary>
        ///  Create a SimpleReadPlan
        /// </summary>
        /// <param name="antennaList">List of antenna numbers.  May be null.</param>
        /// <param name="protocol">Protocol identifier</param>
        /// <param name="filter">Tag filter.  May be null.</param>
        /// <param name="op">Operation mode</param>        
        /// <param name="useFastSearch">Enable fast search</param>
        public SimpleReadPlan(ICollection<int> antennaList, TagProtocol protocol, TagFilter filter, TagOp op, bool useFastSearch)
            : base(DEFAULT_WEIGHT)
        {
            Antennas = (antennaList != null) ? CollUtil.ToArray(antennaList) : null;
            Protocol = protocol;
            Filter = filter;
            Op = op;
            UseFastSearch = useFastSearch;
        }

        /// <summary>
        ///  Create a SimpleReadPlan
        /// </summary>
        /// <param name="antennaList">List of antenna numbers.  May be null.</param>
        /// <param name="protocol">Protocol identifier</param>
        /// <param name="filter">Tag filter.  May be null.</param>
        /// <param name="op">Operation mode</param>
        /// <param name="weight">Relative scheduling weight</param>
        /// <param name="useFastSearch">Enable fast search</param>
        public SimpleReadPlan(ICollection<int> antennaList, TagProtocol protocol, TagFilter filter, TagOp op, bool useFastSearch, int weight)
            : base(weight)
        {
            Antennas = (antennaList != null) ? CollUtil.ToArray(antennaList) : null;
            Protocol = protocol;
            Filter = filter;
            Op = op;
            UseFastSearch = useFastSearch;
        }

        #endregion

        #region ToString

        /// <summary>
        /// Human-readable representation
        /// </summary>
        /// <returns>Human-readable representation; e.g.,
        /// SimpleReadPlan:[[GEN2],[1,2],1000]</returns>
        public override string ToString()
        {
            return String.Format(
                "SimpleReadPlan:[{0},{1},{2},{3:D}]",
                ArrayToString(Antennas),
                Protocol.ToString(),
                (null == Filter) ? "null" : Filter.ToString(),
                Weight);
        }

        #endregion

        #region ArrayToString

        private static string ArrayToString(Array array)
        {
            if (null == array)
                return "null";

            List<string> words = new List<string>();

            foreach (Object elt in array)
                words.Add(elt.ToString());

            return String.Format("[{0}]", String.Join(",", words.ToArray()));
        } 
        #endregion
    }
}
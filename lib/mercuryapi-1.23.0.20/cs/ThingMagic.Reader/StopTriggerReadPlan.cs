/*
 * Copyright (c) 2013 ThingMagic.
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
using System.Text;

namespace ThingMagic
{
    /// <summary>
    /// StopTriggerReadPlan consisting of stopontagcount, list of antennas, tag protocol, filter and weight
    /// </summary>
    public class StopTriggerReadPlan : SimpleReadPlan
    {
        /// <summary>
        /// Stop on tag count
        /// </summary>
        public StopOnTagCount stopOnCount = null;

        #region Construction

        /// <summary>
        /// Create StopTriggerReadPlan
        /// </summary>
        /// <param name="sotc">StopOnTagCount object</param>
        public StopTriggerReadPlan(StopOnTagCount sotc) : base()
        {
            stopOnCount = sotc;
        }

        /// <summary>
        /// Create StopTriggerReadPlan
        /// </summary>
        /// <param name="sotc">StopOnTagCount object</param>
        /// <param name="antennaList">List of antenna numbers</param>
        /// <param name="protocol">Protocol identifier</param>
        public StopTriggerReadPlan(StopOnTagCount sotc, ICollection<int> antennaList, TagProtocol protocol)
            : base(antennaList, protocol)
        {
            stopOnCount = sotc;
        }

        /// <summary>
        /// Create StopTriggerReadPlan
        /// </summary>
        /// <param name="sotc">StopOnTagCount object</param>
        /// <param name="antennaList">List of antenna numbers</param>
        /// <param name="protocol">Protocol identifier</param>
        /// <param name="filter">Tag filter.  May be null.</param>
        /// <param name="weight">Relative scheduling weight</param>
        public StopTriggerReadPlan(StopOnTagCount sotc, ICollection<int> antennaList, TagProtocol protocol, TagFilter filter, int weight)
            : base(antennaList, protocol, filter, weight)
        {
            stopOnCount = sotc;
        }

        /// <summary>
        /// Create StopTriggerReadPlan
        /// </summary>
        /// <param name="sotc">StopOnTagCount object</param>
        /// <param name="antennaList">List of antenna numbers</param>
        /// <param name="protocol">Protocol identifier</param>
        /// <param name="filter">Tag filter.  May be null.</param>
        /// <param name="op">Operation mode</param>
        /// <param name="weight">Relative scheduling weight</param>
        public StopTriggerReadPlan(StopOnTagCount sotc, ICollection<int> antennaList, TagProtocol protocol, TagFilter filter, TagOp op, int weight)
            : base(antennaList, protocol, filter, op, weight)
        {
            stopOnCount = sotc;
        }

        /// <summary>
        /// Create StopTriggerReadPlan
        /// </summary>
        /// <param name="sotc">StopOnTagCount object</param>
        /// <param name="antennaList">List of antenna numbers</param>
        /// <param name="protocol">Protocol identifier</param>
        /// <param name="filter">Tag filter.  May be null.</param>
        /// <param name="op">Operation mode</param>
        /// <param name="useFastSearch">Enable fast search</param>
        public StopTriggerReadPlan(StopOnTagCount sotc, ICollection<int> antennaList, TagProtocol protocol, TagFilter filter, TagOp op, bool useFastSearch)
            : base(antennaList, protocol, filter, op, useFastSearch)
        {
            stopOnCount = sotc;
        }

        /// <summary>
        /// Create StopTriggerReadPlan
        /// </summary>
        /// <param name="sotc">StopOnTagCount object</param>
        /// <param name="antennaList">List of antenna numbers</param>
        /// <param name="protocol">Protocol identifier</param>
        /// <param name="filter">Tag filter.  May be null.</param>
        /// <param name="op">Operation mode</param>
        /// <param name="useFastSearch">Enable fast search</param>
        /// <param name="weight">Relative scheduling weight</param>
        public StopTriggerReadPlan(StopOnTagCount sotc, ICollection<int> antennaList, TagProtocol protocol, TagFilter filter, TagOp op, bool useFastSearch, int weight)
            : base(antennaList, protocol, filter, op, useFastSearch, weight)
        {
            stopOnCount = sotc;
        }

        #endregion Construction
    }
}
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
    /// ReadPlan containing a list of read plans to be executed sequentially
    /// with time divided according to relative weights.
    /// </summary>
    public class MultiReadPlan : ReadPlan
    {
        #region Fields

        /// <summary>
        /// List of read plans
        /// </summary>
        public ReadPlan[] Plans;

        /// <summary>
        /// Sum of contained ReadPlan weights
        /// </summary>
        public int TotalWeight; 

        #endregion

        #region Construction

        /// <summary>
        /// Create MultiReadPlan
        /// </summary>
        /// <param name="plans">List of read plans</param>
        public MultiReadPlan(ICollection<ReadPlan> plans) : this(plans, ReadPlan.DEFAULT_WEIGHT) { }

        /// <summary>
        /// Create MultiReadPlan
        /// </summary>
        /// <param name="plans">List of read plans</param>
        /// <param name="weight">Relative weight of MultiReadPlan</param>
        public MultiReadPlan(ICollection<ReadPlan> plans, int weight)
            : base(weight)
        {
            Plans       = CollUtil.ToArray(plans);
            TotalWeight = 0;

            foreach (ReadPlan rp in plans)
                TotalWeight += rp.Weight;
        }

        #endregion

        /// <summary>
        /// Returns a String that represents the current Object.
        /// </summary>
        /// <returns>A String that represents the current Object.</returns>
        public override string ToString()
        {
            List<string> substrings = new List<string>();
            foreach (ReadPlan subitem in Plans)
            {
                substrings.Add(subitem.ToString());
            }
            return String.Format("MultiReadPlan:[{0}]",
                String.Join(", ", substrings.ToArray()));
        }
    }
}
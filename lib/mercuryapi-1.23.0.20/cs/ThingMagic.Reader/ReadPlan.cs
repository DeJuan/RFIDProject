
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

namespace ThingMagic
{
    /// <summary>
    /// Tag read (search) constraints
    /// </summary>
    public class ReadPlan
    {
        #region Constants
        /// <summary>
        /// Default for Weight
        /// </summary>
        protected const int DEFAULT_WEIGHT = 1000;

        #endregion

        #region Fields

        /// <summary>
        /// Relative weight for time allocation against other read plans
        /// </summary>
        public int Weight; 

        #endregion

        #region Construction

        /// <summary>
        /// Create read plan with default weight
        /// </summary>
        protected ReadPlan() : this(DEFAULT_WEIGHT) { }

        /// <summary>
        /// Create read plan
        /// </summary>
        /// <param name="weight">Relative scheduling weight</param>
        protected ReadPlan(int weight)
        {
            Weight = weight;
        }

        #endregion
    }
}
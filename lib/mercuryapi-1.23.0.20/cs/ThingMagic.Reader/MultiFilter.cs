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
    /// This represents an ordered sequence of filter operations.
    /// </summary>
    public class MultiFilter : TagFilter
    {
        #region Fields

        TagFilter[] filters;

        #endregion

        #region Constructors

        /// <summary>
        /// Create ordered sequence of filters
        /// </summary>
        /// <param name="filters">List of filters</param>
        public MultiFilter(ICollection<TagFilter> filters)
        {
            this.filters = CollUtil.ToArray(filters);
        }

        #endregion

        #region Matches

        /// <summary>
        /// Test if a tag Matches this filter. Only applies to selects based
        /// on the EPC.
        /// </summary>
        /// <param name="t">tag data to screen</param>
        /// <returns>Return true to allow tag through the filter.
        /// Return false to reject tag.</returns>
        public bool Matches(TagData t)
        {
            foreach( TagFilter filter in filters )
            {
                if( !filter.Matches(t) )
                    return false;
            }

            return true;
        }

        #endregion

        /// <summary>
        /// Returns a String that represents the current Object.
        /// </summary>
        /// <returns>A String that represents the current Object.</returns>
        public override string ToString()
        {
            List<string> substrings = new List<string>();
            foreach (TagFilter subitem in filters)
            {
                substrings.Add(subitem.ToString());
            }
            return String.Format("MultiFilter:[{0}]",
                String.Join(", ", substrings.ToArray()));
        }
    }
}
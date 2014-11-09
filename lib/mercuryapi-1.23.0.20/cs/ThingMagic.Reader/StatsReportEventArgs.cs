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
    /// ReaderStatsReport
    /// </summary>
    public class ReaderStatsReport
    {
        /// <summary>
        /// Reader stat values
        /// </summary>
        private Reader.Stat.Values stats = null;

        /// <summary>
        /// Default constructor
        /// </summary>
        public ReaderStatsReport() { }

        /// <summary>
        /// Get/set reader stat values
        /// </summary>
        public Reader.Stat.Values STATS
        {
            get { return stats; }
            set { stats = value; }
        }

        /// <summary>
        /// Human-readable representation
        /// </summary>
        /// <returns>Human-readable representation</returns>
        public override string ToString()
        {
            StringBuilder statsReport = new StringBuilder();

            if (null != stats)
            {
                statsReport.Append(STATS);
            }

            return statsReport.ToString();
        }
    }

    /// <summary>
    /// This object sends reader statics to StatusListener
    /// </summary>
    public class StatsReportEventArgs : EventArgs
    {

        private ReaderStatsReport _statsReport;

        /// <summary>
        /// StatusReport
        /// </summary>
        public ReaderStatsReport StatsReport
        {
            get { return _statsReport; }
            set { _statsReport = value; }
        }

        /// <summary>
        /// Default constructor
        /// </summary>
        public StatsReportEventArgs()
        {
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="statsReport">array of status reports</param>
        public StatsReportEventArgs(ReaderStatsReport statsReport)
        {
            _statsReport = statsReport;
        }

        /// <summary>
        /// Human-readable representation
        /// </summary>
        /// <returns>Human-readable representation</returns>
        public override string ToString()
        {

            StringBuilder statsReport = new StringBuilder();

            statsReport.Append(_statsReport);

            return statsReport.ToString();
        }
    }
}

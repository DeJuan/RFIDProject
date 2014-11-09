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
using System.Text;

namespace ThingMagic
{  
    /// <summary>
    /// This object contains the information related to status reports
    /// sent by the module during continuous reading
    /// </summary>
    public class StatusReport
    {
    }

    /// <summary>
    /// AntennaStatusReport
    /// </summary>
    public  class AntennaStatusReport : StatusReport
    {
        /// <summary>
        /// antenna
        /// </summary>
        private int? antenna = null;

        /// <summary>
        /// Default constructor
        /// </summary>
        public AntennaStatusReport() { }

        /// <summary>
        /// Antenna
        /// </summary>
        public int? Antenna
        {
            get { return antenna; }
            set { antenna = value; }
        }

        /// <summary>
        /// Human-readable representation
        /// </summary>
        /// <returns>Human-readable representation</returns>
        public override string ToString()
        {
            StringBuilder statusReport = new StringBuilder();

            if (null != antenna)
            {
                statusReport.Append("Antenna : ");
                statusReport.Append(antenna);
            }

            return statusReport.ToString();
        }


    }
    /// <summary>
    /// FrequencyStatusReport
    /// </summary>
    public  class FrequencyStatusReport : StatusReport
    {
      
        /// <summary>
        ///  module actually reports a u24value, but 32-bit is the closest data type.
        /// </summary>
        private int? frequency = null;

        /// <summary>
        /// Default constructor
        /// </summary>
        public FrequencyStatusReport() { }

        /// <summary>
        /// Frequency
        /// </summary>
        public int? Frequency
        {
            get { return frequency; }
            set { frequency = value; }
        }

        /// <summary>
        /// Human-readable representation
        /// </summary>
        /// <returns>Human-readable representation</returns>
        public override string ToString()
        {
            StringBuilder statusReport = new StringBuilder();

            if (null != frequency)
            {
                statusReport.Append("Frequency : ");
                statusReport.Append(frequency);
            }

            return statusReport.ToString();
        }

    }
    /// <summary>
    /// TemperatureStatusReport
    /// </summary>
    public  class TemperatureStatusReport : StatusReport
    {
        /// <summary>
        /// Temperature
        /// </summary>
        private int? temperature = null;

        /// <summary>
        /// Default constructor
        /// </summary>
        public TemperatureStatusReport() { }

        /// <summary>
        /// Temperature
        /// </summary>
        public int? Temperature
        {
            get { return temperature; }
            set { temperature = value; }
        }

        /// <summary>
        /// Human-readable representation
        /// </summary>
        /// <returns>Human-readable representation</returns>
        public override string ToString()
        {
            StringBuilder statusReport = new StringBuilder();

            if (null != temperature)
            {
                statusReport.Append("Temperature : ");
                statusReport.Append(temperature);
            }

            return statusReport.ToString();
        }
    }

    /// <summary>
    /// This object sends reader statics to StatusListener
    /// </summary>
    public class StatusReportEventArgs : EventArgs
    {

        private StatusReport _statusReport;

        /// <summary>
        /// StatusReport
        /// </summary>
        public StatusReport StatusReport
        {
            get { return _statusReport; }
            set { _statusReport = value; }
        }
        
        /// <summary>
        /// Default constructor
        /// </summary>
        public StatusReportEventArgs() 
        {
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="statusReport">array of status reports</param>
        public StatusReportEventArgs(StatusReport statusReport)
        {
            _statusReport = statusReport;
        }

        /// <summary>
        /// Human-readable representation
        /// </summary>
        /// <returns>Human-readable representation</returns>
        public override string ToString()
        {
            StringBuilder statusReport = new StringBuilder();


            if (_statusReport is FrequencyStatusReport)
            {
                statusReport.Append("Frequency : ");
                statusReport.Append(((FrequencyStatusReport)_statusReport).Frequency);
            }

            if (_statusReport is TemperatureStatusReport)
            {
                statusReport.Append("Temperature : ");
                statusReport.Append(((TemperatureStatusReport)_statusReport).Temperature);                
            }

            if (_statusReport is AntennaStatusReport)
            {
                statusReport.Append("Antenna : ");
                statusReport.Append(((AntennaStatusReport)_statusReport).Antenna);
            }

            return statusReport.ToString();
        }
    } 
}

using System;
using System.Linq;
using System.Collections.Generic;
using System.Text;

namespace ThingMagic.RFIDSearchLight
{
    public class LogProvider
    {
        /// <summary>
        /// Function signature for log message handlers
        /// </summary>
        /// <param name="message"></param>
        public delegate void LogHandler(string message);

        /// <summary>
        /// Subscribe a LogHandler to Log to receive log messages
        /// </summary>
        public event LogHandler Log;

        /// <summary>
        /// Call OnLog to send a log message
        /// </summary>
        /// <param name="message"></param>
        public void OnLog(string message)
        {
            if (null != Log) { Log(message); }
        }
    }
}

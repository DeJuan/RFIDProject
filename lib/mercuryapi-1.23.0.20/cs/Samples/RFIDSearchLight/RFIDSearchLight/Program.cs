using log4net;
using log4net.Config;
using System;
using System.Linq;
using System.Collections.Generic;
using System.Windows.Forms;

namespace ThingMagic.RFIDSearchLight
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [MTAThread]
        static void Main()
        {
            XmlConfigurator.Configure();
            ILog logger = LogManager.GetLogger("Program");
            logger.Info("Program started");
            Application.Run(new Info());
        }
    }
}
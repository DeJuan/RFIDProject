/**
 * Sample program that get stats in the background and prints the
 * stats.
 */
// Import the API
package samples;

import com.thingmagic.*;
import com.thingmagic.SerialReader.StatusReport;
import java.text.SimpleDateFormat;
import java.util.Calendar;

public class ReaderStatisticsAsync
{
    static SerialPrinter serialPrinter;
    static StringPrinter stringPrinter;
    static TransportListener currentListener;
    static void usage()
    {
        System.out.printf("Usage: demo reader-uri <command> [args]\n"
                + "  (URI: 'tmr:///COM1' or 'tmr://astra-2100d3/' "
                + "or 'tmr:///dev/ttyS0')\n\n"
                + "Available commands:\n");
        System.exit(1);
    }

    public static void setTrace(Reader r, String args[])
    {
      if (args[0].toLowerCase().equals("on"))
      {
        r.addTransportListener(Reader.simpleTransportListener);
        currentListener = Reader.simpleTransportListener;
      }
      else if (currentListener != null)
      {
        r.removeTransportListener(Reader.simpleTransportListener);
      }
    }
    static class SerialPrinter implements TransportListener
    {
      public void message(boolean tx, byte[] data, int timeout)
      {
        System.out.print(tx ? "Sending: " : "Received:");
        for (int i = 0; i < data.length; i++)
        {
          if (i > 0 && (i & 15) == 0)
          System.out.printf("\n         ");
          System.out.printf(" %02x", data[i]);
        }
        System.out.printf("\n");
      }
    }
    static class StringPrinter implements TransportListener
    {
      public void message(boolean tx, byte[] data, int timeout)
      {
        System.out.println((tx ? "Sending:\n" : "Receiving:\n") +
         new String(data));
      }
    }

    public static void main(String argv[]) {
        // Program setup
        Reader r = null;
        int nextarg = 0;
        boolean trace = false;

        if (argv.length < 1)
        {
            usage();
        }

        if (argv[nextarg].equals("-v"))
        {
            trace = true;
            nextarg++;
        }

        // Create Reader object, connecting to physical device
        try
        {
            r = Reader.create(argv[nextarg]);
            if (trace)
            {
                setTrace(r, new String[]{"on"});
            }
            r.connect();
            if (Reader.Region.UNSPEC == (Reader.Region) r.paramGet("/reader/region/id"))
            {
                Reader.Region[] supportedRegions = (Reader.Region[]) r.paramGet(TMConstants.TMR_PARAM_REGION_SUPPORTEDREGIONS);
                if (supportedRegions.length < 1)
                {
                    throw new Exception("Reader doesn't support any regions");
                }
                else
                {
                    r.paramSet("/reader/region/id", supportedRegions[0]);
                }
            }

            String model = (String)r.paramGet(TMConstants.TMR_PARAM_VERSION_MODEL);
            if (TMConstants.TMR_READER_M6E_MICRO.equals(model))
            {
              SimpleReadPlan plan = new SimpleReadPlan(new int[]{1}, TagProtocol.GEN2, null, null, 1000);
              r.paramSet("/reader/read/plan", plan);
            }

            SerialReader.ReaderStatsFlag[] READER_STATISTIC_FLAGS = { SerialReader.ReaderStatsFlag.RF_ON_TIME,
            SerialReader.ReaderStatsFlag.FREQUENCY,SerialReader.ReaderStatsFlag.ANTENNA,
            SerialReader.ReaderStatsFlag.CONNECTED_ANTENNA_PORTS,SerialReader.ReaderStatsFlag.ANTENNA.NOISE_FLOOR_SEARCH_RX_TX_WITH_TX_ON,
            SerialReader.ReaderStatsFlag.ANTENNA.PROTOCOL,SerialReader.ReaderStatsFlag.TEMPERATURE};

            r.paramSet(TMConstants.TMR_PARAM_READER_STATS_ENABLE, READER_STATISTIC_FLAGS);
            SerialReader.ReaderStatsFlag[] getReaderStatisticFlag = (SerialReader.ReaderStatsFlag[]) r.paramGet(TMConstants.TMR_PARAM_READER_STATS_ENABLE);
            if (READER_STATISTIC_FLAGS.equals(getReaderStatisticFlag))
            {
                System.out.println("GetReaderStatsEnable--pass");
            }
            else
            {
                System.out.println("GetReaderStatsEnable--Fail");
            }

            ReadExceptionListener exceptionListener = new TagReadExceptionReceiver();
            r.addReadExceptionListener(exceptionListener);
            // Create and add tag listener
            ReadListener readListener = new PrintListener();
            r.addReadListener(readListener);
            StatsListener statsListener = new ReaderStatsListener();
            r.addStatsListener(statsListener);
            // search for tags in the background
            r.startReading();
            System.out.println("Do other work here 1");
            Thread.sleep(1000);
            System.out.println("Do other work here 2");
            Thread.sleep(1000);
            r.stopReading();

            r.removeReadListener(readListener);
            r.removeReadExceptionListener(exceptionListener);
            
            r.destroy();
        }
        catch (ReaderException re)
        {
            re.printStackTrace();
            System.out.println("Reader Exception : " + re.getMessage());
        }
        catch (Exception re)
        {
            System.out.println("Exception : " + re.getMessage());
        }
    }
    static class PrintListener implements ReadListener
    {
      public void tagRead(Reader r, TagReadData tr)
      {
        System.out.println("Background read: " + tr.toString());
      }
    }

    static class TagReadExceptionReceiver implements ReadExceptionListener
    {
      String strDateFormat = "M/d/yyyy h:m:s a";
      SimpleDateFormat sdf = new SimpleDateFormat(strDateFormat);
      public void tagReadException(com.thingmagic.Reader r, ReaderException re)
      {
        String format = sdf.format(Calendar.getInstance().getTime());
        System.out.println("Reader Exception: " + re.getMessage() + " Occured on :" + format);
        if(re.getMessage().equals("Connection Lost"))
        {
          System.exit(1);
        }
      }
    }
    static class ReaderStatsListener implements StatsListener
    {
      public void statsRead(SerialReader.ReaderStats readerStats)
      {
        System.out.println("Frequency   :  " + readerStats.frequency + " kHz");
        System.out.println("Temperature :  " + readerStats.temperature + " C");
        System.out.println("Protocol    :  " + readerStats.protocol);
        System.out.println("Connected antenna port : " + readerStats.antenna);

        int[] connectedAntennaPorts = readerStats.connectedAntennaPorts;
        for (int index = 0; index < connectedAntennaPorts.length; index++)
        {
          int antenna = connectedAntennaPorts[index];
          System.out.println("Antenna  [" + antenna + "] is : Connected" );
        }

        int[] rfontimes = readerStats.rfOnTime;
        for (int antenna = 0; antenna < rfontimes.length; antenna++)
        {
          System.out.println("RF_ON_TOME for antenna [" + (antenna + 1) + "] is : " + rfontimes[antenna] +" ms");
        }

        byte[] noiseFloorTxOn = readerStats.noiseFloorTxOn;
        for (int antenna = 0; antenna < noiseFloorTxOn.length; antenna++)
        {
          System.out.println("NOICE_FLOOR_TX_ON for antenna [" + (antenna + 1) + "] is : " + noiseFloorTxOn[antenna] +" db");
        }
      }
    }
    static class ReaderStatusListener implements  StatusListener
    {
        public void statusMessage(Reader r, StatusReport[] statusReport)
        {
            for (StatusReport statusReport1 : statusReport)
            {
                System.out.println("statusReport1 :"+ statusReport1.toString());
            }
        }
    }
}

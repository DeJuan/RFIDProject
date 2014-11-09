/**
 * Sample program that gets and prints the reader stats
 */
// Import the API
package samples;

import com.thingmagic.*;
import java.util.ArrayList;
import java.util.List;

public class ReaderStatistics
{

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
           r.addTransportListener(r.simpleTransportListener);
        }
    }

    static class TagReadListener implements ReadListener
    {

        public void tagRead(Reader r, TagReadData t)
        {
            System.out.println("Tag Read " + t);
        }
    }

    public static void main(String argv[])
    {
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

            SerialReader.ReaderStatsFlag[] READER_STATISTIC_FLAGS = {SerialReader.ReaderStatsFlag.ANTENNA};

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

            TagReadData[] tagReads;
            tagReads = r.read(500);
            // Print tag reads
            List<String> epcList = new ArrayList<String>();
            for (TagReadData tr : tagReads)
            {
                String epcString = tr.getTag().epcString();
                System.out.println(tr.toString());
                if (!epcList.contains(epcString))
                {
                    epcList.add(epcString);
                }
            }

            SerialReader.ReaderStats readerStats = (SerialReader.ReaderStats) r.paramGet(TMConstants.TMR_PARAM_READER_STATS);

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
            
            /* Get the antenna return loss value, this parameter is not the part of reader stats */
            int[][] returnLoss=(int[][]) r.paramGet(TMConstants.TMR_PARAM_ANTENNA_RETURNLOSS);
            for (int[] rl : returnLoss)
            {
              System.out.println("Antenna ["+rl[0] +"] returnloss :"+ rl[1]);
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
            r.destroy();
        }
        catch (ReaderException re)
        {
            System.out.println("Reader Exception : " + re.getMessage());
        } 
        catch (Exception re)
        {
            System.out.println("Exception : " + re.getMessage());
        }
    }
}

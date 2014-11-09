/*
 * Sample program that shows how to create a
 * simpleReadPlan that uses a list of antennas as passed by the user
 * and prints the tags found.
 */
package samples;

import com.thingmagic.Reader;
import com.thingmagic.SimpleReadPlan;
import com.thingmagic.TMConstants;
import com.thingmagic.TagProtocol;
import com.thingmagic.TagReadData;
import com.thingmagic.TransportListener;

/**
 *
 * @author qvantel
 */
public class AntennaList
{
    static void usage()
    {
        System.out.printf("Please provide reader URL, such as:\n"
                + "  (URI: 'tmr:///COM1' or 'tmr://astra-2100d3/' "
                + "or 'tmr:///dev/ttyS0')\n\n");
        System.exit(1);
    }

    public static void setTrace(Reader r, String args[])
    {
        if (args[0].toLowerCase().equals("on"))
        {
            r.addTransportListener(r.simpleTransportListener);
        }
    }

    static class StringPrinter implements TransportListener
    {
        public void message(boolean tx, byte[] data, int timeout)
        {
            System.out.println((tx ? "Sending:\n" : "Receiving:\n")
                    + new String(data));
        }
    }

    static class SerialPrinter implements TransportListener
    {
        public void message(boolean tx, byte[] data, int timeout)
        {
            System.out.print(tx ? "Sending: " : "Received:");
            for (int i = 0; i < data.length; i++)
            {
                if (i > 0 && (i & 15) == 0) {
                    System.out.printf("\n         ");
                }
                System.out.printf(" %02x", data[i]);
            }
            System.out.printf("\n");
        }
    }

    public static void main(String argv[])
    {
        Reader r = null;
        int[] antennaList = new int[]{1};
        TagReadData[] tagReads;
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

        try
        {
            r = Reader.create(argv[nextarg]);
            if (trace)
            {
                setTrace(r, new String[]{"on"});
            }
            r.connect();
            if (Reader.Region.UNSPEC == (Reader.Region) r.paramGet(TMConstants.TMR_PARAM_REGION_ID))
            {
                Reader.Region[] supportedRegions = (Reader.Region[]) r.paramGet(TMConstants.TMR_PARAM_REGION_SUPPORTEDREGIONS);
                if (supportedRegions.length < 1) {
                    throw new Exception("Reader doesn't support any regions");
                } else {
                    r.paramSet(TMConstants.TMR_PARAM_REGION_ID, supportedRegions[0]);
                }
            }
            /*
             * For antenna configuration we need  specifies  a list of antennas and set to  read plan.
             */
            SimpleReadPlan plan = new SimpleReadPlan(antennaList, TagProtocol.GEN2, null, null, 1000);
            r.paramSet(TMConstants.TMR_PARAM_READ_PLAN, plan);
            // Read tags
            tagReads = r.read(500);
            for (TagReadData tr : tagReads)
            {
                System.out.println("Tag : " + tr.toString());
            }
            // Shut down reader
            r.destroy();
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }
}

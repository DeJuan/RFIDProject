/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package samples;

import com.thingmagic.Iso180006b;
import com.thingmagic.ReadListener;
import com.thingmagic.Reader;
import com.thingmagic.ReaderException;
import com.thingmagic.SimpleReadPlan;
import com.thingmagic.TMConstants;
import com.thingmagic.TagFilter;
import com.thingmagic.TagProtocol;
import com.thingmagic.TagReadData;
import com.thingmagic.TransportListener;

/**
 *
 * @author qvantel
 */
public class ReadAsyncFilterISO18k6b
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
                {
                    System.out.printf("\n         ");
                }
                System.out.printf(" %02x", data[i]);
            }
            System.out.printf("\n");
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

    public static void main(String argv[]) throws ReaderException
    {

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
        
        try
        {
            r = Reader.create(argv[nextarg]);
            if (trace)
            {
                setTrace(r, new String[]{"on"});
            }
            r.connect();
            if (Reader.Region.UNSPEC == (Reader.Region)r.paramGet("/reader/region/id"))
            {
                Reader.Region[] supportedRegions = (Reader.Region[])r.paramGet(TMConstants.TMR_PARAM_REGION_SUPPORTEDREGIONS);
                if (supportedRegions.length < 1)
                {
                    throw new Exception("Reader doesn't support any regions");
                }
                else
                {
                    r.paramSet("/reader/region/id", supportedRegions[0]);
                }
            }
            // Create and add tag listener
            ReadListener rl = new PrintListener();
            r.addReadListener(rl);
            r.paramSet(TMConstants.TMR_PARAM_ISO180006B_DELIMITER, Iso180006b.Delimiter.DELIMITER1);
            Iso180006b.Select filt = new Iso180006b.Select(false, Iso180006b.SelectOp.NOTEQUALS, (byte) 0, (byte) 0xFF, new byte[] { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });

            SimpleReadPlan plan = new SimpleReadPlan(null, TagProtocol.ISO180006B, filt, null, 1000);
            r.paramSet(TMConstants.TMR_PARAM_READ_PLAN, plan);
             
            // search for tags in the background
            r.startReading();
            System.out.println("Do other work here");
            Thread.sleep(1000);
            System.out.println("Do other work here");
            Thread.sleep(500);
            r.stopReading();

            r.removeReadListener(rl);

        }
        catch (ReaderException re)
        {
            System.out.println("ReaderException: " + re.getMessage());
        }
        catch (Exception re)
        {
            System.out.println("ReaderException: " + re.getMessage());
        }
        finally
        {
            // Shut down reader
            r.destroy();
        }
    }

    static class PrintListener implements ReadListener
    {
        public void tagRead(Reader r, TagReadData tr)
        {
            System.out.println("Background read: " + tr.toString());
        }
    }
}

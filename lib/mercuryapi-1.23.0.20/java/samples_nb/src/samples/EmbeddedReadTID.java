package samples;

import com.thingmagic.*;
import java.io.IOException;

public class EmbeddedReadTID
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

    public static void main(String argv[]) throws IOException
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

            TagReadData[] tagReads;

            r = Reader.create(argv[nextarg]);
            if (trace)
            {
                setTrace(r, new String[]{"on"});
            }
            r.connect();
            if (Reader.Region.UNSPEC == (Reader.Region) r.paramGet("/reader/region/id"))
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
            byte length;
            String model = (String)r.paramGet("/reader/version/model");
            if ("M6e".equals(model) || "M6e Micro".equals(model) || "M6e PRC".equals(model) || "Mercury6".equals(model) || "Astra-EX".equals(model))
            {
                // Specifying the readLength = 0 will return full TID for any tag read in case of M6e, M6 and Astra-EX reader.
                length = 0;
            }
            else
            {
                length = 2;
            }
            TagOp op = new Gen2.ReadData(Gen2.Bank.TID, 0, length);
            SimpleReadPlan plan = new SimpleReadPlan(null, TagProtocol.GEN2, null, op, 1000);
            r.paramSet("/reader/read/plan", plan);
            // Read tags
            tagReads = r.read(500);
            for (TagReadData tr : tagReads)
            {
                for (byte b : tr.getData())
                {
                    System.out.printf("%02x", b);
                    System.out.printf("\n");
                }
            }
        }
        catch (ReaderException re)
        {
            System.out.println("ReaderException: " + re.getMessage());
        }
        catch (Exception re)
        {
            System.out.println("Exception: " + re.getMessage());
        }
    }
}

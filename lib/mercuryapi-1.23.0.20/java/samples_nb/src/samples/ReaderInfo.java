/*
 * Sample program that display reader parameters
 */
package samples;

import com.thingmagic.Reader;
import com.thingmagic.TMConstants;
import com.thingmagic.TransportListener;

/**
 *
 * @author qvantel
 */
public class ReaderInfo
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

            try
            {
                String version_hardware = (String) r.paramGet(TMConstants.TMR_PARAM_VERSION_HARDWARE);
                System.out.println("Hardware Version :" + version_hardware);
            } catch (Exception ex) {
                System.out.println("Hardware Version :" + ex.getMessage());
            }

            try
            {
                String version_serial = (String) r.paramGet(TMConstants.TMR_PARAM_VERSION_SERIAL);
                System.out.println("Serial Version :" + version_serial);
            } catch (Exception ex) {
                System.out.println("Serial Version :" + ex.getMessage());
            }
            try
            {
                String version_model = (String) r.paramGet(TMConstants.TMR_PARAM_VERSION_MODEL);
                System.out.println("Model Version  :" + version_model);
            } catch (Exception ex) {
                System.out.println("Model Version :" + ex.getMessage());
            }

            try
            {
                String version_software = (String) r.paramGet(TMConstants.TMR_PARAM_VERSION_SOFTWARE);
                System.out.println("Software Version :" + version_software);
            } catch (Exception ex) {
                System.out.println("Software Version :" + ex.getMessage());
            }

            try
            {
                String reader_uri = (String) r.paramGet(TMConstants.TMR_PARAM_READER_URI);
                System.out.println("Reader Uri :" + reader_uri);
            } catch (Exception ex) {
                System.out.println("Reader Uri :" + ex.getMessage());
            }

            try
            {
                String reader_productgroupid = (String) r.paramGet(TMConstants.TMR_PARAM_READER_PRODUCTGROUPID);
                System.out.println("Reader Product GroupId :" + reader_productgroupid);
            } catch (Exception ex) {
                System.out.println("Reader Product GroupId :" + ex.getMessage());
            }

            try
            {
                String reader_productgroup = (String) r.paramGet(TMConstants.TMR_PARAM_READER_PRODUCTGROUP);
                System.out.println("Reader Product Group :" + reader_productgroup);
            } catch (Exception ex) {
                System.out.println("Reader Product Group :" + ex.getMessage());
            }

            try
            {
                String reader_productid = (String) r.paramGet(TMConstants.TMR_PARAM_READER_PRODUCTID);
                System.out.println("Reader Product Id :" + reader_productid);
            } catch (Exception ex) {
                System.out.println("Reader Product Id :" + ex.getMessage());
            }

            try
            {
                String reader_description = (String) r.paramGet(TMConstants.TMR_PARAM_READER_DESCRIPTION);
                System.out.println("Reader Description :" + reader_description);
            } catch (Exception ex) {
                System.out.println("Reader Description :" + ex.getMessage());
            }
            // Shut down reader
            r.destroy();
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }
}

/**
 * Sample program that demonstrates the Monza4QT tag fastid operation.
 * @file FastId.java
 */
package samples;

import com.thingmagic.Gen2;
import com.thingmagic.Reader;
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
public class FastId
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
                if (i > 0 && (i & 15) == 0)
                {
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
            TagReadData[] tagReads;
            TagFilter filter;
            byte[] mask = new byte[4];
            Gen2.Impinj.Monza4.QTPayload payLoad;
            Gen2.Impinj.Monza4.QTControlByte controlByte;
            Gen2.Impinj.Monza4.QTReadWrite readWrite;
            int accesspassword = 0;

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

            /* setup the reader */
            int readPower = 0x0BB8;
            r.paramSet(TMConstants.TMR_PARAM_RADIO_READPOWER, readPower);

            int writePower = 0x0BB8;
            r.paramSet(TMConstants.TMR_PARAM_RADIO_WRITEPOWER, writePower);

            r.paramSet(TMConstants.TMR_PARAM_TAGOP_ANTENNA, 1);

            Gen2.Session session = Gen2.Session.S0;
            r.paramSet(TMConstants.TMR_PARAM_GEN2_SESSION, session);

            SimpleReadPlan readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, null, null, 1000);
            r.paramSet("/reader/read/plan", readPlan);

            // Reading tags with a Monza 4 public EPC in response
            System.out.println("Reading tags with a Monza 4 public EPC in response");
            tagReads = r.read(1000);
            for (TagReadData tagData : tagReads)
            {
                 System.out.println("monza4_tag_epc: " +tagData.epcString());
            }

            /* Initialize the payload and the controlByte of Monza4 */
            payLoad = new Gen2.Impinj.Monza4.QTPayload();
            controlByte = new Gen2.Impinj.Monza4.QTControlByte();

            System.out.println("Changing to private Mode ");
            /* executing Monza4 QT Write Set Private tagop */
            payLoad.QTMEM = false;
            payLoad.QTSR = false;
            controlByte.QTReadWrite = true;
            controlByte.Persistence = true;

            readWrite = new Gen2.Impinj.Monza4.QTReadWrite(accesspassword, payLoad, controlByte);
            r.executeTagOp(readWrite, null);

            /* setting the session to S2 */
            session = Gen2.Session.S2;
            r.paramSet(TMConstants.TMR_PARAM_GEN2_SESSION, session);

            /*Enable filter */
            mask[0] = (byte) 0x20;
            mask[1] = (byte) 0x01;
            mask[2] = (byte) 0xB0;
            mask[3] = (byte) 0x00;
            filter = new Gen2.Select(true, Gen2.Bank.TID, 0x04, 0x18, mask);

            System.out.println("reading tags private Mode with session s2 ");
            readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, null, 1000);
            r.paramSet("/reader/read/plan", readPlan);
            // Reading tags with a Monza 4 FastID with TID in response
            tagReads = r.read(1000);
            for (TagReadData tagData : tagReads)
            {
                 System.out.println("monza4_tag_epc: " +tagData.epcString());
            }
            
            System.out.println("setting the session to S0");

            /* setting the session to S0 */
            session = Gen2.Session.S0;
            r.paramSet(TMConstants.TMR_PARAM_GEN2_SESSION, session);

            mask[0] = (byte) 0xE2;
            mask[1] = (byte) 0x80;
            mask[2] = (byte) 0x11;
            mask[3] = (byte) 0x05;
            filter = new Gen2.Select(false, Gen2.Bank.TID, 0x00, 0x20, mask);
            System.out.println("reading tags private Mode with session s0 ");
            readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, null, 1000);
            r.paramSet("/reader/read/plan", readPlan);
            //Reading tags with a Monza 4 FastID with NO TID in response
            tagReads = r.read(1000);
            for (TagReadData tagData : tagReads)
            {
                 System.out.println("monza4_tag_epc: " +tagData.epcString());
            }
            System.out.println("Converting to public mode");
            /* executing  Monza4 QT Write Set Public tagop */
            payLoad.QTMEM = true;
            payLoad.QTSR = false;
            controlByte.QTReadWrite = true;
            controlByte.Persistence = true;

            readWrite = new Gen2.Impinj.Monza4.QTReadWrite(accesspassword, payLoad, controlByte);
            r.executeTagOp(readWrite, null);

            /*Enable filter */
            mask[0] = (byte) 0x20;
            mask[1] = (byte) 0x01;
            mask[2] = (byte) 0xB0;
            mask[3] = (byte) 0x00;
            filter = new Gen2.Select(true, Gen2.Bank.TID, 0x04, 0x18, mask);
            System.out.println("reading tags public Mode with session s0 ");
            readPlan = new SimpleReadPlan(null, TagProtocol.GEN2, filter, null, 1000);
            r.paramSet("/reader/read/plan", readPlan);
            // Reading tags with a Monza 4 FastID with TID in response
            tagReads = r.read(1000);
            for (TagReadData tagData : tagReads)
            {
                 System.out.println("monza4_tag_epc: " +tagData.epcString());
            }
           
            System.out.println("/*Reset the Read protect on */");
            /* Reset the Read protect on */
            payLoad.QTMEM = false;
            payLoad.QTSR = false;
            controlByte.QTReadWrite = false;
            controlByte.Persistence = false;

            readWrite = new Gen2.Impinj.Monza4.QTReadWrite(accesspassword, payLoad, controlByte);
            r.executeTagOp(readWrite, null);

            // Shut down reader
            r.destroy();
        } 
        catch (Exception ex)
        {
            ex.printStackTrace();
        }
    }
}

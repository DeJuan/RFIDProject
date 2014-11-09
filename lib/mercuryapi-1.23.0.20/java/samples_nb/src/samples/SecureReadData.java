/**
 * Sample program that reads tags in the background and prints the
 * tags found with corresponding password.
 */
// Import the API
package samples;

import com.thingmagic.*;
import java.text.SimpleDateFormat;
import java.util.Calendar;

public class SecureReadData
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


    public static void main(String argv[])
    {
        // Program setup
        Reader reader = null;
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
            reader = Reader.create(argv[nextarg]);
            if (trace)
            {
                setTrace(reader, new String[]{"on"});
            }
            reader.connect();
            if (Reader.Region.UNSPEC == (Reader.Region) reader.paramGet("/reader/region/id"))
            {
                Reader.Region[] supportedRegions = (Reader.Region[]) reader.paramGet(TMConstants.TMR_PARAM_REGION_SUPPORTEDREGIONS);
                if (supportedRegions.length < 1)
                {
                    throw new Exception("Reader doesn't support any regions");
                }
                else
                {
                    reader.paramSet("/reader/region/id", supportedRegions[0]);
                }
            }
//          Set tag password one time for each tag.  In real life, this occurs
//          before the tag reaches the end user.
//
//          short newpassword[] = new short[]{(short) 0x8888, (short) 0x7777};
//          TagReadData[] tags = reader.read(1000);
//          String gen2_read_tag_epc = getEPC(tags);
//          TagFilter filterToUse = new TagData(gen2_read_tag_epc);
//          Gen2.WriteData write = new Gen2.WriteData(Gen2.Bank.RESERVED, 2, newpassword);
//          reader.executeTagOp(write, filterToUse);
//
//          Gen2.ReadData rOp1 = new Gen2.ReadData(Gen2.Bank.RESERVED, 0, (byte)0);
//          short[] rdata = (short[]) reader.executeTagOp(rOp1, filterToUse);

            //Embedded Secure Read Tag Operation - Standalone operation not supported
            Gen2.Password password = new Gen2.Password(0);
            Gen2.SecureReadData secureReadDataTagOp = new Gen2.SecureReadData(Gen2.Bank.TID, 0, (byte) 0, Gen2.SecureTagType.HIGGS3, password);

            
            TagProtocol[] protocolList = (TagProtocol[]) reader.paramGet("/reader/version/supportedProtocols");
            ReadPlan setSecureReadPlan[] = new ReadPlan[protocolList.length];
            for (int i = 0; i < protocolList.length; i++)
            {
                setSecureReadPlan[i] = new SimpleReadPlan(new int[]{1, 2}, protocolList[i], null, secureReadDataTagOp, 100);
            }
            MultiReadPlan setSecureMultiReadPlan = new MultiReadPlan(setSecureReadPlan);
            reader.paramSet("/reader/read/plan", setSecureMultiReadPlan);

            // Create and add tag read authentication listener
            ReadAuthenticationListener authenticationListener = new TagReadAuthentication();
            reader.addReadAuthenticationListener(authenticationListener);
            // Create and add tag read exception listener
            ReadExceptionListener exceptionListener = new TagReadExceptionReceiver();
            reader.addReadExceptionListener(exceptionListener);
            // Create and add tag read listener
            ReadListener readListener = new PrintListener();
            reader.addReadListener(readListener);

            // search for tags in the background
            reader.startReading();

            System.out.println("Do other work here");
            Thread.sleep(600);
             System.out.println("Do other work here");
            Thread.sleep(500);
            reader.stopReading();

            //Remove listeners after stoping reading
            reader.removeReadListener(readListener);
            reader.removeReadExceptionListener(exceptionListener);
            reader.removeReadAuthenticationListener(authenticationListener);

            // Shut down reader
            reader.destroy();
        } catch (ReaderException re) {
            System.out.println("ReaderException: " + re.getMessage());
        } catch (Exception re) {
            System.out.println("Exception: " + re.getMessage());
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
            if (re.getMessage().equals("Connection Lost"))
            {
                System.exit(1);
            }
        }
    }

    static class TagReadAuthentication implements ReadAuthenticationListener
    {
        public void readTagAuthentication(TagReadData t, Reader reader) throws ReaderException
        {
            try
            {
                int accessPassword = 0;
                int tagAccessPassword1 = 0x11111111;
                int tagAccessPassword2 = 0x22222222;
                int tagAccessPassword3 = 0x33333333;
                int tagAccessPassword5 = 0x55555555;
                System.out.println("Tag epc  : " + t.epcString());
                Double tagEPC = Double.parseDouble(t.epcString());
                Double index = tagEPC % 4;
                if (index == 0) {
                    accessPassword = tagAccessPassword5;
                } else if (index == 1) {
                    accessPassword = tagAccessPassword1;
                } else if (index == 2) {
                    accessPassword = tagAccessPassword2;
                } else if (index == 3) {
                    accessPassword = tagAccessPassword3;
                }
                //Set access posword  for corresponding tag epc 
                Gen2.Password authPassword = new Gen2.Password(accessPassword);
                reader.paramSet(TMConstants.TMR_PARAM_GEN2_ACCESSPASSWORD, authPassword);
            }
            catch (ReaderException re)
            {
                re.printStackTrace();
            }
        }
    }
}

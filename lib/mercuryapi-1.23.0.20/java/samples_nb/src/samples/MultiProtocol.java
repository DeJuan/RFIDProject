/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package samples;
import com.thingmagic.*;
/**
 *
 * @author rsoni
 */
public class MultiProtocol
{
  static SerialPrinter serialPrinter;
  static StringPrinter stringPrinter;
  static TransportListener currentListener;

  static void usage()
  {
    System.out.printf("Usage: demo reader-uri <command> [args]\n" +
                      "  (URI: 'tmr:///COM1' or 'tmr://astra-2100d3/' " +
                      "or 'tmr:///dev/ttyS0')\n\n" +
                      "Available commands:\n");
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
public static void main(String argv[]) throws ReaderException
  {
    Reader r = null;
    int nextarg = 0;
    boolean trace = false;

    if (argv.length < 1)
      usage();

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
        TagProtocol[] protocolList = (TagProtocol[])r.paramGet("/reader/version/supportedProtocols");
        ReadPlan rp[]=new ReadPlan[protocolList.length];
        for(int i = 0; i < protocolList.length; i++)
        {
            rp[i] = new SimpleReadPlan(null, protocolList[i],null, null, 10);
        }
        MultiReadPlan testMultiReadPlan = new MultiReadPlan(rp);
        r.paramSet("/reader/read/plan", testMultiReadPlan);
        TagReadData[] t;
        try
        {
            t = r.read(1000);
        }
        catch (ReaderException re)
        {
            System.out.printf("Error reading tags: %s\n", re.getMessage());
            return;
        }
        for(TagReadData trd : t)
        {
            System.out.println(trd.getTag().getProtocol().toString()+": "+trd.toString());
        }

        r.destroy();
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

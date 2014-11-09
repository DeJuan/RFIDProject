/**
 * Sample program that sets an access password on a tag and locks its EPC.
 */

// Import the API
package samples;
import com.thingmagic.*;

public class locktag
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
  public static void main(String argv[])
  {
    // Program setup
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

    // Create Reader object, connecting to physical device
    try
    {

      TagReadData[] tagReads;

      r = Reader.create(argv[nextarg]);
      if (trace)
      {
        setTrace(r, new String[] {"on"});
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

      // In the current system, sequences of Gen2 operations require Session 0,
      // since each operation resingulates the tag.  In other sessions,
      // the tag will still be "asleep" from the preceding singulation.
      Gen2.Session oldSession = (Gen2.Session)r.paramGet("/reader/gen2/session");
      Gen2.Session newSession = Gen2.Session.S0;
      System.out.println("Changing to Session "+newSession+" (from Session "+oldSession+")");
      r.paramSet("/reader/gen2/session", newSession);

      try
      {
        // Find a tag to work on
        tagReads = r.read(500);
        if (tagReads.length == 0)
        {
            System.out.println("No tags found to work on");
            return;
        }
        TagData t = tagReads[0].getTag();

        r.executeTagOp(new Gen2.Lock(0, new Gen2.LockAction(Gen2.LockAction.EPC_LOCK)), t);
        System.out.println("Locked EPC of tag " + t.toString());

        // Unlock the tag
        r.executeTagOp(new Gen2.Lock(0, new Gen2.LockAction(Gen2.LockAction.EPC_UNLOCK)), t);
        System.out.println("Unlocked EPC of tag " + t.toString());
      }
      finally
      {
          // Restore original settings
          System.out.println("Restoring Session " + oldSession);
          r.paramSet("/reader/gen2/session", oldSession);
      }

      // Shut down reader
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

/**
 * Sample program that demonstrates different types and uses of
 * TagFilter objects.
 */

// Import the API
package samples;
import com.thingmagic.*;

public class filter
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

      TagReadData[] tagReads,filteredTagReads;
      TagFilter filter;
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
      System.out.println();

      try
      {
        System.out.println("Unfiltered Read:");
        // Read the tags in the field
        tagReads = r.read(500);
        for (TagReadData tr : tagReads)
          System.out.println(tr.toString());
        System.out.println();

        if (0 == tagReads.length)
        {
          System.out.println("No tags found.");
        }
        else
        {
          // A TagData object may be used as a filter, for example to
          // perform a tag data operation on a particular tag.
          System.out.println("Filtered Tagop:");
          // Read kill password of tag found in previous operation
          filter = tagReads[0].getTag();
          System.out.printf("Read kill password of tag %s\n", filter);
          Gen2.ReadData tagop = new Gen2.ReadData(Gen2.Bank.RESERVED, 0, (byte)2);
          try
          {
            short[] data = (short[])r.executeTagOp(tagop, filter);
            for (short word : data)
            {
              System.out.printf("%04X", word);
            }
            System.out.println();
          }
          catch (ReaderException re)
          {
            System.out.printf("Can't read tag: %s\n", re);
          }
          System.out.println();


          // Filter objects that apply to multiple tags are most useful in
          // narrowing the set of tags that will be read. This is
          // performed by setting a read plan that contains a filter.

          // A TagData with a short EPC will filter for tags whose EPC
          // starts with the same sequence.
          filter = new TagData(tagReads[0].getTag().epcString().substring(0, 4));
          System.out.printf("EPCs that begin with %s\n", filter);
          r.paramSet("/reader/read/plan",
                  new SimpleReadPlan(null, TagProtocol.GEN2, filter, 1000));
          filteredTagReads = r.read(500);
          for (TagReadData tr : filteredTagReads)
            System.out.println(tr.toString());
          System.out.println();

          // A filter can also be a full Gen2 Select operation.  For
          // example, this filter matches all Gen2 tags where bits 8-19 of
          // the TID are 0x003 (that is, tags manufactured by Alien
          // Technology).
          System.out.println("Tags with Alien Technology TID");
          filter = new Gen2.Select(false, Gen2.Bank.TID, 8, 12, new byte[] {0, 0x30});
          r.paramSet("/reader/read/plan",
                  new SimpleReadPlan(null, TagProtocol.GEN2, filter, 1000));
          System.out.printf("Reading tags with a TID manufacturer of 0x003\n",
                            filter.toString());
          filteredTagReads = r.read(500);
          for (TagReadData tr : filteredTagReads)
            System.out.println(tr.toString());
          System.out.println();

          // Gen2 Select may also be inverted, to give all non-matching tags
          System.out.println("Tags without Alien Technology TID");
          filter = new Gen2.Select(true, Gen2.Bank.TID, 8, 12, new byte[] {0, 0x30});
          r.paramSet("/reader/read/plan",
                  new SimpleReadPlan(null, TagProtocol.GEN2, filter, 1000));
          System.out.printf("Reading tags with a TID manufacturer of 0x003\n",
                            filter.toString());
          filteredTagReads = r.read(500);
          for (TagReadData tr : filteredTagReads)
            System.out.println(tr.toString());
          System.out.println();

          // Filters can also be used to match tags that have already been
          // read. This form can only match on the EPC, as that's the only
          // data from the tag's memory that is contained in a TagData
          // object.
          // Note that this filter has invert=true. This filter will match
          // tags whose bits do not match the selection mask.
          // Also note the offset - the EPC code starts at bit 32 of the
          // EPC memory bank, after the StoredCRC and StoredPC.
          filter =
            new Gen2.Select(true, Gen2.Bank.EPC, 32, 2,
                            new byte[] {(byte)0xC0});
          System.out.println("EPCs with first 2 bits equal to zero (post-filtered):");
          for (TagReadData tr : tagReads) // unfiltered tag reads from the first example
            if (filter.matches(tr.getTag()))
              System.out.println(tr.toString());
          System.out.println();
        }
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

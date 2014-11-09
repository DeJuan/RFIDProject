/**
 * Sample program that reads tags for a fixed period of time (500ms)
 * and prints the tags found.
 */

// Import the API
package samples;
import com.thingmagic.*;

public class read
{  
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
      r.addTransportListener(r.simpleTransportListener);
    }    
  }
   

   static class TagReadListener implements ReadListener
  {
        public void tagRead(Reader r, TagReadData t) {
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
      r.addReadListener(new TagReadListener() );
      // Read tags
      tagReads = r.read(500);
      // Print tag reads
      for (TagReadData tr : tagReads)
        System.out.println(tr.toString());

      // Shut down reader
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

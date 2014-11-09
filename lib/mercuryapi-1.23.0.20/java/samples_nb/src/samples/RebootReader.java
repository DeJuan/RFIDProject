/*
 * Sample program that reboots the reader
 * 
 */
package samples;

import com.thingmagic.*;


public class RebootReader 
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
      r.reboot();      
      r.destroy();
      boolean notConnected = true;
      int retryCount =1;
      do
      {
          try
          {
              System.out.println("Trying to reconnect.... Attempt:"+retryCount);
              r = Reader.create(argv[nextarg]);
              r.connect();
              notConnected = false;
          }
          catch(Exception ex)
          {
          }
          retryCount++; 
      }while(notConnected);
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

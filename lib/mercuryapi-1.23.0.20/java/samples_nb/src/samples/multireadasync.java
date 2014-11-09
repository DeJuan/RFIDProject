/**
 * Sample program that reads tags in the background and prints the
 * tags found.
 */

// Import the API
package samples;
import com.thingmagic.*;

public class multireadasync
{
  public static void main(String argv[])
  {
    // Program setup
    /*if (argv.length != 1)
    {
      System.out.print("Please provide reader URL, such as:\n"
                       + "tmr:///com4\n"
                       + "tmr://my-reader.example.com\n");
      System.exit(1);
    }*/
    
    // Create Reader object, connecting to physical device
    try
    {
        Reader[] r = new Reader[argv.length];
        TagReadData[] tags;
        for (int i = 0; i < argv.length; i++) 
        {

            r[i] = Reader.create(argv[i]);
            r[i].connect();
            if (Reader.Region.UNSPEC == (Reader.Region) r[i].paramGet("/reader/region/id"))
            {
                Reader.Region[] supportedRegions = (Reader.Region[]) r[i].paramGet(TMConstants.TMR_PARAM_REGION_SUPPORTEDREGIONS);
                if (supportedRegions.length < 1)
                {
                    throw new Exception("Reader doesn't support any regions");
                }
                else
                {
                    r[i].paramSet("/reader/region/id", supportedRegions[0]);
                }
            }

            // Create and add tag listener
            ReadListener rl = new PrintListener();
            r[i].addReadListener(rl);

            ReadExceptionListener exceptionListener = new TagReadExceptionReceiver();
            r[i].addReadExceptionListener(exceptionListener);

            // search for tags in the background
            r[i].startReading();
        }
        Thread.sleep(1000);

        for (int i = 0; i < argv.length; i++) 
        {
            r[i].stopReading();
            // Shut down reader
            r[i].destroy();
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

  static class PrintListener implements ReadListener
  {
    public void tagRead(Reader r, TagReadData tr)
    {
      System.out.println("Background read: " + tr.toString());
    }

  }
   static class TagReadExceptionReceiver implements ReadExceptionListener
  {
        public void tagReadException(com.thingmagic.Reader r, ReaderException re)
        {
            System.out.println("Reader Exception: " + re.getMessage());
            if(re.getMessage().equals("Connection Lost"))
            {
                System.exit(1);
            }
        }
    }

}

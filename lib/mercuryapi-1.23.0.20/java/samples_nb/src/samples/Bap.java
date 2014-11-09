package samples;
import com.thingmagic.*;

public class Bap
{
  static void usage()
  {
    System.out.printf("Please provide reader URL, such as:\n"
        +"tmr:///com4\n tmr://my-reader.example.com\n");
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
      Gen2.Bap bap;
      System.out.println("case 1: read by using the default bap parameter values \n");
      System.out.println("Get Bap default parameters");
      bap = (Gen2.Bap) r.paramGet(TMConstants.TMR_PARAM_GEN2_BAP);
      System.out.println("powerupdelay : "+bap.powerUpDelayUs +" freqHopOfftimeUs : "+ bap.freqHopOfftimeUs);
      //  Read tags
      tagReads = r.read(500);
      for (TagReadData tagReadData : tagReads)
      {
            System.out.println("EPC :"+tagReadData.epcString());
      }

      System.out.println("case 2: read by setting the bap parameters  \n");
      bap= new Gen2.Bap(10000, 20000);
      //set the parameters to the module
      r.paramSet(TMConstants.TMR_PARAM_GEN2_BAP, bap);
      System.out.println("Get Bap  parameters");
      bap = (Gen2.Bap) r.paramGet(TMConstants.TMR_PARAM_GEN2_BAP);
      System.out.println("powerupdelay : "+bap.powerUpDelayUs +" freqHopOfftimeUs : "+ bap.freqHopOfftimeUs);
      //  Read tags
      tagReads = r.read(500);
      for (TagReadData tagReadData : tagReads)
      {
            System.out.println("EPC :"+tagReadData.epcString());
      }

      System.out.println("case 3: read by disbling the bap option  \n");
      //initialize the with -1
      //set the parameters to the module
      r.paramSet(TMConstants.TMR_PARAM_GEN2_BAP, null);
      System.out.println("Get Bap  parameters");
      bap = (Gen2.Bap) r.paramGet(TMConstants.TMR_PARAM_GEN2_BAP);
      System.out.println("powerupdelay : "+bap.powerUpDelayUs +" freqHopOfftimeUs : "+ bap.freqHopOfftimeUs);
      //  Read tags
      tagReads = r.read(500);
      for (TagReadData tagReadData : tagReads)
      {
            System.out.println("EPC :"+tagReadData.epcString());
      }

      // Shut down reader
      r.destroy();
    }
    catch (ReaderException re)
    {
      re.printStackTrace();
      System.out.println("Reader Exception : " + re.getMessage());
    }
    catch (Exception re)
    {
        re.printStackTrace();
        System.out.println("Exception : " + re.getMessage());
    }
  }
}

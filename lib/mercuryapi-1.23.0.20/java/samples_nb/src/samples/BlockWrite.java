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
public class BlockWrite
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
    TagFilter target = null;
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

      //read data from user bank
      Gen2.ReadData readOp = new Gen2.ReadData(Gen2.Bank.USER, 0, (byte)0x02);
      short[] readData = (short[])r.executeTagOp(readOp, target);
      System.out.print("Read Data before blockwrite operation : ");
      for(short dt:readData)
      {
        System.out.printf("%02x",dt);
      }
      
      // reading for tags
      TagReadData[] tagReads = r.read(500);

      short writeData[]= {(short)0x4455,(short)0x6677};
      Gen2.BlockWrite tagop=new Gen2.BlockWrite(Gen2.Bank.USER,0,(byte)2,writeData);

      //TagData filter = new TagData(tagReads[0].getTag().epcBytes());
      Gen2.Select select=new Gen2.Select(false,Gen2.Bank.EPC,32,96,tagReads[0].getTag().epcBytes());

      //block write operation on the first tag found
      r.executeTagOp(tagop,select);
      //r.executeTagOp(tagop,filter);


      //read data from user bank to verify the block write above
      readOp = new Gen2.ReadData(Gen2.Bank.USER, 0, (byte)0x02);
      readData = (short[])r.executeTagOp(readOp, target);
      System.out.print("\nRead Data after block write operation : ");
      for(short dt:readData)
      {
        System.out.printf("%02x",dt);
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

/**
 * Sample program that reads tags in the background and prints the
 * tags found.
 */

// Import the API
package com.thingmagic;

import java.util.concurrent.ConcurrentHashMap;
import com.thingmagic.rfidreader.TagRecord;

public class Readasync
{
	
    public static int totalTagCount =0;  
    public static  ConcurrentHashMap<String,TagRecord> epcToReadDataMap;
    static ReadListener readListener = new PrintListener();
    
    public static void readTag(Reader r) throws Exception 
    {
    epcToReadDataMap=new ConcurrentHashMap<String, TagRecord>();
    // Create Reader object, connecting to physical device
    try
    {
      // add tag listener
      r.addReadListener(readListener);
      // search for tags in the background
      r.startReading();   
    } 
    catch (Exception re)
    {
       throw re;
    }
  }

  public static void stopReading(Reader reader) throws Exception
  {
	reader.stopReading();
	reader.removeReadListener(readListener);
	
  }
  
  static class PrintListener implements ReadListener
  {
	  
    public void tagRead(Reader r, TagReadData tr)
    {
    	String epcString = tr.getTag().epcString();
    	totalTagCount +=tr.getReadCount();
    	if(epcToReadDataMap.keySet().contains(epcString)){
			TagRecord tempTR=epcToReadDataMap.get(epcString);
    		tempTR.setReadCount(tempTR.getReadCount() + tr.getReadCount());
    		epcToReadDataMap.put(epcString, tempTR);
    	}else{
    		TagRecord tagRecord=new TagRecord();
    		tagRecord.setEpcString(epcString);
    		tagRecord.setReadCount(tr.getReadCount());
    		epcToReadDataMap.put(epcString, tagRecord);
    	}
    }

  }

  
}

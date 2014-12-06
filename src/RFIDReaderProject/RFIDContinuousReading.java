package RFIDReaderProject;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.thingmagic.ReadListener;
import com.thingmagic.ReadPlan;
import com.thingmagic.Reader;
import com.thingmagic.ReaderException;
import com.thingmagic.SimpleReadPlan;
import com.thingmagic.TagProtocol;
import com.thingmagic.TagReadData;

public class RFIDContinuousReading{
	Reader reader; 
	public RFIDContinuousReading(String protocol) throws ReaderException{
		if (protocol == "DeJuan"){
			this.reader = Reader.create("tmr:///com4");
		}
		else if (protocol == "Cynthia"){
			this.reader = Reader.create("tmr:///com3");
		}
		else if (protocol == "Linux"){
			//May be broken because of permissions
			this.reader = Reader.create("eapi:///dev/ttyUSB0");
		}
		else{
			this.reader = Reader.create(protocol);
		}
		this.reader.connect();
		this.reader.paramSet("/reader/region/id", Reader.Region.NA);
		int hop[]={915000};
		this.reader.paramSet("/reader/region/hopTable", hop);
		int[] antennasToUse = {1,3,4};
		ReadPlan antennaSettings = new SimpleReadPlan(antennasToUse, TagProtocol.GEN2);
		this.reader.paramSet("/reader/read/plan", antennaSettings);
	}
	
	public TagReadData[] readTags() throws ReaderException{
		TagReadData[] readTags = reader.read(150);
		return readTags;
	}
	
	public static void main(String args[]) throws InterruptedException{
		try {
			RFIDReadingTest reader = new RFIDReadingTest("Cynthia");
			int totalTimeToRead = 2000;
			int aSyncOnTime = 250;
			int aSyncOffTime = 150;
			reader.reader.paramSet("reader/read/asyncOnTime", aSyncOnTime);
			reader.reader.paramSet("reader/read/asyncOffTime", aSyncOffTime);
			SeperateRecordsListener srl = new SeperateRecordsListener();
			reader.reader.addReadListener(srl);
			reader.reader.startReading();
			Thread.sleep(totalTimeToRead);
			reader.reader.stopReading();
			Map<String, List<TagReadData>> allReadData = srl.returnDataCollected();
			reader.reader.removeReadListener(srl);
			TagReadData[] readings = reader.readTags();
			Map<String, Double> radianStorage = new HashMap<String, Double>();
			Map<String, Double> timeStorage = new HashMap<String, Double>();
			if (readings.length == 0){
				System.err.println("No tags were detected.");
			}
			else{
				//Current implementation works for exactly two readings, not more; you need to do averaging/other work in that case.
				List<String> doubleDetectedTags = new ArrayList<String>();
				for (int i = 0; i < readings.length; i++){
					String currentTag = readings[i].epcString();
					double timeStamp = readings[i].getTime();
					double radians = readings[i].getPhase()*Math.PI;
					radians = radians/180.0;
					if(!radianStorage.containsKey(currentTag)){
						radianStorage.put(currentTag, radians);
					}
					else{
						//double overlapDistance = Math.PI - Math.max(radianStorage.get(currentTag), radians);
						//overlapDistance += Math.min(radianStorage.get(currentTag), radians);
						System.err.printf("Currently, radianStorage has the value %f for the tag %s." + System.getProperty("line.separator"), radianStorage.get(currentTag), currentTag);
						//radianStorage.put(currentTag, Math.min(Math.abs(radianStorage.get(currentTag)-radians), overlapDistance));
						radianStorage.put(currentTag, radianStorage.get(currentTag)-radians);
						System.err.printf("After comparison, radianStorage has the value %f for the tag %s." + System.getProperty("line.separator"), radianStorage.get(currentTag), currentTag);
						doubleDetectedTags.add(currentTag);
					}
					if(!timeStorage.containsKey(currentTag)){
						timeStorage.put(currentTag, timeStamp);
					}
					else{
						timeStorage.put(currentTag, timeStorage.get(currentTag) - timeStamp);
					}
				}
				System.err.println("Note: The higher the RSSI, the stronger the received signal is. Watch for negatives!");
				System.err.println("");
				for (int i = 0; i < readings.length; i++){
					double radians = readings[i].getPhase()*Math.PI;
					radians = radians/180.0;
					System.err.printf("Tag %s was read: "
							+ "This tag is number %d in the order of detected tags."
							+ System.getProperty("line.separator") 
							+ "The above tag has frequency %d and phase %d, with Received Signal Strength Indication(RSSI) of %d."
							+ System.getProperty("line.separator")
							+ "In radians, the phase is %f."
							+ System.getProperty("line.separator")
							+ System.getProperty("line.separator"), readings[i].toString(), i, readings[i].getFrequency(), 
							readings[i].getPhase(), readings[i].getRssi(), radians);
					
				}
				for(String tag : radianStorage.keySet()){
					if (!doubleDetectedTags.contains(tag)){
						continue;
					}
					System.err.printf("For the tag %s, the phase difference detected between the two antenna readings is %f", tag, radianStorage.get(tag));
					System.err.println("");
					System.err.printf("The time difference between the two antenna readings is %f", timeStorage.get(tag));
					System.err.println("");
				}
			}

		} catch (ReaderException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
	}



static class SeperateRecordsListener implements ReadListener
{
	
  Map<String, List<TagReadData>> readTags = new HashMap<String, List<TagReadData>>();
  int durationToRead;
  int durationToPause;
  
  SeperateRecordsListener(int readDuration, int pauseDuration)
  {
    durationToRead = readDuration;
    durationToPause = pauseDuration;
  }
  
  SeperateRecordsListener(){
	  durationToRead = 250;
	  durationToPause = 150;
  }

  public void tagRead(Reader r, TagReadData tr)
  {	
	// Test if we've read the tag or not already; if we haven't, record our only reading of it, else ignore it. 
	  String name = tr.epcString();
    if(!readTags.containsKey(name)){
    	List<TagReadData> tagListing = new ArrayList<TagReadData>();
    	tagListing.add(tr);
    	readTags.put(name, tagListing);
    }
    else{
    	readTags.get(name).add(tr);
    }
  }
  
  public Map<String, List<TagReadData>> returnDataCollected(){
	  return readTags;
  }

}
}

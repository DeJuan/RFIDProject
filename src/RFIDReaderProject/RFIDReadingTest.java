package RFIDReaderProject;


import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.thingmagic.ReadPlan;
import com.thingmagic.Reader;
import com.thingmagic.ReaderException;
import com.thingmagic.SimpleReadPlan;
import com.thingmagic.TagProtocol;
import com.thingmagic.TagReadData;

public class RFIDReadingTest {
	Reader reader; 
	public RFIDReadingTest(String protocol) throws ReaderException{
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
		int[] antennasToUse = {1,4};
		ReadPlan antennaSettings = new SimpleReadPlan(antennasToUse, TagProtocol.GEN2);
		this.reader.paramSet("/reader/read/plan", antennaSettings);
	}
	
	public TagReadData[] readTags() throws ReaderException{
		TagReadData[] readTags = reader.read(2000);
		return readTags;
	}
	
	public static void main(String args[]){
		try {
			RFIDReadingTest reader = new RFIDReadingTest("DeJuan");
			TagReadData[] readings = reader.readTags();
			Map<String, Double> radianStorage = new HashMap<String, Double>();
			if (readings.length == 0){
				System.err.println("No tags were detected.");
			}
			else{
				//Current implementation works for exactly two readings, not more; you need to do averaging/other work in that case.
				List<String> doubleDetectedTags = new ArrayList<String>();
				for (int i = 0; i < readings.length; i++){
					String currentTag = readings[i].epcString();
					double radians = readings[i].getPhase()*Math.PI;
					radians = radians/180.0;
					if(!radianStorage.containsKey(currentTag)){
						radianStorage.put(currentTag, radians);
					}
					else{
						double overlapDistance = Math.PI - Math.max(radianStorage.get(currentTag), radians);
						overlapDistance += Math.min(radianStorage.get(currentTag), radians);
						System.err.printf("Currently, radianStorage has the value %f for the tag %s." + System.getProperty("line.separator"), radianStorage.get(currentTag), currentTag);
						radianStorage.put(currentTag, Math.min(Math.abs(radianStorage.get(currentTag)-radians), overlapDistance));
						System.err.printf("After comparison, radianStorage has the value %f for the tag %s." + System.getProperty("line.separator"), radianStorage.get(currentTag), currentTag);
						doubleDetectedTags.add(currentTag);
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
				}
			}

		} catch (ReaderException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
	}
}

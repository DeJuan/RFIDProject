package RFIDReaderProject;

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
		else if (protocol == "Cathleen"){
			this.reader = Reader.create("eapi:///dev/ttyUSB0");
		}
		else{
			this.reader = Reader.create(protocol);
		}
		
		this.reader.connect();
		this.reader.paramSet("/reader/region/id", Reader.Region.NA);
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
			RFIDReadingTest reader = new RFIDReadingTest("Cynthia");
			TagReadData[] readings = reader.readTags();
			for (int i = 0; i < readings.length; i++){
				System.err.printf("Tag %s was read: "
						+ "This tag is number %d in the order of detected tags."
						+ System.getProperty("line.separator") 
						+ "The above tag has frequency %d and phase %d, with Received Signal Strength Indication(RSSI) of %d. The higher the RSSI, the stronger the received signal."
						+ System.getProperty("Line.separator"), readings[i].toString(), i, readings[i].getFrequency(), readings[i].getPhase(), readings[i].getRssi());
			}
			if (readings.length == 0){
				System.err.println("No tags were detected.");
			}
		} catch (ReaderException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
	}
}

package RFIDReaderProject;

import com.thingmagic.Reader;
import com.thingmagic.ReaderException;
import com.thingmagic.TagReadData;

public class RFIDReadingTest {
	Reader reader; 
	public RFIDReadingTest(String protocol) throws ReaderException{
		if (protocol == "auto"){
			this.reader = Reader.create("tmr:///com3");
			this.reader.connect();
			this.reader.paramSet("/reader/region/id", Reader.Region.NA);
		}
		else{
			this.reader = Reader.create(protocol);
			this.reader.connect();
		}
	}
	
	public TagReadData[] readTags() throws ReaderException{
		TagReadData[] readTags = reader.read(5000);
		return readTags;
	}
	
	public static void main(String args[]){
		try {
			RFIDReadingTest reader = new RFIDReadingTest("auto");
			TagReadData[] readings = reader.readTags();
			for (int i = 0; i < readings.length; i++){
				System.err.printf("Tag %s was read: "
						+ "This tag is number %d in the order of detected tags"
						+ System.getProperty("line.separator"), readings[i].toString(), i);
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

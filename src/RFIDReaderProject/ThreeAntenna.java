package RFIDReaderProject;



import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.OutputStreamWriter;
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

public class ThreeAntenna {
	Reader reader; 
	public ThreeAntenna(String protocol) throws ReaderException{
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
		int[] antennasToUse = {1, 3, 4};
		ReadPlan antennaSettings = new SimpleReadPlan(antennasToUse, TagProtocol.GEN2);
		this.reader.paramSet("/reader/read/plan", antennaSettings);
	}
	
	public TagReadData[] readTags() throws ReaderException{
		TagReadData[] readTags = reader.read(150);
		return readTags;
	}
	
	public static void main(String args[]) throws IOException{
		try {
			ThreeAntenna reader = new ThreeAntenna("Cynthia");
			BufferedWriter bw = new BufferedWriter(new FileWriter("ps5.2.1.txt", true));

			for (int j = 0; j < 20 ; j++){
				TagReadData[] readings = reader.readTags();
				Map<Integer, Double> radianStorage = new HashMap<Integer, Double>();
				if (readings.length == 0){
					System.err.println("No tags were detected.");
				}
				else{
					System.err.println("");
					for (int i = 0; i < readings.length; i++){
						double degrees = readings[i].getPhase();
						double radians = degrees*Math.PI/180.0;
						radianStorage.put(readings[i].getAntenna(), radians);
						bw.write(readings[i].getTag() + "    "
								+ Integer.toString(readings[i].getAntenna()) + "     "
								+ Double.toString(radians) + "     "
								+ Double.toString(degrees));
						bw.newLine();
//						System.err.printf("Tag %s was read: "
//								+ "This tag is number %d in the order of detected tags."
//								+ System.getProperty("line.separator") 
//								+ "The above tag has frequency %d and phase %d, with Received Signal Strength Indication(RSSI) of %d."
//								+ System.getProperty("line.separator")
//								+ "In radians, the phase is %f."
//								+ System.getProperty("line.separator")
//								+ System.getProperty("line.separator"), readings[i].toString(), i, readings[i].getFrequency(), 
//								readings[i].getPhase(), readings[i].getRssi(), radians);
						
					}
					
					bw.newLine();
					
					//calculate difference between antenna 1/3
					if(radianStorage.containsKey(1) && radianStorage.containsKey(3)){
						double oneThreeDiff = radianStorage.get(3) - radianStorage.get(1);
						System.err.printf("1/3 diff in radians = %f", oneThreeDiff);
					}else{
						System.err.printf("couldn't get 1/3 difference because we didn't get enough tag info");
					}
					System.err.println("");
					//calculate difference between antenna 4/3
					if(radianStorage.containsKey(4) && radianStorage.containsKey(3)){
						double fourThreeDiff = radianStorage.get(4) - radianStorage.get(3);
						System.err.printf("4/3 diff in radians = %f", fourThreeDiff);

					}else{
						System.err.printf("couldn't get 4/3 difference because we didn't get enough tag info");
					}
					System.err.println("");
					//calculate difference between antenna 1/4
					if(radianStorage.containsKey(1) && radianStorage.containsKey(4)){
						double onefourDiff = radianStorage.get(4) - radianStorage.get(1);
						System.err.printf("1/4 diff in radians = %f", onefourDiff);

					}else{
						System.err.printf("couldn't get 1/4 difference because we didn't get enough tag info");
					}
				}
			}


			bw.write("END OF TEST RUN FOR 3 sec");
			bw.newLine();
			bw.close();

		} catch (ReaderException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
}


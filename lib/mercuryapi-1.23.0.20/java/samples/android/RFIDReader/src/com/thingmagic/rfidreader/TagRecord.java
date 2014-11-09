package com.thingmagic.rfidreader;

public class TagRecord {

	int antenna = 0;
	public int readCount = 0;
	int rssi = 0;
	int frequency = 0;
	String epcString;
	byte[] data = new byte[0];
	public byte[] getData() {
		return data;
	}
	public void setData(byte[] data) {
		this.data = data;
	}
	public int getAntenna() {
		return antenna;
	}
	public void setAntenna(int antenna) {
		this.antenna = antenna;
	}
	public int getReadCount() {
		return readCount;
	}
	public void setReadCount(int readCount) {
		this.readCount = readCount;
	}
	public int getRssi() {
		return rssi;
	}
	public void setRssi(int rssi) {
		this.rssi = rssi;
	}
	public int getFrequency() {
		return frequency;
	}
	public void setFrequency(int frequency) {
		this.frequency = frequency;
	}
	public String getEpcString() {
		return epcString;
	}
	public void setEpcString(String epcString) {
		this.epcString = epcString;
	}
}

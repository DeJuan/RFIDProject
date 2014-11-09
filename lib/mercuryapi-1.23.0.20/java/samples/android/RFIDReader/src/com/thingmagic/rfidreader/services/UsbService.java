package com.thingmagic.rfidreader.services;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Set;


import com.ftdi.j2xx.D2xxManager;
import com.ftdi.j2xx.FT_Device;
import com.thingmagic.rfidreader.ReaderActivity;
import com.thingmagic.util.LoggerUtil;

import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.app.Activity;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;

import com.thingmagic.AndroidUsbReflection;

public class UsbService {
	
	private static final String TAG = "UsbService";
	private static final String ACTION_USB_PERMISSION = "com.thingmagic.rfidreader.services.USB_PERMISSION"; 
	PendingIntent mPermissionIntent; 
	private D2xxManager ftD2xx  = null;
    private FT_Device   ftDev   = null;
    private Context mContext;
    private static final int    USB_OPEN_INDEX      = 0;
	public ArrayList<String> getConnectedUSBdevices(ReaderActivity activity){
		ArrayList<String>  connectedDeviceNames=new ArrayList<String>();
		
		try{
			
			// Get UsbManager from Android.
			UsbManager manager = (UsbManager) activity.getSystemService(Context.USB_SERVICE);
			HashMap<String, UsbDevice> deviceList = manager.getDeviceList();
			Set<String> keySet = deviceList.keySet();			
			for(String key : keySet){
				connectedDeviceNames.add(deviceList.get(key).getDeviceName());
			}

		}catch(Exception e){

		}
		return connectedDeviceNames;

	}

	public void setUsbManager(String device, ReaderActivity activity){
         try{
        	 UsbManager manager = (UsbManager) activity.getSystemService(Context.USB_SERVICE);
        	 HashMap<String, UsbDevice> deviceList = manager.getDeviceList();
        	 Set<String> keySet = deviceList.keySet();			
        	 for(String key : keySet){
        		 if(deviceList.get(key).getDeviceName().equalsIgnoreCase(device))
        		 {
        			if( !manager.hasPermission(deviceList.get(key))){
	        			 mPermissionIntent = PendingIntent.getActivity(activity.getApplicationContext(), 0, new Intent( 
	        						ACTION_USB_PERMISSION), 0); 
	        			 manager.requestPermission(deviceList.get(key), mPermissionIntent);
        			}
        			int deviceClass = deviceList.get(key).getDeviceClass();
        			if(deviceClass == 0)
        			{
        			    ftDev = useFTDIdriver(activity, deviceList.get(key));
        			}
        		  new AndroidUsbReflection(manager,ftDev, deviceList.get(key), deviceClass);
        		 }
        	 }
        	 
         }catch(Exception ex){
        	 
         }	
	}
	
	public FT_Device useFTDIdriver(ReaderActivity activity, UsbDevice device) throws Exception 
	{
		mContext = activity.getApplicationContext();		
		if(ftD2xx == null) {
            try {            	         	
                ftD2xx = D2xxManager.getInstance(mContext);
            } catch (D2xxManager.D2xxException ex) {
            	LoggerUtil.debug(TAG, "useDriver: D2xxManager.  "+ ex.getMessage());
            }
        }
		try{
		  if(ftDev == null) {			 
	            int devCount = 0;
	            devCount = ftD2xx.createDeviceInfoList(mContext);
	            D2xxManager.FtDeviceInfoListNode[] deviceList = new D2xxManager.FtDeviceInfoListNode[devCount];
	            ftD2xx.getDeviceInfoList(devCount, deviceList);
	            ftDev = ftD2xx.openByIndex(mContext, 0);
	            if(ftDev == null)
	            	throw new Exception();
	        }
		}
		catch(Exception ex)
		{
			throw new Exception();
			
		}
		return ftDev;
	}

}

package com.thingmagic.util;


import java.io.InputStream;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;

import org.apache.log4j.PropertyConfigurator;

import com.thingmagic.Gen2;

import android.R.color;
import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.telephony.TelephonyManager;
import android.view.View;
import android.view.View.OnFocusChangeListener;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

public class Utilities {
	
	  
	private static final String EMPTY_STRING = "";
	private static int redColor = 0xffff0000;
	public static Map<String,Gen2.Bank> gen2BankMap = new HashMap<String, Gen2.Bank>();
	
	static{
		gen2BankMap.put("EPC", Gen2.Bank.EPC);
		gen2BankMap.put("TID", Gen2.Bank.TID);
		gen2BankMap.put("Reserved", Gen2.Bank.RESERVED);
		gen2BankMap.put("User", Gen2.Bank.USER);
	}
	
	
	public static boolean validateIPAddress(TextView validationField,String ipAddress) {

		if (ipAddress.length() == 0) {
			validationField.setText("* Field cannot be empty.");
			return false;
		}else{
			String[] tokens = ipAddress.split("\\.");
			if (tokens.length != 4) {
				validationField.setText("* provide valid address.");
				return false;
			}
			try {
				for (String str : tokens) {
					int i = Integer.parseInt(str);
					if ((i < 0) || (i > 255)) {
						validationField.setText("* provide valid address.");
						return false;
					}
				}
			} catch (NumberFormatException nfe) {
				validationField.setText("* provide valid address.");
				return false;
			}
		}
		return true;
	}
	public static boolean validateReadTimeout(TextView validationField,String timout) {

		if (timout.length() == 0) {
			validationField.setText("* Timeout value cannot be empty.");
			validationField.setTextColor(redColor);
			return false;
		}else{
			int timeout= Integer.parseInt(timout);
			if(timeout < 30){
				validationField.setText("* Timeout value cannot be lessthen 30 ms.");
				validationField.setTextColor(redColor);
				return false;
			}
		}
		validationField.setText("");
		validationField.setTextColor(color.black);
		return true;
	}
	public static  void longToast(Context context,CharSequence message) 
	{
		Toast.makeText(context, message, Toast.LENGTH_LONG).show();
	}
	
	public static  void shortToast(Context context,CharSequence message) 
	{
		Toast.makeText(context, message, Toast.LENGTH_SHORT).show();
	}
	
	public static void loadLog4j(Context context) throws Exception 
	{
		InputStream inputStream = null;
		try 
		{
			LoggerUtil.debug("Utilities", "loading log4j");
			inputStream = context.getAssets().open(
					"log4j.properties");
			Properties prop = new Properties();
			prop.load(inputStream);
			PropertyConfigurator.configure(prop);
		}
		catch (Exception ex) {
			throw ex;
		}
		finally 
		{
			if (inputStream != null) 
			{
				inputStream.close();
			}
		}
	}
	
	public static  class DftTextOnFocusListener implements OnFocusChangeListener 
	{

		private String defaultText;

		public DftTextOnFocusListener(String defaultText)
		{
			this.defaultText = defaultText;
		}

		public void onFocusChange(View v, boolean hasFocus) 
		{
			if (v instanceof EditText) 
			{
				EditText focusedEditText = (EditText) v;
				// handle obtaining focus
				if (hasFocus) 
				{
					if (focusedEditText.getText().toString()
							.equals(defaultText)) 
					{
						focusedEditText.setText(EMPTY_STRING);
					}
				}
				// handle losing focus
				else 
				{
					if (focusedEditText.getText().toString()
							.equals(EMPTY_STRING)) 
					{
						focusedEditText.setText(defaultText);
					}
				}
			}
		}
	}
	
	//For Android 2.3 and above:
	private void setMobileDataEnabled(Context context, boolean enabled) throws Exception {
	    final ConnectivityManager conman = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
	    final Class conmanClass = Class.forName(conman.getClass().getName());
	    final Field iConnectivityManagerField = conmanClass.getDeclaredField("mService");
	    iConnectivityManagerField.setAccessible(true);
	    final Object iConnectivityManager = iConnectivityManagerField.get(conman);
	    final Class iConnectivityManagerClass = Class.forName(iConnectivityManager.getClass().getName());
	    final Method setMobileDataEnabledMethod = iConnectivityManagerClass.getDeclaredMethod("setMobileDataEnabled", Boolean.TYPE);
	    setMobileDataEnabledMethod.setAccessible(true);

	    setMobileDataEnabledMethod.invoke(iConnectivityManager, enabled);
	}
	
	//For Android 2.2 and below:
	private void setMobileDataEnabled2(Context context, boolean enabled) throws Exception {

		Method dataConnSwitchmethod;
		Class telephonyManagerClass;
		Object ITelephonyStub;
		Class ITelephonyClass;
		boolean enable3G = true;

		TelephonyManager telephonyManager = (TelephonyManager) context
		        .getSystemService(Context.TELEPHONY_SERVICE);

		telephonyManagerClass = Class.forName(telephonyManager.getClass().getName());
		Method getITelephonyMethod = telephonyManagerClass.getDeclaredMethod("getITelephony");
		getITelephonyMethod.setAccessible(true);
		ITelephonyStub = getITelephonyMethod.invoke(telephonyManager);
		ITelephonyClass = Class.forName(ITelephonyStub.getClass().getName());

		Method dataConnSwitchmethod_OFF = 
                ITelephonyClass.getDeclaredMethod("disableDataConnectivity");
		Method dataConnSwitchmethod_ON = ITelephonyClass.getDeclaredMethod("enableDataConnectivity"); 

		if(enable3G){
			dataConnSwitchmethod_ON.setAccessible(true);
			dataConnSwitchmethod_ON.invoke(ITelephonyStub);
		}else{
			dataConnSwitchmethod_OFF.setAccessible(true);
			dataConnSwitchmethod_OFF.setAccessible(true);
		}
	}

	public  boolean checkIfWiFiEnabled(Activity activity)  {
		
		boolean wifiEnabled = true;
		final WifiManager wifiManager = (WifiManager)activity
					.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
		wifiEnabled = wifiManager.isWifiEnabled();
		ConnectivityManager manager = (ConnectivityManager) activity
				.getApplicationContext().getSystemService(
						activity.CONNECTIVITY_SERVICE);
		boolean is3g = manager.getNetworkInfo(ConnectivityManager.TYPE_MOBILE)
				.isConnectedOrConnecting();
		boolean isWifi = manager.getNetworkInfo(ConnectivityManager.TYPE_WIFI)
				.isConnectedOrConnecting();

		if (!is3g && !isWifi) {
			System.out.println("is isWifi ? "+isWifi );
			AlertDialog  wifiAlert =new AlertDialog.Builder(activity)
	        .setIcon(android.R.drawable.ic_dialog_alert)
	        .setTitle("WiFi permission request")
	        .setMessage("An app wants to turn on Wifi. Allow ?")
	        .setPositiveButton("Yes", new DialogInterface.OnClickListener()
	        {
		        @Override
		        public void onClick(DialogInterface dialog, int which) {
		        	wifiManager.setWifiEnabled(true);
		        	notify();
		        }
	        })
	        .setNegativeButton("No",null)
	        .setCancelable(false)
	        .show();
			

			System.out.println("is showing ? "+wifiAlert.isShowing() );
//			while (wifiAlert.isShowing()){
//				
//				try {
//					Thread.sleep(1);
//				} catch (InterruptedException e) {
//					// TODO Auto-generated catch block
//					e.printStackTrace();
//				}
//			}
			wifiEnabled = wifiManager.isWifiEnabled();
			
		}
		return wifiEnabled;
	}
	
	public static  boolean  checkIfBluetoothEnabled(Activity activity) {
		boolean bluetoothEnabled = true;
		int REQUEST_ENABLE_BT = 3;
		if (!BluetoothAdapter.getDefaultAdapter().isEnabled()) {
			// Prompt user to turn on Bluetooth
			new AlertDialog.Builder(activity)
	        .setIcon(android.R.drawable.ic_dialog_alert)
	        .setTitle("Bluetooth permission request")
	        .setMessage("An app wants to turn on Bluetooth. Allow ?")
	        .setPositiveButton("Yes", new DialogInterface.OnClickListener()
	        {
		        @Override
		        public void onClick(DialogInterface dialog, int which) {
		        	BluetoothAdapter.getDefaultAdapter().enable();
		        }
	        })
	        .setNegativeButton("No",null)
	        .setCancelable(false)
	        .show();
			Intent enableBtIntent = new Intent(
					BluetoothAdapter.ACTION_REQUEST_ENABLE);
			boolean result =activity.startActivityIfNeeded(enableBtIntent, REQUEST_ENABLE_BT);
			System.out.println("result :"+ result);
		}
		bluetoothEnabled = BluetoothAdapter.getDefaultAdapter().isEnabled();
		return bluetoothEnabled;
	}
	
}

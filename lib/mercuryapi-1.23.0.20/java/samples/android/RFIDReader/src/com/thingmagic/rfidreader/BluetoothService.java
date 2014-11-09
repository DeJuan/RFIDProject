package com.thingmagic.rfidreader;


import java.util.Set;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.content.Intent;
import android.widget.Toast;

@SuppressLint("NewApi")
public class BluetoothService {
	private static final String TAG = "BluetoothService";
	private static final int REQUEST_ENABLE_BT = 3;

	private static BluetoothAdapter btAdapter = null;
	private static Set<BluetoothDevice> pairedDevices;

	public boolean checkBTState(Context context, ReaderActivity activity) {
		boolean enabled = true;
		btAdapter = BluetoothAdapter.getDefaultAdapter();
		// Check for Bluetooth support and then check to make sure it is turned
		// on
		// Emulator doesn't support Bluetooth and will return null
		if (btAdapter == null) {
			enabled = false;
			errorExit(context, "Fatal Error", "Bluetooth not support");
		}
//		else {
//			if (!btAdapter.isEnabled()) {	
//				enabled = false;
//				// Prompt user to turn on Bluetooth
//				Intent enableBtIntent = new Intent(
//						BluetoothAdapter.ACTION_REQUEST_ENABLE);
//				activity.startActivityForResult(enableBtIntent,
//						REQUEST_ENABLE_BT);
//			}
//			if (btAdapter.isEnabled()) {
//				enabled = true;
//			}
//		}		
		return enabled;
	}
	 


	public Set<BluetoothDevice> getPairedDevices() {
		if (btAdapter == null) {
			btAdapter = BluetoothAdapter.getDefaultAdapter();
		}
		pairedDevices = btAdapter.getBondedDevices();
		btAdapter.cancelDiscovery();
		return pairedDevices;
	}
	
	private static void errorExit(Context context, String title, String message) {
		Toast.makeText(context, title + " - " + message, Toast.LENGTH_LONG)
				.show();
	}

}

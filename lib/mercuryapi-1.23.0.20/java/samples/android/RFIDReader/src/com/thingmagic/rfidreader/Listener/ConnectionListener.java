package com.thingmagic.rfidreader.Listener;

import java.util.Scanner;

import android.app.ProgressDialog;
import android.content.Context;
import android.os.AsyncTask;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Spinner;
import android.widget.TableLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.thingmagic.Reader;
import com.thingmagic.ReaderConnect;
import com.thingmagic.rfidreader.R;
import com.thingmagic.rfidreader.ReaderActivity;
import com.thingmagic.rfidreader.services.UsbService;
import com.thingmagic.util.LoggerUtil;
import com.thingmagic.util.Utilities;

public class ConnectionListener implements View.OnClickListener {

	private static EditText ntReaderField;
	private static ReaderActivity mReaderActivity;
	private static Reader reader = null;
	private static LinearLayout servicelayout;
	private static Spinner serialList = null;
	private static RadioGroup readerRadioGroup = null;
	private static RadioButton serialReaderRadioButton = null;
	private static RadioButton networkReaderRadioButton = null;
	private static TableLayout table = null;
	private static TextView validationField;
	private static TextView searchResultCount = null;
	private static TextView totalTagCountView = null;
	private static Button connectButton;
	private static Button readButton = null;
	private static ProgressDialog pDialog = null;

	private static String TAG = "ConnectionListener";
	private static String readerName = null;
	private static String readerChecked;

	public ConnectionListener(ReaderActivity readerActivity) {
		mReaderActivity = readerActivity;
		pDialog = new ProgressDialog(readerActivity);
		pDialog.setCancelable(false);
		pDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
		findAllViewsById();
	}

	private void findAllViewsById() {
		ntReaderField = (EditText) mReaderActivity
				.findViewById(R.id.search_edit_text);
		connectButton = (Button) mReaderActivity
				.findViewById(R.id.Connect_button);
		servicelayout = (LinearLayout) mReaderActivity
				.findViewById(R.id.ServiceLayout);
		validationField = (TextView) mReaderActivity
				.findViewById(R.id.ValidationField);
		serialList = (Spinner) mReaderActivity.findViewById(R.id.SerialList);
		readerRadioGroup = (RadioGroup) mReaderActivity
				.findViewById(R.id.Reader_radio_group);
		serialReaderRadioButton = (RadioButton) mReaderActivity
				.findViewById(R.id.SerialReader_radio_button);
		networkReaderRadioButton = (RadioButton) mReaderActivity
				.findViewById(R.id.NetworkReader_radio_button);
		table = (TableLayout) mReaderActivity.findViewById(R.id.tablelayout);
		searchResultCount = (TextView) mReaderActivity
				.findViewById(R.id.search_result_view);
		totalTagCountView = (TextView) mReaderActivity
				.findViewById(R.id.totalTagCount_view);

		readButton = (Button) mReaderActivity.findViewById(R.id.Read_button);
	}

	@Override
	public void onClick(View arg0) {
		validationField.setText("");
		validationField.setVisibility(8);
		searchResultCount.setText("");
		totalTagCountView.setText("");
		table.removeAllViews();
		int id = readerRadioGroup.getCheckedRadioButtonId();
		RadioButton readerRadioButton = (RadioButton) mReaderActivity
				.findViewById(id);
		readerChecked = readerRadioButton.getText().toString();
		Utilities utilities = new Utilities();
		String query = "";
		boolean validPort = true;
		if(connectButton.getText().toString().equalsIgnoreCase("Connect"))
		{
			if (readerChecked.equalsIgnoreCase("SerialReader")) {
	//			if(!Utilities.checkIfBluetoothEnabled(mReaderActivity)){
	//				return;
	//			}
				if(serialList.getCount()<1){
				  return;
			    }
				query = serialList.getSelectedItem().toString();
				readerName = query.split(" \n ")[0];
				LoggerUtil.debug(TAG, "reader name  after split : "+readerName);
				if(query.startsWith("/dev")){
					LoggerUtil.debug(TAG, "in if condition : ");
					UsbService usbService = new UsbService();
					usbService.setUsbManager(query, mReaderActivity);
				   query = "tmr:///"+ query.split("/")[1];
				}else{
					 query = "tmr:///"+ query.split(" \n ")[1];
				}
				System.out.println("query :"+ query);
			} else if (readerChecked.equalsIgnoreCase("NetworkReader")) {
	//			if(!utilities.checkIfWiFiEnabled(mReaderActivity)){
	//				return;
	//			}
				query = ntReaderField.getText().toString().trim();
				readerName = query;
	//			query="172.16.16.121";
				System.out.println("readerName :"+ readerName);
				 if(!query.equalsIgnoreCase("")){
					 Scanner input = new Scanner(query);
					 if(!Character.isDigit( query.charAt(0)) && (ntReaderField.getTag() != null)){
						 query = ntReaderField.getTag().toString(); 
					 }
	//			 query="172.16.16.24";
				 }
				 System.out.println("query :"+ query);
	
				validPort = Utilities.validateIPAddress(validationField, query);
				if (validPort) {
					query = "tmr://" + query;
					validationField.setVisibility(8);
					// Closing keyPad manually
					InputMethodManager imm = (InputMethodManager) mReaderActivity
							.getSystemService(Context.INPUT_METHOD_SERVICE);
					imm.hideSoftInputFromWindow(ntReaderField.getWindowToken(), 0);
				} else {
					validationField.setVisibility(0);
					return;
				}
			}
		}

		ReaderConnectionThread readerConnectionThread = new ReaderConnectionThread(
				query, connectButton.getText().toString());
		readerConnectionThread.execute();
	}



	private static class ReaderConnectionThread extends
			AsyncTask<Void, Void, String> {
		private String uriString = "";
		private String operation;
		private boolean operationStatus = true;

		public ReaderConnectionThread(String requestedQuery, String operation) {
			this.uriString = requestedQuery;
			this.operation = operation;

		}

		@Override
		protected void onPreExecute() {
			LoggerUtil.debug(TAG, "** onPreExecute **");			
			if (operation.equalsIgnoreCase("Connect")) {
				disableEdit();
				pDialog.setMessage("Connecting. Please wait...");
			} else {
				readButton.setClickable(false);
				disableEdit();
				pDialog.setMessage("Disconnecting. Please wait...");
			}
			pDialog.show();
			searchResultCount.setText("");

		}

		@Override
		protected String doInBackground(Void... params) {
			String exception = "Exception :";
			try {
				if (operation.equalsIgnoreCase("Connect")) {
					reader = ReaderConnect.connect(uriString);
					LoggerUtil.debug(TAG, "Reader Connected");
				} else {
					reader.destroy();
					LoggerUtil.debug(TAG, "Reader Disconnected");
				}

			} catch (Exception ex) {
				operationStatus = false;
				if (ex.getMessage().contains("Connection is not created")
						|| ex.getMessage().startsWith("Failed to connect")) {
					exception += "Failed to connect to " + readerName;
				} else {
					exception += ex.getMessage();
				}
				LoggerUtil.error(TAG, "Exception while Connecting :", ex);
			}
			return exception;
		}

		@Override
		protected void onPostExecute(String exception) {
			pDialog.dismiss();
			LoggerUtil.debug(TAG, "** onPostExecute **");
			if (!operationStatus) {
				validationField.setText(exception);
				validationField.setVisibility(0);
				totalTagCountView.setText("");
				if (operation.equalsIgnoreCase("Connect")) {
					connectButton.setText("Connect");
					enableEdit();
					mReaderActivity.reader = null;
				}
			} else {
				validationField.setText("");
				validationField.setVisibility(8);
				if (operation.equalsIgnoreCase("Connect")) {
					connectButton.setText("Disconnect");
					servicelayout.setVisibility(0);
					readerRadioGroup.setVisibility(8);
					disableEdit();
					connectButton.setClickable(true);
					readButton.setClickable(true);
					mReaderActivity.reader = reader;
				} else {
					connectButton.setText("Connect");
					servicelayout.setVisibility(8);
					readerRadioGroup.setVisibility(0);
					enableEdit();
					totalTagCountView.setText("");
					mReaderActivity.reader = null;
					System.gc();
				}
			}
		}

		private void disableEdit() {
			connectButton.setClickable(false);
			ntReaderField.setEnabled(false);
			serialList.setEnabled(false);
			serialReaderRadioButton.setClickable(false);
			networkReaderRadioButton.setClickable(false);
		}

		private void enableEdit() {
			connectButton.setClickable(true);
			ntReaderField.setEnabled(true);
			serialList.setEnabled(true);
			serialReaderRadioButton.setClickable(true);
			networkReaderRadioButton.setClickable(true);
		}
	}
}

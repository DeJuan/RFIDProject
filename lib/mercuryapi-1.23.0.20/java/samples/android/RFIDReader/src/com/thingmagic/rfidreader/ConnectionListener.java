package com.thingmagic.rfidreader;

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

import com.thingmagic.Reader;
import com.thingmagic.ReaderConnect;
import com.thingmagic.util.LoggerUtil;
import com.thingmagic.util.Utilities;

public class ConnectionListener implements View.OnClickListener {

	static EditText ntReaderField;
	String readerChecked;
	static Button connectButton;
	static TextView validationField;
	static ReaderActivity readerActivity;
	static LinearLayout servicelayout;
	private static Reader reader = null;
	private static RadioGroup readerRadioGroup = null;
	private static String TAG="ConnectionListener";
	private static Spinner serialList=null;
	private static RadioButton serialReaderRadioButton = null;
	private static RadioButton networkReaderRadioButton = null;
	private static TableLayout table ;
	private static TextView searchResultCount = null;
	public static TextView totalTagCountView = null;
//	public static TextView readrateView = null;
	private static Button readButton = null;

	// Progress Dialog
    private static ProgressDialog pDialog =null;
    private static String readerName =null;
	

	public ConnectionListener(ReaderActivity readerActivity) {
		this.readerActivity = readerActivity;
		pDialog = new ProgressDialog(readerActivity);
		pDialog.setCancelable(false);
	}

	private void findAllViewsById() {
		ntReaderField = (EditText) readerActivity
				.findViewById(R.id.search_edit_text);
		connectButton = (Button) readerActivity
				.findViewById(R.id.Connect_button);
		readerRadioGroup = (RadioGroup) readerActivity
				.findViewById(R.id.Reader_radio_group);
		servicelayout = (LinearLayout) readerActivity
				.findViewById(R.id.ServiceLayout);
		validationField = (TextView) readerActivity
				.findViewById(R.id.ValidationField);
		serialList = (Spinner) readerActivity
				.findViewById(R.id.SerialList);
		serialReaderRadioButton = (RadioButton) readerActivity
				.findViewById(R.id.SerialReader_radio_button);
		networkReaderRadioButton = (RadioButton) readerActivity
				.findViewById(R.id.NetworkReader_radio_button);
		table = (TableLayout) readerActivity
				.findViewById(R.id.tablelayout);
		searchResultCount = (TextView) readerActivity
				.findViewById(R.id.search_result_view);
		totalTagCountView = (TextView) readerActivity
				.findViewById(R.id.totalTagCount_view);	
//		readrateView = (TextView) readerActivity
//				.findViewById(R.id.readRate_view);
		
		readButton = (Button) readerActivity
				.findViewById(R.id.Read_button);
		
	}

	public Reader getConnectedReader() {
		return reader;
	}

	@Override
	public void onClick(View arg0) 
	{
		findAllViewsById();
		validationField.setText("");
		validationField.setVisibility(8);
		searchResultCount.setText("");
		table.removeAllViews();
		int id = readerRadioGroup.getCheckedRadioButtonId();
		RadioButton readerRadioButton = (RadioButton) readerActivity
				.findViewById(id);
		readerChecked = readerRadioButton.getText().toString();
		
		String query="";
		boolean validPort = true;
		if (readerChecked.equalsIgnoreCase("SerialReader")) 
		{
			query=serialList.getSelectedItem().toString();
			readerName = query.split("-")[0];
			query ="tmr:///"+query.split("-")[1];
			
			
		}
		else if (readerChecked.equalsIgnoreCase("NetworkReader"))
		{
			query = ntReaderField.getText().toString().trim();
			readerName = query;
//			if(query.equalsIgnoreCase("")){
//				//query="172.16.16.124";
//				query="10.2.0.103";
//			}

			validPort = Utilities.validateIPAddress(validationField, query);
			if (validPort) {
				query ="tmr://"+query;
				validationField.setVisibility(8);
				// Closing keyPad manually
				InputMethodManager imm = (InputMethodManager) readerActivity
						.getSystemService(Context.INPUT_METHOD_SERVICE);
				imm.hideSoftInputFromWindow(ntReaderField.getWindowToken(), 0);
			}
			else
			{
				validationField.setVisibility(0);
				return;
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
		private boolean operationStatus=true;

		public ReaderConnectionThread(String requestedQuery, String operation) {
			this.uriString = requestedQuery;
			this.operation = operation;

		}

		@Override
		protected void onPreExecute() {
			LoggerUtil.debug(TAG, "** onPreExecute **");	
			pDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
			if (operation.equalsIgnoreCase("Connect")) {
				disableEdit();
				pDialog.setMessage("Connecting. Please wait...");
			}else{
				readButton.setClickable(false);
				disableEdit();
				pDialog.setMessage("DisConnecting. Please wait...");
			}
			pDialog.show();
			searchResultCount.setText("");
			
		}

		@Override
		protected String doInBackground(Void... params) {
			Long startTime = System.currentTimeMillis();
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
				operationStatus=false;
				if(ex.getMessage().contains("Connection is not created")||ex.getMessage().contains("failed to connect")){
					exception  += "Failed to connect to "+readerName;
				}else{
					exception  += ex.getMessage();	
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
				if (operation.equalsIgnoreCase("Connect")) {
					connectButton.setText("Connect");
					enableEdit();
					readerActivity.reader = null;
				}
			}
			else
			{
				validationField.setText("");
				validationField.setVisibility(8);
				if (operation.equalsIgnoreCase("Connect")) 
				{
					connectButton.setText("Disconnect");
					servicelayout.setVisibility(0);
					readerRadioGroup.setVisibility(8);
					disableEdit();
					connectButton.setClickable(true);
					readButton.setClickable(true);
					readerActivity.reader = reader;
			    }
				else 
				{
					connectButton.setText("Connect");
					servicelayout.setVisibility(8);
					readerRadioGroup.setVisibility(0);
					enableEdit();					
					totalTagCountView.setText("");
//					readrateView.setText("");
					readerActivity.reader = null;
				}
			}
		}
		private void disableEdit()
		{
			connectButton.setClickable(false);
			ntReaderField.setEnabled(false);
			serialList.setEnabled(false);
			serialReaderRadioButton.setClickable(false);
			networkReaderRadioButton.setClickable(false);
		}
		private void enableEdit()
		{			
			connectButton.setClickable(true);
			ntReaderField.setEnabled(true);
			serialList.setEnabled(true);
			serialReaderRadioButton.setClickable(true);
			networkReaderRadioButton.setClickable(true);
		}
	}

}

package com.thingmagic.rfidreader;

import java.util.ArrayList;
import java.util.Set;

import android.R.layout;
import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Configuration;
import android.os.Bundle;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.inputmethod.InputMethodManager;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ExpandableListView;
import android.widget.ImageView;
import android.widget.RadioButton;
import android.widget.Spinner;
import android.widget.TextView;

import com.thingmagic.Reader;
import com.thingmagic.rfidreader.Listener.ConnectionListener;
import com.thingmagic.rfidreader.Listener.ReaderSearchListener;
import com.thingmagic.rfidreader.Listener.ServiceListener;
import com.thingmagic.rfidreader.customViews.CustomEditText;
import com.thingmagic.rfidreader.customViews.ExpandableListAdapter;
import com.thingmagic.rfidreader.services.UsbService;
import com.thingmagic.util.GlobalExceptionHandler;
import com.thingmagic.util.LoggerUtil;
import com.thingmagic.util.Utilities;



public class ReaderActivity extends Activity 
{

	private static final String TAG = "ReaderActivity";
	private static CustomEditText readEditText;
	private RadioButton serialReaderRadioButton = null;
	private RadioButton networkReaderRadioButton = null;
	private RadioButton syncReadRadioButton = null;
	private RadioButton asyncReadSearchRadioButton = null;
	private static Button connectButton = null;
	private static Button readButton = null;
	private static Button clearButton = null;
	private static Button settingsButton = null;
	private static Button homeButton = null ;
	private static Spinner serialList=null;	
	private static Display display=null;
	private static TextView searchResultCount = null;
	private static TextView rowNumberLabelView = null;
	private static TextView epcDataLabelView = null;
	private static TextView epcCountLabelView = null;
	private int windowWidth;
	public int rowNumberWidth=0;
	public int epcDataWidth=0;
	public int epcCountWidth=0;
	public Reader reader=null;
	private ReaderActivity activity;
	public View readOptions;
	public View performance_metrics;
	public View firmware;

	public ExpandableListAdapter expandableListAdapter;
	
	/**
     * The flag indicating whether content is loaded (text is shown) or not (setting view is
     * shown).
     */
    private boolean mContentLoaded;
    private View mSettingView;
    private View mDisplayView;
    
	@Override
	public void onCreate(Bundle savedInstanceState) 
	{
		try{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_reader);
		
		mSettingView = findViewById(R.id.settings);
		mDisplayView = findViewById(R.id.displayView);

        // Initially hide the setting view.
        mSettingView.setVisibility(View.GONE);
     
		activity=this;
		try 
		{
			Utilities.loadLog4j(this);
		} 
		catch (Exception ex) 
		{
			LoggerUtil.error(TAG, "Error loading log4j", ex);
		}
		this.findAllViewsById();
		windowWidth=display.getWidth();
		serialReaderRadioButton.setOnClickListener(readerRadioButtonListener);
		networkReaderRadioButton.setOnClickListener(readerRadioButtonListener);	
		
		readEditText.setDrawableClickListener(new ReaderSearchListener(this));


		connectButton.setOnClickListener(new ConnectionListener(this));
		
		syncReadRadioButton.setOnClickListener(serviceRadioButtonListener);
		asyncReadSearchRadioButton.setOnClickListener(serviceRadioButtonListener);
		
		loadSettingConntentView();
				
		readButton.setOnClickListener(new ServiceListener(this));
		
		readEditText.setOnFocusChangeListener(new Utilities.DftTextOnFocusListener(
				getString(R.string.Read)));
		searchResultCount.setText("");
		clearButton.setOnClickListener(new ServiceListener(this).clearListener);
		
		settingsButton.setOnClickListener(new OnClickListener() {			
			@Override
			public void onClick(View view) {
			  mContentLoaded = !mContentLoaded;
              showSettingsOrTagsView(mContentLoaded);
              InputMethodManager imm = (InputMethodManager) view.getContext()
						.getSystemService(Context.INPUT_METHOD_SERVICE);
				imm.hideSoftInputFromWindow(readEditText.getWindowToken(), 0);
			}
		});

		setPortraitTableLayout();
		System.gc();
		Thread.currentThread().setDefaultUncaughtExceptionHandler(new GlobalExceptionHandler(activity));
		}catch(Exception ex){
			ex.printStackTrace();
		}
	}

	private  void loadSettingConntentView(){
		ExpandableListView expandableList = (ExpandableListView) findViewById(R.id.expandableListView1);

		LayoutInflater infalInflater = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		readOptions = infalInflater.inflate(R.layout.settings_read_options, null);
		performance_metrics = infalInflater.inflate(R.layout.performance_metrics, null);
		firmware = infalInflater.inflate(R.layout.firmware, null);
		ArrayList<View> ChildViews = new ArrayList<View>();
		ChildViews.add(readOptions);
		ChildViews.add(performance_metrics);
		ChildViews.add(firmware);

		String[] groups = { "Read Options", "Statistics", "Firmware" };
		expandableListAdapter = new ExpandableListAdapter(this, groups, ChildViews);

		// Set this blank adapter to the list view
		expandableList.setAdapter(expandableListAdapter);
		homeButton.setOnClickListener(new OnClickListener() {			
			@Override
			public void onClick(View view) {				
				mContentLoaded = !mContentLoaded;
	            showSettingsOrTagsView(mContentLoaded);
			}
		});
	}
	
	/**
     * Cross-fades between {@link #mSettingView} and {@link #mDisplayView}.
     */
    private void showSettingsOrTagsView(boolean contentLoaded) {
        // Decide which view to hide and which to show.
        final View showView = contentLoaded ? mSettingView : mDisplayView;
        final View hideView = contentLoaded ? mDisplayView : mSettingView;

        showView.setVisibility(View.VISIBLE);
        showView.setVisibility(View.KEEP_SCREEN_ON);
        hideView.setVisibility(View.GONE);
    }
	    
	/**
	 * This is called when the screen rotates.
	 * (onCreate is no longer called when screen rotates due to manifest, see: android:configChanges)
	 */
	@Override
	public void onConfigurationChanged(Configuration newConfig)
	{
	    super.onConfigurationChanged(newConfig);
	    int orientation = newConfig.orientation;
	    if (orientation == Configuration.ORIENTATION_PORTRAIT){
	    	setPortraitTableLayout();
	    }        
	    else if (orientation == Configuration.ORIENTATION_LANDSCAPE){
	    	setPortraitTableLayout();
	    } 
	}
	
	private void setPortraitTableLayout()
	{
		rowNumberLabelView.setWidth((int) (windowWidth*(0.1)));
		epcDataLabelView.setWidth((int) (windowWidth*(0.73)));
		epcCountLabelView.setWidth((int) (windowWidth*(0.17)));
		rowNumberWidth=(int) (windowWidth*(0.09));
		epcDataWidth=(int) (windowWidth*(0.75));
		epcCountWidth=(int) (windowWidth*(0.16));
	}
	
	@Override
	public void onBackPressed() {
		String message = "";
		String title = "";
		if(reader !=null)
		{
			title = "Run in background";
			message = "Are you sure you want to run this application in background?";
		}
		else
		{
			title = "Closing application";
			message = "Are you sure you want to close this application?";
		}
			
	    new AlertDialog.Builder(this)
	        .setIcon(android.R.drawable.ic_dialog_alert)
	        .setTitle(title)
	        .setMessage(message)
	        .setPositiveButton("Yes", new DialogInterface.OnClickListener()
	    {
	        @Override
	        public void onClick(DialogInterface dialog, int which) {
	        	if(reader != null)
        		{
	        		moveTaskToBack(true);	
        		}
	        	else
	        	{
	        		System.gc();
        			finish();
        		}
	        }
	    })
	    .setNegativeButton("No", null)
	    .setCancelable(false)
	    .show();
	}
		
	private void findAllViewsById() 
	{
		readEditText = (CustomEditText) findViewById(R.id.search_edit_text);
		connectButton = (Button) findViewById(R.id.Connect_button);
		serialReaderRadioButton = (RadioButton) findViewById(R.id.SerialReader_radio_button);
		networkReaderRadioButton = (RadioButton) findViewById(R.id.NetworkReader_radio_button);
		syncReadRadioButton = (RadioButton) findViewById(R.id.SyncRead_radio_button);
		asyncReadSearchRadioButton = (RadioButton) findViewById(R.id.AsyncRead_radio_button);
		readButton = (Button) findViewById(R.id.Read_button);
		searchResultCount = (TextView) findViewById(R.id.search_result_view);
		serialList = (Spinner) findViewById(R.id.SerialList);
		clearButton = (Button) findViewById(R.id.Clear);
		settingsButton =(Button) findViewById(R.id.settings_view);
		display = ((android.view.WindowManager)getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
		rowNumberLabelView = (TextView) findViewById(R.id.SNOLabel);
		epcDataLabelView = (TextView) findViewById(R.id.EPCLabel);
		epcCountLabelView = (TextView) findViewById(R.id.COUNTLabel);
		homeButton = (Button)findViewById(R.id.btn_back_main);
	}


	private OnClickListener readerRadioButtonListener = new OnClickListener() {
		public void onClick(View v) 
		{
			RadioButton radioButton = (RadioButton) v;
			if (radioButton.getText().equals("SerialReader")) 
			{
				try
				{
					// Closing keyPad manually
					InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
					imm.hideSoftInputFromWindow(readEditText.getWindowToken(), 0);
					BluetoothService bluetoothService= new BluetoothService();
					boolean btEnabled =bluetoothService.checkBTState(v.getContext(), activity);
					 ArrayAdapter<String> adapter = new ArrayAdapter<String>(
				        		activity,
								android.R.layout.simple_spinner_dropdown_item);
					if (btEnabled) 
					{
						Set<BluetoothDevice> pairedDevices = bluetoothService
								.getPairedDevices();
						for (BluetoothDevice bluetoothDevice : pairedDevices) 
						{
							adapter.add(bluetoothDevice.getName()+" \n "+bluetoothDevice.getAddress());
						}
					}else{
						radioButton.setSelected(false);
					}
					LoggerUtil.debug(TAG, "Getting USB device list");
					UsbService usbService = new UsbService();
					ArrayList<String> connectedUSBDeviceNames = usbService.getConnectedUSBdevices(activity);
					for(String deviceName: connectedUSBDeviceNames){
						adapter.add(deviceName);
					}
					serialList.setAdapter(adapter);
					readEditText.setVisibility(8);
					serialList.setVisibility(0);
				}
				catch(Exception ex)
				{
					LoggerUtil.error(TAG, "Error loading paired bluetooth devices :", ex);
				}
			} else if (radioButton.getText().equals("NetworkReader")) {
				serialList.setVisibility(8);
				readEditText.setVisibility(0);
			}
		}
	};

	private OnClickListener serviceRadioButtonListener = new OnClickListener() {
		public void onClick(View v) {
			RadioButton radioButton = (RadioButton) v;
			if (radioButton.getText().equals("AsyncRead")) {
				readButton.setText("Start Reading");
			} else if (readButton.getText().equals("Start Reading")) {
				readButton.setText("Read");
			}
		}
	};
}
package com.thingmagic.rfidreader.services;

import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.Spinner;

import com.thingmagic.Gen2;
import com.thingmagic.Reader;
import com.thingmagic.SimpleReadPlan;
import com.thingmagic.TagOp;
import com.thingmagic.TagProtocol;
import com.thingmagic.rfidreader.R;
import com.thingmagic.rfidreader.ReaderActivity;
import com.thingmagic.util.Utilities;

public class SettingsService {
	private static CheckBox embeddedCheckBox = null;
	private static Spinner memBankSpinner = null;
	private static EditText embedded_start_view=null;
	private static EditText embedded_length_view=null;
	
	private static ReaderActivity mReaderActivity;
	public SettingsService(ReaderActivity readerActivity){
		mReaderActivity= readerActivity;
		findAllViewsById();
	}

	private void findAllViewsById() 
	{
		embeddedCheckBox =(CheckBox)mReaderActivity.findViewById(R.id.embedded_enabled);
		memBankSpinner =(Spinner)mReaderActivity.findViewById(R.id.embedded_bank);
		embedded_start_view =(EditText)mReaderActivity.findViewById(R.id.embedded_start);
		embedded_length_view =(EditText)mReaderActivity.findViewById(R.id.embedded_length);
	}

	
	public void loadReadPlan(Reader reader) throws Exception{
		SimpleReadPlan readPlan = null;
		TagOp readTagOp=null;
		boolean useFastSearch = false;
		
		// Verify Embedded read enabled
		if(embeddedCheckBox.isChecked()){
			int wordAddress= Integer.parseInt(embedded_start_view.getText().toString());
			byte length= Byte.valueOf(embedded_length_view.getText().toString());
			String bank=memBankSpinner.getSelectedItem().toString();
			Gen2.Bank gen2Bank=Utilities.gen2BankMap.get(bank);
			 readTagOp= new Gen2.ReadData(gen2Bank, wordAddress, length);
		}
		readPlan = new SimpleReadPlan(new int[]{1,2}, TagProtocol.GEN2, null, readTagOp, 1000,useFastSearch );
		reader.paramSet("/reader/read/plan", readPlan);	
	}
	
}

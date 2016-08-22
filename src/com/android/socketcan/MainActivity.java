package com.android.socketcan;

import java.io.IOException;

import com.android.socketcan.CanSocket.*;

import android.os.Bundle;
import android.app.Activity;
import android.view.Menu;

public class MainActivity extends Activity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		try {
			CanUtils.configCan0Device();
			CanUtils.initCan0();
			CanUtils.sendCan0Data(new byte[]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}
}

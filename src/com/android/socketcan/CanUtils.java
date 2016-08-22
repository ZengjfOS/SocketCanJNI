package com.android.socketcan;

import java.io.IOException;
import java.util.Arrays;

import android.R.integer;
import android.util.Log;

import com.android.socketcan.CanSocket.CanFrame;
import com.android.socketcan.CanSocket.CanId;
import com.android.socketcan.CanSocket.CanInterface;
import com.android.socketcan.CanSocket.Mode;

public class CanUtils {
	
	private static String TAG = "CanUtils";
	
	private static CanSocket socket ;
	private static CanInterface canif;

	private static String cmdList[] = {
	    "su 0 netcfg can0 down",
	    "su 0 ip link set can0 type can bitrate 1000000 triple-sampling on",
	    "su 0 netcfg can0 up",
	};
	

	public static void configCan0Device(){
		for (String cmd : cmdList) {
			try {
				Log.e(TAG, cmd);
				Log.e(TAG, ShellExecute.execute(cmd));
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}
	
	public static void initCan0(){
		try {
			socket = new CanSocket(Mode.RAW);
			canif = new CanInterface(socket, "can0");
			socket.bind(canif);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	public static CanFrame revCan0Data(){
   		try {
			return socket.recv();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
   		return null;
	}
	
	public static void sendCan0Data(byte[] data) {
		try {
			CanId id = new CanId(0x05);
			int i = 0;
			byte currentData[] = null;
			for(; i*8 < data.length - 8; i++) {
				currentData = Arrays.copyOfRange(data, i*8, (i+1)*8);
				socket.send(new CanFrame(canif, id, currentData));
			}
			currentData = Arrays.copyOfRange(data, i*8, data.length);
			socket.send(new CanFrame(canif, id, currentData));
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

}

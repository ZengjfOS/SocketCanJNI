package com.android.socketcan;

import java.io.File;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

/**
 * 本类主要用于在Java层执行Linux shell命令，获取一些系统下的信息
 * 本例中的dmesg需要一些额外的权限才能使用
 * @author aplex
 */
public class ShellExecute {
	/**
	 * 本函数用于执行Linux shell命令 
	 * 
	 * @param command		shell命令，支持管道，重定向
	 * @param directory 	在指定目录下执行命令
	 * @return				返回shell命令执行结果
	 * @throws IOException	抛出IOException
	 */
    public static String execute ( String command, String directory )  
    		throws IOException {  
    	
    	// check the arguments
    	if (null == command || command.trim().equals("")) 
    		return "";

    	if (null == directory || directory.trim().equals("")) 
			directory = "/";

    	String result = "" ;  

    	List<String> cmds = new ArrayList<String>(); 
    	cmds.add("sh"); 
    	cmds.add("-c"); 
    	cmds.add(command); 

    	try {  
    		ProcessBuilder builder = new ProcessBuilder(cmds);  
      
    		if ( directory != null && directory.length() != 0)  
    			builder.directory ( new File ( directory ) ) ;  

    		builder.redirectErrorStream (true) ;  
    		Process process = builder.start ( ) ;  
      
    		//得到命令执行后的结果   
    		InputStream is = process.getInputStream ( ) ;  
    		byte[] buffer = new byte[1024] ;  
    		while ( is.read(buffer) != -1 )
    			result = result + new String (buffer) ;  

    		is.close ( ) ;  
    	} catch ( Exception e ) {  
    		e.printStackTrace ( ) ;  
    	}  
    	return result.trim() ;  
    }  

    /**
     * 本函数用于执行Linux shell命令，执行目录被指定为:"/"
     * 
	 * @param command		shell命令，支持管道，重定向
	 * @return				返回shell命令执行结果
	 * @throws IOException	抛出IOException
     */
    public static String execute (String command) throws IOException {  

    	// check the arguments
    	if (null == command || command.trim().equals("")) 
			return "";

    	return execute(command, "/");
    }  
    
    /**
     * 本函数用于判断dmesg中是否存在pattern字符串，执行目录被指定为:"/"
     * 
	 * @param pattern		给grep匹配的字符串	
	 * @return				true:  dmesg中存在pattern中的字符串<br>
	 * 						false：  dmesg中不存在pattern中的字符串
	 * @throws IOException	抛出IOException
     */
    public static boolean devExist(String pattern) throws IOException{

    	// check the arguments
    	if (null == pattern) 
			return false;

    	if (pattern == null || pattern.trim().equals("")) 
			return false;

    	return execute("dmesg | grep " + pattern).length() > 0;
    }
}

/***
 * Excerpted from "Programming Your Home",
 * published by The Pragmatic Bookshelf.
 * Copyrights apply to this code. It may not be used to create training material, 
 * courses, books, articles, and the like. Contact us if you are in doubt.
 * We make no guarantees that this code is fit for any purpose. 
 * Visit http://www.pragmaticprogrammer.com/titles/mrhome for more book information.
***/
package com.mysampleapp.androiddoorlockserver;

import java.io.IOException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.LinkedList;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;

public class Server extends Thread {
	private ServerSocket listener = null;
	private static Handler mHandler;
	private boolean running = true;
	
	public static LinkedList<Socket> clientList = new LinkedList<Socket>();
	
    public Server(String ip, int port, Handler handler) throws IOException {
		super();
		mHandler = handler;
		InetAddress ipadr = InetAddress.getByName(ip);
		listener = new ServerSocket(port,0,ipadr);
	}
    
    private static void send(String s) {
    	Message msg = new Message();
    	Bundle b = new Bundle();
    	b.putString("msg", s);
    	msg.setData(b);
    	mHandler.sendMessage(msg);
    }
    
	@Override
	public void run() {
		while( running ) {
			try {
//				send("Waiting for connections.");
//				send("Current Lock Status = " + LockStatus.getInstance().getLockStatus());
				Socket client = listener.accept();
//			    send("New connection from " + client.getInetAddress().toString());

				new ServerHandler(client).start();
				LockStatus.getInstance().setMyVar(true);
				
//				send("Current Lock Status = " + LockStatus.getInstance().getLockStatus());
				clientList.add(client);
				
			} catch (IOException e) {
//				send(e.getMessage());
			}
		}
	}

	public void stopServer() {
		running = false;
		LockStatus.getInstance().setMyVar(false);
		try {
			listener.close();
		} catch (IOException e) {
//			send(e.getMessage());
		}
	}
	
	public synchronized static void remove(Socket s) {
//	    send("Closing connection: " + s.getInetAddress().toString());
        clientList.remove(s);      
    }
}

package com.example.tcp;

import java.io.IOException;
import java.net.SocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.SocketChannel;
import java.nio.charset.Charset;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;

public abstract class Client implements Runnable {
	private static final int INIT_RECONNECT_INTERVAL = 500; // 500 ms
	private static final int MAX_RECONNECT_INTERVAL = 30000; // 30000 sec
	private static final int READ_BUFFER_SIZE = 1024 * 1024;
	
	private int reconnectInterval = INIT_RECONNECT_INTERVAL;
	
	/*Buffer for incoming messages.*/
	private ByteBuffer readBuf = ByteBuffer.allocate(READ_BUFFER_SIZE);
	
	/*Thread handle*/
	private final Thread thread = new Thread(this);
	
	/*Socket specific data.*/
	private SocketAddress address;
	private Selector selector;
	private SocketChannel channel;
	
	/*Connection status flag*/
	private boolean connected = false;
	
	/*Changes to the socket are carried out only by the selector thread to avoid blocking
	 *in case of naive NIO implementation.*/
	/*A list that registers pending changes to the selector.*/
	private final List<ChangeRequest> pendingChanges = new LinkedList<>();

	/*Send requests are pushed in this queue after which a pending change request is made.*/
	private final List<ByteBuffer> pendingData = new LinkedList<>();
	
	
	public void start()  throws IOException{
		thread.start();
		System.out.println("Starting!");
	}
	
	public boolean isConnected()
	{
		return connected;
	}
	
	/*Event Handlers. Override with something useful.*/
	protected abstract void onRead(ByteBuffer buf);
	protected abstract void onConnected();
	protected abstract void onDisconnect();
	
	/*Configure socket channel*/
	
	private void configureChannel(SocketChannel channel) throws IOException{
		channel.configureBlocking(false);
		channel.socket().setSendBufferSize(1024*1024);
		channel.socket().setReceiveBufferSize(1024*1024);
		channel.socket().setKeepAlive(true);
		channel.socket().setReuseAddress(true);
		channel.socket().setSoLinger(false, 0);
		channel.socket().setSoTimeout(0);
		channel.socket().setTcpNoDelay(true);
	}
	
	
	/*Main method that delegates all of the client tasks.*/
	public void run() {
		try {
			while(!Thread.interrupted()) // reconnection loop.
			{
				try{
					/*Create a channel and socket.*/
					selector = Selector.open();
					channel = SocketChannel.open();
					configureChannel(channel);
					
					/*Connect to the given address.*/
					channel.connect(address);
					channel.register(selector, SelectionKey.OP_CONNECT);
					
					/*Enter main loop*/
					while(!thread.isInterrupted() && channel.isOpen()) {// events multiplexing loop
						processPendingChanges(); // update Selection flags.
						if(selector.select() > 0)
							processSelectedKeys(selector.selectedKeys());
					}
					
				}catch (Exception e)
				{
					//e.printStackTrace();
					System.out.println("Exception: " + e );
				} finally {
					connected = false;
					onDisconnect(); // call event handler.
					//reset state.
					pendingData.clear();
					pendingChanges.clear();
					readBuf.clear();

					try {
						if (channel != null) channel.close();
						if (selector != null) selector.close();
					} catch(IOException e) {
						e.printStackTrace();
					}
					System.out.println("Connection closed!");
				}
				
				
				try{
					Thread.sleep(reconnectInterval);
					if(reconnectInterval < MAX_RECONNECT_INTERVAL)
						reconnectInterval *= 2;
					System.out.println("Reconnecting to: " + address);
				} catch (InterruptedException e) {
					break;
				}
					
			}
		} catch(Exception e)
		{
			System.out.println("Unrecoverable error! : " + e);		
		}
		System.out.println("Exiting Main loop!");
	}
	
	private void processPendingChanges() throws Exception {
		// Process any pending changes
		synchronized (pendingChanges) {
			Iterator<ChangeRequest> changes = pendingChanges.iterator();
			SelectionKey key;
			
			while (changes.hasNext()) {
				ChangeRequest change = (ChangeRequest) changes.next();
				switch (change.type) {
					case ChangeRequest.CHANGEOPS:
						key = change.socket.keyFor(this.selector);
						key.interestOps(change.ops);
						break;
					case ChangeRequest.REGISTER:
						change.socket.register(this.selector, change.ops);
						break;
					case ChangeRequest.APPENDOPS:
						System.out.println("Prcoessing pending changes! Append Pos!");
						key = change.socket.keyFor(this.selector);
						key.interestOps(key.interestOps() | change.ops);
						break;
					}
			}
			pendingChanges.clear();
		}
	}
	private void processSelectedKeys(Set<SelectionKey> keys) throws Exception {
		Iterator<SelectionKey> itr = keys.iterator();
		while(itr.hasNext()) {
			SelectionKey key = itr.next();
			
			if(!key.isValid())
				continue;
			
			if(key.isReadable()) 
				processRead(key);
			if(key.isWritable())
				processWrite(key);
			if(key.isConnectable())
				processConnect(key);
			itr.remove();
		}
	}
	
	private void processRead(SelectionKey key) throws Exception {
		System.out.println("I am reading!");
		SocketChannel ch = (SocketChannel) key.channel();
		/*Clear out our read buffer so it's ready for new data*/
		readBuf.clear();
		
		/*Attempt to read off the channel*/
		int numRead;
		try {
			numRead = ch.read(readBuf);
		} catch (IOException e) {
			/*The remote forcibly closed the connection, cancel
			 * the selection key and close the channel*/
			channel.close();
			e.printStackTrace();
			return;
		}
		if (numRead == -1) {
			// Remote entity shut the socket down cleanly. Do the
			// same from our end and cancel the channel.
			System.out.println("Peer closed read channel.");
			channel.close();
			return;
		}
		/*Copy buffer and pass it to callback.*/
		//readBuf.flip();
		byte[] rspData = new byte[numRead];
		System.arraycopy(readBuf.array(), 0, rspData, 0, numRead);
		onRead(ByteBuffer.wrap(rspData));
	}
	
	private void processWrite(SelectionKey key) throws Exception {
		System.out.println("I am writing!");
		SocketChannel ch = (SocketChannel)key.channel();
		
		synchronized (pendingData) {
			// Write until there's not more data
			while (!pendingData.isEmpty()) {
				ByteBuffer buf = (ByteBuffer) pendingData.get(0);
				
				ch.write(buf);
				System.out.println(new String(buf.array(), Charset.forName("UTF-8")));
				if (buf.remaining() > 0) {
					// the socket buffer is full exit.
					System.out.println("Socket buffer is full!");
					break;
				}
				pendingData.remove(0);
			}

			if (pendingData.isEmpty()) {
				System.out.println("No more data to write!");
				// We wrote away all data, so we're no longer interested
				// in writing on this socket.
				//key.interestOps(key.interestOps() ^ SelectionKey.OP_WRITE);
				key.interestOps(SelectionKey.OP_READ);
			}
		}
	}
	
	private void processConnect(SelectionKey key) throws Exception {
		SocketChannel ch = (SocketChannel) key.channel();
		
		try {
			if (ch.finishConnect()) {
				System.out.println("connected to " + address);
				/*
				key.interestOps(key.interestOps() ^ SelectionKey.OP_CONNECT);
				key.interestOps(key.interestOps() | SelectionKey.OP_READ);
				*/
				key.interestOps(SelectionKey.OP_READ);
				reconnectInterval = INIT_RECONNECT_INTERVAL;
				connected = true;
				onConnected();
			}
		} catch (IOException e) {
			// Cancel the channel's registration with our selector
			System.out.println(e);
			key.cancel();
			return;
		}
		
		System.out.println("Processing connection!");
	}
	
	
	public SocketAddress getAddress(){
		return address;
	}
	
	public void setAddress(SocketAddress address)
	{
		this.address = address;
	}
	
	public void send(byte[] data) throws IOException {
		/*The caller thread is not the client thread, so add a pending change notification.*/
		
		synchronized(pendingChanges) {
			pendingChanges.add(new ChangeRequest(channel, ChangeRequest.APPENDOPS, SelectionKey.OP_WRITE));
		}
		
		/* Queue the data we want written*/
		synchronized (pendingData) {
			/*if (pendingData == null) {
				System.out.println("List is null ");
				pendingData = new ArrayList<>();
			}*/
			pendingData.add(ByteBuffer.wrap(data));
		}
		
		/*Notify selector.*/
		selector.wakeup();
	}
/*
	public static void main(String args[]) throws Exception {
		Client client = new Client () {
			@Override
			protected void onRead(byte[] buf) {
				System.out.println(new String(buf, Charset.forName("UTF-8")));	
			}
			@Override
			protected void onConnected() {
				System.out.println("Connected!");	
			}
			@Override
			protected void onDisconnect() {
				System.out.println("Disconnect!");
			}
		};
		
		client.setAddress(new InetSocketAddress("172.19.10.76", 80));
		
		try{
			client.start();
		} catch(IOException e) {
			e.printStackTrace();
		}
		while(!client.isConnected()) {
			Thread.sleep(500);
		}
		
		for(int i = 0; i<100; i++)
			client.send(("h").getBytes());
		client.send("\n".getBytes());
	
		//Send a number of messages.
		while(true)
		{
			Thread.sleep(10);
		}
		
	}
	*/
}
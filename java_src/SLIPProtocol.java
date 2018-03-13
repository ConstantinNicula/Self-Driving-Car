package com.example.tcp;

import java.nio.ByteBuffer;
import java.util.Arrays;

public class SLIPProtocol {
	/*Special character bytes.*/
	public static final byte SLIP_END = (byte) 0xC0;
	public static final byte SLIP_ESC = (byte) 0xDB;
	public static final byte SLIP_ESC_END = (byte) 0xDC;
	public static final byte SLIP_ESC_ESC = (byte) 0xDD;
	
	/*Encode message.*/
	public static ByteBuffer encode(ByteBuffer buf) {
		/*Create a new byte buffer to accommodate new escaped sequence */
		ByteBuffer escapedBuf = ByteBuffer.allocate(buf.limit() * 2);
		for(int i = 0; i < buf.limit(); i++) {
			byte value = buf.get();
			
			switch(value) {
			case SLIP_END:
				escapedBuf.put(SLIP_ESC);
				escapedBuf.put(SLIP_ESC_END);
				break;
			case SLIP_ESC:
				escapedBuf.put(SLIP_ESC);
				escapedBuf.put(SLIP_ESC_ESC);
				break;
			default:
				escapedBuf.put(value);
				break;
			}
		}
		
		/*Flip buffer for reading.*/
		escapedBuf.flip();
		return escapedBuf;
	}
	
	/*Decode message.*/
	
	public static ByteBuffer decode(ByteBuffer buf) {	
		/*Create a new byte buffer to accommodate new escaped sequence */
		ByteBuffer decodedBuf = ByteBuffer.allocate(buf.limit());
		boolean isEscaped = false;
		
		for(int i = 0; i < buf.limit(); i++) {
			byte value = buf.get();
			
			if(value == SLIP_ESC)
				isEscaped = true;
			else {
				if(isEscaped)
				{
					if(value == SLIP_ESC_END) 
						value = SLIP_END;
					else if(value == SLIP_ESC_ESC ) 
						value = SLIP_ESC;
					
					isEscaped = false; 
				}
				
				decodedBuf.put(value);
			}
		}
		
		/*Flip buffer for reading.*/
		decodedBuf.flip();
		return decodedBuf;
	}
	
	/*For testing*/
	static public void main(String []args) throws Exception {
		ByteBuffer buf = ByteBuffer.allocate(5);
		buf.put((byte) 192);
		buf.put((byte) 219);
		buf.put((byte) 220);
		buf.put((byte) 221);
		buf.flip();
		System.out.println(Arrays.toString(buf.array()));
		ByteBuffer encodedBuf = SLIPProtocol.encode(buf);
		System.out.println(Arrays.toString(encodedBuf.array()));
		ByteBuffer decodedBuf = SLIPProtocol.decode(encodedBuf);		
		System.out.println(Arrays.toString(decodedBuf.array()));
	}
	
}

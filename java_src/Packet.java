package com.example.tcp;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;

public class Packet {
	
	/*CMD field*/
	private byte cmd;
	/*Length of packet data*/
	private byte length;
	/*payload */
	private ByteBuffer payload;
	/*crc value*/
	private short crc;
	
	
	public Packet(byte cmd, byte length, ByteBuffer data, short crc)
	{
		this.cmd = cmd;
		this.length = length;
		
		/*Deep copy rest of message into byte buffer.*/
		payload = ByteBuffer.allocate(data.limit()).put(data);
		payload.order(ByteOrder.LITTLE_ENDIAN);
		payload.flip(); // prepare for read.

		this.crc = crc;
	}

	public Packet(byte cmd, byte length, ByteBuffer data)
	{
		this.cmd = cmd;
		this.length = length;
		
		/*Deep copy rest of message into byte buffer.*/
		payload = ByteBuffer.allocate(data.limit()).put(data);
		payload.order(ByteOrder.LITTLE_ENDIAN);
		payload.flip(); // prepare for read.
	
		// compute crc.
		this.crc = computeCRC();
	}
	
	public Packet(byte cmd, ByteBuffer data) 
	{
		this.cmd = cmd;
		this.length = (byte)data.limit();
		
		/*Deep copy rest of message into byte buffer.*/
		payload = ByteBuffer.allocate(data.limit()).put(data);
		payload.order(ByteOrder.LITTLE_ENDIAN);
		payload.flip(); // prepare for read.
	
		// compute crc.
		this.crc = computeCRC();
	}
	
	public short computeCRC() {
		CRC16 crc = new CRC16(0);
		crc.reset(0);
		crc.update(cmd);
		crc.update(length);
		return crc.update(payload, 0, payload.limit());
	}

	public ByteBuffer toEncodedByteBuffer(){
		// do not export invalid messages.
		if(!isValid())
			return null;
		/*Write data to temp buffer.*/
		ByteBuffer rez = ByteBuffer.allocate(length + 4);
		rez.order(ByteOrder.LITTLE_ENDIAN);
		rez.put(cmd);
		rez.put(length);
		rez.put(payload);
		rez.putShort(crc);
		rez.flip();
		
		payload.flip();
		
		return SLIPProtocol.encode(rez);
	}
	
	public ByteBuffer createFramed(){
		if(!isValid())
			return null;
		ByteBuffer packetAsByteBuf = toEncodedByteBuffer();
		ByteBuffer rez = ByteBuffer.allocate(packetAsByteBuf.limit() + 2);
		rez.put(SLIPProtocol.SLIP_END);
		rez.put(packetAsByteBuf);
		rez.put(SLIPProtocol.SLIP_END);
		rez.flip();
		
		return rez;
	}
	
	public boolean isValid()
	{
		return this.crc == computeCRC() && (length == payload.limit());
	}
	
	public byte getCMD()
	{
		return cmd;
	}
	
	public byte getLength()
	{	
		return length;
	}
	
	public ByteBuffer getData()
	{
		return payload;
	}
	
	public short getCrc()
	{
		return crc;
	}

	public void print()
	{
		System.out.println("CMD :" + cmd + " length: " + length + ", CRC: "+ crc);
		System.out.println(Arrays.toString(payload.array()));
	}
}

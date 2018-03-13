package com.example.tcp;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.Iterator;


public class MessageParser {
	/* Data buffer. */
	private static ByteBuffer buffer = null;
	
	public MessageParser(){
	}
	
	public static ArrayList<Packet> Parse(ByteBuffer data)
	{
		/*Create message array list.*/
		ArrayList<Packet> packets = new ArrayList<Packet>();
		
		/*Append inbounds data to buffer*/
		if(buffer != null)
			buffer = ByteBuffer.allocate(buffer.limit() + data.limit()).put(buffer).put(data);
		else
			buffer = ByteBuffer.allocate(data.limit()).put(data);
		buffer.flip();
		
		/*Loop through new data and check if we have a correctly framed packet.*/
		int length = 0;
		ByteBuffer msg = ByteBuffer.allocate(buffer.limit());
		
		while(buffer.hasRemaining())
		{
			byte value = buffer.get();
			if((value ==  SLIPProtocol.SLIP_END)) {
				if(length > 0)
				{
					/*Decode message. */
					msg.flip();
					ByteBuffer dec_msg = SLIPProtocol.decode(msg);
					dec_msg.order(ByteOrder.LITTLE_ENDIAN);
					byte cmd = dec_msg.get();
					byte payloadLen = dec_msg.get();
					short rcv_crc = dec_msg.getShort(dec_msg.limit() - 2); // last two bytes are crc.
					/*Get payload.*/
					dec_msg.position(2);
					dec_msg.limit(dec_msg.limit() - 2);
					ByteBuffer payload =  dec_msg.slice();
					
					/*Create packet and check if it is valid.*/
					Packet p = new Packet(cmd,payloadLen, payload , rcv_crc);
					if(p.isValid())
						packets.add(p);
					else {
						System.out.println("Invalid data!");
					}
				}

				/*reset counter.*/
				msg.clear();
				length = 0;
				buffer.compact();
				buffer.flip();
			}else {
				msg.put(value);
				length++;
			}
		}
		
		buffer.position(0); //reset buffer position for next read.
		return packets;
	}
	/*For testing*/
	public static void main(String [] args) throws Exception {
		
		byte cmd = 1;
		ByteBuffer payload = ByteBuffer.allocate(20);
		payload.order(ByteOrder.LITTLE_ENDIAN);
		payload.putInt(6969);
		payload.putFloat((float) 45.8969);
		payload.flip();
		Packet packet = new Packet(cmd, payload);



		ArrayList<Packet> packets = MessageParser.Parse(packet.createFramed());
		Iterator<Packet> itr = packets.iterator();
		while(itr.hasNext()) {
			Packet p = itr.next();
			p.print();
		}
	}
}

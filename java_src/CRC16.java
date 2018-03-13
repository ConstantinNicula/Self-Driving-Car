package com.example.tcp;
import java.nio.ByteBuffer;

public class CRC16 {
	/* Accumulator */
	private int acc;

	public CRC16(int acc) {
		this.acc = acc;
	}

	public int update(byte b) {
		acc = ((acc >> 8) | (acc << 8)) & 0xffff;
		acc ^= b & 0xff;
		acc ^= (acc & 0xff) >> 4;
		acc ^= (acc << 12) & 0xffff;
		acc ^= ((acc & 0xff) << 5) & 0xffff;

		return acc;
	}

	public void reset(int acc) {
		this.acc = acc;
	}

	public short update(byte[] b, int o, int l) {
		for (int i = o; i < l; i++)
			update(b[i]);
		return (short) acc;
	}

	public short update(ByteBuffer b, int o, int l) {
		return update(b.array(), o, l);
	}

	public short compute(byte b) {
		reset(0);
		return (short)update(b);
	}
	
	public short compute(byte[]b, int o, int l) {
		reset(0);
		return (short)update(b, o, l);
	}
	
	public short compute(ByteBuffer b, int o, int l) {
		reset(0);
		return (short)update(b, o , l);
	} 
	
	public short get() {
		return (short)acc;
	}
}

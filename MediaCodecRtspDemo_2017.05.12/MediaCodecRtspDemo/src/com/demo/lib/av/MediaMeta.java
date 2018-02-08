package com.demo.lib.av;

public class MediaMeta {
	public int Width;
	public int Height;
	public byte []Sps;
	public byte []Pps;
	
	@Override
	public String toString() {
		String val = "width " + Width + " Height " + Height;
		
		val += " sps ";
		if (null != Sps)
			for (int i = 0; i < Sps.length; i++)
				val += " " + Sps[i];
		
		val += " pps ";
		if (null != Pps)
			for (int i = 0; i < Pps.length; i++)
				val += " " + Pps[i];
		
		return val;
	}
}

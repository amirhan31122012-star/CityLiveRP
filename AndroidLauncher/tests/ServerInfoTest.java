package ru.citylive.launcher;
import java.io.*;
import java.nio.*;
import java.nio.charset.Charset;
import java.util.*;
public class ServerInfoTest {
 static final byte[] request={'S','A','M','P',127,0,0,1,(byte)0x93,0x45,'i'};
 static void field(ByteArrayOutputStream out,String value)throws Exception{
  byte[] data=value.getBytes(Charset.forName("windows-1251"));out.write(ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(data.length).array());out.write(data);
 }
 static void reject(byte[] packet)throws Exception{try{ServerInfo.decode(request,packet);throw new AssertionError("Accepted malformed packet");}catch(IOException expected){}}
 public static void main(String[] args)throws Exception{
  ByteArrayOutputStream out=new ByteArrayOutputStream();out.write(request);out.write(0);out.write(new byte[]{50,0,100,0});field(out,"Сервер CityLive");field(out,"CityLive RP C++");field(out,"Russian");
  byte[] packet=out.toByteArray();ServerInfo info=ServerInfo.decode(request,packet);
  if(info.players!=50||info.slots!=100||!info.hostname.equals("Сервер CityLive")||!info.mode.equals("CityLive RP C++"))throw new AssertionError("Incorrect info query parsing");
  for(int n=0;n<packet.length;++n)reject(Arrays.copyOf(packet,n));
  byte[] invalid=packet.clone();invalid[0]='X';reject(invalid);
  invalid=packet.clone();invalid[12]=101;reject(invalid);
  invalid=packet.clone();Arrays.fill(invalid,16,20,(byte)0xff);reject(invalid);
  invalid=packet.clone();ByteBuffer.wrap(invalid).order(ByteOrder.LITTLE_ENDIAN).putInt(16,2048);reject(invalid);
  System.out.println("PASS: SA-MP info parsing, CP1251, truncation, wrong header, player count and hostile string lengths");
 }
}

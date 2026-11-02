package ru.citylive.launcher;

import java.io.IOException;
import java.net.*;
import java.nio.*;
import java.nio.charset.Charset;
import java.util.Arrays;

/** Stock SA-MP public info query. No account credentials are sent. */
public final class ServerInfo {
    public final int players, slots;
    public final String hostname, mode;
    private ServerInfo(int players, int slots, String hostname, String mode) {
        this.players=players; this.slots=slots; this.hostname=hostname; this.mode=mode;
    }
    public static ServerInfo query(String host, int port) throws IOException {
        InetAddress address=InetAddress.getByName(host);
        if (address.getAddress().length!=4 || port<1 || port>65535) throw new IOException("IPv4 endpoint required");
        byte[] request=new byte[11];
        System.arraycopy(new byte[]{'S','A','M','P'},0,request,0,4);
        System.arraycopy(address.getAddress(),0,request,4,4);
        request[8]=(byte)port; request[9]=(byte)(port>>8); request[10]='i';
        try (DatagramSocket socket=new DatagramSocket()) {
            socket.connect(address,port); socket.setSoTimeout(3000);
            socket.send(new DatagramPacket(request,request.length));
            byte[] buffer=new byte[4096]; DatagramPacket reply=new DatagramPacket(buffer,buffer.length);
            socket.receive(reply);
            return decode(request,Arrays.copyOf(buffer,reply.getLength()));
        }
    }
    static ServerInfo decode(byte[] request, byte[] reply) throws IOException {
        if (reply.length<16 || !Arrays.equals(request,Arrays.copyOf(reply,11))) throw new IOException("Invalid SA-MP response");
        ByteBuffer data=ByteBuffer.wrap(reply).order(ByteOrder.LITTLE_ENDIAN); data.position(12);
        int players=Short.toUnsignedInt(data.getShort()),slots=Short.toUnsignedInt(data.getShort());
        if (players>slots) throw new IOException("Invalid player count");
        String name=string(data),mode=string(data); string(data); // language
        return new ServerInfo(players,slots,name,mode);
    }
    private static String string(ByteBuffer data) throws IOException {
        if(data.remaining()<4) throw new IOException("Truncated response");
        int length=data.getInt();
        if(length<0 || length>1024 || length>data.remaining()) throw new IOException("Invalid response length");
        byte[] bytes=new byte[length]; data.get(bytes);
        return new String(bytes,Charset.forName("windows-1251"));
    }
}

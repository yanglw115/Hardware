package com.intellegentcar.yangliwei.intellegentcar;

import android.content.Context;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.RequiresApi;
import android.widget.Toast;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.math.BigDecimal;
import java.math.RoundingMode;
import java.net.InetSocketAddress;
import java.net.Socket;

/**
 * Created by Administrator on 2017/6/5.
 */

public class TcpClient implements Runnable {
    public Socket m_sock;
    /* Handler的作用是在新启动的线程中发送消息或在主线程中获取，处理消息 */
    private Handler m_handlerSend;
    public Handler m_handlerReceive = null;
    //BufferedReader m_bufferedReader;
    /* 这里不用BufferedReader，而改用InputStream，是因为BufferedReader.readLine()在使用InputStreamReader的时候将byte转换成了char
     * ，从而导致原始的byte数据被破坏，特别是byte的值大于127，即byte高位是1的时候 */
    InputStream m_inputStream;
    OutputStream m_outputStream;
    Context m_context;
    Looper m_looper;


    public TcpClient(Handler handler, Context context) {
        /* 这里的handler来自activity，这里用于向activity发送数据 */
        this.m_handlerSend = handler;
        this.m_context = context;
    }

    public void sendWiFiSerialData(String strData) {
        try {
            m_outputStream.write(strData.getBytes("utf-8"));
            m_outputStream.flush();
        } catch (IOException e) {
            e.printStackTrace();
            Toast.makeText(m_context, "Error", Toast.LENGTH_SHORT);
        }
    }

    public void sendWiFiSerialData(byte[] bytes) {
        try {
            m_outputStream.write(bytes);
            m_outputStream.flush();
        } catch (IOException e) {
            e.printStackTrace();
            Toast.makeText(m_context, "Error", Toast.LENGTH_SHORT);
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    public void stopRun() {
        if (m_looper != null) {
            m_looper.quitSafely();
            m_looper = null;
            if (m_sock != null && m_sock.isConnected()) {
                try {
                    m_sock.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
                m_sock = null;
            }
        }
    }

    @Override
    public void run() {
        try {
            synchronized (this) {
                String strIP = MainActivity.getServerIP();
                String strPort = MainActivity.getServerPort();
                int nPort = Integer.parseInt(strPort);
                InetSocketAddress serverAddr = new InetSocketAddress(strIP, Integer.valueOf(strPort));
                m_sock = new Socket();
                m_sock.connect(serverAddr, 5000);
                //m_sock = new Socket(strIP, Integer.valueOf(strPort));
                //m_sock.setSoTimeout(5000);
                //m_sock.setKeepAlive(true);
                m_sock.setTcpNoDelay(true);
                if (m_sock.isConnected()) {
                    m_handlerSend.sendEmptyMessage(R.string.MSG_TCP_Connected);
                } else {
                    m_handlerSend.sendEmptyMessage(R.string.MSG_TCP_Disconnected);
                    return;
                }
            }

            //m_bufferedReader = new BufferedReader(new InputStreamReader(m_sock.getInputStream(), "UTF-8"));
            m_inputStream = m_sock.getInputStream();
            m_outputStream = m_sock.getOutputStream();

            // 启动一条子线程来读取服务器响应的数据
            new Thread() {
                @Override
                public void run() {
                    String strContent = null;
                    int lenContent = 0;

                    while (true) {
                        byte[] bytesTemp = new byte[16];
                        try {
                            lenContent = m_inputStream.read(bytesTemp, 0, bytesTemp.length);
                            if (lenContent != -1) {
                                Message msg = Message.obtain();
                                if (lenContent > 0) {
                                    /* 这里需要将接收到的十六进制字节转成字符串 */
                                    strContent = UtilDigitalTrans.byte2HexStr(bytesTemp, lenContent, false);
                                    strContent.trim();
                                    /* 这里不能使用Toast等进行UI相关的操作，只能在主线程中进行，所以需要使用handler将消息发给主线程 */
                                    System.out.println(strContent);
                                    msg.what = R.string.MSG_TYPE_STRING;
                                    msg.obj = strContent;

                                    if (null != m_handlerReceive) {
                                        m_handlerReceive.sendMessage(msg);
                                    }
                                }
                            } else {
                                if (m_sock != null && m_sock.isConnected()) {
                                    Message msg = Message.obtain();
                                    msg.what = R.string.MSG_TCP_Disconnected;
                                    if (null != m_handlerReceive) {
                                        m_handlerReceive.sendMessage(msg);
                                    }
                                }
                                break;
                            }
                        } catch (IOException e) {
                            e.printStackTrace();
                            if (m_sock != null && m_sock.isConnected()) {
                                Message msg = Message.obtain();
                                msg.what = R.string.MSG_TCP_Disconnected;
                                if (null != m_handlerReceive) {
                                    m_handlerReceive.sendMessage(msg);
                                }
                            }
                            break;
                        }
                    }

                }
            }.start();

            /* 为当前线程初始化Looper */
            Looper.prepare();
            m_looper = Looper.myLooper();
            /* 处理来自子线程发送的数据 */
            m_handlerReceive = new Handler() {
                @Override
                public void handleMessage(Message msg) {
                    if (msg.what == R.string.MSG_TYPE_STRING) {
                        String strMessage = (String) msg.obj;
                        /* do something else */
                        //Toast.makeText(m_context, strMessage, Toast.LENGTH_SHORT).show();
                        if (strMessage.contains(m_context.getResources().getString(R.string.MSG_TYPE_WIFI_TEMPERATURE_HEAD))
                                && strMessage.length() >= 14) {
                            //Toast.makeText(m_context, strMessage.substring(10, 11), Toast.LENGTH_SHORT).show();
                            String sub1 = strMessage.substring(10, 12);
                            String sub2 = strMessage.substring(12, 14);
                            int high = Integer.valueOf(sub1, 16);
                            int low = Integer.valueOf(sub2, 16);
                            double temperature = (low + (high << 8)) * 0.0625;
                            BigDecimal bigDecimal = new BigDecimal(temperature);
                            bigDecimal = bigDecimal.setScale(2, RoundingMode.HALF_UP);

                            //Toast.makeText(m_context, String.valueOf(temperature), Toast.LENGTH_SHORT).show();
                            Message msgSend = Message.obtain();
                            msgSend.what = R.string.MSG_DATA_TEMPERATURE;
                            if (temperature >= 135.0 || temperature < -55.0)
                                msgSend.obj = "?";
                            else
                                msgSend.obj = bigDecimal.toPlainString();
                            m_handlerSend.sendMessage(msgSend);
                        }

                    } else if (msg.what == R.string.MSG_TCP_Disconnected) {
                        m_handlerSend.sendEmptyMessage(R.string.MSG_TCP_Disconnected);
                    }
                }
            };

            Looper.loop();
        } catch (IOException e) {
            m_handlerSend.sendEmptyMessage(R.string.MSG_TCP_Disconnected);
            e.printStackTrace();
        }
    }
}

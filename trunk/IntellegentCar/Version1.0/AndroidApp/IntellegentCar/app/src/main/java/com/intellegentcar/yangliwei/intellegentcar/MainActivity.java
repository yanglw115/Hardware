package com.intellegentcar.yangliwei.intellegentcar;

import android.app.Activity;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.RequiresApi;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.PopupWindow;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import com.suke.widget.SwitchButton;

import static android.view.Gravity.CENTER;
import static com.intellegentcar.yangliwei.intellegentcar.MainActivity.enumDirection.DIRECTION_STOP;

//public class MainActivity extends AppCompatActivity {
public class MainActivity extends Activity implements GestureDetector.OnGestureListener {
    enum enumDirection {
        DIRECTION_LEFT,
        DIRECTION_RIGHT,
        DIRECTION_UP,
        DIRECTION_DOWN,
        DIRECTION_STOP
    }

    ;

    com.suke.widget.SwitchButton buttonConnect = null, buttonControlMode = null;
    Button buttonForward = null, buttonBackward = null, buttonLeft = null, buttonRight = null;
    static EditText editTextServerIP;
    static EditText editTextServerPort;
    static TextView textSpeedValue, textTemperature;
    static SeekBar seekBarSpeed;
    //    static LinearLayout mainLayout;
    static enumDirection currentDirection = DIRECTION_STOP;

    TcpClient m_tcpClient = null;
    Thread m_threadTcp = null;
    static Handler m_handler = null;

    static private boolean s_controlByGuesture = false; // default false 手势控制

    GestureDetector gestureDetector;

    static String getServerIP() {
        return editTextServerIP.getText().toString();
    }

    static String getServerPort() {
        return editTextServerPort.getText().toString();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        buttonConnect = (com.suke.widget.SwitchButton) findViewById(R.id.idSwitchButtonConnect);
        buttonControlMode = (com.suke.widget.SwitchButton) findViewById(R.id.idSwitchControlMode);
        buttonForward = (Button) findViewById(R.id.idControlForward);
        buttonLeft = (Button) findViewById(R.id.idControlLeft);
        buttonRight = (Button) findViewById(R.id.idControlRight);
        buttonBackward = (Button) findViewById(R.id.idControlBackward);
        editTextServerIP = (EditText) findViewById(R.id.idTextWiFiIP);
        editTextServerPort = (EditText) findViewById(R.id.idTextWiFiPort);
        textSpeedValue = (TextView) findViewById(R.id.idSpeedValue);
        textTemperature = (TextView) findViewById(R.id.idTemperature);
        seekBarSpeed = (SeekBar) findViewById(R.id.idSeekBarSpeed);
//        mainLayout = (LinearLayout) findViewById(R.id.activity_main);

        final View noteView = MainActivity.this.getLayoutInflater().inflate(R.layout.popup_window_note_wifi_connect, null);
        final PopupWindow popupWindow = new PopupWindow(noteView, 600, 800);
        ImageView buttonNote = (ImageView) findViewById(R.id.idButtonNote);
        buttonNote.setOnClickListener(new View.OnClickListener() {
            @RequiresApi(api = Build.VERSION_CODES.KITKAT)
            @Override
            public void onClick(View v) {
                setBackGroundAlpha(0.3f);
                popupWindow.setFocusable(true);
                //popupWindow.setBackgroundDrawable(new ColorDrawable(0xffffff));
                popupWindow.showAtLocation((TextView) findViewById(R.id.idTextViewWiFiConnect), CENTER, 0, 0);
            }
        });

        noteView.findViewById(R.id.idPopMenuOK).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                setBackGroundAlpha(1f);
                popupWindow.dismiss();
            }
        });

        gestureDetector = new GestureDetector(this, this);

        initView();

        m_handler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                if (msg.what == R.string.MSG_TCP_Connected) {
                    setControlButtonEnable(true);
                    Toast.makeText(MainActivity.this, "Connect success!", Toast.LENGTH_SHORT).show();
                } else if (msg.what == R.string.MSG_TCP_Disconnected) {
                    setControlButtonEnable(false);
                    buttonConnect.setChecked(false);
                    Toast.makeText(MainActivity.this, "Connect failed!", Toast.LENGTH_SHORT).show();
                    m_tcpClient.stopRun();
                } else if (msg.what == R.string.MSG_DATA_TEMPERATURE) {
                        textTemperature.setText((String)msg.obj + "℃");
                }
            }
        };

        m_tcpClient = new TcpClient(m_handler, this);
        setButtonOnTouchListener();
    }

    void setBackGroundAlpha(float alpha)
    {
        WindowManager.LayoutParams lp = getWindow().getAttributes();
        lp.alpha = alpha;
        getWindow().setAttributes(lp);
    }

//    just for test
//    @Override
//    public boolean dispatchTouchEvent(MotionEvent ev) {
//        switch (ev.getAction()) {
//            case MotionEvent.ACTION_DOWN:
//                System.out.println("Activity---dispatchTouchEvent---DOWN");
//                break;
//            case MotionEvent.ACTION_MOVE:
//                System.out.println("Activity---dispatchTouchEvent---MOVE");
//                break;
//            case MotionEvent.ACTION_UP:
//                System.out.println("Activity---dispatchTouchEvent---UP");
//                break;
//            default:
//                break;
//        }
//
//        return super.dispatchTouchEvent(ev);
//    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        switch (event.getAction()) {
            /* 这里重写onTouchEvent是因为想用滑屏方式代替上下左右控件，但是滑屏方式实现后却发现ACTION_UP不容易捕获到，所以只能曲折地在这里去捕获
             * 其实应该有更好的方法去实现，比如重写LinearLayout的dispatchTouchEvent方法等。 */
            case MotionEvent.ACTION_UP:
                if (currentDirection != DIRECTION_STOP) {
                    sendCommandToCar(R.string.CMD_Motor_Stop);
                }
                break;
            default:
                break;
        }
        if (s_controlByGuesture) {
            return gestureDetector.onTouchEvent(event);
        }
        return super.onTouchEvent(event);
    }

    private void setControlButtonEnable(boolean bEnable) {
        if (!s_controlByGuesture) {
            buttonForward.setEnabled(bEnable);
            buttonLeft.setEnabled(bEnable);
            buttonRight.setEnabled(bEnable);
            buttonBackward.setEnabled(bEnable);
        } else {
            buttonForward.setEnabled(false);
            buttonLeft.setEnabled(false);
            buttonRight.setEnabled(false);
            buttonBackward.setEnabled(false);
        }
        /* 手势控制方向时可以调速 */
        if (s_controlByGuesture && !bEnable) {
            seekBarSpeed.setEnabled(true);
        } else {
            seekBarSpeed.setEnabled(bEnable);
        }
    }

    private void hideControlButton(boolean bHide) {
        int visible;
        if (bHide)
            visible = View.INVISIBLE;
        else
            visible = View.VISIBLE;
        buttonForward.setVisibility(visible);
        buttonLeft.setVisibility(visible);
        buttonRight.setVisibility(visible);
        buttonBackward.setVisibility(visible);
    }

    void sendCommandToCar(int cmd) {
        byte[] bytes = UtilDigitalTrans.HexCommandtoByte(getResources().getString(cmd).getBytes());
        if (m_tcpClient != null)
            m_tcpClient.sendWiFiSerialData(bytes);
    }

    void initView() {
        setControlButtonEnable(false);
        textSpeedValue.setText(String.valueOf(seekBarSpeed.getProgress()));

        if (s_controlByGuesture) {
            hideControlButton(true);
        }

        seekBarSpeed.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                textSpeedValue.setText(String.valueOf(progress));
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                //Toast.makeText(MainActivity.this, textSpeedValue.getText(), Toast.LENGTH_SHORT).show();
                byte[] bytes = UtilDigitalTrans.HexCommandtoByte(getResources().getString(R.string.CMD_Motor_Speed).getBytes());
                bytes[5] = (byte) seekBarSpeed.getProgress();
                for (int i = 0; i < bytes.length - 1; ++i) {
                    bytes[6] += bytes[i];
                }
                m_tcpClient.sendWiFiSerialData(bytes);
            }
        });
    }

    void setButtonOnTouchListener() {

        buttonConnect.setOnCheckedChangeListener(new SwitchButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(SwitchButton switchButton, boolean b) {
                if (b) {
                    if ((m_tcpClient.m_sock != null) && (m_tcpClient.m_sock.isConnected())) {
                        ; //
                    } else {
                        if (m_threadTcp != null) {
                            m_threadTcp = null;
                        }
                        m_threadTcp = new Thread(m_tcpClient);
                        m_threadTcp.start();
                    }
                } else {
                    setControlButtonEnable(false);
                    if ((m_tcpClient.m_sock != null) && (m_tcpClient.m_sock.isConnected())) {
                        m_tcpClient.stopRun();
                        m_threadTcp = null;
                    } else {
                        ; //
                    }
                }
            }
        });

        buttonControlMode.setOnCheckedChangeListener(new SwitchButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(SwitchButton switchButton, boolean b) {
                if (b) {
                    s_controlByGuesture = true;
                    setControlButtonEnable(false);
                    hideControlButton(true);
                } else {
                    s_controlByGuesture = false;
                    hideControlButton(false);
                    if ((m_tcpClient.m_sock != null) && (m_tcpClient.m_sock.isConnected()))
                        setControlButtonEnable(true);
                    else
                        setControlButtonEnable(false);
                }
            }
        });

        /* 1、这里原本想使用mainLayout去处理ACTION_UP事件，因为在onScroll中并不能够得到这个事件；
         * 2、但是mainLayout要响应onTouch事件，必须在xml中设置clickAble为true；设置为true之后MainActivity的onTouchEvent却得不到执行，因此二者冲突；
         * 3、所以只能让mainLayout的clickAble属性为false，然后通过MainActivity的dispatchTouchEvent或onTouchEvent中去获取ACTION_UP事件；*/
//        mainLayout.setOnTouchListener(new View.OnTouchListener() {
//            @Override
//            public boolean onTouch(View v, MotionEvent event) {
//                switch (event.getAction()) {
//                    case MotionEvent.ACTION_UP:
//                        m_tcpClient.sendWiFiSerialData("stop");
//                        break;
//                }
//                return false;
//            }
//        });

        /* 说明：这里目前只能一个个地设置view的setOnTouchListener，无法放在上面屏蔽掉的代码中通过v.getId()去判断，因为mainLayout的
         * setOnTouchListener只针对空白区域才能得到回调 */

        buttonForward.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (s_controlByGuesture)
                    return false;
                switch (v.getId()) {
                    case R.id.idControlForward:
                        switch (event.getAction()) {
                            case MotionEvent.ACTION_DOWN:
                                sendCommandToCar(R.string.CMD_Motor_Forward);
                                break;
                            case MotionEvent.ACTION_UP:
                                sendCommandToCar(R.string.CMD_Motor_Stop);
                                break;
                        }
                        break;
                }
                return false;
            }
        });

        buttonLeft.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (s_controlByGuesture)
                    return false;
                switch (v.getId()) {
                    case R.id.idControlLeft:
                        switch (event.getAction()) {
                            case MotionEvent.ACTION_DOWN:
                                sendCommandToCar(R.string.CMD_Motor_Left);
                                break;
                            case MotionEvent.ACTION_UP:
                                sendCommandToCar(R.string.CMD_Motor_Stop);
                                break;
                        }
                        break;
                }
                return false;
            }
        });


        buttonRight.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (s_controlByGuesture) {
                    return false;
                }
                switch (v.getId()) {
                    case R.id.idControlRight:
                        switch (event.getAction()) {
                            case MotionEvent.ACTION_DOWN:
                                sendCommandToCar(R.string.CMD_Motor_Right);
                                break;
                            case MotionEvent.ACTION_UP:
                                sendCommandToCar(R.string.CMD_Motor_Stop);
                                break;
                        }
                        break;
                }
                return false;
            }
        });


        buttonBackward.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (s_controlByGuesture)
                    return false;
                switch (v.getId()) {
                    case R.id.idControlBackward:
                        switch (event.getAction()) {
                            case MotionEvent.ACTION_DOWN:
                                sendCommandToCar(R.string.CMD_Motor_Backward);
                                break;
                            case MotionEvent.ACTION_UP:
                                sendCommandToCar(R.string.CMD_Motor_Stop);
                                break;
                        }
                        break;
                }
                return false;
            }
        });

    }

    @Override
    public boolean onDown(MotionEvent e) {
        return false;
    }

    @Override
    public void onShowPress(MotionEvent e) {

    }

    @Override
    public boolean onSingleTapUp(MotionEvent e) {
        //m_tcpClient.sendWiFiSerialData("stop");
        return false;
    }

    @Override
    public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
        int minDistance = 20;
        /* 1、这里的e1与e2是move停止之后的位置，具体上一次move是多少需要记录；
        *  2、distanceX和distanceY是这次move的x与y方向的大小，可能是正，也可能是负，是上一次的值减去当前值得到的结果
        */
        boolean bHorizontalMove = (Math.abs(distanceX) > Math.abs(distanceY));
        enumDirection moveDirection;

        if (Math.abs(distanceX) < minDistance && Math.abs(distanceY) < minDistance) {
            return false;
        }

        if (bHorizontalMove) {
            moveDirection = (distanceX > 0 ? enumDirection.DIRECTION_LEFT : enumDirection.DIRECTION_RIGHT);
        } else {
            moveDirection = (distanceY > 0 ? enumDirection.DIRECTION_UP : enumDirection.DIRECTION_DOWN);
        }

        // 对于ACTION_UP时才stop，但是ACTION_UP只能在MainActivity的dispatchTouchEvent或onTouchEvent中才能捕获到，所以这里设置一下标志
        currentDirection = moveDirection;

        switch (moveDirection) {
            case DIRECTION_LEFT:
                sendCommandToCar(R.string.CMD_Motor_Left);
                break;
            case DIRECTION_RIGHT:
                sendCommandToCar(R.string.CMD_Motor_Right);
                break;
            case DIRECTION_UP:
                sendCommandToCar(R.string.CMD_Motor_Forward);
                break;
            case DIRECTION_DOWN:
                sendCommandToCar(R.string.CMD_Motor_Backward);
                break;
            default:
                break;

        }
        return false;
    }

    @Override
    public void onLongPress(MotionEvent e) {

    }

    @Override
    public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {

        return false;
    }
}

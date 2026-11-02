package ru.citylive.launcher;

import android.app.Activity;
import android.content.*;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.view.*;
import android.widget.*;
import java.util.concurrent.*;

public final class MainActivity extends Activity {
    private static final int WHITE=Color.rgb(239,241,246), MUTED=Color.rgb(158,166,186), RED=Color.rgb(239,89,77);
    private final ExecutorService worker=Executors.newSingleThreadExecutor();
    private EditText endpoint,nickname;
    private TextView status;
    private Button refresh;
    private String checkedEndpoint="";
    private int querySequence=0;
    private int dp(int value){return Math.round(value*getResources().getDisplayMetrics().density);}
    private GradientDrawable background(int color,int radius){GradientDrawable d=new GradientDrawable();d.setColor(color);d.setCornerRadius(dp(radius));return d;}
    private TextView label(String value,int size,int color){TextView v=new TextView(this);v.setText(value);v.setTextColor(color);v.setTextSize(size);v.setPadding(0,dp(8),0,dp(8));return v;}
    private EditText field(String hint,String value){EditText v=new EditText(this);v.setSingleLine(true);v.setTextColor(WHITE);v.setHintTextColor(MUTED);v.setHint(hint);v.setText(value);v.setTextSize(17);v.setPadding(dp(16),dp(10),dp(16),dp(10));v.setBackground(background(0xff202534,12));v.setInputType(android.text.InputType.TYPE_CLASS_TEXT|android.text.InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);return v;}
    private Button button(String text,Runnable action){Button b=new Button(this);b.setText(text);b.setAllCaps(false);b.setTextColor(WHITE);b.setBackgroundTintList(android.content.res.ColorStateList.valueOf(RED));b.setOnClickListener(v->action.run());return b;}
    @Override public void onCreate(Bundle state){
        super.onCreate(state);
        ScrollView scroll=new ScrollView(this);scroll.setFillViewport(true);scroll.setBackgroundColor(0xff090b10);
        LinearLayout page=new LinearLayout(this);page.setOrientation(LinearLayout.VERTICAL);page.setPadding(dp(24),dp(24),dp(24),dp(24));scroll.addView(page);
        // Apply Android 15 system-bar insets when targeting API 35.
        page.setOnApplyWindowInsetsListener((view,insets)->{int left,top,right,bottom;if(android.os.Build.VERSION.SDK_INT>=30){android.graphics.Insets bars=insets.getInsets(WindowInsets.Type.systemBars());left=bars.left;top=bars.top;right=bars.right;bottom=bars.bottom;}else{left=insets.getSystemWindowInsetLeft();top=insets.getSystemWindowInsetTop();right=insets.getSystemWindowInsetRight();bottom=insets.getSystemWindowInsetBottom();}view.setPadding(dp(24)+left,dp(24)+top,dp(24)+right,dp(24)+bottom);return insets;});
        TextView title=label("CITYLIVE RP",32,WHITE);title.setTypeface(null,Typeface.BOLD);page.addView(title);
        page.addView(label("Лаунчер сервера",16,RED));
        page.addView(label("Нужен отдельно установленный совместимый SA-MP-клиент. Это приложение проверяет сервер и передаёт адрес клиенту.",15,MUTED));
        android.content.SharedPreferences prefs=getSharedPreferences("launcher",MODE_PRIVATE);
        page.addView(label("Адрес сервера",14,MUTED));endpoint=field("IP:порт",prefs.getString("endpoint","5.181.187.57:7811"));page.addView(endpoint);
        page.addView(label("Имя персонажа",14,MUTED));nickname=field("First_Last",prefs.getString("nickname",""));page.addView(nickname);
        status=label("Нажмите «Проверить сервер»",17,WHITE);status.setPadding(dp(16),dp(18),dp(16),dp(18));status.setBackground(background(0xff141924,16));
        LinearLayout.LayoutParams card=new LinearLayout.LayoutParams(-1,-2);card.topMargin=dp(24);card.bottomMargin=dp(16);page.addView(status,card);
        refresh=button("Проверить сервер",this::query);page.addView(refresh);
        page.addView(button("Открыть в игровом клиенте",this::play));
        page.addView(button("Скопировать адрес",()->{String value=endpoint.getText().toString().trim();((ClipboardManager)getSystemService(CLIPBOARD_SERVICE)).setPrimaryClip(ClipData.newPlainText("CityLive RP",value));Toast.makeText(this,"Адрес скопирован",Toast.LENGTH_SHORT).show();}));
        page.addView(label("Если клиент не принимает ссылку samp://, добавьте адрес и ник вручную в его списке серверов. Пароль персонажа вводится только после подключения к серверу.",14,MUTED));
        setContentView(scroll);query();
    }
    private String[] address(){String text=endpoint.getText().toString().trim();String[] a=text.split(":",-1);try{if(a.length!=2||!a[0].matches("[A-Za-z0-9.-]{1,253}")||Integer.parseInt(a[1])<1||Integer.parseInt(a[1])>65535)throw new IllegalArgumentException();}catch(Exception e){Toast.makeText(this,"Введите адрес в формате IP:порт",Toast.LENGTH_LONG).show();return null;}return a;}
    private void persist(){getSharedPreferences("launcher",MODE_PRIVATE).edit().putString("endpoint",endpoint.getText().toString().trim()).putString("nickname",nickname.getText().toString().trim()).apply();}
    private void query(){
        String[] a=address();if(a==null)return;persist();int sequence=++querySequence;checkedEndpoint=a[0]+":"+a[1];refresh.setEnabled(false);status.setText(getString(R.string.checking_server,checkedEndpoint));
        worker.execute(()->{String result;try{ServerInfo info=ServerInfo.query(a[0],Integer.parseInt(a[1]));result=info.hostname+"\nИгроки: "+info.players+" / "+info.slots+"\n"+info.mode;}catch(Exception e){result="Сервер не ответил на запрос за 3 секунды. Проверьте адрес и запуск в панели. Запрос мониторинга также может быть отключён.";}String text=result;runOnUiThread(()->{if(isFinishing()||isDestroyed()||sequence!=querySequence)return;status.setText(text);refresh.setEnabled(true);});});
    }
    private void play(){
        String[] a=address();if(a==null)return;String name=nickname.getText().toString().trim();
        if(!name.matches("[A-Za-z]{2,20}_[A-Za-z]{2,20}")){Toast.makeText(this,"Ник должен быть вида First_Last",Toast.LENGTH_LONG).show();return;}
        persist();Uri uri=new Uri.Builder().scheme("samp").encodedAuthority(a[0]+":"+a[1]).appendQueryParameter("nickname",name).build();
        try{startActivity(new Intent(Intent.ACTION_VIEW,uri));}catch(ActivityNotFoundException e){new android.app.AlertDialog.Builder(this).setTitle("Игровой клиент не найден").setMessage("Установите совместимый клиент с правомерно полученными файлами GTA SA. Затем добавьте сервер "+a[0]+":"+a[1]+" и ник "+name+" в его настройках. Лаунчер не содержит игрового движка или GTA-файлов.").setPositiveButton("Понятно",null).show();}
    }
    @Override protected void onDestroy(){++querySequence;worker.shutdownNow();super.onDestroy();}
}

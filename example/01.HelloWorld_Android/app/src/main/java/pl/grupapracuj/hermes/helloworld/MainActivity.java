package pl.grupapracuj.hermes.helloworld;

import android.content.Context;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.text.method.ScrollingMovementMethod;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Arrays;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("main");
    }

    Button mButtonExecuteJNI = null;
    Button mButtonClear = null;
    TextView mTextOutput = null;
    String mCertificateFilename = "certificate.pem";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);

		mTextOutput = (TextView) findViewById(R.id.tv_result);
        mButtonExecuteJNI = (Button) findViewById(R.id.btn_call_jni);
        mButtonClear = (Button) findViewById(R.id.btn_clear_results);

        mButtonClear.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mTextOutput.scrollTo(0, 0);
                mTextOutput.setText("");
            }
        });

        mButtonExecuteJNI.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                executeHelloWorld();
            }
        });

        mTextOutput.setMovementMethod(new ScrollingMovementMethod());

        String certificatePath = copyData(mCertificateFilename);
        createHelloWorld(certificatePath);
    }

    @Override
    protected void onDestroy() {
        destroyHelloWorld();
        super.onDestroy();
    }

    public void addText(String pText) {
        if (isFinishing())
            return;

        StringBuilder sb = new StringBuilder(mTextOutput.getText().toString());

        if (sb.length() > 0) {
            sb.append("\n");
        }

        sb.append(pText);

        mTextOutput.setText(sb.toString());
    }

    public native void createHelloWorld(String pCertificatePath);
    public native void destroyHelloWorld();
    public native void executeHelloWorld();

    private String copyData(String pFilename)
    {
        String result = "";

        Context context = getApplicationContext();
        boolean fileExist = false;

        try {
            fileExist = context != null && Arrays.asList(context.getResources().getAssets().list("")).contains(pFilename);
        } catch (IOException ignore) {
        }

        if (fileExist) {
            String destinationDirectory = context.getApplicationInfo().dataDir;
            String destinationPath = destinationDirectory + "/" + pFilename;
            AssetManager assetManager = context.getAssets();
            InputStream inputStream = null;
            OutputStream outputStream = null;

            boolean fileNotExist = true;
            File certificateFile = new File(destinationPath);

            if (certificateFile.exists()) {
                fileNotExist = certificateFile.delete();
            }

            if (fileNotExist) {
                try {
                    inputStream = assetManager.open(pFilename);
                    fileExist = certificateFile.createNewFile();

                    if (fileExist) {
                        outputStream = new FileOutputStream(destinationPath);

                        byte[] buffer = new byte[1024];
                        int length;

                        while ((length = inputStream.read(buffer)) > 0) {
                            outputStream.write(buffer, 0, length);
                        }

                        outputStream.flush();
                        result = destinationPath;
                    }
                } catch (IOException ignore) {
                } finally {
                    try {
                        if (outputStream != null) {
                            outputStream.close();
                        }

                        if (inputStream != null) {
                            inputStream.close();
                        }
                    } catch (IOException ignore) {
                    }
                }
            }
        }

        return result;
    }
}

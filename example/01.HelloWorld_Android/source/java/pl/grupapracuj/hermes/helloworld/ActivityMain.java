// Copyright (C) 2017-2020 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

package pl.grupapracuj.hermes.helloworld;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Arrays;

import pl.grupapracuj.hermes.ext.jni.ObjectNative;

public class ActivityMain extends Activity {
    static {
        System.loadLibrary("helloworld");
    }

    private TextView mTextView;
    private TextView mButton;
    private ObjectNative mNativeHelloWorld;
    private int mRequestIndex = 1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(null);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        buildLayout();

        mNativeHelloWorld = nativeCreate(getClassLoader(), assetCopy("certificate.pem"));
        mButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View lpView) {
                mButton.setEnabled(false);
                mTextView.setText("");
                nativeExecute(mNativeHelloWorld.pointer(), mRequestIndex);

                mRequestIndex++;
                if (mRequestIndex > 7)
                    mRequestIndex = 1;
            }
        });
    }

    @Override
    protected void onDestroy() {
        if (mNativeHelloWorld != null)
            mNativeHelloWorld.destroy();

        super.onDestroy();
    }

    private void callbackExecute(String pOutputText) {
        mButton.setEnabled(true);
        mTextView.setText(pOutputText);
    }

    private void buildLayout() {
        DisplayMetrics displayMetrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        final int blockSize = displayMetrics.widthPixels / 10;
        final int orange = Color.rgb(255, 130, 28);

        FrameLayout layout = new FrameLayout(this);
        layout.setBackgroundColor(Color.WHITE);
        layout.setLayoutParams(new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT, Gravity.CENTER));

        mTextView = new TextView(this);
        mTextView.setBackgroundColor(orange);
        mTextView.setGravity(Gravity.START | Gravity.CENTER_VERTICAL);
        mTextView.setTextSize(TypedValue.COMPLEX_UNIT_PX, blockSize * 0.6f);
        mTextView.setTextColor(Color.WHITE);
        mTextView.setText("");
        layout.addView(mTextView);
        {
            FrameLayout.LayoutParams layoutParams = new FrameLayout.LayoutParams(displayMetrics.widthPixels - blockSize * 2, blockSize * 3);
            layoutParams.leftMargin = blockSize;
            layoutParams.topMargin = blockSize;
            mTextView.setLayoutParams(layoutParams);
        }

        mButton = new Button(this);
        mButton.setBackgroundColor(orange);
        mButton.setGravity(Gravity.CENTER);
        mButton.setTextSize(TypedValue.COMPLEX_UNIT_PX, blockSize);
        mButton.setTextColor(Color.WHITE);
        layout.addView(mButton);
        {
            final String displayName = "EXECUTE";
            Rect bounds = new Rect();
            mButton.getPaint().getTextBounds(displayName, 0, displayName.length(), bounds);
            mButton.setPadding(0, (int)(mButton.getPaint().getFontMetrics().top - bounds.top - 0.5f) + blockSize / 4, 0, 0);
            mButton.setText(displayName);

            FrameLayout.LayoutParams layoutParams = new FrameLayout.LayoutParams(displayMetrics.widthPixels - blockSize * 2, (int)(blockSize * 1.5f));
            layoutParams.leftMargin = blockSize;
            layoutParams.topMargin = blockSize * 5;
            mButton.setLayoutParams(layoutParams);
        }

        setContentView(layout);
    }

    private String assetCopy(String pFilename)
    {
        String result = "";
        boolean fileExist = false;

        try {
            final String[] list = getResources().getAssets().list("");
            fileExist = list != null && Arrays.asList(list).contains(pFilename);
        } catch (IOException ignore) {}

        if (fileExist) {
            String destinationDirectory = getApplicationInfo().dataDir;
            String destinationPath = destinationDirectory + "/" + pFilename;
            InputStream inputStream = null;
            OutputStream outputStream = null;

            boolean fileNotExist = true;
            File certificateFile = new File(destinationPath);

            if (certificateFile.exists()) {
                fileNotExist = certificateFile.delete();
            }

            if (fileNotExist) {
                try {
                    inputStream = getAssets().open(pFilename);
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
                    } catch (IOException ignore) {}
                }
            }
        }

        return result;
    }

    private native ObjectNative nativeCreate(ClassLoader pClassLoader, String pCertificatePath);
    private native void nativeExecute(long pPointer, int pRequestIndex);
}

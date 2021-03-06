// Copyright (C) 2017-2021 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

package pl.grupapracuj.hermes.helloworld;

import android.app.Activity;
import android.content.res.AssetManager;
import android.graphics.Color;
import android.graphics.Insets;
import android.graphics.Rect;
import android.os.Build;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowMetrics;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.TextView;

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

        mNativeHelloWorld = nativeCreate(getClassLoader(), getAssets());
        mButton.setOnClickListener(lpView -> {
            mButton.setEnabled(false);
            mTextView.setText("");
            nativeExecute(mNativeHelloWorld.pointer(), mRequestIndex);

            mRequestIndex++;
            if (mRequestIndex > 7)
                mRequestIndex = 1;
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
        int width;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R)
        {
            WindowMetrics windowMetrics = getWindowManager().getCurrentWindowMetrics();
            Insets insets = windowMetrics.getWindowInsets().getInsetsIgnoringVisibility(WindowInsets.Type.systemBars());
            width = windowMetrics.getBounds().width() - insets.left - insets.right;
        }
        else
        {
            DisplayMetrics displayMetrics = new DisplayMetrics();
            getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
            width = displayMetrics.widthPixels;
        }

        final int blockSize = width / 10;
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
            FrameLayout.LayoutParams layoutParams = new FrameLayout.LayoutParams(width - blockSize * 2, blockSize * 3);
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

            FrameLayout.LayoutParams layoutParams = new FrameLayout.LayoutParams(width - blockSize * 2, (int)(blockSize * 1.5f));
            layoutParams.leftMargin = blockSize;
            layoutParams.topMargin = blockSize * 5;
            mButton.setLayoutParams(layoutParams);
        }

        setContentView(layout);
    }

    private native ObjectNative nativeCreate(ClassLoader pClassLoader, AssetManager pAssetManager);
    private native void nativeExecute(long pPointer, int pRequestIndex);
}

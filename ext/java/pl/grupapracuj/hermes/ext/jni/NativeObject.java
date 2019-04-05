// Copyright (C) 2017-2019 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

package pl.grupapracuj.hermes.ext.jni;

public class NativeObject {
    private long mPointer = 0;

    public NativeObject() {
    }

    public void destroy() {
        nativeDestroy(mPointer);
        mPointer = 0;
    }

    public long getPointer() {
        return mPointer;
    }

    private native void nativeDestroy(long pPointer);
}

// Copyright (C) 2017-2021 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

package pl.grupapracuj.hermes.ext.jni;

public class ObjectNative {
    private boolean mDestroy = true;
    private long mPointer = 0;

    private ObjectNative() {
    }

    public void destroy() {
        if (mDestroy) {
            nativeDestroy(mPointer);
            mPointer = 0;
            mDestroy = false;
        }
    }

    public long pointer() {
        return mPointer;
    }

    private native void nativeDestroy(long pPointer);
}

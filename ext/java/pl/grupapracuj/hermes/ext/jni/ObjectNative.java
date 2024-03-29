// Copyright (C) 2017-2023 Grupa Pracuj S.A.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

package pl.grupapracuj.hermes.ext.jni;

public class ObjectNative {
    private boolean mInvalid = false;
    private long mPointer = 0;

    private ObjectNative() {
    }

    public void destroy() {
        if (!mInvalid) {
            nativeDestroy(mPointer);
            mPointer = 0;
            mInvalid = true;
        }
    }

    public long pointer() {
        if (mInvalid)
            throw new IllegalStateException("hmsJNI: pointer was invalidated");

        return mPointer;
    }

    public boolean valid() {
        return !mInvalid;
    }

    private native void nativeDestroy(long pPointer);
}

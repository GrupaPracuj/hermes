// Copyright (C) 2017-2020 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

package pl.grupapracuj.hermes.ext.jni;

public class ObjectNative {
    private long mPointer = 0;

    public ObjectNative() {
    }

    public void destroy() {
        nativeDestroy(mPointer);
        mPointer = 0;
    }

    public long pointer() {
        return mPointer;
    }

    private native void nativeDestroy(long pPointer);
}

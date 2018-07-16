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

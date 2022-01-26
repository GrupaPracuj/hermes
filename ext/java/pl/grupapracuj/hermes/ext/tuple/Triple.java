// Copyright (C) 2017-2022 Grupa Pracuj S.A.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

package pl.grupapracuj.hermes.ext.tuple;

public class Triple<A, B, C> {
    private A mFirst;
    private B mSecond;
    private C mThird;

    public Triple(A pFirst, B pSecond, C pThird) {
        mFirst = pFirst;
        mSecond = pSecond;
        mThird = pThird;
    }

    public A first() {
        return mFirst;
    }

    public void first(A pFirst) {
        mFirst = pFirst;
    }

    public B second() {
        return mSecond;
    }

    public void second(B pSecond) {
        mSecond = pSecond;
    }

    public C third() {
        return mThird;
    }

    public void third(C pThird) {
        mThird = pThird;
    }
}

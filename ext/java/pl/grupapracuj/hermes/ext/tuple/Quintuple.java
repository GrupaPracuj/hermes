// Copyright (C) 2017-2020 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

package pl.grupapracuj.hermes.ext.tuple;

public class Quintuple<A, B, C, D, E> {
    private A mFirst;
    private B mSecond;
    private C mThird;
    private D mFourth;
    private E mFifth;

    public Quintuple(A pFirst, B pSecond, C pThird, D pFourth, E pFifth) {
        mFirst = pFirst;
        mSecond = pSecond;
        mThird = pThird;
        mFourth = pFourth;
        mFifth = pFifth;
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

    public D fourth() {
        return mFourth;
    }

    public void fourth(D pFourth) {
        mFourth = pFourth;
    }

    public E fifth() {
        return mFifth;
    }

    public void fifth(E pFifth) {
        mFifth = pFifth;
    }
}

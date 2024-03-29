// Copyright (C) 2017-2023 Grupa Pracuj S.A.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

package pl.grupapracuj.hermes.ext.tuple;

public class Quadruple<A, B, C, D> {
    private A mFirst;
    private B mSecond;
    private C mThird;
    private D mFourth;

    public Quadruple(A pFirst, B pSecond, C pThird, D pFourth) {
        mFirst = pFirst;
        mSecond = pSecond;
        mThird = pThird;
        mFourth = pFourth;
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
}

// Copyright (C) 2017-2023 Grupa Pracuj S.A.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

package pl.grupapracuj.hermes.ext.tuple;

public class Nontuple<A, B, C, D, E, F, G, H, I> {
    private A mFirst;
    private B mSecond;
    private C mThird;
    private D mFourth;
    private E mFifth;
    private F mSixth;
    private G mSeventh;
    private H mEighth;
    private I mNinth;

    public Nontuple(A pFirst, B pSecond, C pThird, D pFourth, E pFifth, F pSixth, G pSeventh, H pEighth, I pNinth) {
        mFirst = pFirst;
        mSecond = pSecond;
        mThird = pThird;
        mFourth = pFourth;
        mFifth = pFifth;
        mSixth = pSixth;
        mSeventh = pSeventh;
        mEighth = pEighth;
        mNinth = pNinth;
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

    public F sixth() {
        return mSixth;
    }

    public void sixth(F pSixth) {
        mSixth = pSixth;
    }

    public G seventh() {
        return mSeventh;
    }

    public void seventh(G pSeventh) {
        mSeventh = pSeventh;
    }

    public H eighth() {
        return mEighth;
    }

    public void eighth(H pEighth) {
        mEighth = pEighth;
    }

    public I ninth() {
        return mNinth;
    }

    public void ninth(I pNinth) {
        mNinth = pNinth;
    }
}

// Copyright (C) 2017-2020 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

package pl.grupapracuj.hermes.ext.tuple;

public class Pair<A, B> {
    private A mFirst;
    private B mSecond;

    public Pair(A pFirst, B pSecond) {
        mFirst = pFirst;
        mSecond = pSecond;
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
}

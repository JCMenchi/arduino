/***
 * Excerpted from "Programming Your Home",
 * published by The Pragmatic Bookshelf.
 * Copyrights apply to this code. It may not be used to create training material, 
 * courses, books, articles, and the like. Contact us if you are in doubt.
 * We make no guarantees that this code is fit for any purpose. 
 * Visit http://www.pragmaticprogrammer.com/titles/mrhome for more book information.
***/
package com.mysampleapp.androiddoorlockserver;

public class LockStatus {

    public boolean getLockStatus() {
        return myLockStatus;
    }

    public void setMyVar(boolean myLockStatus) {
        this.myLockStatus = myLockStatus;
    }

    private boolean myLockStatus = false;
    private static LockStatus instance;

    static {
        instance = new LockStatus();
    }

    private LockStatus() {
    }

    public static LockStatus getInstance() {
        return LockStatus.instance;
    }

}
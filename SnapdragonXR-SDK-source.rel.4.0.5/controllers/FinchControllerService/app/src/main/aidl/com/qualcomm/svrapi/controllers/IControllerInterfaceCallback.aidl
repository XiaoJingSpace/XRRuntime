// IControllerInterfaceCallback.aidl
package com.qualcomm.svrapi.controllers;

interface IControllerInterfaceCallback{
    oneway void onStateChanged(int handle, int what, int arg1, int arg2);
}


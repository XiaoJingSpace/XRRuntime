// IControllerInterface.aidl
package com.qualcomm.svrapi.controllers;
import com.qualcomm.svrapi.controllers.IControllerInterfaceCallback;

// Declare any non-default types here with import statements

interface IControllerInterface {
    void registerCallback(IControllerInterfaceCallback callback);
    void unregisterCallback();
    void Start(in int handle, in String desc, in ParcelFileDescriptor pfd, in int fdSize, in ParcelFileDescriptor qvrFd, int qvrFdSize);
    void Stop(in int handle);
    void SendMessage(in int handle, in int type, in int arg1, in int arg2);
    int  QueryInt(in int handle, in int type);
    String QueryString(in int handle, in int type);

}

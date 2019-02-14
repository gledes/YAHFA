//
// Created by lrk on 5/4/17.
//
#include <jni.h>

extern "C"
JNIEXPORT jstring JNICALL
Java_lab_galaxy_yahfa_demoApp_ClassWithJNIMethod_fromJNI(JNIEnv *env, jclass clazz) {
    return env->NewStringUTF("hello from JNI");
}
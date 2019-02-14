#include "jni.h"
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>

#include "common.h"
#include "env.h"
#include "trampoline.h"

int SDKVersion;
//static int OFFSET_entry_point_from_interpreter_in_ArtMethod;
int OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod;
//static int OFFSET_dex_method_index_in_ArtMethod;
//static int OFFSET_dex_cache_resolved_methods_in_ArtMethod;
//static int OFFSET_array_in_PointerArray;
//static int OFFSET_ArtMehod_in_Object;
static int OFFSET_access_flags_in_ArtMethod;
static size_t ArtMethodSize;
static int kAccNative = 0x0100;
static int kAccCompileDontBother = 0x02000000;
//static size_t kDexCacheMethodCacheSize = 1024;


static inline uint32_t read32(void *addr) {
    return *((uint32_t *) addr);
}

static inline uint64_t read64(void *addr) {
    return *((uint64_t *) addr);
}


static void setNonCompilable(void *method) {
    int access_flags = read32((char *) method + OFFSET_access_flags_in_ArtMethod);
    LOGI("setNonCompilable: access flags is 0x%x", access_flags);
//    int value = read32(&access_flags);
    access_flags |= kAccCompileDontBother;
//    value = read32(&access_flags);
    memcpy(
            (char *) method + OFFSET_access_flags_in_ArtMethod,
            &access_flags,
            4
    );
}

static int doBackupAndHook(void *targetMethod, void *hookMethod, void *backupMethod) {

    if (hookCount >= hookCap) {
        LOGW("not enough capacity. Allocating...");
        if (doInitHookCap(DEFAULT_CAP)) {
            LOGE("cannot hook method");
            return 1;
        }
        LOGI("Allocating done");
    }

//    setNonCompilable(targetMethod);
//    setNonCompilable(hookMethod);

    if (backupMethod) {
        memcpy(backupMethod, targetMethod, ArtMethodSize);
    }

    void *newEntrypoint = genTrampoline(hookMethod);
    memcpy((char *) targetMethod + OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod,
           &newEntrypoint,
           pointer_size);

    int access_flags = read32((char *) targetMethod + OFFSET_access_flags_in_ArtMethod);
    access_flags |= kAccNative;
    memcpy(
            (char *) targetMethod + OFFSET_access_flags_in_ArtMethod,
            &access_flags,
            4
    );
    LOGI("access flags is 0x%x", access_flags);

    LOGI("hook and backup done");
    hookCount += 1;
    return 0;
}

void Java_lab_galaxy_yahfa_HookMain_init(JNIEnv *env, jclass clazz, jint sdkVersion) {
    SDKVersion = sdkVersion;
    LOGI("init to SDK %d", sdkVersion);
    kAccCompileDontBother = 0x02000000;
//    OFFSET_ArtMehod_in_Object = 0;
    OFFSET_access_flags_in_ArtMethod = 4;
    //OFFSET_dex_method_index_in_ArtMethod = 4*3;
    OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod =
            roundUpToPtrSize(4 * 4 + 2 * 2) + pointer_size;
    ArtMethodSize = roundUpToPtrSize(4 * 4 + 2 * 2) + pointer_size * 2;

    setupTrampoline();
}

jobject Java_lab_galaxy_yahfa_HookMain_findMethodNative(JNIEnv *env, jclass clazz,
                                                        jclass targetClass, jstring methodName,
                                                        jstring methodSig) {
    const char *c_methodName = (*env)->GetStringUTFChars(env, methodName, NULL);
    const char *c_methodSig = (*env)->GetStringUTFChars(env, methodSig, NULL);
    jobject ret = NULL;


    //Try both GetMethodID and GetStaticMethodID -- Whatever works :)
    jmethodID method = (*env)->GetMethodID(env, targetClass, c_methodName, c_methodSig);
    if (!(*env)->ExceptionCheck(env)) {
        ret = (*env)->ToReflectedMethod(env, targetClass, method, JNI_FALSE);
    } else {
        (*env)->ExceptionClear(env);
        method = (*env)->GetStaticMethodID(env, targetClass, c_methodName, c_methodSig);
        if (!(*env)->ExceptionCheck(env)) {
            ret = (*env)->ToReflectedMethod(env, targetClass, method, JNI_TRUE);
        }
    }

    (*env)->ReleaseStringUTFChars(env, methodName, c_methodName);
    (*env)->ReleaseStringUTFChars(env, methodSig, c_methodSig);
    return ret;
}


jboolean Java_lab_galaxy_yahfa_HookMain_backupAndHookNative(JNIEnv *env, jclass clazz,
                                                            jobject target, jobject hook,
                                                            jobject backup) {

    if (!doBackupAndHook(
            (void *) (*env)->FromReflectedMethod(env, target),
            (void *) (*env)->FromReflectedMethod(env, hook),
            backup == NULL ? NULL : (void *) (*env)->FromReflectedMethod(env, backup)
    )) {
        (*env)->NewGlobalRef(env,
                             hook); // keep a global ref so that the hook method would not be GCed
        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}

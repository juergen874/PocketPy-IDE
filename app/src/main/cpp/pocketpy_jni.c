#include <jni.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <android/log.h>
#include "pocketpy.h"

#define TAG "PocketPyJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static JavaVM* g_vm = NULL;
static jobject g_current_callback = NULL;
static jmethodID g_mid_on_output = NULL;
static jmethodID g_mid_on_error = NULL;

static void jni_print_callback(const char* text) {
    if (!text || !g_vm || !g_current_callback || !g_mid_on_output) return;

    JNIEnv* env = NULL;
    int env_status = (*g_vm)->GetEnv(g_vm, (void**)&env, JNI_VERSION_1_6);
    bool should_detach = false;

    if (env_status == JNI_EDETACHED) {
        if ((*g_vm)->AttachCurrentThread(g_vm, (void**)&env, NULL) != 0) {
            return;
        }
        should_detach = true;
    }

    jstring jstr = (*env)->NewStringUTF(env, text);
    if (jstr) {
        (*env)->CallVoidMethod(env, g_current_callback, g_mid_on_output, jstr);
        (*env)->DeleteLocalRef(env, jstr);
    }

    if (should_detach) {
        (*g_vm)->DetachCurrentThread(g_vm);
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_vm = vm;
    return JNI_VERSION_1_6;
}

JNIEXPORT jboolean JNICALL
Java_com_pocketpy_ide_engine_PocketPyEngine_nativeInit(JNIEnv *env, jobject thiz) {
    return JNI_TRUE;
}

JNIEXPORT jobject JNICALL
Java_com_pocketpy_ide_engine_PocketPyEngine_nativeExecute(
    JNIEnv *env,
    jobject thiz,
    jstring jcode,
    jstring jfilename,
    jobject jcallback
) {
    if (!jcode) return NULL;

    const char* code = (*env)->GetStringUTFChars(env, jcode, NULL);
    const char* filename = jfilename ? (*env)->GetStringUTFChars(env, jfilename, NULL) : "<script>";

    py_initialize();

    if (jcallback) {
        jclass cbClass = (*env)->GetObjectClass(env, jcallback);
        g_mid_on_output = (*env)->GetMethodID(env, cbClass, "onOutput", "(Ljava/lang/String;)V");
        g_mid_on_error = (*env)->GetMethodID(env, cbClass, "onError", "(Ljava/lang/String;)V");
        g_current_callback = (*env)->NewGlobalRef(env, jcallback);
        py_callbacks()->print = jni_print_callback;
    }

    bool success = py_exec(code, filename, EXEC_MODE, NULL);

    char* error_msg = NULL;
    if (!success) {
        error_msg = py_formatexc();
        if (error_msg && g_current_callback && g_mid_on_error) {
            jstring jerr = (*env)->NewStringUTF(env, error_msg);
            if (jerr) {
                (*env)->CallVoidMethod(env, g_current_callback, g_mid_on_error, jerr);
                (*env)->DeleteLocalRef(env, jerr);
            }
        }
    }

    if (g_current_callback) {
        (*env)->DeleteGlobalRef(env, g_current_callback);
        g_current_callback = NULL;
    }
    py_callbacks()->print = NULL;

    jclass resClass = (*env)->FindClass(env, "com/pocketpy/ide/engine/ExecutionResult");
    jmethodID resConstructor = (*env)->GetMethodID(env, resClass, "<init>", "(ZLjava/lang/String;)V");

    jstring jerrResult = error_msg ? (*env)->NewStringUTF(env, error_msg) : (*env)->NewStringUTF(env, "");
    jobject resultObj = (*env)->NewObject(env, resClass, resConstructor, (jboolean)success, jerrResult);

    if (error_msg) free(error_msg);
    (*env)->ReleaseStringUTFChars(env, jcode, code);
    if (jfilename) (*env)->ReleaseStringUTFChars(env, jfilename, filename);

    py_finalize();

    return resultObj;
}

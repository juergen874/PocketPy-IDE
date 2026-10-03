#include <jni.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <android/log.h>
#include "pocketpy.h"
#include "pocketpy_ext.h"

#define TAG "PocketPyJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static JavaVM* g_vm = NULL;
static jobject g_current_callback = NULL;
static jobject g_engine_obj = NULL;
static jmethodID g_mid_on_output = NULL;
static jmethodID g_mid_on_error = NULL;

static jmethodID g_mid_toast = NULL;
static jmethodID g_mid_vibrate = NULL;
static jmethodID g_mid_notify = NULL;
static jmethodID g_mid_speak = NULL;
static jmethodID g_mid_battery_level = NULL;
static jmethodID g_mid_battery_charging = NULL;
static jmethodID g_mid_clip_set = NULL;
static jmethodID g_mid_clip_get = NULL;
static jmethodID g_mid_beep = NULL;
static bool g_ext_registered = false;

static JNIEnv* get_jni_env(bool* should_detach) {
    if (!g_vm) return NULL;
    JNIEnv* env = NULL;
    int env_status = (*g_vm)->GetEnv(g_vm, (void**)&env, JNI_VERSION_1_6);
    *should_detach = false;
    if (env_status == JNI_EDETACHED) {
        if ((*g_vm)->AttachCurrentThread(g_vm, (void**)&env, NULL) != 0) {
            return NULL;
        }
        *should_detach = true;
    }
    return env;
}

static void release_jni_env(bool should_detach) {
    if (should_detach && g_vm) {
        (*g_vm)->DetachCurrentThread(g_vm);
    }
}

// Safely create a java.lang.String from standard UTF-8 (including 4-byte emojis)
static jstring create_java_string(JNIEnv* env, const char* str) {
    if (!env || !str) return NULL;
    int len = (int)strlen(str);
    if (len == 0) return (*env)->NewStringUTF(env, "");

    jbyteArray bytes = (*env)->NewByteArray(env, len);
    if (!bytes) {
        if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
        return NULL;
    }
    (*env)->SetByteArrayRegion(env, bytes, 0, len, (const jbyte*)str);

    jclass strClass = (*env)->FindClass(env, "java/lang/String");
    if (!strClass) {
        if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, bytes);
        return NULL;
    }
    jmethodID ctor = (*env)->GetMethodID(env, strClass, "<init>", "([BLjava/lang/String;)V");
    jstring encoding = (*env)->NewStringUTF(env, "UTF-8");
    jstring result = NULL;
    if (ctor && encoding) {
        result = (jstring)(*env)->NewObject(env, strClass, ctor, bytes, encoding);
    }
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        result = NULL;
    }
    (*env)->DeleteLocalRef(env, bytes);
    if (encoding) (*env)->DeleteLocalRef(env, encoding);
    (*env)->DeleteLocalRef(env, strClass);
    return result;
}

static jmethodID safe_get_method(JNIEnv* env, jclass clazz, const char* name, const char* sig) {
    if (!env || !clazz) return NULL;
    jmethodID mid = (*env)->GetMethodID(env, clazz, name, sig);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        return NULL;
    }
    return mid;
}

static void jni_print_callback(const char* text) {
    if (!text || !g_current_callback || !g_mid_on_output) return;
    bool detach = false;
    JNIEnv* env = get_jni_env(&detach);
    if (!env) return;

    jstring jstr = create_java_string(env, text);
    if (jstr) {
        (*env)->CallVoidMethod(env, g_current_callback, g_mid_on_output, jstr);
        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionClear(env);
        }
        (*env)->DeleteLocalRef(env, jstr);
    }
    release_jni_env(detach);
}

// Android Bridge JNI Implementations
static void android_bridge_toast(const char* msg, bool is_long) {
    if (!g_engine_obj || !g_mid_toast || !msg) return;
    bool detach = false;
    JNIEnv* env = get_jni_env(&detach);
    if (!env) return;
    jstring jstr = create_java_string(env, msg);
    if (jstr) {
        (*env)->CallVoidMethod(env, g_engine_obj, g_mid_toast, jstr, (jboolean)is_long);
        if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, jstr);
    }
    release_jni_env(detach);
}

static void android_bridge_vibrate(int64_t ms) {
    if (!g_engine_obj || !g_mid_vibrate) return;
    bool detach = false;
    JNIEnv* env = get_jni_env(&detach);
    if (!env) return;
    (*env)->CallVoidMethod(env, g_engine_obj, g_mid_vibrate, (jlong)ms);
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    release_jni_env(detach);
}

static void android_bridge_notify(const char* title, const char* text, int id) {
    if (!g_engine_obj || !g_mid_notify) return;
    bool detach = false;
    JNIEnv* env = get_jni_env(&detach);
    if (!env) return;
    jstring jtitle = create_java_string(env, title ? title : "");
    jstring jtext = create_java_string(env, text ? text : "");
    if (jtitle && jtext) {
        (*env)->CallVoidMethod(env, g_engine_obj, g_mid_notify, jtitle, jtext, (jint)id);
        if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    }
    if (jtitle) (*env)->DeleteLocalRef(env, jtitle);
    if (jtext) (*env)->DeleteLocalRef(env, jtext);
    release_jni_env(detach);
}

static void android_bridge_speak(const char* text) {
    if (!g_engine_obj || !g_mid_speak || !text) return;
    bool detach = false;
    JNIEnv* env = get_jni_env(&detach);
    if (!env) return;
    jstring jstr = create_java_string(env, text);
    if (jstr) {
        (*env)->CallVoidMethod(env, g_engine_obj, g_mid_speak, jstr);
        if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, jstr);
    }
    release_jni_env(detach);
}

static void android_bridge_battery(int* level, int* charging) {
    if (!g_engine_obj || !g_mid_battery_level || !g_mid_battery_charging) {
        if (level) *level = 100;
        if (charging) *charging = 0;
        return;
    }
    bool detach = false;
    JNIEnv* env = get_jni_env(&detach);
    if (!env) return;
    jint lvl = (*env)->CallIntMethod(env, g_engine_obj, g_mid_battery_level);
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    jboolean chg = (*env)->CallBooleanMethod(env, g_engine_obj, g_mid_battery_charging);
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    if (level) *level = (int)lvl;
    if (charging) *charging = chg ? 1 : 0;
    release_jni_env(detach);
}

static void android_bridge_clip_set(const char* text) {
    if (!g_engine_obj || !g_mid_clip_set || !text) return;
    bool detach = false;
    JNIEnv* env = get_jni_env(&detach);
    if (!env) return;
    jstring jstr = create_java_string(env, text);
    if (jstr) {
        (*env)->CallVoidMethod(env, g_engine_obj, g_mid_clip_set, jstr);
        if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, jstr);
    }
    release_jni_env(detach);
}

static char* android_bridge_clip_get(void) {
    if (!g_engine_obj || !g_mid_clip_get) return NULL;
    bool detach = false;
    JNIEnv* env = get_jni_env(&detach);
    if (!env) return NULL;
    jstring jstr = (jstring)(*env)->CallObjectMethod(env, g_engine_obj, g_mid_clip_get);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        release_jni_env(detach);
        return NULL;
    }
    char* result = NULL;
    if (jstr) {
        const char* utf = (*env)->GetStringUTFChars(env, jstr, NULL);
        if (utf) {
            result = strdup(utf);
            (*env)->ReleaseStringUTFChars(env, jstr, utf);
        }
        (*env)->DeleteLocalRef(env, jstr);
    }
    release_jni_env(detach);
    return result;
}

static void android_bridge_beep(int freq, int duration_ms) {
    if (!g_engine_obj || !g_mid_beep) return;
    bool detach = false;
    JNIEnv* env = get_jni_env(&detach);
    if (!env) return;
    (*env)->CallVoidMethod(env, g_engine_obj, g_mid_beep, (jint)freq, (jint)duration_ms);
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    release_jni_env(detach);
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

    g_engine_obj = (*env)->NewGlobalRef(env, thiz);
    jclass engineClass = (*env)->GetObjectClass(env, thiz);
    g_mid_toast = safe_get_method(env, engineClass, "showToast", "(Ljava/lang/String;Z)V");
    g_mid_vibrate = safe_get_method(env, engineClass, "vibratePhone", "(J)V");
    g_mid_notify = safe_get_method(env, engineClass, "showNotification", "(Ljava/lang/String;Ljava/lang/String;I)V");
    g_mid_speak = safe_get_method(env, engineClass, "speakText", "(Ljava/lang/String;)V");
    g_mid_battery_level = safe_get_method(env, engineClass, "getBatteryLevel", "()I");
    g_mid_battery_charging = safe_get_method(env, engineClass, "isBatteryCharging", "()Z");
    g_mid_clip_set = safe_get_method(env, engineClass, "copyToClipboard", "(Ljava/lang/String;)V");
    g_mid_clip_get = safe_get_method(env, engineClass, "getClipboard", "()Ljava/lang/String;");
    g_mid_beep = safe_get_method(env, engineClass, "beep", "(II)V");
    if (engineClass) (*env)->DeleteLocalRef(env, engineClass);

    AndroidBridgeHooks hooks = {
        .toast = android_bridge_toast,
        .vibrate = android_bridge_vibrate,
        .notify = android_bridge_notify,
        .speak = android_bridge_speak,
        .battery = android_bridge_battery,
        .clip_set = android_bridge_clip_set,
        .clip_get = android_bridge_clip_get,
        .beep = android_bridge_beep
    };
    set_android_hooks(&hooks);

    py_initialize();
    if (!g_ext_registered) {
        register_all_pocketpy_extensions();
        g_ext_registered = true;
    }

    if (jcallback) {
        jclass cbInterface = (*env)->FindClass(env, "com/pocketpy/ide/engine/PocketPyCallback");
        if (cbInterface) {
            g_mid_on_output = safe_get_method(env, cbInterface, "onOutput", "(Ljava/lang/String;)V");
            g_mid_on_error = safe_get_method(env, cbInterface, "onError", "(Ljava/lang/String;)V");
            (*env)->DeleteLocalRef(env, cbInterface);
        }
        if (!g_mid_on_output) {
            jclass cbClass = (*env)->GetObjectClass(env, jcallback);
            g_mid_on_output = safe_get_method(env, cbClass, "onOutput", "(Ljava/lang/String;)V");
            g_mid_on_error = safe_get_method(env, cbClass, "onError", "(Ljava/lang/String;)V");
            if (cbClass) (*env)->DeleteLocalRef(env, cbClass);
        }
        g_current_callback = (*env)->NewGlobalRef(env, jcallback);
        py_callbacks()->print = jni_print_callback;
    }

    bool success = py_exec(code, filename, EXEC_MODE, NULL);

    char* error_msg = NULL;
    if (!success) {
        error_msg = py_formatexc();
        if (error_msg && g_current_callback && g_mid_on_error) {
            jstring jerr = create_java_string(env, error_msg);
            if (jerr) {
                (*env)->CallVoidMethod(env, g_current_callback, g_mid_on_error, jerr);
                if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
                (*env)->DeleteLocalRef(env, jerr);
            }
        }
    }

    if (g_current_callback) {
        (*env)->DeleteGlobalRef(env, g_current_callback);
        g_current_callback = NULL;
    }
    if (g_engine_obj) {
        (*env)->DeleteGlobalRef(env, g_engine_obj);
        g_engine_obj = NULL;
    }
    py_callbacks()->print = NULL;

    jclass resClass = (*env)->FindClass(env, "com/pocketpy/ide/engine/ExecutionResult");
    jmethodID resConstructor = safe_get_method(env, resClass, "<init>", "(ZLjava/lang/String;)V");

    jstring jerrResult = create_java_string(env, error_msg ? error_msg : "");
    jobject resultObj = NULL;
    if (resClass && resConstructor && jerrResult) {
        resultObj = (*env)->NewObject(env, resClass, resConstructor, (jboolean)success, jerrResult);
    }
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);

    if (resClass) (*env)->DeleteLocalRef(env, resClass);
    if (jerrResult) (*env)->DeleteLocalRef(env, jerrResult);

    if (error_msg) free(error_msg);
    (*env)->ReleaseStringUTFChars(env, jcode, code);
    if (jfilename) (*env)->ReleaseStringUTFChars(env, jfilename, filename);

    return resultObj;
}

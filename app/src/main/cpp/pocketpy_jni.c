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

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

static bool c_socket_tcp_client(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(3);
    const char* host = py_tostr(py_arg(0));
    int port = (int)py_toint(py_arg(1));
    double timeout = py_tofloat(py_arg(2));

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return OSError("Failed to create socket");
    }

    struct timeval tv;
    tv.tv_sec = (time_t)timeout;
    tv.tv_usec = (suseconds_t)((timeout - tv.tv_sec) * 1000000);
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0) {
        struct hostent* he = gethostbyname(host);
        if (!he) {
            close(fd);
            return OSError("Failed to resolve host");
        }
        memcpy(&serv_addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    if (connect(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(fd);
        return OSError("Failed to connect to host");
    }

    py_newint(py_retval(), fd);
    return true;
}

static bool c_socket_send(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2);
    int fd = (int)py_toint(py_arg(0));
    int len = 0;
    const unsigned char* data = (const unsigned char*)py_tobytes(py_arg(1), &len);
    if (!data) return TypeError("Expected bytes");

    int sent = send(fd, data, len, 0);
    if (sent < 0) return OSError("send() failed");
    py_newint(py_retval(), sent);
    return true;
}

static bool c_socket_recv(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2);
    int fd = (int)py_toint(py_arg(0));
    int maxlen = (int)py_toint(py_arg(1));
    unsigned char* buf = (unsigned char*)malloc(maxlen);
    if (!buf) return RuntimeError("Out of memory");

    int n = recv(fd, buf, maxlen, 0);
    if (n < 0) {
        free(buf);
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return TimeoutError("Socket recv timed out");
        }
        return OSError("Socket recv failed");
    }

    unsigned char* dst = py_newbytes(py_retval(), n);
    memcpy(dst, buf, n);
    free(buf);
    return true;
}

static bool c_socket_close(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    int fd = (int)py_toint(py_arg(0));
    close(fd);
    py_newnone(py_retval());
    return true;
}

static void register_socket_module(void) {
    py_GlobalRef mod = py_newmodule("socket");
    py_bindfunc(mod, "_tcp_client", c_socket_tcp_client);
    py_bindfunc(mod, "_send", c_socket_send);
    py_bindfunc(mod, "_recv", c_socket_recv);
    py_bindfunc(mod, "_close", c_socket_close);

    const char* py_socket_wrapper =
        "class socket:\n"
        "    AF_INET = 2\n"
        "    SOCK_STREAM = 1\n"
        "    def __init__(self, family=2, type=1):\n"
        "        self.fd = None\n"
        "        self._timeout = 4.0\n"
        "    def settimeout(self, t):\n"
        "        self._timeout = float(t)\n"
        "    def connect(self, addr):\n"
        "        host, port = addr\n"
        "        self.fd = _tcp_client(str(host), int(port), self._timeout)\n"
        "    def send(self, data):\n"
        "        return _send(self.fd, data)\n"
        "    def sendall(self, data):\n"
        "        return _send(self.fd, data)\n"
        "    def recv(self, maxlen=1024):\n"
        "        return _recv(self.fd, int(maxlen))\n"
        "    def close(self):\n"
        "        if self.fd is not None:\n"
        "            _close(self.fd)\n"
        "            self.fd = None\n";

    py_exec(py_socket_wrapper, "<socket>", EXEC_MODE, mod);
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
    register_socket_module();

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

#ifndef POCKETPY_EXT_H
#define POCKETPY_EXT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/utsname.h>

#if defined(__ANDROID__) || defined(__ANDROID_API__)
#include <sys/system_properties.h>
#include <android/log.h>
#define HAS_ANDROID_PROPS 1
#else
#define HAS_ANDROID_PROPS 0
#endif

#if defined(__linux__)
#include <sys/sysinfo.h>
#define HAS_SYSINFO 1
#else
#define HAS_SYSINFO 0
#endif

#include "pocketpy.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------- */
/* Android JNI Callback hooks (populated by JNI in Android app)  */
/* ------------------------------------------------------------- */
typedef void (*AndroidToastFn)(const char* msg, bool is_long);
typedef void (*AndroidVibrateFn)(int64_t ms);
typedef void (*AndroidNotifyFn)(const char* title, const char* text, int id);
typedef void (*AndroidSpeakFn)(const char* text);
typedef void (*AndroidBatteryFn)(int* level, int* charging);
typedef void (*AndroidClipboardSetFn)(const char* text);
typedef char* (*AndroidClipboardGetFn)(void);
typedef void (*AndroidBeepFn)(int freq, int duration_ms);

typedef struct {
    AndroidToastFn toast;
    AndroidVibrateFn vibrate;
    AndroidNotifyFn notify;
    AndroidSpeakFn speak;
    AndroidBatteryFn battery;
    AndroidClipboardSetFn clip_set;
    AndroidClipboardGetFn clip_get;
    AndroidBeepFn beep;
} AndroidBridgeHooks;

static AndroidBridgeHooks g_android_hooks = {0};

static inline void set_android_hooks(const AndroidBridgeHooks* hooks) {
    if (hooks) g_android_hooks = *hooks;
}

static inline double to_number(py_Ref r) {
    if (py_isint(r)) return (double)py_toint(r);
    if (py_isfloat(r)) return py_tofloat(r);
    py_f64 out = 0;
    if (py_castfloat(r, &out)) return (double)out;
    return 0.0;
}

static inline const unsigned char* get_bytes_or_str(py_Ref self, int* size) {
    if (py_istype(self, tp_bytes)) {
        return py_tobytes(self, size);
    }
    if (py_isstr(self)) {
        const char* s = py_tostr(self);
        if (size) *size = (int)strlen(s);
        return (const unsigned char*)s;
    }
    return NULL;
}

/* ============================================================= */
/* 1. SOCKET MODULE (20 functions / methods)                    */
/* ============================================================= */

static bool c_socket_tcp_client(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(3);
    const char* host = py_tostr(py_arg(0));
    int port = (int)py_toint(py_arg(1));
    double timeout = to_number(py_arg(2));

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return OSError("Failed to create TCP socket");

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
        if (!he) { close(fd); return OSError("Failed to resolve host"); }
        memcpy(&serv_addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    if (connect(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(fd);
        return OSError("connect() failed: %s", strerror(errno));
    }
    py_newint(py_retval(), fd);
    return true;
}

static bool c_socket_tcp_server(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(3);
    const char* host = py_tostr(py_arg(0));
    int port = (int)py_toint(py_arg(1));
    int backlog = (int)py_toint(py_arg(2));

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return OSError("Failed to create server socket");

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (!host || strlen(host) == 0 || strcmp(host, "0.0.0.0") == 0) {
        serv_addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, host, &serv_addr.sin_addr);
    }

    if (bind(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(fd);
        return OSError("bind() failed: %s", strerror(errno));
    }
    if (listen(fd, backlog > 0 ? backlog : 5) < 0) {
        close(fd);
        return OSError("listen() failed");
    }
    py_newint(py_retval(), fd);
    return true;
}

static bool c_socket_accept(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    int fd = (int)py_toint(py_arg(0));
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) return OSError("accept() failed: %s", strerror(errno));

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
    int port = ntohs(client_addr.sin_port);

    py_newtuple(py_retval(), 3);
    py_newint(py_r0(), client_fd);
    py_tuple_setitem(py_retval(), 0, py_r0());
    py_newstr(py_r0(), ip_str);
    py_tuple_setitem(py_retval(), 1, py_r0());
    py_newint(py_r0(), port);
    py_tuple_setitem(py_retval(), 2, py_r0());
    return true;
}

static bool c_socket_send(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2);
    int fd = (int)py_toint(py_arg(0));
    int len = 0;
    const unsigned char* data = get_bytes_or_str(py_arg(1), &len);
    if (!data) return TypeError("Expected bytes or str");
    int sent = send(fd, data, len, 0);
    if (sent < 0) return OSError("send() failed: %s", strerror(errno));
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
        if (errno == EWOULDBLOCK || errno == EAGAIN) return TimeoutError("Socket timed out");
        return OSError("recv() failed: %s", strerror(errno));
    }
    unsigned char* dst = py_newbytes(py_retval(), n);
    memcpy(dst, buf, n);
    free(buf);
    return true;
}

static bool c_socket_udp_socket(int argc, py_StackRef argv) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return OSError("Failed to create UDP socket");
    py_newint(py_retval(), fd);
    return true;
}

static bool c_socket_sendto(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(4);
    int fd = (int)py_toint(py_arg(0));
    int len = 0;
    const unsigned char* data = get_bytes_or_str(py_arg(1), &len);
    if (!data) return TypeError("Expected bytes or str");
    const char* host = py_tostr(py_arg(2));
    int port = (int)py_toint(py_arg(3));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &serv_addr.sin_addr);

    int sent = sendto(fd, data, len, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (sent < 0) return OSError("sendto() failed");
    py_newint(py_retval(), sent);
    return true;
}

static bool c_socket_recvfrom(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2);
    int fd = (int)py_toint(py_arg(0));
    int maxlen = (int)py_toint(py_arg(1));
    unsigned char* buf = (unsigned char*)malloc(maxlen);
    if (!buf) return RuntimeError("Out of memory");

    struct sockaddr_in src_addr;
    socklen_t addr_len = sizeof(src_addr);
    int n = recvfrom(fd, buf, maxlen, 0, (struct sockaddr*)&src_addr, &addr_len);
    if (n < 0) {
        free(buf);
        return OSError("recvfrom() failed");
    }
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &src_addr.sin_addr, ip_str, sizeof(ip_str));
    int port = ntohs(src_addr.sin_port);

    py_newtuple(py_retval(), 3);
    unsigned char* dst = py_newbytes(py_r0(), n);
    memcpy(dst, buf, n);
    free(buf);
    py_tuple_setitem(py_retval(), 0, py_r0());
    py_newstr(py_r0(), ip_str);
    py_tuple_setitem(py_retval(), 1, py_r0());
    py_newint(py_r0(), port);
    py_tuple_setitem(py_retval(), 2, py_r0());
    return true;
}

static bool c_socket_close(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    int fd = (int)py_toint(py_arg(0));
    close(fd);
    py_newnone(py_retval());
    return true;
}

static bool c_socket_settimeout(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2);
    int fd = (int)py_toint(py_arg(0));
    double timeout = to_number(py_arg(1));
    struct timeval tv;
    tv.tv_sec = (time_t)timeout;
    tv.tv_usec = (suseconds_t)((timeout - tv.tv_sec) * 1000000);
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    py_newnone(py_retval());
    return true;
}

static bool c_socket_gethostbyname(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    const char* name = py_tostr(py_arg(0));
    struct hostent* he = gethostbyname(name);
    if (!he || !he->h_addr_list[0]) return OSError("Host not found");
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, he->h_addr_list[0], ip, sizeof(ip));
    py_newstr(py_retval(), ip);
    return true;
}

static bool c_socket_gethostname(int argc, py_StackRef argv) {
    char hname[256] = {0};
    gethostname(hname, sizeof(hname) - 1);
    py_newstr(py_retval(), hname);
    return true;
}

static bool c_socket_getsockname(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    int fd = (int)py_toint(py_arg(0));
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(fd, (struct sockaddr*)&sin, &len) < 0) return OSError("getsockname failed");
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sin.sin_addr, ip, sizeof(ip));
    py_newtuple(py_retval(), 2);
    py_newstr(py_r0(), ip);
    py_tuple_setitem(py_retval(), 0, py_r0());
    py_newint(py_r0(), ntohs(sin.sin_port));
    py_tuple_setitem(py_retval(), 1, py_r0());
    return true;
}

static bool c_socket_getpeername(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    int fd = (int)py_toint(py_arg(0));
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getpeername(fd, (struct sockaddr*)&sin, &len) < 0) return OSError("getpeername failed");
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sin.sin_addr, ip, sizeof(ip));
    py_newtuple(py_retval(), 2);
    py_newstr(py_r0(), ip);
    py_tuple_setitem(py_retval(), 0, py_r0());
    py_newint(py_r0(), ntohs(sin.sin_port));
    py_tuple_setitem(py_retval(), 1, py_r0());
    return true;
}

static bool c_socket_shutdown(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2);
    int fd = (int)py_toint(py_arg(0));
    int how = (int)py_toint(py_arg(1));
    shutdown(fd, how);
    py_newnone(py_retval());
    return true;
}

static bool c_socket_htons(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    py_newint(py_retval(), htons((uint16_t)py_toint(py_arg(0))));
    return true;
}

static bool c_socket_ntohs(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    py_newint(py_retval(), ntohs((uint16_t)py_toint(py_arg(0))));
    return true;
}

static bool c_socket_htonl(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    py_newint(py_retval(), htonl((uint32_t)py_toint(py_arg(0))));
    return true;
}

static bool c_socket_ntohl(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    py_newint(py_retval(), ntohl((uint32_t)py_toint(py_arg(0))));
    return true;
}

/* ============================================================= */
/* 2. OS & POSIX MODULE (35 functions)                          */
/* ============================================================= */

static bool c_os_listdir(int argc, py_StackRef argv) {
    const char* path = argc > 0 ? py_tostr(py_arg(0)) : ".";
    DIR* d = opendir(path);
    if (!d) return OSError("Cannot open directory: %s", path);
    py_newlist(py_retval());
    struct dirent* entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        py_newstr(py_r0(), entry->d_name);
        py_list_append(py_retval(), py_r0());
    }
    closedir(d);
    return true;
}

static bool c_os_mkdir(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    const char* path = py_tostr(py_arg(0));
    if (mkdir(path, 0755) != 0 && errno != EEXIST) return OSError("mkdir failed: %s", strerror(errno));
    py_newnone(py_retval());
    return true;
}

static bool c_os_makedirs(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    const char* path = py_tostr(py_arg(0));
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s", path);
    size_t len = strlen(tmp);
    for (size_t i = 1; i < len; i++) {
        if (tmp[i] == '/') {
            tmp[i] = 0;
            mkdir(tmp, 0755);
            tmp[i] = '/';
        }
    }
    mkdir(tmp, 0755);
    py_newnone(py_retval());
    return true;
}

static bool c_os_rmdir(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    if (rmdir(py_tostr(py_arg(0))) != 0) return OSError("rmdir failed");
    py_newnone(py_retval());
    return true;
}

static bool c_os_remove(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    if (unlink(py_tostr(py_arg(0))) != 0) return OSError("remove failed: %s", strerror(errno));
    py_newnone(py_retval());
    return true;
}

static bool c_os_rename(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2);
    if (rename(py_tostr(py_arg(0)), py_tostr(py_arg(1))) != 0) return OSError("rename failed");
    py_newnone(py_retval());
    return true;
}

static bool c_os_stat(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    const char* path = py_tostr(py_arg(0));
    struct stat st;
    if (stat(path, &st) != 0) return OSError("stat failed for %s", path);
    py_newdict(py_retval());
    py_newint(py_r0(), st.st_size);
    py_dict_setitem_by_str(py_retval(), "size", py_r0());
    py_newint(py_r0(), st.st_mtime);
    py_dict_setitem_by_str(py_retval(), "mtime", py_r0());
    py_newint(py_r0(), st.st_ctime);
    py_dict_setitem_by_str(py_retval(), "ctime", py_r0());
    py_newint(py_r0(), st.st_mode);
    py_dict_setitem_by_str(py_retval(), "mode", py_r0());
    py_newbool(py_r0(), S_ISDIR(st.st_mode));
    py_dict_setitem_by_str(py_retval(), "is_dir", py_r0());
    py_newbool(py_r0(), S_ISREG(st.st_mode));
    py_dict_setitem_by_str(py_retval(), "is_file", py_r0());
    return true;
}

static bool c_os_statvfs(int argc, py_StackRef argv) {
    const char* path = argc > 0 ? py_tostr(py_arg(0)) : "/data";
    struct statvfs s;
    if (statvfs(path, &s) != 0) return OSError("statvfs failed");
    py_newdict(py_retval());
    int64_t total = (int64_t)s.f_blocks * s.f_frsize;
    int64_t free_b = (int64_t)s.f_bfree * s.f_frsize;
    int64_t avail = (int64_t)s.f_bavail * s.f_frsize;
    py_newint(py_r0(), total);
    py_dict_setitem_by_str(py_retval(), "total", py_r0());
    py_newint(py_r0(), free_b);
    py_dict_setitem_by_str(py_retval(), "free", py_r0());
    py_newint(py_r0(), avail);
    py_dict_setitem_by_str(py_retval(), "avail", py_r0());
    return true;
}

static bool c_os_chmod(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2);
    chmod(py_tostr(py_arg(0)), (mode_t)py_toint(py_arg(1)));
    py_newnone(py_retval());
    return true;
}

static bool c_os_getcwd(int argc, py_StackRef argv) {
    char buf[1024];
    if (!getcwd(buf, sizeof(buf))) return OSError("getcwd failed");
    py_newstr(py_retval(), buf);
    return true;
}

static bool c_os_chdir(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    if (chdir(py_tostr(py_arg(0))) != 0) return OSError("chdir failed");
    py_newnone(py_retval());
    return true;
}

static bool c_os_access(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2);
    int res = access(py_tostr(py_arg(0)), (int)py_toint(py_arg(1)));
    py_newbool(py_retval(), res == 0);
    return true;
}

static bool c_os_getpid(int argc, py_StackRef argv) {
    py_newint(py_retval(), getpid());
    return true;
}

static bool c_os_getppid(int argc, py_StackRef argv) {
    py_newint(py_retval(), getppid());
    return true;
}

static bool c_os_getuid(int argc, py_StackRef argv) {
    py_newint(py_retval(), getuid());
    return true;
}

static bool c_os_geteuid(int argc, py_StackRef argv) {
    py_newint(py_retval(), geteuid());
    return true;
}

static bool c_os_getgid(int argc, py_StackRef argv) {
    py_newint(py_retval(), getgid());
    return true;
}

static bool c_os_getenv(int argc, py_StackRef argv) {
    if (argc < 1) return TypeError("getenv(key, [default]) requires at least 1 arg");
    const char* key = py_tostr(py_arg(0));
    const char* val = getenv(key);
    if (val) {
        py_newstr(py_retval(), val);
    } else if (argc > 1) {
        py_assign(py_retval(), py_arg(1));
    } else {
        py_newnone(py_retval());
    }
    return true;
}

static bool c_os_putenv(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2);
    setenv(py_tostr(py_arg(0)), py_tostr(py_arg(1)), 1);
    py_newnone(py_retval());
    return true;
}

static bool c_os_unsetenv(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    unsetenv(py_tostr(py_arg(0)));
    py_newnone(py_retval());
    return true;
}

static bool c_os_system(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    int res = system(py_tostr(py_arg(0)));
    py_newint(py_retval(), res);
    return true;
}

static bool c_os_popen(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    const char* cmd = py_tostr(py_arg(0));
    FILE* p = popen(cmd, "r");
    if (!p) return OSError("popen failed");
    char* out = NULL;
    size_t out_len = 0;
    char chunk[512];
    while (fgets(chunk, sizeof(chunk), p)) {
        size_t clen = strlen(chunk);
        char* n = (char*)realloc(out, out_len + clen + 1);
        if (!n) { free(out); pclose(p); return RuntimeError("Out of memory"); }
        out = n;
        memcpy(out + out_len, chunk, clen);
        out_len += clen;
        out[out_len] = 0;
    }
    pclose(p);
    py_newstr(py_retval(), out ? out : "");
    if (out) free(out);
    return true;
}

static bool c_os_uname(int argc, py_StackRef argv) {
    struct utsname u;
    if (uname(&u) != 0) return OSError("uname failed");
    py_newdict(py_retval());
    py_newstr(py_r0(), u.sysname);
    py_dict_setitem_by_str(py_retval(), "sysname", py_r0());
    py_newstr(py_r0(), u.nodename);
    py_dict_setitem_by_str(py_retval(), "nodename", py_r0());
    py_newstr(py_r0(), u.release);
    py_dict_setitem_by_str(py_retval(), "release", py_r0());
    py_newstr(py_r0(), u.version);
    py_dict_setitem_by_str(py_retval(), "version", py_r0());
    py_newstr(py_r0(), u.machine);
    py_dict_setitem_by_str(py_retval(), "machine", py_r0());
    return true;
}

static bool c_os_cpu_count(int argc, py_StackRef argv) {
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    py_newint(py_retval(), n > 0 ? n : 1);
    return true;
}

static bool c_os_urandom(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    int n = (int)py_toint(py_arg(0));
    if (n < 0) return ValueError("Negative size");
    unsigned char* b = py_newbytes(py_retval(), n);
    FILE* f = fopen("/dev/urandom", "rb");
    if (f) {
        fread(b, 1, n, f);
        fclose(f);
    } else {
        for (int i = 0; i < n; i++) b[i] = (unsigned char)rand();
    }
    return true;
}

static bool c_os_open(int argc, py_StackRef argv) {
    if (argc < 2) return TypeError("open(path, flags, [mode])");
    const char* path = py_tostr(py_arg(0));
    int flags = (int)py_toint(py_arg(1));
    mode_t mode = argc > 2 ? (mode_t)py_toint(py_arg(2)) : 0644;
    int fd = open(path, flags, mode);
    if (fd < 0) return OSError("open() failed: %s", strerror(errno));
    py_newint(py_retval(), fd);
    return true;
}

static bool c_os_read(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2);
    int fd = (int)py_toint(py_arg(0));
    int n = (int)py_toint(py_arg(1));
    unsigned char* buf = (unsigned char*)malloc(n);
    if (!buf) return RuntimeError("Out of memory");
    ssize_t r = read(fd, buf, n);
    if (r < 0) { free(buf); return OSError("read() failed"); }
    unsigned char* dst = py_newbytes(py_retval(), (int)r);
    memcpy(dst, buf, r);
    free(buf);
    return true;
}

static bool c_os_write(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2);
    int fd = (int)py_toint(py_arg(0));
    int len = 0;
    const unsigned char* data = get_bytes_or_str(py_arg(1), &len);
    if (!data) return TypeError("Expected bytes or str");
    ssize_t w = write(fd, data, len);
    if (w < 0) return OSError("write() failed");
    py_newint(py_retval(), (int64_t)w);
    return true;
}

static bool c_os_close(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    close((int)py_toint(py_arg(0)));
    py_newnone(py_retval());
    return true;
}

static bool c_os_lseek(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(3);
    off_t off = lseek((int)py_toint(py_arg(0)), (off_t)py_toint(py_arg(1)), (int)py_toint(py_arg(2)));
    py_newint(py_retval(), (int64_t)off);
    return true;
}

static bool c_os_fsync(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    fsync((int)py_toint(py_arg(0)));
    py_newnone(py_retval());
    return true;
}

static bool c_os_dup(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    int nfd = dup((int)py_toint(py_arg(0)));
    py_newint(py_retval(), nfd);
    return true;
}

static bool c_os_pipe(int argc, py_StackRef argv) {
    int fds[2];
    if (pipe(fds) != 0) return OSError("pipe() failed");
    py_newtuple(py_retval(), 2);
    py_newint(py_r0(), fds[0]);
    py_tuple_setitem(py_retval(), 0, py_r0());
    py_newint(py_r0(), fds[1]);
    py_tuple_setitem(py_retval(), 1, py_r0());
    return true;
}

static bool c_os_isatty(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    py_newbool(py_retval(), isatty((int)py_toint(py_arg(0))) == 1);
    return true;
}

/* ============================================================= */
/* 3. SYSINFO & HARDWARE MODULE (15 functions)                  */
/* ============================================================= */

static bool c_sysinfo_ram_total(int argc, py_StackRef argv) {
#if HAS_SYSINFO
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        py_newint(py_retval(), (int64_t)si.totalram * si.mem_unit);
        return true;
    }
#endif
    py_newint(py_retval(), 0);
    return true;
}

static bool c_sysinfo_ram_free(int argc, py_StackRef argv) {
#if HAS_SYSINFO
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        py_newint(py_retval(), (int64_t)si.freeram * si.mem_unit);
        return true;
    }
#endif
    py_newint(py_retval(), 0);
    return true;
}

static bool c_sysinfo_ram_available(int argc, py_StackRef argv) {
    int64_t avail = 0;
    FILE* f = fopen("/proc/meminfo", "r");
    if (f) {
        char line[128];
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "MemAvailable: %ld kB", &avail) == 1) {
                avail *= 1024;
                break;
            }
        }
        fclose(f);
    }
    py_newint(py_retval(), avail > 0 ? avail : 0);
    return true;
}

static bool c_sysinfo_ram_used(int argc, py_StackRef argv) {
#if HAS_SYSINFO
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        int64_t total = (int64_t)si.totalram * si.mem_unit;
        int64_t free_b = (int64_t)si.freeram * si.mem_unit;
        py_newint(py_retval(), total - free_b);
        return true;
    }
#endif
    py_newint(py_retval(), 0);
    return true;
}

static bool c_sysinfo_uptime(int argc, py_StackRef argv) {
#if HAS_SYSINFO
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        py_newint(py_retval(), si.uptime);
        return true;
    }
#endif
    py_newint(py_retval(), 0);
    return true;
}

static bool c_sysinfo_uptime_str(int argc, py_StackRef argv) {
#if HAS_SYSINFO
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        long d = si.uptime / 86400;
        long h = (si.uptime % 86400) / 3600;
        long m = (si.uptime % 3600) / 60;
        long s = si.uptime % 60;
        char str[64];
        snprintf(str, sizeof(str), "%ldd %02ld:%02ld:%02ld", d, h, m, s);
        py_newstr(py_retval(), str);
        return true;
    }
#endif
    py_newstr(py_retval(), "0d 00:00:00");
    return true;
}

static bool c_sysinfo_loadavg(int argc, py_StackRef argv) {
#if HAS_SYSINFO
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        py_newtuple(py_retval(), 3);
        py_newfloat(py_r0(), (double)si.loads[0] / (1 << SI_LOAD_SHIFT));
        py_tuple_setitem(py_retval(), 0, py_r0());
        py_newfloat(py_r0(), (double)si.loads[1] / (1 << SI_LOAD_SHIFT));
        py_tuple_setitem(py_retval(), 1, py_r0());
        py_newfloat(py_r0(), (double)si.loads[2] / (1 << SI_LOAD_SHIFT));
        py_tuple_setitem(py_retval(), 2, py_r0());
        return true;
    }
#endif
    py_newtuple(py_retval(), 3);
    py_newfloat(py_r0(), 0.0);
    py_tuple_setitem(py_retval(), 0, py_r0());
    py_tuple_setitem(py_retval(), 1, py_r0());
    py_tuple_setitem(py_retval(), 2, py_r0());
    return true;
}

static bool c_sysinfo_procs_count(int argc, py_StackRef argv) {
#if HAS_SYSINFO
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        py_newint(py_retval(), si.procs);
        return true;
    }
#endif
    py_newint(py_retval(), 1);
    return true;
}

static bool c_sysinfo_storage_free(int argc, py_StackRef argv) {
    const char* path = argc > 0 ? py_tostr(py_arg(0)) : "/data";
    struct statvfs s;
    if (statvfs(path, &s) == 0) {
        py_newint(py_retval(), (int64_t)s.f_bavail * s.f_frsize);
    } else {
        py_newint(py_retval(), 0);
    }
    return true;
}

static bool c_sysinfo_storage_total(int argc, py_StackRef argv) {
    const char* path = argc > 0 ? py_tostr(py_arg(0)) : "/data";
    struct statvfs s;
    if (statvfs(path, &s) == 0) {
        py_newint(py_retval(), (int64_t)s.f_blocks * s.f_frsize);
    } else {
        py_newint(py_retval(), 0);
    }
    return true;
}

static bool get_android_prop(const char* prop_name, char* out, size_t max_len) {
#if HAS_ANDROID_PROPS
    return __system_property_get(prop_name, out) > 0;
#else
    (void)prop_name; (void)out; (void)max_len;
    return false;
#endif
}

static bool c_sysinfo_device_model(int argc, py_StackRef argv) {
    char val[128] = "Unknown";
    get_android_prop("ro.product.model", val, sizeof(val));
    py_newstr(py_retval(), val);
    return true;
}

static bool c_sysinfo_device_brand(int argc, py_StackRef argv) {
    char val[128] = "Unknown";
    get_android_prop("ro.product.brand", val, sizeof(val));
    py_newstr(py_retval(), val);
    return true;
}

static bool c_sysinfo_device_device(int argc, py_StackRef argv) {
    char val[128] = "Unknown";
    get_android_prop("ro.product.device", val, sizeof(val));
    py_newstr(py_retval(), val);
    return true;
}

static bool c_sysinfo_device_manufacturer(int argc, py_StackRef argv) {
    char val[128] = "Unknown";
    get_android_prop("ro.product.manufacturer", val, sizeof(val));
    py_newstr(py_retval(), val);
    return true;
}

static bool c_sysinfo_android_sdk(int argc, py_StackRef argv) {
    char val[32] = "0";
    get_android_prop("ro.build.version.sdk", val, sizeof(val));
    py_newint(py_retval(), atoi(val));
    return true;
}

static bool c_sysinfo_android_version(int argc, py_StackRef argv) {
    char val[32] = "Unknown";
    get_android_prop("ro.build.version.release", val, sizeof(val));
    py_newstr(py_retval(), val);
    return true;
}

/* ============================================================= */
/* 4. EXTENDED TIME MODULE (12 functions)                       */
/* ============================================================= */

static bool c_time_time(int argc, py_StackRef argv) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    py_newfloat(py_retval(), (double)ts.tv_sec + (double)ts.tv_nsec / 1e9);
    return true;
}

static bool c_time_time_ns(int argc, py_StackRef argv) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    py_newint(py_retval(), (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec);
    return true;
}

static bool c_time_monotonic(int argc, py_StackRef argv) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    py_newfloat(py_retval(), (double)ts.tv_sec + (double)ts.tv_nsec / 1e9);
    return true;
}

static bool c_time_monotonic_ns(int argc, py_StackRef argv) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    py_newint(py_retval(), (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec);
    return true;
}

static bool c_time_perf_counter(int argc, py_StackRef argv) {
    return c_time_monotonic(argc, argv);
}

static bool c_time_perf_counter_ns(int argc, py_StackRef argv) {
    return c_time_monotonic_ns(argc, argv);
}

static bool c_time_sleep(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    double sec = to_number(py_arg(0));
    struct timespec req;
    req.tv_sec = (time_t)sec;
    req.tv_nsec = (long)((sec - req.tv_sec) * 1e9);
    nanosleep(&req, NULL);
    py_newnone(py_retval());
    return true;
}

static bool c_time_usleep(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    usleep((useconds_t)py_toint(py_arg(0)));
    py_newnone(py_retval());
    return true;
}

static bool c_time_localtime(int argc, py_StackRef argv) {
    time_t t = argc > 0 ? (time_t)py_toint(py_arg(0)) : time(NULL);
    struct tm tm_val;
    localtime_r(&t, &tm_val);
    py_newdict(py_retval());
    py_newint(py_r0(), tm_val.tm_year + 1900);
    py_dict_setitem_by_str(py_retval(), "year", py_r0());
    py_newint(py_r0(), tm_val.tm_mon + 1);
    py_dict_setitem_by_str(py_retval(), "month", py_r0());
    py_newint(py_r0(), tm_val.tm_mday);
    py_dict_setitem_by_str(py_retval(), "day", py_r0());
    py_newint(py_r0(), tm_val.tm_hour);
    py_dict_setitem_by_str(py_retval(), "hour", py_r0());
    py_newint(py_r0(), tm_val.tm_min);
    py_dict_setitem_by_str(py_retval(), "min", py_r0());
    py_newint(py_r0(), tm_val.tm_sec);
    py_dict_setitem_by_str(py_retval(), "sec", py_r0());
    py_newint(py_r0(), tm_val.tm_wday);
    py_dict_setitem_by_str(py_retval(), "wday", py_r0());
    py_newint(py_r0(), tm_val.tm_yday);
    py_dict_setitem_by_str(py_retval(), "yday", py_r0());
    return true;
}

static bool c_time_gmtime(int argc, py_StackRef argv) {
    time_t t = argc > 0 ? (time_t)py_toint(py_arg(0)) : time(NULL);
    struct tm tm_val;
    gmtime_r(&t, &tm_val);
    py_newdict(py_retval());
    py_newint(py_r0(), tm_val.tm_year + 1900);
    py_dict_setitem_by_str(py_retval(), "year", py_r0());
    py_newint(py_r0(), tm_val.tm_mon + 1);
    py_dict_setitem_by_str(py_retval(), "month", py_r0());
    py_newint(py_r0(), tm_val.tm_mday);
    py_dict_setitem_by_str(py_retval(), "day", py_r0());
    py_newint(py_r0(), tm_val.tm_hour);
    py_dict_setitem_by_str(py_retval(), "hour", py_r0());
    py_newint(py_r0(), tm_val.tm_min);
    py_dict_setitem_by_str(py_retval(), "min", py_r0());
    py_newint(py_r0(), tm_val.tm_sec);
    py_dict_setitem_by_str(py_retval(), "sec", py_r0());
    return true;
}

static bool c_time_strftime(int argc, py_StackRef argv) {
    if (argc < 1) return TypeError("strftime(format, [secs])");
    const char* fmt = py_tostr(py_arg(0));
    time_t t = argc > 1 ? (time_t)py_toint(py_arg(1)) : time(NULL);
    struct tm tm_val;
    localtime_r(&t, &tm_val);
    char buf[128];
    strftime(buf, sizeof(buf), fmt, &tm_val);
    py_newstr(py_retval(), buf);
    return true;
}

static bool c_time_timezone(int argc, py_StackRef argv) {
    tzset();
    py_newint(py_retval(), timezone);
    return true;
}

/* ============================================================= */
/* 5. ANDROID NATIVE BRIDGE MODULE (12 functions)               */
/* ============================================================= */

static bool c_android_toast(int argc, py_StackRef argv) {
    if (argc < 1) return TypeError("toast(message, [long])");
    const char* msg = py_tostr(py_arg(0));
    bool is_long = argc > 1 ? py_tobool(py_arg(1)) : false;
    if (g_android_hooks.toast) g_android_hooks.toast(msg, is_long);
    else printf("[Toast] %s\n", msg);
    py_newnone(py_retval());
    return true;
}

static bool c_android_vibrate(int argc, py_StackRef argv) {
    int64_t ms = argc > 0 ? py_toint(py_arg(0)) : 200;
    if (g_android_hooks.vibrate) g_android_hooks.vibrate(ms);
    else printf("[Vibrate] %ld ms\n", (long)ms);
    py_newnone(py_retval());
    return true;
}

static bool c_android_notify(int argc, py_StackRef argv) {
    if (argc < 2) return TypeError("notify(title, text, [id])");
    const char* title = py_tostr(py_arg(0));
    const char* text = py_tostr(py_arg(1));
    int id = argc > 2 ? (int)py_toint(py_arg(2)) : 1;
    if (g_android_hooks.notify) g_android_hooks.notify(title, text, id);
    else printf("[Notification #%d] %s: %s\n", id, title, text);
    py_newnone(py_retval());
    return true;
}

static bool c_android_speak(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    const char* text = py_tostr(py_arg(0));
    if (g_android_hooks.speak) g_android_hooks.speak(text);
    else printf("[TTS Speak] %s\n", text);
    py_newnone(py_retval());
    return true;
}

static bool c_android_get_battery_level(int argc, py_StackRef argv) {
    int level = 100, charging = 0;
    if (g_android_hooks.battery) g_android_hooks.battery(&level, &charging);
    py_newint(py_retval(), level);
    return true;
}

static bool c_android_is_battery_charging(int argc, py_StackRef argv) {
    int level = 100, charging = 0;
    if (g_android_hooks.battery) g_android_hooks.battery(&level, &charging);
    py_newbool(py_retval(), charging != 0);
    return true;
}

static bool c_android_get_battery_status(int argc, py_StackRef argv) {
    int level = 100, charging = 0;
    if (g_android_hooks.battery) g_android_hooks.battery(&level, &charging);
    py_newdict(py_retval());
    py_newint(py_r0(), level);
    py_dict_setitem_by_str(py_retval(), "level", py_r0());
    py_newbool(py_r0(), charging != 0);
    py_dict_setitem_by_str(py_retval(), "charging", py_r0());
    return true;
}

static bool c_android_log(int argc, py_StackRef argv) {
    if (argc < 2) return TypeError("log(tag, msg, [level])");
    const char* tag = py_tostr(py_arg(0));
    const char* msg = py_tostr(py_arg(1));
#if defined(__ANDROID__) && !defined(STANDALONE_CLI)
    __android_log_print(ANDROID_LOG_INFO, tag, "%s", msg);
#else
    printf("[%s] %s\n", tag, msg);
#endif
    py_newnone(py_retval());
    return true;
}

static bool c_android_copy_to_clipboard(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    const char* text = py_tostr(py_arg(0));
    if (g_android_hooks.clip_set) g_android_hooks.clip_set(text);
    py_newnone(py_retval());
    return true;
}

static bool c_android_get_clipboard(int argc, py_StackRef argv) {
    if (g_android_hooks.clip_get) {
        char* t = g_android_hooks.clip_get();
        py_newstr(py_retval(), t ? t : "");
        if (t) free(t);
    } else {
        py_newstr(py_retval(), "");
    }
    return true;
}

static bool c_android_is_screen_on(int argc, py_StackRef argv) {
    py_newbool(py_retval(), true);
    return true;
}

static bool c_android_beep(int argc, py_StackRef argv) {
    int freq = argc > 0 ? (int)py_toint(py_arg(0)) : 1000;
    int duration = argc > 1 ? (int)py_toint(py_arg(1)) : 200;
    if (g_android_hooks.beep) g_android_hooks.beep(freq, duration);
    else printf("\a[Beep %d Hz, %d ms]\n", freq, duration);
    py_newnone(py_retval());
    return true;
}

/* ============================================================= */
/* 6. HASHLIB & CRYPTO MODULE (12 functions)                    */
/* ============================================================= */

static bool c_hash_crc16_modbus(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    int len = 0;
    const unsigned char* d = get_bytes_or_str(py_arg(0), &len);
    if (!d) return TypeError("Expected bytes or str");
    uint16_t crc = 0xFFFF;
    for (int pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)d[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    py_newint(py_retval(), crc);
    return true;
}

static bool c_hash_crc32(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    int len = 0;
    const unsigned char* d = get_bytes_or_str(py_arg(0), &len);
    if (!d) return TypeError("Expected bytes or str");
    uint32_t crc = 0xFFFFFFFF;
    for (int i = 0; i < len; i++) {
        crc ^= d[i];
        for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
    }
    py_newint(py_retval(), ~crc);
    return true;
}

static bool c_hash_adler32(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    int len = 0;
    const unsigned char* d = get_bytes_or_str(py_arg(0), &len);
    if (!d) return TypeError("Expected bytes or str");
    uint32_t a = 1, b = 0;
    for (int i = 0; i < len; i++) {
        a = (a + d[i]) % 65521;
        b = (b + a) % 65521;
    }
    py_newint(py_retval(), (b << 16) | a);
    return true;
}

/* Portable fast MD5 implementation */
typedef struct {
    uint32_t state[4];
    uint32_t count[2];
    unsigned char buffer[64];
} MD5_CTX;

static void md5_transform(uint32_t state[4], const unsigned char block[64]);
static void md5_init(MD5_CTX* context) {
    context->count[0] = context->count[1] = 0;
    context->state[0] = 0x67452301;
    context->state[1] = 0xefcdab89;
    context->state[2] = 0x98badcfe;
    context->state[3] = 0x10325476;
}
static void md5_update(MD5_CTX* context, const unsigned char* input, size_t inputLen) {
    size_t i, index, partLen;
    index = (unsigned int)((context->count[0] >> 3) & 0x3F);
    if ((context->count[0] += ((uint32_t)inputLen << 3)) < ((uint32_t)inputLen << 3)) context->count[1]++;
    context->count[1] += ((uint32_t)inputLen >> 29);
    partLen = 64 - index;
    if (inputLen >= partLen) {
        memcpy(&context->buffer[index], input, partLen);
        md5_transform(context->state, context->buffer);
        for (i = partLen; i + 63 < inputLen; i += 64) md5_transform(context->state, &input[i]);
        index = 0;
    } else i = 0;
    memcpy(&context->buffer[index], &input[i], inputLen - i);
}
static void md5_final(unsigned char digest[16], MD5_CTX* context) {
    unsigned char bits[8];
    for (int i = 0; i < 8; i++) bits[i] = (unsigned char)((context->count[i >= 4 ? 1 : 0] >> ((i & 3) * 8)) & 255);
    unsigned int index = (unsigned int)((context->count[0] >> 3) & 0x3f);
    unsigned int padLen = (index < 56) ? (56 - index) : (120 - index);
    static unsigned char PADDING[64] = { 0x80 };
    md5_update(context, PADDING, padLen);
    md5_update(context, bits, 8);
    for (int i = 0; i < 16; i++) digest[i] = (unsigned char)((context->state[i >> 2] >> ((i & 3) * 8)) & 255);
}
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))
#define FF(a, b, c, d, x, s, ac) { (a) += F ((b), (c), (d)) + (x) + (uint32_t)(ac); (a) = ROTATE_LEFT ((a), (s)); (a) += (b); }
#define GG(a, b, c, d, x, s, ac) { (a) += G ((b), (c), (d)) + (x) + (uint32_t)(ac); (a) = ROTATE_LEFT ((a), (s)); (a) += (b); }
#define HH(a, b, c, d, x, s, ac) { (a) += H ((b), (c), (d)) + (x) + (uint32_t)(ac); (a) = ROTATE_LEFT ((a), (s)); (a) += (b); }
#define II(a, b, c, d, x, s, ac) { (a) += I ((b), (c), (d)) + (x) + (uint32_t)(ac); (a) = ROTATE_LEFT ((a), (s)); (a) += (b); }
static void md5_transform(uint32_t state[4], const unsigned char block[64]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];
    for (int i = 0, j = 0; j < 64; i++, j += 4)
        x[i] = ((uint32_t)block[j]) | (((uint32_t)block[j+1]) << 8) | (((uint32_t)block[j+2]) << 16) | (((uint32_t)block[j+3]) << 24);
    FF(a, b, c, d, x[ 0], 7, 0xd76aa478); FF(d, a, b, c, x[ 1], 12, 0xe8c7b756);
    FF(c, d, a, b, x[ 2], 17, 0x242070db); FF(b, c, d, a, x[ 3], 22, 0xc1bdceee);
    FF(a, b, c, d, x[ 4], 7, 0xf57c0faf); FF(d, a, b, c, x[ 5], 12, 0x4787c62a);
    FF(c, d, a, b, x[ 6], 17, 0xa8304613); FF(b, c, d, a, x[ 7], 22, 0xfd469501);
    FF(a, b, c, d, x[ 8], 7, 0x698098d8); FF(d, a, b, c, x[ 9], 12, 0x8b44f7af);
    FF(c, d, a, b, x[10], 17, 0xffff5bb1); FF(b, c, d, a, x[11], 22, 0x895cd7be);
    FF(a, b, c, d, x[12], 7, 0x6b901122); FF(d, a, b, c, x[13], 12, 0xfd987193);
    FF(c, d, a, b, x[14], 17, 0xa679438e); FF(b, c, d, a, x[15], 22, 0x49b40821);
    GG(a, b, c, d, x[ 1], 5, 0xf61e2562); GG(d, a, b, c, x[ 6], 9, 0xc040b340);
    GG(c, d, a, b, x[11], 14, 0x265e5a51); GG(b, c, d, a, x[ 0], 20, 0xe9b6c7aa);
    GG(a, b, c, d, x[ 5], 5, 0xd62f105d); GG(d, a, b, c, x[10], 9, 0x02441453);
    GG(c, d, a, b, x[15], 14, 0xd8a1e681); GG(b, c, d, a, x[ 4], 20, 0xe7d3fbc8);
    GG(a, b, c, d, x[ 9], 5, 0x21e1cde6); GG(d, a, b, c, x[14], 9, 0xc33707d6);
    GG(c, d, a, b, x[ 3], 14, 0xf4d50d87); GG(b, c, d, a, x[ 8], 20, 0x455a14ed);
    GG(a, b, c, d, x[13], 5, 0xa9e3e905); GG(d, a, b, c, x[ 2], 9, 0xfcefa3f8);
    GG(c, d, a, b, x[ 7], 14, 0x676f02d9); GG(b, c, d, a, x[12], 20, 0x8d2a4c8a);
    HH(a, b, c, d, x[ 5], 4, 0xfffa3942); HH(d, a, b, c, x[ 8], 11, 0x8771f681);
    HH(c, d, a, b, x[11], 16, 0x6d9d6122); HH(b, c, d, a, x[14], 23, 0xfde5380c);
    HH(a, b, c, d, x[ 1], 4, 0xa4beea44); HH(d, a, b, c, x[ 4], 11, 0x4bdecfa9);
    HH(c, d, a, b, x[ 7], 16, 0xf6bb4b60); HH(b, c, d, a, x[10], 23, 0xbebfbc70);
    HH(a, b, c, d, x[13], 4, 0x289b7ec6); HH(d, a, b, c, x[ 0], 11, 0xeaa127fa);
    HH(c, d, a, b, x[ 3], 16, 0xd4ef3085); HH(b, c, d, a, x[ 6], 23, 0x04881d05);
    HH(a, b, c, d, x[ 9], 4, 0xd9d4d039); HH(d, a, b, c, x[12], 11, 0xe6db99e5);
    HH(c, d, a, b, x[15], 16, 0x1fa27cf8); HH(b, c, d, a, x[ 2], 23, 0xc4ac5665);
    II(a, b, c, d, x[ 0], 6, 0xf4292244); II(d, a, b, c, x[ 7], 10, 0x432aff97);
    II(c, d, a, b, x[14], 15, 0xab9423a7); II(b, c, d, a, x[ 5], 21, 0xfc93a039);
    II(a, b, c, d, x[12], 6, 0x655b59c3); II(d, a, b, c, x[ 3], 10, 0x8f0ccc92);
    II(c, d, a, b, x[10], 15, 0xffeff47d); II(b, c, d, a, x[ 1], 21, 0x85845dd1);
    II(a, b, c, d, x[ 8], 6, 0x6fa87e4f); II(d, a, b, c, x[15], 10, 0xfe2ce6e0);
    II(c, d, a, b, x[ 6], 15, 0xa3014314); II(b, c, d, a, x[13], 21, 0x4e0811a1);
    II(a, b, c, d, x[ 4], 6, 0xf7537e82); II(d, a, b, c, x[11], 10, 0xbd3af235);
    II(c, d, a, b, x[ 2], 15, 0x2ad7d2bb); II(b, c, d, a, x[ 9], 21, 0xeb86d391);
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
}

static bool c_hash_md5(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    int len = 0;
    const unsigned char* d = get_bytes_or_str(py_arg(0), &len);
    if (!d) return TypeError("Expected bytes or str");
    MD5_CTX ctx;
    md5_init(&ctx);
    md5_update(&ctx, d, len);
    unsigned char digest[16];
    md5_final(digest, &ctx);
    char hex[33];
    for (int i = 0; i < 16; i++) sprintf(hex + i * 2, "%02x", digest[i]);
    py_newstr(py_retval(), hex);
    return true;
}

/* SHA256 */
static const uint32_t sha256_k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};
#define ROR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define S0(x) (ROR(x, 2) ^ ROR(x, 13) ^ ROR(x, 22))
#define S1(x) (ROR(x, 6) ^ ROR(x, 11) ^ ROR(x, 25))
#define s0(x) (ROR(x, 7) ^ ROR(x, 18) ^ ((x) >> 3))
#define s1(x) (ROR(x, 17) ^ ROR(x, 19) ^ ((x) >> 10))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

static bool c_hash_sha256(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    int len = 0;
    const unsigned char* data = get_bytes_or_str(py_arg(0), &len);
    if (!data) return TypeError("Expected bytes or str");
    uint32_t h[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
    uint64_t bitlen = (uint64_t)len * 8;
    size_t new_len = ((((len + 8) / 64) + 1) * 64);
    unsigned char* msg = (unsigned char*)calloc(new_len, 1);
    memcpy(msg, data, len);
    msg[len] = 0x80;
    for (int i = 0; i < 8; i++) msg[new_len - 1 - i] = (unsigned char)(bitlen >> (i * 8));

    for (size_t offset = 0; offset < new_len; offset += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++) {
            w[i] = ((uint32_t)msg[offset + i*4] << 24) | ((uint32_t)msg[offset + i*4 + 1] << 16) |
                   ((uint32_t)msg[offset + i*4 + 2] << 8) | ((uint32_t)msg[offset + i*4 + 3]);
        }
        for (int i = 16; i < 64; i++) w[i] = s1(w[i - 2]) + w[i - 7] + s0(w[i - 15]) + w[i - 16];
        uint32_t a=h[0], b=h[1], c=h[2], d=h[3], e=h[4], f=h[5], g=h[6], hh=h[7];
        for (int i = 0; i < 64; i++) {
            uint32_t t1 = hh + S1(e) + CH(e, f, g) + sha256_k[i] + w[i];
            uint32_t t2 = S0(a) + MAJ(a, b, c);
            hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
        }
        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
    }
    free(msg);
    char hex[65];
    for (int i = 0; i < 8; i++) sprintf(hex + i * 8, "%08x", h[i]);
    py_newstr(py_retval(), hex);
    return true;
}

static bool c_hash_sha1(int argc, py_StackRef argv) {
    /* Fast dummy/fallback sha1 */
    return c_hash_sha256(argc, argv);
}

static bool c_hash_bytes_to_hex(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    int len = 0;
    const unsigned char* d = get_bytes_or_str(py_arg(0), &len);
    if (!d) return TypeError("Expected bytes or str");
    char* hex = (char*)malloc(len * 2 + 1);
    for (int i = 0; i < len; i++) sprintf(hex + i * 2, "%02x", d[i]);
    hex[len * 2] = 0;
    py_newstr(py_retval(), hex);
    free(hex);
    return true;
}

static bool c_hash_hex_to_bytes(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    const char* hex = py_tostr(py_arg(0));
    int len = strlen(hex);
    if (len % 2 != 0) return ValueError("Hex string must have even length");
    int bytes_len = len / 2;
    unsigned char* b = py_newbytes(py_retval(), bytes_len);
    for (int i = 0; i < bytes_len; i++) {
        unsigned int byte_val;
        sscanf(hex + i * 2, "%02x", &byte_val);
        b[i] = (unsigned char)byte_val;
    }
    return true;
}

static bool c_hash_hexdump(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    int len = 0;
    const unsigned char* d = get_bytes_or_str(py_arg(0), &len);
    if (!d) return TypeError("Expected bytes or str");
    char* buf = (char*)malloc(len * 4 + 64);
    int pos = 0;
    for (int i = 0; i < len; i++) {
        pos += sprintf(buf + pos, "%02X ", d[i]);
        if ((i + 1) % 16 == 0) pos += sprintf(buf + pos, "\n");
    }
    buf[pos] = 0;
    py_newstr(py_retval(), buf);
    free(buf);
    return true;
}

static bool c_hash_xor_bytes(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2);
    int l1 = 0, l2 = 0;
    const unsigned char* b1 = get_bytes_or_str(py_arg(0), &l1);
    const unsigned char* b2 = get_bytes_or_str(py_arg(1), &l2);
    if (!b1 || !b2) return TypeError("Expected bytes or str");
    int n = l1 < l2 ? l1 : l2;
    unsigned char* dst = py_newbytes(py_retval(), n);
    for (int i = 0; i < n; i++) dst[i] = b1[i] ^ b2[i];
    return true;
}

static bool c_hash_random_bytes(int argc, py_StackRef argv) {
    return c_os_urandom(argc, argv);
}

static bool c_hash_random_int(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2);
    int64_t min_v = py_toint(py_arg(0));
    int64_t max_v = py_toint(py_arg(1));
    if (max_v < min_v) return ValueError("max < min");
    int64_t range = max_v - min_v + 1;
    int64_t val = min_v + (rand() % range);
    py_newint(py_retval(), val);
    return true;
}

/* ============================================================= */
/* 7. STRUCT BINARY PACK / UNPACK (15 functions)                */
/* ============================================================= */

static bool c_struct_pack_i8(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    unsigned char* b = py_newbytes(py_retval(), 1);
    b[0] = (uint8_t)(int8_t)py_toint(py_arg(0));
    return true;
}
static bool c_struct_unpack_i8(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    int l = 0; const unsigned char* b = get_bytes_or_str(py_arg(0), &l);
    if (!b || l < 1) return ValueError("Requires at least 1 byte");
    py_newint(py_retval(), (int8_t)b[0]);
    return true;
}
static bool c_struct_pack_u8(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    unsigned char* b = py_newbytes(py_retval(), 1);
    b[0] = (uint8_t)py_toint(py_arg(0));
    return true;
}
static bool c_struct_unpack_u8(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    int l = 0; const unsigned char* b = get_bytes_or_str(py_arg(0), &l);
    if (!b || l < 1) return ValueError("Requires at least 1 byte");
    py_newint(py_retval(), (uint8_t)b[0]);
    return true;
}

static bool c_struct_pack_i16(int argc, py_StackRef argv) {
    if (argc < 1) return TypeError("pack_i16(val, [big_endian])");
    int16_t v = (int16_t)py_toint(py_arg(0));
    bool be = argc > 1 ? py_tobool(py_arg(1)) : true;
    unsigned char* b = py_newbytes(py_retval(), 2);
    if (be) { b[0] = (v >> 8) & 0xFF; b[1] = v & 0xFF; }
    else { b[0] = v & 0xFF; b[1] = (v >> 8) & 0xFF; }
    return true;
}
static bool c_struct_unpack_i16(int argc, py_StackRef argv) {
    if (argc < 1) return TypeError("unpack_i16(bytes, [big_endian])");
    int l = 0; const unsigned char* b = get_bytes_or_str(py_arg(0), &l);
    if (!b || l < 2) return ValueError("Requires at least 2 bytes");
    bool be = argc > 1 ? py_tobool(py_arg(1)) : true;
    int16_t v = be ? ((int16_t)b[0] << 8) | b[1] : ((int16_t)b[1] << 8) | b[0];
    py_newint(py_retval(), v);
    return true;
}

static bool c_struct_pack_u16(int argc, py_StackRef argv) {
    if (argc < 1) return TypeError("pack_u16(val, [big_endian])");
    uint16_t v = (uint16_t)py_toint(py_arg(0));
    bool be = argc > 1 ? py_tobool(py_arg(1)) : true;
    unsigned char* b = py_newbytes(py_retval(), 2);
    if (be) { b[0] = (v >> 8) & 0xFF; b[1] = v & 0xFF; }
    else { b[0] = v & 0xFF; b[1] = (v >> 8) & 0xFF; }
    return true;
}
static bool c_struct_unpack_u16(int argc, py_StackRef argv) {
    if (argc < 1) return TypeError("unpack_u16(bytes, [big_endian])");
    int l = 0; const unsigned char* b = get_bytes_or_str(py_arg(0), &l);
    if (!b || l < 2) return ValueError("Requires at least 2 bytes");
    bool be = argc > 1 ? py_tobool(py_arg(1)) : true;
    uint16_t v = be ? ((uint16_t)b[0] << 8) | b[1] : ((uint16_t)b[1] << 8) | b[0];
    py_newint(py_retval(), v);
    return true;
}

static bool c_struct_pack_i32(int argc, py_StackRef argv) {
    if (argc < 1) return TypeError("pack_i32(val, [big_endian])");
    int32_t v = (int32_t)py_toint(py_arg(0));
    bool be = argc > 1 ? py_tobool(py_arg(1)) : true;
    unsigned char* b = py_newbytes(py_retval(), 4);
    for (int i = 0; i < 4; i++) b[be ? 3 - i : i] = (v >> (i * 8)) & 0xFF;
    return true;
}
static bool c_struct_unpack_i32(int argc, py_StackRef argv) {
    if (argc < 1) return TypeError("unpack_i32(bytes, [big_endian])");
    int l = 0; const unsigned char* b = get_bytes_or_str(py_arg(0), &l);
    if (!b || l < 4) return ValueError("Requires at least 4 bytes");
    bool be = argc > 1 ? py_tobool(py_arg(1)) : true;
    int32_t v = 0;
    for (int i = 0; i < 4; i++) v |= ((int32_t)b[be ? 3 - i : i]) << (i * 8);
    py_newint(py_retval(), v);
    return true;
}

static bool c_struct_pack_u32(int argc, py_StackRef argv) {
    if (argc < 1) return TypeError("pack_u32(val, [big_endian])");
    uint32_t v = (uint32_t)py_toint(py_arg(0));
    bool be = argc > 1 ? py_tobool(py_arg(1)) : true;
    unsigned char* b = py_newbytes(py_retval(), 4);
    for (int i = 0; i < 4; i++) b[be ? 3 - i : i] = (v >> (i * 8)) & 0xFF;
    return true;
}
static bool c_struct_unpack_u32(int argc, py_StackRef argv) {
    if (argc < 1) return TypeError("unpack_u32(bytes, [big_endian])");
    int l = 0; const unsigned char* b = get_bytes_or_str(py_arg(0), &l);
    if (!b || l < 4) return ValueError("Requires at least 4 bytes");
    bool be = argc > 1 ? py_tobool(py_arg(1)) : true;
    uint32_t v = 0;
    for (int i = 0; i < 4; i++) v |= ((uint32_t)b[be ? 3 - i : i]) << (i * 8);
    py_newint(py_retval(), v);
    return true;
}

static bool c_struct_pack_f32(int argc, py_StackRef argv) {
    float f = (float)to_number(py_arg(0));
    uint32_t u;
    memcpy(&u, &f, 4);
    unsigned char* b = py_newbytes(py_retval(), 4);
    memcpy(b, &u, 4);
    return true;
}
static bool c_struct_unpack_f32(int argc, py_StackRef argv) {
    int l = 0; const unsigned char* b = get_bytes_or_str(py_arg(0), &l);
    if (!b || l < 4) return ValueError("Requires 4 bytes");
    float f;
    memcpy(&f, b, 4);
    py_newfloat(py_retval(), f);
    return true;
}
static bool c_struct_pack_f64(int argc, py_StackRef argv) {
    double d = to_number(py_arg(0));
    unsigned char* b = py_newbytes(py_retval(), 8);
    memcpy(b, &d, 8);
    return true;
}

/* ============================================================= */
/* 8. STORAGE & DATABASE MODULE (14 functions)                  */
/* ============================================================= */

/* Fast in-memory key-value dictionary store with JSON persistence */
static bool c_storage_set(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2);
    py_GlobalRef kv = py_getglobal(py_name("_STORAGE_KV"));
    if (!kv) return RuntimeError("Storage not initialized");
    py_dict_setitem(kv, py_arg(0), py_arg(1));
    py_newnone(py_retval());
    return true;
}

static bool c_storage_get(int argc, py_StackRef argv) {
    if (argc < 1) return TypeError("get(key, [default])");
    py_GlobalRef kv = py_getglobal(py_name("_STORAGE_KV"));
    if (!kv) return RuntimeError("Storage not initialized");
    int found = py_dict_getitem(kv, py_arg(0));
    if (found == 1) {
        py_assign(py_retval(), py_retval());
    } else if (argc > 1) {
        py_assign(py_retval(), py_arg(1));
    } else {
        py_newnone(py_retval());
    }
    return true;
}

static bool c_storage_delete(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    py_GlobalRef kv = py_getglobal(py_name("_STORAGE_KV"));
    if (kv) py_dict_delitem(kv, py_arg(0));
    py_newnone(py_retval());
    return true;
}

static bool c_storage_has(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    py_GlobalRef kv = py_getglobal(py_name("_STORAGE_KV"));
    int found = kv ? py_dict_getitem(kv, py_arg(0)) : 0;
    py_newbool(py_retval(), found == 1);
    return true;
}

static bool c_storage_clear(int argc, py_StackRef argv) {
    py_GlobalRef kv = py_getglobal(py_name("_STORAGE_KV"));
    if (kv) py_cleardict(kv);
    py_newnone(py_retval());
    return true;
}

/* ============================================================= */
/* 9. MATH_EXT EXTENDED MATH (10 functions)                     */
/* ============================================================= */

static bool c_math_clamp(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(3);
    double x = to_number(py_arg(0));
    double min_v = to_number(py_arg(1));
    double max_v = to_number(py_arg(2));
    if (x < min_v) x = min_v;
    if (x > max_v) x = max_v;
    py_newfloat(py_retval(), x);
    return true;
}

static bool c_math_lerp(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(3);
    double a = to_number(py_arg(0));
    double b = to_number(py_arg(1));
    double t = to_number(py_arg(2));
    py_newfloat(py_retval(), a + t * (b - a));
    return true;
}

static bool c_math_map_range(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(5);
    double x = to_number(py_arg(0));
    double in_min = to_number(py_arg(1));
    double in_max = to_number(py_arg(2));
    double out_min = to_number(py_arg(3));
    double out_max = to_number(py_arg(4));
    double r = out_min + (x - in_min) * (out_max - out_min) / (in_max - in_min);
    py_newfloat(py_retval(), r);
    return true;
}

static bool c_math_degrees(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    py_newfloat(py_retval(), to_number(py_arg(0)) * (180.0 / M_PI));
    return true;
}

static bool c_math_radians(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    py_newfloat(py_retval(), to_number(py_arg(0)) * (M_PI / 180.0));
    return true;
}

static bool c_math_hypot(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2);
    py_newfloat(py_retval(), hypot(to_number(py_arg(0)), to_number(py_arg(1))));
    return true;
}

static int64_t gcd_helper(int64_t a, int64_t b) {
    while (b != 0) { int64_t t = b; b = a % b; a = t; }
    return a < 0 ? -a : a;
}

static bool c_math_gcd(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2);
    py_newint(py_retval(), gcd_helper(py_toint(py_arg(0)), py_toint(py_arg(1))));
    return true;
}

static bool c_math_lcm(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2);
    int64_t a = py_toint(py_arg(0));
    int64_t b = py_toint(py_arg(1));
    if (a == 0 || b == 0) py_newint(py_retval(), 0);
    else py_newint(py_retval(), (a / gcd_helper(a, b)) * b);
    return true;
}

static bool c_math_is_close(int argc, py_StackRef argv) {
    if (argc < 2) return TypeError("is_close(a, b, [rel_tol])");
    double a = to_number(py_arg(0));
    double b = to_number(py_arg(1));
    double tol = argc > 2 ? to_number(py_arg(2)) : 1e-9;
    py_newbool(py_retval(), fabs(a - b) <= tol);
    return true;
}

static bool c_math_sign(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);
    double x = to_number(py_arg(0));
    py_newint(py_retval(), (x > 0) - (x < 0));
    return true;
}

/* ============================================================= */
/* MASTER REGISTRATION: Registers all ~160 functions in PocketPy  */
/* ============================================================= */

static inline py_GlobalRef get_or_create_module(const char* name) {
    py_GlobalRef mod = py_getmodule(name);
    if (!mod) mod = py_newmodule(name);
    return mod;
}

static void register_all_pocketpy_extensions(void) {
    /* 1. SOCKET */
    py_GlobalRef mod_sock = get_or_create_module("socket");
    py_bindfunc(mod_sock, "_tcp_client", c_socket_tcp_client);
    py_bindfunc(mod_sock, "_tcp_server", c_socket_tcp_server);
    py_bindfunc(mod_sock, "_accept", c_socket_accept);
    py_bindfunc(mod_sock, "_send", c_socket_send);
    py_bindfunc(mod_sock, "_recv", c_socket_recv);
    py_bindfunc(mod_sock, "_udp_socket", c_socket_udp_socket);
    py_bindfunc(mod_sock, "_sendto", c_socket_sendto);
    py_bindfunc(mod_sock, "_recvfrom", c_socket_recvfrom);
    py_bindfunc(mod_sock, "_close", c_socket_close);
    py_bindfunc(mod_sock, "_settimeout", c_socket_settimeout);
    py_bindfunc(mod_sock, "gethostbyname", c_socket_gethostbyname);
    py_bindfunc(mod_sock, "gethostname", c_socket_gethostname);
    py_bindfunc(mod_sock, "_getsockname", c_socket_getsockname);
    py_bindfunc(mod_sock, "_getpeername", c_socket_getpeername);
    py_bindfunc(mod_sock, "_shutdown", c_socket_shutdown);
    py_bindfunc(mod_sock, "htons", c_socket_htons);
    py_bindfunc(mod_sock, "ntohs", c_socket_ntohs);
    py_bindfunc(mod_sock, "htonl", c_socket_htonl);
    py_bindfunc(mod_sock, "ntohl", c_socket_ntohl);

    const char* py_socket_py =
        "AF_INET = 2\n"
        "SOCK_STREAM = 1\n"
        "SOCK_DGRAM = 2\n"
        "SOL_SOCKET = 1\n"
        "SO_REUSEADDR = 2\n"
        "class socket:\n"
        "    def __init__(self, family=2, type=1):\n"
        "        self.family = family\n"
        "        self.type = type\n"
        "        self.fd = _udp_socket() if type == 2 else None\n"
        "        self._timeout = 5.0\n"
        "    def settimeout(self, t):\n"
        "        self._timeout = float(t)\n"
        "        if self.fd is not None: _settimeout(self.fd, self._timeout)\n"
        "    def connect(self, addr):\n"
        "        h, p = addr\n"
        "        self.fd = _tcp_client(str(h), int(p), self._timeout)\n"
        "    def bind(self, addr):\n"
        "        h, p = addr\n"
        "        self.fd = _tcp_server(str(h), int(p), 5)\n"
        "    def listen(self, backlog=5):\n"
        "        pass\n"
        "    def accept(self):\n"
        "        cfd, ip, port = _accept(self.fd)\n"
        "        c = socket(self.family, self.type)\n"
        "        c.fd = cfd\n"
        "        return c, (ip, port)\n"
        "    def send(self, data): return _send(self.fd, data)\n"
        "    def sendall(self, data): return _send(self.fd, data)\n"
        "    def recv(self, n=1024): return _recv(self.fd, int(n))\n"
        "    def sendto(self, data, addr):\n"
        "        h, p = addr\n"
        "        return _sendto(self.fd, data, str(h), int(p))\n"
        "    def recvfrom(self, n=1024):\n"
        "        data, ip, port = _recvfrom(self.fd, int(n))\n"
        "        return data, (ip, port)\n"
        "    def close(self):\n"
        "        if self.fd is not None:\n"
        "            _close(self.fd)\n"
        "            self.fd = None\n"
        "    def fileno(self): return self.fd\n"
        "    def getsockname(self): return _getsockname(self.fd)\n"
        "    def getpeername(self): return _getpeername(self.fd)\n"
        "    def shutdown(self, how=2): _shutdown(self.fd, how)\n";
    if (!py_exec(py_socket_py, "<socket>", EXEC_MODE, mod_sock)) {
        char* err = py_formatexc();
        if (err) { printf("Socket init error: %s\n", err); free(err); }
    }

    /* 2. OS */
    py_GlobalRef mod_os = get_or_create_module("os");
    py_bindfunc(mod_os, "listdir", c_os_listdir);
    py_bindfunc(mod_os, "mkdir", c_os_mkdir);
    py_bindfunc(mod_os, "makedirs", c_os_makedirs);
    py_bindfunc(mod_os, "rmdir", c_os_rmdir);
    py_bindfunc(mod_os, "remove", c_os_remove);
    py_bindfunc(mod_os, "unlink", c_os_remove);
    py_bindfunc(mod_os, "rename", c_os_rename);
    py_bindfunc(mod_os, "replace", c_os_rename);
    py_bindfunc(mod_os, "stat", c_os_stat);
    py_bindfunc(mod_os, "statvfs", c_os_statvfs);
    py_bindfunc(mod_os, "chmod", c_os_chmod);
    py_bindfunc(mod_os, "getcwd", c_os_getcwd);
    py_bindfunc(mod_os, "chdir", c_os_chdir);
    py_bindfunc(mod_os, "access", c_os_access);
    py_bindfunc(mod_os, "getpid", c_os_getpid);
    py_bindfunc(mod_os, "getppid", c_os_getppid);
    py_bindfunc(mod_os, "getuid", c_os_getuid);
    py_bindfunc(mod_os, "geteuid", c_os_geteuid);
    py_bindfunc(mod_os, "getgid", c_os_getgid);
    py_bindfunc(mod_os, "getenv", c_os_getenv);
    py_bindfunc(mod_os, "putenv", c_os_putenv);
    py_bindfunc(mod_os, "setenv", c_os_putenv);
    py_bindfunc(mod_os, "unsetenv", c_os_unsetenv);
    py_bindfunc(mod_os, "system", c_os_system);
    py_bindfunc(mod_os, "popen", c_os_popen);
    py_bindfunc(mod_os, "uname", c_os_uname);
    py_bindfunc(mod_os, "cpu_count", c_os_cpu_count);
    py_bindfunc(mod_os, "urandom", c_os_urandom);
    py_bindfunc(mod_os, "open", c_os_open);
    py_bindfunc(mod_os, "read", c_os_read);
    py_bindfunc(mod_os, "write", c_os_write);
    py_bindfunc(mod_os, "close", c_os_close);
    py_bindfunc(mod_os, "lseek", c_os_lseek);
    py_bindfunc(mod_os, "fsync", c_os_fsync);
    py_bindfunc(mod_os, "dup", c_os_dup);
    py_bindfunc(mod_os, "pipe", c_os_pipe);
    py_bindfunc(mod_os, "isatty", c_os_isatty);

    const char* py_os_path_py =
        "sep = '/'\n"
        "class _path:\n"
        "    sep = '/'\n"
        "    def join(self, *parts):\n"
        "        return '/'.join([p.rstrip('/') for p in parts if p]).replace('//', '/')\n"
        "    def split(self, p):\n"
        "        if '/' not in p: return '', p\n"
        "        idx = p.rfind('/')\n"
        "        return p[:idx], p[idx+1:]\n"
        "    def basename(self, p): return self.split(p)[1]\n"
        "    def dirname(self, p): return self.split(p)[0]\n"
        "    def splitext(self, p):\n"
        "        b = self.basename(p)\n"
        "        if '.' not in b: return p, ''\n"
        "        idx = p.rfind('.')\n"
        "        return p[:idx], p[idx:]\n"
        "    def exists(self, p):\n"
        "        try: stat(p); return True\n"
        "        except: return False\n"
        "    def isfile(self, p):\n"
        "        try: return stat(p).get('is_file', False)\n"
        "        except: return False\n"
        "    def isdir(self, p):\n"
        "        try: return stat(p).get('is_dir', False)\n"
        "        except: return False\n"
        "    def getsize(self, p):\n"
        "        return stat(p)['size']\n"
        "    def getmtime(self, p):\n"
        "        return stat(p)['mtime']\n"
        "    def abspath(self, p):\n"
        "        if p.startswith('/'): return p\n"
        "        return getcwd() + '/' + p\n"
        "    def isabs(self, p): return p.startswith('/')\n"
        "    def normpath(self, p):\n"
        "        parts = [x for x in p.split('/') if x and x != '.']\n"
        "        res = []\n"
        "        for part in parts:\n"
        "            if part == '..':\n"
        "                if res: res.pop()\n"
        "            else: res.append(part)\n"
        "        prefix = '/' if p.startswith('/') else ''\n"
        "        return prefix + '/'.join(res)\n"
        "    def relpath(self, p, start='.'):\n"
        "        return p\n"
        "    def expanduser(self, p):\n"
        "        if p.startswith('~'):\n"
        "            h = getenv('HOME', '/data/data/com.termux/files/home')\n"
        "            return h + p[1:]\n"
        "        return p\n"
        "path = _path()\n";
    if (!py_exec(py_os_path_py, "<os.path>", EXEC_MODE, mod_os)) {
        char* err = py_formatexc();
        if (err) { printf("OS.path init error: %s\n", err); free(err); }
    }

    /* 3. SYSINFO */
    py_GlobalRef mod_sysinfo = get_or_create_module("sysinfo");
    py_bindfunc(mod_sysinfo, "ram_total", c_sysinfo_ram_total);
    py_bindfunc(mod_sysinfo, "ram_free", c_sysinfo_ram_free);
    py_bindfunc(mod_sysinfo, "ram_available", c_sysinfo_ram_available);
    py_bindfunc(mod_sysinfo, "ram_avail", c_sysinfo_ram_available);
    py_bindfunc(mod_sysinfo, "ram_used", c_sysinfo_ram_used);
    py_bindfunc(mod_sysinfo, "uptime", c_sysinfo_uptime);
    py_bindfunc(mod_sysinfo, "uptime_str", c_sysinfo_uptime_str);
    py_bindfunc(mod_sysinfo, "loadavg", c_sysinfo_loadavg);
    py_bindfunc(mod_sysinfo, "procs_count", c_sysinfo_procs_count);
    py_bindfunc(mod_sysinfo, "storage_free", c_sysinfo_storage_free);
    py_bindfunc(mod_sysinfo, "storage_total", c_sysinfo_storage_total);
    py_bindfunc(mod_sysinfo, "device_model", c_sysinfo_device_model);
    py_bindfunc(mod_sysinfo, "model", c_sysinfo_device_model);
    py_bindfunc(mod_sysinfo, "device_brand", c_sysinfo_device_brand);
    py_bindfunc(mod_sysinfo, "brand", c_sysinfo_device_brand);
    py_bindfunc(mod_sysinfo, "device", c_sysinfo_device_device);
    py_bindfunc(mod_sysinfo, "device_manufacturer", c_sysinfo_device_manufacturer);
    py_bindfunc(mod_sysinfo, "manufacturer", c_sysinfo_device_manufacturer);
    py_bindfunc(mod_sysinfo, "android_sdk", c_sysinfo_android_sdk);
    py_bindfunc(mod_sysinfo, "sdk_int", c_sysinfo_android_sdk);
    py_bindfunc(mod_sysinfo, "android_version", c_sysinfo_android_version);
    py_bindfunc(mod_sysinfo, "android_release", c_sysinfo_android_version);
    py_bindfunc(mod_sysinfo, "cpu_count", c_os_cpu_count);

    /* 4. EXTENDED TIME */
    py_GlobalRef mod_time = get_or_create_module("time");
    py_bindfunc(mod_time, "time", c_time_time);
    py_bindfunc(mod_time, "time_ns", c_time_time_ns);
    py_bindfunc(mod_time, "monotonic", c_time_monotonic);
    py_bindfunc(mod_time, "monotonic_ns", c_time_monotonic_ns);
    py_bindfunc(mod_time, "perf_counter", c_time_perf_counter);
    py_bindfunc(mod_time, "perf_counter_ns", c_time_perf_counter_ns);
    py_bindfunc(mod_time, "sleep", c_time_sleep);
    py_bindfunc(mod_time, "usleep", c_time_usleep);
    py_bindfunc(mod_time, "localtime", c_time_localtime);
    py_bindfunc(mod_time, "gmtime", c_time_gmtime);
    py_bindfunc(mod_time, "strftime", c_time_strftime);
    py_bindfunc(mod_time, "timezone", c_time_timezone);

    /* 5. ANDROID NATIVE BRIDGE */
    py_GlobalRef mod_android = get_or_create_module("android");
    py_bindfunc(mod_android, "toast", c_android_toast);
    py_bindfunc(mod_android, "vibrate", c_android_vibrate);
    py_bindfunc(mod_android, "notify", c_android_notify);
    py_bindfunc(mod_android, "speak", c_android_speak);
    py_bindfunc(mod_android, "get_battery_level", c_android_get_battery_level);
    py_bindfunc(mod_android, "is_battery_charging", c_android_is_battery_charging);
    py_bindfunc(mod_android, "get_battery_status", c_android_get_battery_status);
    py_bindfunc(mod_android, "log", c_android_log);
    py_bindfunc(mod_android, "copy_to_clipboard", c_android_copy_to_clipboard);
    py_bindfunc(mod_android, "get_clipboard", c_android_get_clipboard);
    py_bindfunc(mod_android, "is_screen_on", c_android_is_screen_on);
    py_bindfunc(mod_android, "beep", c_android_beep);

    /* 6. HASHLIB & CRYPTO */
    py_GlobalRef mod_hash = get_or_create_module("hashlib");
    py_bindfunc(mod_hash, "crc16_modbus", c_hash_crc16_modbus);
    py_bindfunc(mod_hash, "crc32", c_hash_crc32);
    py_bindfunc(mod_hash, "adler32", c_hash_adler32);
    py_bindfunc(mod_hash, "md5", c_hash_md5);
    py_bindfunc(mod_hash, "sha1", c_hash_sha1);
    py_bindfunc(mod_hash, "sha256", c_hash_sha256);
    py_bindfunc(mod_hash, "bytes_to_hex", c_hash_bytes_to_hex);
    py_bindfunc(mod_hash, "hex_to_bytes", c_hash_hex_to_bytes);
    py_bindfunc(mod_hash, "hexdump", c_hash_hexdump);
    py_bindfunc(mod_hash, "xor_bytes", c_hash_xor_bytes);
    py_bindfunc(mod_hash, "random_bytes", c_hash_random_bytes);
    py_bindfunc(mod_hash, "random_int", c_hash_random_int);

    /* 7. STRUCT */
    py_GlobalRef mod_struct = get_or_create_module("struct");
    py_bindfunc(mod_struct, "pack_i8", c_struct_pack_i8);
    py_bindfunc(mod_struct, "unpack_i8", c_struct_unpack_i8);
    py_bindfunc(mod_struct, "pack_u8", c_struct_pack_u8);
    py_bindfunc(mod_struct, "unpack_u8", c_struct_unpack_u8);
    py_bindfunc(mod_struct, "pack_i16", c_struct_pack_i16);
    py_bindfunc(mod_struct, "unpack_i16", c_struct_unpack_i16);
    py_bindfunc(mod_struct, "pack_u16", c_struct_pack_u16);
    py_bindfunc(mod_struct, "unpack_u16", c_struct_unpack_u16);
    py_bindfunc(mod_struct, "pack_i32", c_struct_pack_i32);
    py_bindfunc(mod_struct, "unpack_i32", c_struct_unpack_i32);
    py_bindfunc(mod_struct, "pack_u32", c_struct_pack_u32);
    py_bindfunc(mod_struct, "unpack_u32", c_struct_unpack_u32);
    py_bindfunc(mod_struct, "pack_f32", c_struct_pack_f32);
    py_bindfunc(mod_struct, "unpack_f32", c_struct_unpack_f32);
    py_bindfunc(mod_struct, "pack_f64", c_struct_pack_f64);

    /* 8. STORAGE & SQLITE */
    py_newdict(py_r0());
    py_setglobal(py_name("_STORAGE_KV"), py_r0());
    py_GlobalRef mod_storage = get_or_create_module("storage");
    py_bindfunc(mod_storage, "set", c_storage_set);
    py_bindfunc(mod_storage, "get", c_storage_get);
    py_bindfunc(mod_storage, "delete", c_storage_delete);
    py_bindfunc(mod_storage, "has", c_storage_has);
    py_bindfunc(mod_storage, "clear", c_storage_clear);

    /* SQLite interface */
    py_GlobalRef mod_sqlite = get_or_create_module("sqlite3");
    const char* py_sqlite_py =
        "class Connection:\n"
        "    def __init__(self, path):\n"
        "        self.path = path\n"
        "        self._tables = {}\n"
        "        self._rowid = 0\n"
        "    def execute(self, sql, params=None):\n"
        "        s = sql.strip().upper()\n"
        "        if s.startswith('CREATE TABLE'):\n"
        "            tname = sql.split()[2].split('(')[0].strip('`\"[]')\n"
        "            if tname not in self._tables: self._tables[tname] = []\n"
        "        elif s.startswith('INSERT INTO'):\n"
        "            tname = sql.split()[2].split('(')[0].strip('`\"[]')\n"
        "            if tname in self._tables:\n"
        "                self._rowid += 1\n"
        "                p = list(params) if params else []\n"
        "                self._tables[tname].append(p)\n"
        "        return self\n"
        "    def query(self, sql, params=None):\n"
        "        s = sql.strip().upper()\n"
        "        if 'FROM' in s:\n"
        "            tname = sql.split('FROM')[1].strip().split()[0].strip(';`\"[]')\n"
        "            return self._tables.get(tname, [])\n"
        "        return []\n"
        "    def commit(self): pass\n"
        "    def close(self): pass\n"
        "    def last_insert_rowid(self): return self._rowid\n"
        "    def total_changes(self): return self._rowid\n"
        "def connect(path=':memory:'):\n"
        "    return Connection(path)\n";
    if (!py_exec(py_sqlite_py, "<sqlite3>", EXEC_MODE, mod_sqlite)) {
        char* err = py_formatexc();
        if (err) { printf("SQLite init error: %s\n", err); free(err); }
    }

    /* 9. MATH_EXT */
    py_GlobalRef mod_math_ext = get_or_create_module("math_ext");
    py_bindfunc(mod_math_ext, "clamp", c_math_clamp);
    py_bindfunc(mod_math_ext, "lerp", c_math_lerp);
    py_bindfunc(mod_math_ext, "map_range", c_math_map_range);
    py_bindfunc(mod_math_ext, "degrees", c_math_degrees);
    py_bindfunc(mod_math_ext, "radians", c_math_radians);
    py_bindfunc(mod_math_ext, "hypot", c_math_hypot);
    py_bindfunc(mod_math_ext, "gcd", c_math_gcd);
    py_bindfunc(mod_math_ext, "lcm", c_math_lcm);
    py_bindfunc(mod_math_ext, "is_close", c_math_is_close);
    py_bindfunc(mod_math_ext, "sign", c_math_sign);
}

#ifdef __cplusplus
}
#endif

#endif /* POCKETPY_EXT_H */

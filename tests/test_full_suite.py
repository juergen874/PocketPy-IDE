# =====================================================================
# PocketPy 2.0 Comprehensive Function Test Suite
# Tests all 160+ native functions across all modules
# =====================================================================

passed = 0
failed = 0
errors = []

def test(name, condition, detail=""):
    global passed, failed, errors
    if condition:
        passed += 1
        print(f"  [PASS] {name}")
    else:
        failed += 1
        msg = f"{name}: {detail}"
        errors.append(msg)
        print(f"  [FAIL] {msg}")

print("================================================================")
print("     STARTING POCKETPY 2.0 EXHAUSTIVE FUNCTION TESTS")
print("================================================================")

# ---------------------------------------------------------------------
# 1. SOCKET MODULE
# ---------------------------------------------------------------------
print("\n--- [1/9] Testing 'socket' Module ---")
import socket

test("socket.AF_INET", socket.AF_INET == 2)
test("socket.AF_INET6", socket.AF_INET6 == 10)
test("socket.SOCK_STREAM", socket.SOCK_STREAM == 1)
test("socket.SOCK_DGRAM", socket.SOCK_DGRAM == 2)
test("socket.SOL_SOCKET", socket.SOL_SOCKET == 1)
test("socket.SO_REUSEADDR", socket.SO_REUSEADDR == 2)

s_tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
test("socket() creation", s_tcp is not None)
s_tcp.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
test("socket.setsockopt()", True)
test("socket.getsockopt()", s_tcp.getsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR) == 1)
s_tcp.settimeout(3.5)
test("socket.settimeout() & gettimeout()", abs(s_tcp.gettimeout() - 3.5) < 0.01)
s_tcp.setblocking(True)
test("socket.setblocking()", True)
s_tcp.close()
test("socket.close()", s_tcp.fileno() is None)

# UDP socket
s_udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
test("UDP socket created", s_udp.fileno() is not None)
s_udp.close()
test("UDP socket closed", s_udp.fileno() is None)

# Host & DNS
hname = socket.gethostname()
test("socket.gethostname()", len(hname) > 0, str(hname))
test("socket.gethostbyname('127.0.0.1')", socket.gethostbyname("127.0.0.1") == "127.0.0.1")
test("socket.htons()", socket.htons(80) != 0)
test("socket.ntohs()", socket.ntohs(socket.htons(80)) == 80)
test("socket.htonl()", socket.htonl(12345) != 0)
test("socket.ntohl()", socket.ntohl(socket.htonl(12345)) == 12345)

# TCP Loopback Client-Server Communication
try:
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind(("127.0.0.1", 9922))
    server.listen(1)

    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.connect(("127.0.0.1", 9922))

    conn, c_addr = server.accept()
    test("socket TCP accept()", conn is not None and c_addr[0] == "127.0.0.1")

    client.send(b"PING")
    received = conn.recv(1024)
    test("socket send() & recv()", received == b"PING", str(received))

    conn.send(b"PONG")
    c_received = client.recv(1024)
    test("socket server response", c_received == b"PONG", str(c_received))

    test("socket.getsockname()", conn.getsockname()[1] == 9922)
    test("socket.getpeername()", client.getpeername()[1] == 9922)

    conn.close()
    client.close()
    server.close()
    test("TCP loopback full cycle", True)
except Exception as e:
    test("TCP loopback full cycle", False, str(e))

# ---------------------------------------------------------------------
# 2. OS & OS.PATH MODULE
# ---------------------------------------------------------------------
print("\n--- [2/9] Testing 'os' & 'os.path' Module ---")
import os

cwd = os.getcwd()
test("os.getcwd()", len(cwd) > 0 and (cwd.startswith("/") or (len(cwd) >= 2 and cwd[1] == ":")))
test("os.getpid()", os.getpid() > 0)
test("os.getppid()", os.getppid() > 0)
test("os.getuid()", os.getuid() >= 0)
test("os.geteuid()", os.geteuid() >= 0)
test("os.getgid()", os.getgid() >= 0)
test("os.cpu_count()", os.cpu_count() >= 1)
test("os.uname()", len(os.uname()) >= 3)
test("os.urandom(16)", len(os.urandom(16)) == 16)

os.putenv("PKPY_TEST_VAR", "PocketPyAwesome")
test("os.putenv() & os.getenv()", os.getenv("PKPY_TEST_VAR") == "PocketPyAwesome")
os.unsetenv("PKPY_TEST_VAR")
test("os.unsetenv()", os.getenv("PKPY_TEST_VAR") is None)

# Pipes
p_read, p_write = os.pipe()
test("os.pipe()", p_read >= 0 and p_write >= 0)
os.write(p_write, b"HelloPipe")
data = os.read(p_read, 16)
test("os.write() & os.read()", data == b"HelloPipe")
os.close(p_read)
os.close(p_write)

# Path
test("os.path.exists()", os.path.exists(cwd) == True)
test("os.path.isdir()", os.path.isdir(cwd) == True)
if os.name == "nt":
    test("os.path.join()", os.path.join("C:\\a", "b", "c") == "C:\\a\\b\\c")
    test("os.path.split()", os.path.split("C:\\a\\b\\file.txt") == ("C:\\a\\b", "file.txt"))
    test("os.path.basename()", os.path.basename("C:\\a\\b\\file.txt") == "file.txt")
    test("os.path.dirname()", os.path.dirname("C:\\a\\b\\file.txt") == "C:\\a\\b")
    test("os.path.splitext()", os.path.splitext("C:\\a\\b\\file.txt") == ("C:\\a\\b\\file", ".txt"))
    test("os.path.isabs()", os.path.isabs("C:\\foo") == True and os.path.isabs("foo") == False)
    test("os.path.normpath()", os.path.normpath("C:\\a\\b\\..\\c\\.\\d") == "C:\\a\\c\\d")
else:
    test("os.path.join()", os.path.join("/a", "b", "c") == "/a/b/c")
    test("os.path.split()", os.path.split("/a/b/file.txt") == ("/a/b", "file.txt"))
    test("os.path.basename()", os.path.basename("/a/b/file.txt") == "file.txt")
    test("os.path.dirname()", os.path.dirname("/a/b/file.txt") == "/a/b")
    test("os.path.splitext()", os.path.splitext("/a/b/file.txt") == ("/a/b/file", ".txt"))
    test("os.path.isabs()", os.path.isabs("/foo") == True and os.path.isabs("foo") == False)
    test("os.path.normpath()", os.path.normpath("/a/b/../c/./d") == "/a/c/d")

# Directory operations
test_dir = os.path.join(cwd, "_pkpy_tmp_dir")
try:
    if os.path.exists(test_dir): os.rmdir(test_dir)
    os.mkdir(test_dir)
    test("os.mkdir()", os.path.exists(test_dir))
    nested = os.path.join(test_dir, "nested", "sub")
    os.makedirs(nested)
    test("os.makedirs()", os.path.exists(nested))
    os.rmdir(nested)
    os.rmdir(os.path.join(test_dir, "nested"))
    os.rmdir(test_dir)
    test("os.rmdir() cleanup", not os.path.exists(test_dir))
except Exception as e:
    test("os dir operations", False, str(e))

# ---------------------------------------------------------------------
# 3. SYSINFO MODULE
# ---------------------------------------------------------------------
print("\n--- [3/9] Testing 'sysinfo' Module ---")
import sysinfo

test("sysinfo.ram_total()", sysinfo.ram_total() > 0)
test("sysinfo.ram_free()", sysinfo.ram_free() > 0)
test("sysinfo.ram_available()", sysinfo.ram_available() > 0)
test("sysinfo.ram_avail() [alias]", sysinfo.ram_avail() > 0)
test("sysinfo.ram_used()", sysinfo.ram_used() > 0)
test("sysinfo.uptime()", sysinfo.uptime() > 0)
test("sysinfo.uptime_str()", len(sysinfo.uptime_str()) > 0)
test("sysinfo.cpu_count()", sysinfo.cpu_count() >= 1)
test("sysinfo.brand()", len(sysinfo.brand()) > 0)
test("sysinfo.device_brand()", sysinfo.device_brand() == sysinfo.brand())
test("sysinfo.model()", len(sysinfo.model()) > 0)
test("sysinfo.device_model()", sysinfo.device_model() == sysinfo.model())
test("sysinfo.device()", len(sysinfo.device()) > 0)
test("sysinfo.android_version()", len(sysinfo.android_version()) > 0)
test("sysinfo.android_release() [alias]", sysinfo.android_release() == sysinfo.android_version())
if os.name == "nt":
    test("sysinfo.android_sdk()", sysinfo.android_sdk() >= 0)
else:
    test("sysinfo.android_sdk()", sysinfo.android_sdk() > 0)
test("sysinfo.sdk_int() [alias]", sysinfo.sdk_int() == sysinfo.android_sdk())
test("sysinfo.storage_free()", sysinfo.storage_free() > 0)
test("sysinfo.storage_total()", sysinfo.storage_total() > 0)

# ---------------------------------------------------------------------
# 4. TIME MODULE
# ---------------------------------------------------------------------
print("\n--- [4/9] Testing 'time' Module ---")
import time

t1 = time.time()
test("time.time()", t1 > 1700000000.0)
test("time.time_ns()", time.time_ns() > 1700000000000000000)
test("time.monotonic()", time.monotonic() > 0)
test("time.monotonic_ns()", time.monotonic_ns() > 0)
test("time.perf_counter()", time.perf_counter() > 0)
test("time.perf_counter_ns()", time.perf_counter_ns() > 0)

start_s = time.time()
time.sleep(0.02)
elapsed_s = time.time() - start_s
test("time.sleep(0.02)", elapsed_s >= 0.015, f"slept {elapsed_s:.3f}s")

time.usleep(5000)
test("time.usleep(5000)", True)

lt = time.localtime()
test("time.localtime()", len(lt) >= 6)
test("time.strftime()", len(time.strftime("%Y-%m-%d")) == 10)
test("time.timezone()", isinstance(time.timezone(), int))

# ---------------------------------------------------------------------
# 5. ANDROID NATIVE BRIDGE
# ---------------------------------------------------------------------
print("\n--- [5/9] Testing 'android' Bridge Module ---")
import android

bat = android.get_battery_level()
test("android.get_battery_level()", 0 <= bat <= 100, f"bat={bat}%")
test("android.is_battery_charging()", isinstance(android.is_battery_charging(), bool))
test("android.get_battery_status()", isinstance(android.get_battery_status(), dict))

android.toast("PocketPy Test Suite")
test("android.toast()", True)
android.vibrate(50)
test("android.vibrate()", True)
android.beep()
test("android.beep()", True)
android.notify("PocketPy Test", "Running Exhaustive Tests")
test("android.notify()", True)
android.speak("Test")
test("android.speak()", True)

android.copy_to_clipboard("PocketPySecret123")
test("android.copy_to_clipboard() & get_clipboard()", android.get_clipboard() == "PocketPySecret123")
test("android.is_screen_on()", isinstance(android.is_screen_on(), bool))
android.log("Test log from PocketPy")
test("android.log()", True)

# ---------------------------------------------------------------------
# 6. HASHLIB & CRYPTO MODULE
# ---------------------------------------------------------------------
print("\n--- [6/9] Testing 'hashlib' Module ---")
import hashlib

data_123 = b"123456789"
test("hashlib.crc16_modbus()", hashlib.crc16_modbus(data_123) == 0x4B37, hex(hashlib.crc16_modbus(data_123)))
test("hashlib.crc32()", hashlib.crc32(data_123) == 0xCBF43926, hex(hashlib.crc32(data_123)))
test("hashlib.adler32()", hashlib.adler32(data_123) == 0x091E01DE, hex(hashlib.adler32(data_123)))
test("hashlib.md5()", hashlib.md5(b"hello") == "5d41402abc4b2a76b9719d911017c592")
test("hashlib.sha1()", hashlib.sha1(b"hello") == "aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d")
test("hashlib.sha256()", hashlib.sha256(b"hello") == "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824")
test("hashlib.bytes_to_hex()", hashlib.bytes_to_hex(b"\xde\xad\xbe\xef") == "deadbeef")
test("hashlib.hex_to_bytes()", hashlib.hex_to_bytes("deadbeef") == b"\xde\xad\xbe\xef")
test("hashlib.hexdump()", "DEADBEEF" in hashlib.hexdump(b"\xde\xad\xbe\xef").replace(" ", "").upper())
test("hashlib.xor_bytes()", hashlib.xor_bytes(b"\xaa\x55", b"\x55\xaa") == b"\xff\xff")
test("hashlib.random_bytes(8)", len(hashlib.random_bytes(8)) == 8)
r_int = hashlib.random_int(10, 20)
test("hashlib.random_int(10, 20)", 10 <= r_int <= 20, f"got {r_int}")

# ---------------------------------------------------------------------
# 7. STRUCT MODULE
# ---------------------------------------------------------------------
print("\n--- [7/9] Testing 'struct' Module ---")
import struct

test("struct.pack_i8 / unpack_i8", struct.unpack_i8(struct.pack_i8(-42)) == -42)
test("struct.pack_u8 / unpack_u8", struct.unpack_u8(struct.pack_u8(210)) == 210)
test("struct.pack_i16 / unpack_i16", struct.unpack_i16(struct.pack_i16(-12345)) == -12345)
test("struct.pack_u16 / unpack_u16", struct.unpack_u16(struct.pack_u16(54321)) == 54321)
test("struct.pack_i32 / unpack_i32", struct.unpack_i32(struct.pack_i32(-987654321)) == -987654321)
test("struct.pack_u32 / unpack_u32", struct.unpack_u32(struct.pack_u32(3141592653)) == 3141592653)
f_val = struct.unpack_f32(struct.pack_f32(3.14159))
test("struct.pack_f32 / unpack_f32", abs(f_val - 3.14159) < 0.0001, f"got {f_val}")
test("struct.pack_f64", len(struct.pack_f64(2.718281828459)) == 8)

# ---------------------------------------------------------------------
# 8. STORAGE & SQLITE MODULE
# ---------------------------------------------------------------------
print("\n--- [8/9] Testing 'storage' & 'sqlite3' Module ---")
import storage
import sqlite3

storage.set("test_key", 4242)
test("storage.set() & get()", storage.get("test_key") == 4242)
test("storage.has() == True", storage.has("test_key") == True)
storage.delete("test_key")
test("storage.delete()", storage.has("test_key") == False)
storage.set("k1", 100)
storage.set("k2", 200)
storage.clear()
test("storage.clear()", storage.has("k1") == False and storage.has("k2") == False)

db = sqlite3.connect(":memory:")
db.execute("CREATE TABLE users (id INT, name STR)")
db.execute("INSERT INTO users VALUES (?, ?)", (1, "Alice"))
db.execute("INSERT INTO users VALUES (?, ?)", (2, "Bob"))
rows = db.query("SELECT * FROM users")
test("sqlite3 CREATE / INSERT / SELECT", len(rows) == 2 and rows[0][1] == "Alice", str(rows))
test("sqlite3 last_insert_rowid()", db.last_insert_rowid() == 2)
test("sqlite3 total_changes()", db.total_changes() == 2)

# ---------------------------------------------------------------------
# 9. MATH_EXT MODULE
# ---------------------------------------------------------------------
print("\n--- [9/9] Testing 'math_ext' Module ---")
import math_ext

test("math_ext.clamp()", math_ext.clamp(150, 0, 100) == 100 and math_ext.clamp(-5, 0, 10) == 0)
test("math_ext.lerp()", math_ext.lerp(0, 100, 0.75) == 75.0)
test("math_ext.map_range()", math_ext.map_range(50, 0, 100, 0, 1000) == 500.0)
test("math_ext.degrees()", abs(math_ext.degrees(3.141592653589793) - 180.0) < 0.001)
test("math_ext.radians()", abs(math_ext.radians(180.0) - 3.141592653589793) < 0.001)
test("math_ext.hypot(3, 4)", math_ext.hypot(3, 4) == 5.0)
test("math_ext.gcd(48, 18)", math_ext.gcd(48, 18) == 6)
test("math_ext.lcm(12, 18)", math_ext.lcm(12, 18) == 36)
test("math_ext.is_close()", math_ext.is_close(1.0, 1.0000001, 1e-5) == True)
test("math_ext.sign()", math_ext.sign(-42) == -1 and math_ext.sign(42) == 1 and math_ext.sign(0) == 0)

# ---------------------------------------------------------------------
# SUMMARY
# ---------------------------------------------------------------------
print("\n================================================================")
print(f"TEST RESULTS: {passed} PASSED, {failed} FAILED (TOTAL: {passed + failed})")
if failed > 0:
    print("FAILED TESTS:")
    for err in errors:
        print(f"  - {err}")
    print("================================================================")
    exit(1)
else:
    print("ALL TESTS PASSED WITH 100% SUCCESS!")
    print("================================================================")

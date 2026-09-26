package com.pocketpy.ide.data

object SampleScripts {

    val SAMPLES = listOf(
        "1_pocketpy_speed_test.py" to """# 🚀 PocketPy Ultra-Fast Speed Benchmark
import time

print("=" * 45)
print("  PocketPy Speed & List Comprehension Test")
print("=" * 45)

start = time.time()

# Generate 25,000 prime candidates and calculate powers
evens = [x**2 for x in range(25000) if x % 2 == 0]
total = sum(evens)

elapsed = (time.time() - start) * 1000
print(f"Generated {len(evens)} elements in {elapsed:.2f} ms")
print(f"Checksum Total: {total}")
print("PocketPy native C11 execution: BLAZING FAST!")
""".trimIndent(),

        "2_ascii_sine_waves.py" to """# 🌊 Pure Python ASCII Wave Plotter
import math

print("--- Real-time ASCII Sinusoid Visualizer ---")
width = 40
steps = 20

for i in range(steps):
    val = math.sin(i * 0.35)
    pos = int((val + 1.0) / 2.0 * (width - 1))
    line = [" "] * width
    line[width // 2] = "|"
    line[pos] = "*"
    sign = "+" if val >= 0 else ""
    print(f"{i:02d} |" + "".join(line) + f"| {sign}{val:.3f}")

print("\nDone! Visualized without external plotting dependencies.")
""".trimIndent(),

        "3_interactive_dashboard.html" to """<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { background: #0f172a; color: #f8fafc; font-family: sans-serif; padding: 20px; text-align: center; }
    .card { background: #1e293b; border-radius: 12px; padding: 24px; max-width: 400px; margin: 0 auto; box-shadow: 0 4px 12px rgba(0,0,0,0.5); }
    .val { font-size: 3rem; font-weight: bold; color: #38bdf8; margin: 10px 0; }
    .label { color: #94a3b8; font-size: 0.9rem; text-transform: uppercase; }
    .badge { display: inline-block; background: #0369a1; color: white; padding: 4px 12px; border-radius: 9999px; font-size: 0.8rem; }
  </style>
</head>
<body>
  <div class="card">
    <span class="badge">PocketPy Engine</span>
    <div class="val">2.4 MB</div>
    <div class="label">Total APK Size</div>
    <p style="color: #cbd5e1; font-size: 0.95rem; margin-top: 16px;">
      Ultra-lightweight Python runtime running in native C11 without heavy dependencies.
    </p>
  </div>
</body>
</html>
""".trimIndent(),

        "4_deye_reader.py" to """# ☀️ Deye SUN-12K Hybrid Inverter Live Reader & Visual Dashboard
# Ultra-lightweight: Zero external dependencies (Pure PocketPy + native socket)
# Reads live telemetry via Solarman V5 / Modbus TCP from 192.168.188.128:8899

try:
    import socket
except ImportError:
    socket = None

def to_signed16(val):
    return val if val < 0x8000 else val - 0x10000

def modbus_crc16(data):
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return [crc & 0xFF, (crc >> 8) & 0xFF]

def v5_checksum(frame):
    checksum = 0
    for i in range(1, len(frame) - 2):
        checksum += frame[i] & 0xFF
    return checksum & 0xFF

class PocketSolarmanV5:
    def __init__(self, host="192.168.188.128", port=8899, serial_number=1109501211, slave_id=1, timeout=3.5):
        self.host = host
        self.port = port
        self.serial_number = serial_number
        self.slave_id = slave_id
        self.timeout = timeout
        self.sock = None
        self.seq = 1

    def connect(self):
        if socket is None:
            raise RuntimeError("Socket module not available.")
        self.sock = socket.socket()
        if hasattr(self.sock, "settimeout"):
            self.sock.settimeout(self.timeout)
        self.sock.connect((self.host, self.port))

    def close(self):
        if self.sock:
            try:
                self.sock.close()
            except Exception:
                pass
            self.sock = None

    def build_frame(self, start_reg, count):
        mb_req = [self.slave_id, 3, (start_reg >> 8) & 0xFF, start_reg & 0xFF, (count >> 8) & 0xFF, count & 0xFF]
        mb_req.extend(modbus_crc16(mb_req))
        payload_len = 15 + len(mb_req)
        sn = self.serial_number
        frame = [
            0xA5, payload_len & 0xFF, (payload_len >> 8) & 0xFF, 0x10, 0x45,
            self.seq & 0xFF, 0x00, sn & 0xFF, (sn >> 8) & 0xFF, (sn >> 16) & 0xFF, (sn >> 24) & 0xFF,
            0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        ]
        frame.extend(mb_req)
        frame.append(0)
        frame.append(0x15)
        frame[-2] = v5_checksum(frame)
        self.seq = (self.seq + 1) & 0xFF
        return bytes(frame)

    def parse_response(self, raw_bytes):
        if len(raw_bytes) < 28 or raw_bytes[0] != 0xA5 or raw_bytes[-1] != 0x15:
            raise ValueError("Invalid Solarman V5 frame")
        modbus_payload = raw_bytes[25:-2]
        byte_count = modbus_payload[2]
        regs = []
        for i in range(0, byte_count, 2):
            regs.append((modbus_payload[3 + i] << 8) | modbus_payload[4 + i])
        return regs

    def read_holding_registers(self, start_reg, count):
        req_frame = self.build_frame(start_reg, count)
        if not self.sock:
            self.connect()
        self.sock.send(req_frame)
        buf = self.sock.recv(1024)
        if not buf:
            raise RuntimeError("Empty response from inverter")
        while len(buf) < 3:
            more = self.sock.recv(1024)
            if not more:
                break
            buf = buf + more
        payload_len = buf[1] | (buf[2] << 8)
        expected_len = 13 + payload_len
        while len(buf) < expected_len:
            more = self.sock.recv(expected_len - len(buf))
            if not more:
                break
            buf = buf + more
        return self.parse_response(buf)

class DeyeReader:
    def __init__(self, host="192.168.188.128", port=8899, serial=1109501211):
        self.host = host
        self.port = port
        self.serial = serial
        self.client = None

    def read_telemetry(self):
        data = {}
        try:
            if not self.client:
                self.client = PocketSolarmanV5(self.host, self.port, self.serial)
            b1 = self.client.read_holding_registers(500, 42)
            status_map = {0: "Standby", 1: "Self-Test", 2: "Normal (Hybrid)", 3: "Alarm", 4: "Störung"}
            data["status_code"] = b1[0]
            data["status_text"] = status_map.get(b1[0], f"Status {b1[0]}")
            data["energy_grid_buy_today_kwh"] = round(b1[20] * 0.1, 2)
            data["energy_grid_sell_today_kwh"] = round(b1[21] * 0.1, 2)
            data["energy_bat_charge_today_kwh"] = round(b1[22] * 0.1, 2)
            data["energy_bat_dischg_today_kwh"] = round(b1[23] * 0.1, 2)
            data["energy_load_today_kwh"] = round(b1[26] * 0.1, 2)
            data["energy_pv_today_kwh"] = round(b1[29] * 0.1, 2)
            data["temp_dc_celsius"] = round((b1[40] - 1000) * 0.1, 1) if b1[40] >= 1000 else round(b1[40] * 0.1 - 100, 1)
            data["temp_ac_celsius"] = round((b1[41] - 1000) * 0.1, 1) if b1[41] >= 1000 else round(b1[41] * 0.1 - 100, 1)

            b2 = self.client.read_holding_registers(586, 27)
            raw_bat_temp = b2[0]
            data["temp_battery_celsius"] = round((raw_bat_temp - 1000) * 0.1, 1) if raw_bat_temp >= 1000 else round(raw_bat_temp * 0.1 - 100, 1)
            data["battery_voltage_v"] = round(b2[1] * 0.01, 2)
            data["battery_soc_percent"] = b2[2]
            data["battery_power_w"] = to_signed16(b2[3])
            data["battery_current_a"] = round(to_signed16(b2[4]) * 0.02, 2)
            data["grid_voltage_l1_v"] = round(b2[12] * 0.1, 1)
            data["grid_voltage_l2_v"] = round(b2[13] * 0.1, 1)
            data["grid_voltage_l3_v"] = round(b2[14] * 0.1, 1)
            data["grid_power_l1_w"] = to_signed16(b2[18])
            data["grid_power_l2_w"] = to_signed16(b2[19])
            data["grid_power_l3_w"] = to_signed16(b2[20])
            data["grid_power_total_w"] = to_signed16(b2[21])
            freq_raw = b2[22] if len(b2) > 22 and b2[22] > 0 else 5000
            data["grid_frequency_hz"] = round(freq_raw * 0.01, 2) if freq_raw > 1000 else round(freq_raw * 0.1, 2)

            b3 = self.client.read_holding_registers(625, 29)
            data["load_power_total_w"] = to_signed16(b3[28])

            b4 = self.client.read_holding_registers(672, 8)
            data["pv1_voltage_v"] = round(b4[4] * 0.1, 1)
            data["pv1_current_a"] = round(b4[5] * 0.1, 1)
            data["pv1_power_w"] = round(data["pv1_voltage_v"] * data["pv1_current_a"], 1)
            data["pv2_voltage_v"] = round(b4[6] * 0.1, 1)
            data["pv2_current_a"] = round(b4[7] * 0.1, 1)
            data["pv2_power_w"] = round(data["pv2_voltage_v"] * data["pv2_current_a"], 1)
            data["pv_power_total_w"] = round(data["pv1_power_w"] + data["pv2_power_w"], 1)
            data["logger_serial"] = self.serial
            data["mode"] = "LIVE (Modbus TCP)"
            return data
        except Exception as e:
            if self.client:
                self.client.close()
                self.client = None
            raise e

    def mock_telemetry(self):
        return {
            "status_text": "Normal (Demo)", "pv_power_total_w": 4850.0,
            "pv1_power_w": 2550.0, "pv1_voltage_v": 360.0, "pv1_current_a": 7.1,
            "pv2_power_w": 2300.0, "pv2_voltage_v": 350.0, "pv2_current_a": 6.6,
            "battery_soc_percent": 82, "battery_power_w": 1400, "battery_voltage_v": 53.2,
            "battery_current_a": 26.3, "temp_battery_celsius": 24.0,
            "grid_power_total_w": -1200, "grid_frequency_hz": 50.0,
            "load_power_total_w": 2250, "energy_pv_today_kwh": 21.4,
            "energy_grid_buy_today_kwh": 2.1, "energy_grid_sell_today_kwh": 9.4,
            "temp_dc_celsius": 38.0, "temp_ac_celsius": 36.5,
            "logger_serial": self.serial, "mode": "SIMULATION / DEMO"
        }

def render_terminal_dashboard(data):
    C_RESET = "\\033[0m"
    C_BOLD = "\\033[1m"
    C_GREEN = "\\033[32m"
    C_YELLOW = "\\033[33m"
    C_BLUE = "\\033[34m"
    C_CYAN = "\\033[36m"
    C_RED = "\\033[31m"

    mode = data.get("mode", "LIVE")
    print(f"{C_BOLD}{C_CYAN}========================================================================{C_RESET}")
    print(f"{C_BOLD}{C_YELLOW}        DEYE SUN-12K HYBRID INVERTER MODBUS ({mode}){C_RESET}")
    print(f"        Logger SN: {data.get('logger_serial', 'N/A')}")
    print(f"{C_BOLD}{C_CYAN}========================================================================{C_RESET}")
    print(f"Status: {C_GREEN}{data.get('status_text')}{C_RESET}")
    print("------------------------------------------------------------------------")
    print(f"{C_BOLD}{C_GREEN}☀️  SOLAR: {data.get('pv_power_total_w', 0):.1f} W (Heute: {data.get('energy_pv_today_kwh', 0)} kWh){C_RESET}")
    print(f"   PV1: {data.get('pv1_power_w', 0):.1f} W ({data.get('pv1_voltage_v', 0):.1f} V, {data.get('pv1_current_a', 0):.1f} A)")
    print(f"   PV2: {data.get('pv2_power_w', 0):.1f} W ({data.get('pv2_voltage_v', 0):.1f} V, {data.get('pv2_current_a', 0):.1f} A)")
    print("------------------------------------------------------------------------")
    bat_pow = data.get("battery_power_w", 0)
    bat_status = "Laden" if bat_pow > 0 else ("Entladen" if bat_pow < 0 else "Standby")
    print(f"{C_BOLD}{C_CYAN}🔋 BATTERIE: {data.get('battery_soc_percent')}% | {bat_status} {abs(bat_pow)} W{C_RESET}")
    print(f"   Spannung: {data.get('battery_voltage_v')} V | Strom: {data.get('battery_current_a')} A | Temp: {data.get('temp_battery_celsius')} °C")
    print("------------------------------------------------------------------------")
    grid_pow = data.get("grid_power_total_w", 0)
    grid_st = f"Bezug ({grid_pow} W)" if grid_pow >= 0 else f"Einspeisung ({abs(grid_pow)} W)"
    print(f"{C_BOLD}{C_BLUE}🔌 NETZ: {grid_st} | {data.get('grid_frequency_hz')} Hz{C_RESET}")
    print("------------------------------------------------------------------------")
    print(f"{C_BOLD}{C_YELLOW}🏠 HAUSVERBRAUCH: {data.get('load_power_total_w', 0)} W{C_RESET}")
    print(f"{C_BOLD}{C_CYAN}========================================================================{C_RESET}")

reader = DeyeReader("192.168.188.128", 8899, 1109501211)
try:
    print("Verbinde zu Deye Inverter (192.168.188.128:8899)...")
    data = reader.read_telemetry()
except Exception as e:
    print(f"Inverter offline ({e}) - Schalte auf Demo-Modus...")
    data = reader.mock_telemetry()

render_terminal_dashboard(data)
""".trimIndent()
    )
}

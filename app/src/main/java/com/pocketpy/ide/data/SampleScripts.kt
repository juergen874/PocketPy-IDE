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

        "4_deye_pv_calc.py" to """# ☀️ Deye Inverter Modbus Register Decoder
# Simulates decoding 16-bit telemetry words from a Deye Hybrid Inverter

class DeyeTelemetry:
    def __init__(self, raw_registers):
        # Register 598: PV1 Voltage (0.1 V)
        self.pv1_v = raw_registers.get(598, 0) * 0.1
        # Register 599: PV1 Current (0.1 A)
        self.pv1_a = raw_registers.get(599, 0) * 0.1
        # Register 600: PV2 Voltage (0.1 V)
        self.pv2_v = raw_registers.get(600, 0) * 0.1
        # Register 601: PV2 Current (0.1 A)
        self.pv2_a = raw_registers.get(601, 0) * 0.1
        # Register 588: Battery SOC (%)
        self.battery_soc = raw_registers.get(588, 0)

    @property
    def pv1_watts(self):
        return round(self.pv1_v * self.pv1_a, 1)

    @property
    def pv2_watts(self):
        return round(self.pv2_v * self.pv2_a, 1)

    @property
    def total_solar_watts(self):
        return round(self.pv1_watts + self.pv2_watts, 1)

    def print_summary(self):
        print(f"PV1: {self.pv1_v:.1f} V @ {self.pv1_a:.1f} A -> {self.pv1_watts} W")
        print(f"PV2: {self.pv2_v:.1f} V @ {self.pv2_a:.1f} A -> {self.pv2_watts} W")
        print(f"☀️ Total Solar Power : {self.total_solar_watts} W")
        print(f"🔋 Battery State      : {self.battery_soc} %")

# Sample simulated Deye telemetry register frame
mock_registers = {598: 3450, 599: 82, 600: 3420, 601: 79, 588: 92}
inverter = DeyeTelemetry(mock_registers)
inverter.print_summary()
""".trimIndent()
    )
}

# Embedded System Integration Architecture

## 1. System Topology and Architecture
The Unified Engineering Workbench communicates with three external hardware entities over a modular, host-managed bus hierarchy connected to a Windows 11 host workstation via a dedicated, powered USB 2.0 hub (5.0 V VBUS, 500 mA current budget per downstream port).

```text
+-----------------------------------------------------------------------+
|                       Host PC (Windows 11 x64)                        |
|   Workbench Core Engine (C++17) <---> Win32 / Direct3D 11 Desktop UI   |
+-----------------------------------+-----------------------------------+
                                    | USB 2.0 (Host Controller)
                                    v
+-----------------------------------------------------------------------+
|                    External Powered USB 2.0 Hub                       |
|           (5.0 V VBUS, 500 mA / port, Common Star Ground)             |
+---------------+-------------------+-------------------+---------------+
                |                   |                   |
                v USB               v USB               v USB
      +-----------------+   +-----------------+   +-----------------+
      |  MCU Subsystem  |   | FPGA Subsystem  |   |  JTAG Adapter   |
      |  (STM32F407)    |   | (Artix-7 XC7A35)|   | (SEGGER J-Link) |
      +--------+--------+   +--------+--------+   +--------+--------+
               |                     |                     |
               | UART (3.3V)         | UART (3.3V)         | JTAG (TCK, TMS,
               | 115200 8N1          | 115200 8N1          |       TDI, TDO)
               v                     v                     v
         [Telemetry]           [Telemetry]           [Debug/Control]
```

## 2. Interface and Pin-Voltage Specification Table

| Signal Domain | Source | Destination | Nominal Voltage | Tolerance / Logic Level | Drive Direction / Ownership | Isolation / Protection |
|---|---|---|---|---|---|---|
| **USB VBUS** | Powered Hub | All boards | 5.0 V DC | ±5% (4.75 V – 5.25 V) | Host drives power; max 500 mA | Polyfuse PTC + TVS ESD diodes |
| **GND** | System Hub | All boards | 0.0 V (Ref) | 0.0 V reference | Star-point topology | Direct low-impedance ground plane |
| **MCU VDD** | Onboard LDO | STM32F407 Core/IO | 3.3 V DC | 3.0 V – 3.6 V | Regulated locally | 100 nF decoupling per VDD pin |
| **FPGA VCCINT**| Regulated DC-DC | Artix-7 Core | 1.0 V DC | ±3% (0.97 V – 1.03 V)| Regulated locally | High-efficiency buck regulator |
| **FPGA VCCIO** | Regulated LDO | Artix-7 Bank 34 | 3.3 V DC | ±5% (3.14 V – 3.46 V)| Regulated locally | Decoupled rail |
| **MCU UART TX**| STM32 (PA9) | USB-UART bridge | 3.3 V CMOS | VOH ≥ 2.4 V, VOL ≤ 0.4 V| MCU drives line (115200 baud) | Series 22 Ω damping resistor |
| **MCU UART RX**| USB-UART bridge | STM32 (PA10) | 3.3 V CMOS | VIH ≥ 2.0 V, VIL ≤ 0.8 V| Bridge drives line | Series 22 Ω damping resistor |
| **FPGA UART TX**| Artix-7 I/O | USB-UART bridge | 3.3 V LVCMOS33| VOH ≥ 2.4 V, VOL ≤ 0.4 V| FPGA drives line | Series 22 Ω damping resistor |
| **JTAG TCK** | J-Link Probe | Target MCU/FPGA | VTref (1.65–3.6V)| Level-shifted to VTref | **Probe drives clock** (up to 15 MHz)| Level translator on probe |
| **JTAG TMS** | J-Link Probe | Target MCU/FPGA | VTref (1.65–3.6V)| Level-shifted to VTref | **Probe drives mode select** | Level translator on probe |
| **JTAG TDI** | J-Link Probe | Target MCU/FPGA | VTref (1.65–3.6V)| Level-shifted to VTref | **Probe drives data in** | Level translator on probe |
| **JTAG TDO** | Target MCU/FPGA| J-Link Probe | VTref (1.65–3.6V)| Driven by target | **Target drives data out** | Probe buffer |
| **JTAG VTref** | Target Board | J-Link Probe | 1.65 V – 3.6 V | Analog sense input | **Target provides reference voltage** | Sense input (< 1 mA draw) |

## 3. Dedicated Nature of the JTAG Debug Adapter
A JTAG adapter (such as the SEGGER J-Link) is **strictly a debug probe and control interface**, not an interchangeable compute board:
1. **No User Execution Environment**: It does not execute custom firmware, host tasks, or mathematical solver kernels.
2. **Dynamic Logic Level Tracking**: It does not supply IO power; it monitors `VTref` from the target system and shifts its drivers (`TCK`, `TMS`, `TDI`) dynamically to match the target's operating voltage.
3. **Master-Slave Asymmetry**: The JTAG probe is an invasive debugging master with low-level privileges to halt processor execution, step cycles, inspect register state, and program volatile/non-volatile memory.
4. **Distinct Transport Semantics**: Communication is synchronous bit-clocked shifting along Boundary-Scan IEEE 1149.1 tap controllers, not packetized asynchronous serial communication.

## 4. Staged Safe Hardware Integration Procedure
To avoid over-current, voltage-clamping, or ground-loop damage when physical hardware is connected:
- **Phase 1: Unpowered Visual & Continuity Inspection**: Verify polarity, solder joints, and ensure no shorts between VBUS/VDD and GND.
- **Phase 2: Power Rail Verification (Unconnected)**: Apply power to hub; verify 5.0 V VBUS, 3.3 V MCU rail, and 1.0 V / 3.3 V FPGA rails using a digital multimeter before attaching signal lines.
- **Phase 3: Ground Reference Unification**: Connect star ground terminals before any signal or communication lines to prevent differential potential currents.
- **Phase 4: Telemetry Link Verification**: Connect USB-UART bridges; verify enumeration in Windows Device Manager; open serial ports at 115200 8N1 in read-only telemetry mode.
- **Phase 5: JTAG Sense & TAP Discovery**: Connect `VTref` and `GND`; verify sensed target voltage matches expected target rail; run non-destructive IDCODE scan chain query.
- **Phase 6: Active Control & Flash Operations**: Authorize halt, step, and memory access only after phases 1–5 succeed without error.


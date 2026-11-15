# Design Guide & Rationale

This document explains the radio architecture of the firmware: which radios
exist, what each is for, why there are two nRF24L01 modules, how the tools are
partitioned across the radios, and whether all radios can run at once. It is
meant as a design reference for anyone extending or reworking the firmware.

All page/line references point at the source as a starting place; they may
drift as the code evolves.

## Radios at a glance

The device carries **three independent 2.4 GHz radio subsystems**, plus a
separate sub-GHz radio:

| Radio | Interface | Role |
|-------|-----------|------|
| ESP32 native (WiFi + BLE) | internal to the SoC | Protocol-aware work: real 802.11 / BLE |
| nRF24L01 #1 (NRF1) | SPI, CE 27 / CSN 14 | Raw 2.4 GHz carrier work (scan / TX) |
| nRF24L01 #2 (NRF2) | SPI, CE 17 / CSN 16 | Raw 2.4 GHz carrier work (scan / TX) |
| CC1101 | SPI, CSN 21 | Sub-GHz (separate band, out of scope here) |

Pin assignments live in `include/Pins.h`. The TFT display, both nRF modules and
the CC1101 all share **one SPI bus** (SCK 18 / MISO 19 / MOSI 23), each selected
by its own chip-select line.

## 1. ESP32 native radio — the "protocol-aware" radio

The SoC's own 2.4 GHz radio, driven through the standard stacks
(`WiFi.h` / `esp_wifi`, BLE / NimBLE). It is used by everything that needs to
speak a *real* protocol — associate, parse frames, run an access point:

- **WiFi:** `WifiScanner`, `ProbeSniffer`, `BeaconSpam`, `Deauther`,
  `EvilPortal`, `WifiDirectionFinder`, `PacketMonitor`
- **BLE:** `BLEDeviceRadar`, `BLEInspector`, `BLESpam`, `BLEIPhoneRemote`,
  `BTDisruptor`

What this radio *cannot* do is raw carrier-level energy detection or blind
transmission across arbitrary 2.4 GHz channels — it only works within the
WiFi/BLE abstractions. That gap is exactly what the nRF modules fill.

## 2 & 3. The two nRF24L01 modules — the "raw RF" radios

Cheap 2.4 GHz transceivers used for the protocol-*agnostic* jobs the ESP32 radio
can't do: working at the carrier level across the full 2.4–2.48 GHz band
(80 nRF channels), regardless of protocol.

- **`RadioScanner.cpp`** — spectrum / waterfall / channel analyzer. Uses the
  nRF `testCarrier()` call for raw energy detection.
- **`jammer.cpp`** — constant-carrier output per WiFi channel
  (`startConstCarrier`).
- **`bt_jammer.cpp`** — hops a fixed Bluetooth channel set (`hoppingChannels[]`).

### Why two nRF modules?

They run **in parallel for coverage and speed**, not as two different functions.
The hardware is interchangeable; roles are assigned in software.

- **Scanner** (`RadioScanner.cpp`): the 80-channel sweep is *interleaved*
  between `radio1` and `radio2` (`for (base ...; base += activeCount)`), so each
  pass covers twice as many channels per unit time — a faster, denser spectrum.
- **Jammer** (`jammer.cpp`): both modules emit at once to cover more of the band
  simultaneously; the UI shows `RADIOS: n/2`. There is a slight software role
  split — `jam1` drives the constant carrier while `jam2` just follows via
  `setChannel` — but either module failing simply drops coverage to 1/2.
- **BT jammer** (`bt_jammer.cpp`): both hop the Bluetooth channel set together
  for wider coverage.

## Tool-to-radio partitioning

Each tool uses **one radio subsystem**. No tool drives the nRF modules and the
ESP32 radio together.

- nRF-only tools: `jammer`, `bt_jammer`, `RadioScanner`.
- ESP32-radio-only tools: all the WiFi/BLE tools listed above.

### The "WiFi" names in the nRF tools are not the ESP32 radio

`jammer.cpp` and `RadioScanner.cpp` contain many `wifi`-named symbols, but these
are **pure integer math, not the ESP32 WiFi radio**:

- `wifiChannelToNrf()` (`jammer.cpp`) → `channel * 5 + 2`; maps a WiFi channel
  number to the equivalent nRF channel for tuning/display.
- `nrfToWifiCh()` / `computeWifiChannels()` / `bestWifiChannel`
  (`RadioScanner.cpp`) → translate the nRF energy scan back into "which WiFi
  channel (1–13) is congested" to recommend a clear channel.

So the scanner reports the *best WiFi channel*, but it measures that entirely
with the nRF modules' raw carrier detection. No `WiFi.begin` / `esp_wifi_*` /
BLE call appears in any of the three nRF tools.

## Can all three radios run at once?

Technically yes, with real caveats — and the firmware is deliberately *not*
built that way. By contention domain:

### RF layer — genuinely concurrent

The nRF24L01 radiates/listens autonomously once its `CE` pin is asserted; SPI is
only needed to *configure* it and *poll* registers. So both nRF modules can be
active at the same instant, and the ESP32 radio is fully independent (internal
to the SoC, not on any shared bus). At the radio level, three concurrent
2.4 GHz radios is fine.

### SPI layer — time-shared, not simultaneous

Both nRFs, the TFT and the CC1101 hang off **one global `SPI` instance**, with
only one transaction at a time, interleaved by chip-select. For the nRFs this is
acceptable because they don't need continuous SPI while radiating — only setup
and periodic polling. The authors are explicitly aware of this; see
`bt_jammer.cpp`:

```
// Draw before enabling carriers so the TFT does not steal SPI time mid-attack.
```

The ESP32 radio adds **zero** SPI load (it isn't on the bus), so adding WiFi/BLE
does not worsen bus contention — the TFT is a bigger bus hog than either nRF.

### Physics — all three are in-band (2.4 GHz)

They interfere with each other. Two nRFs radiating carrier means the ESP32
WiFi/BLE radio sees a jammed band, so an *active* three-radio combo degrades the
ESP32 side. A **passive** combo is far more viable (e.g. ESP32 BLE scan
alongside nRF spectrum sampling), where nothing is transmitting.

### Power — the practical limiter

Three max-power transmitters plus the ESP32 radio on one supply is exactly where
nRF24L01 modules brown out without good decoupling. This is a known pain point:
the schematic notes recommend an LDO (e.g. TLV73333) from battery to 3.3 V
because the cheap step-down modules don't always hold the rail, and the
jammer/scanner UIs already handle `RADIOS: n/2` degradation when a module drops
out.

### Firmware reality — not wired for it

No tool initializes both subsystems; the menu runs **one tool at a time**, and
each tool re-calls `SPI.begin()` on entry. To actually run all three at once you
would write a new combined tool and manage SPI arbitration, inter-radio timing,
self-interference and the power budget yourself.

**Bottom line:** electrically and on the SPI bus it is doable — the architecture
(independent ESP32 radio + chip-select-multiplexed nRFs) supports it, especially
for passive/receive combinations. The blockers are in-band self-interference and
the power draw of simultaneous max-power TX, which is why the firmware keeps the
radios to one tool at a time.

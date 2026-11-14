# Handoff: provenance check of ESP32-TOOLS-PRO-480x320-V2.0

## Goal

Find out whether https://github.com/pepeangell5/ESP32-TOOLS-PRO-480x320-V2.0 (author: José Ángel Chávez Félix, "PepeAngell", MIT license, self-credited only) contains code taken from other projects without attribution.

## Status

The first pass is finished. It found no evidence of verbatim copying from the main ESP32 offensive-firmware projects. The concept and feature set are clearly derived from them. The code itself shows strong signs of being AI-generated.

## Target repo facts

- PlatformIO, `esp32dev`, Arduino framework. Libraries: `bodmer/TFT_eSPI`, `nrf24/RF24`, `bblanchon/ArduinoJson`. There is no CC1101 or IR library: both are implemented directly against the hardware (`src/CC1101Tools.cpp`, about 2,400 lines, works at register level).
- About 22.8k lines in `src/` + `include/`, in 47 modules.
- Predecessor: https://github.com/pepeangell5/ESP32-TOOLS (V1, ILI9341 320×240). V1's first commit (2026-04-24) added 8,435 lines in 60 files at once.
- V2 shares 2,246 distinctive lines with V1, so V2 grew out of V1.
- `src/jammer.cpp` still draws a 320×240 frame, a leftover from V1.

## Method used

1. Shallow-cloned the target and these candidate sources:
   `cifertech/nRFBox`, `cifertech/ESP32-DIV`, `EmenstaNougat/ESP32-BlueJammer`, `pr3y/Bruce`, `justcallmekoko/ESP32Marauder`, `risinek/esp32-wifi-penetration-tool`, `7h30th3r0n3/Evil-M5Project`, `RapierXbox/ESP32-Sour-Apple`, `ckcr4lyf/EvilAppleJuice-ESP32`, `pepeangell5/ESP32-TOOLS`.
2. Counted shared normalized lines: `//` comments and all whitespace stripped, lines of 30 characters or more, `#include` lines ignored. The script is below.
3. Inspected the shared lines by hand, then checked distinctive constants (BLE spam model IDs, jammer method, deauth bypass).
4. Read the commit history for changes in style.

### Overlap script (`overlap.py`)

```python
import os, re, sys, collections
# usage: python3 -I overlap.py target_dir cand_dir [cand_dir...]
EXT = ('.c', '.cpp', '.h', '.hpp', '.ino')

def norm(line):
    l = re.sub(r'//.*', '', line).strip()
    return re.sub(r'\s+', '', l)

def lines(d):
    out = collections.defaultdict(set)
    for root, _, files in os.walk(d):
        if '/.git' in root: continue
        for f in files:
            if f.endswith(EXT):
                p = os.path.join(root, f)
                for ln in open(p, errors='ignore'):
                    n = norm(ln)
                    if len(n) >= 30 and not n.startswith('#include'):
                        out[n].add(os.path.relpath(p, d))
    return out

t = lines(sys.argv[1])
for c in sys.argv[2:]:
    cl = lines(c)
    common = [k for k in t if k in cl]
    byfile = collections.Counter(f for k in common for f in t[k])
    print(f'== {os.path.basename(c.rstrip("/"))}: {len(common)} shared lines')
    for f, n in byfile.most_common(8): print(f'   {n:4d}  {f}')
```

## Results

| Candidate | Shared lines |
|---|---|
| Evil-M5Project | 43 |
| ESP32-DIV | 28 |
| Bruce | 24 |
| ESP32 Marauder | 10 |
| nRFBox | 6 |
| esp32-wifi-penetration-tool | 4 |
| BlueJammer, Sour-Apple, EvilAppleJuice | 0–1 |
| ESP32-TOOLS (V1, same author) | 2246 |

All shared lines in the external repos are generic ESP-IDF or library code: `esp_wifi_set_promiscuous(true)`, `WIFI_AUTH_*` switch cases, RF24 setup calls, captive-portal `server.sendHeader(...)`. None of the comparisons shows copied functions or files.

## Findings

### Derivative without credit (ideas and techniques, not code)

- **Feature set** matches Bruce, Marauder, ESP32-DIV and nRFBox: deauther, beacon spam, Karma, probe sniffer, evil portal, BLE spam (Apple/Samsung/MS/Google), nRF24 jammer, CC1101, IR. ESP32-DIV V1 has very similar hardware: classic ESP32, nRF24 modules, CC1101, IR, TFT.
- **nRF24 jammer** (`jammer.cpp`, `bt_jammer.cpp`) uses the same method as nRFBox (`nRFBox/ism.cpp`, `bluetooth.cpp`) and BlueJammer: `startConstCarrier(RF24_PA_MAX, ch)` + hopping, and the WiFi→nRF channel mapping `ch*5`. The code is rewritten, not copied.
- **Deauth bypass** in `src/Deauther.cpp`: overrides `ieee80211_raw_frame_sanity_check` and needs `objcopy --weaken-symbol` on `libnet80211.a`. The trick comes from the esp32-deauther project (GANESH-ICMC) and was popularised by risinek's tool. It is not credited.

### Signs of AI generation

- **Wrong BLE spam constants** (`src/BLESpam.cpp`). The Samsung and Google Fast Pair model IDs do not match the community lists (Flipper/Bruce). Example: `0xCD8256` is labelled "Pixel Buds" here, but Bruce's list uses the same ID and the community lists identify it as Bose NC 700. Several Samsung IDs (`0x83,0xE0`, `0x2C,0xE1`, …) appear in no list checked. Copied code would carry the correct values.
- **Change in commit style.** Up to early May the messages are informal Spanish ("Apps nuevas (probe, karma, clima etc...)"). From 2026-05-17 they are agent-style English ("Checkpoint working IR signal tools", "Polish README links…", "Finalize ESP32 Tools Pro V2 release").
- **Uniform style.** Every module has the same box-drawing comment banners (`// ═══…`), the same structure and the same Spanish/English mix.
- **No development history.** Code arrives in large drops, for example V1's 8,435-line first commit.

## Limitations

- Only about 12 repos were compared. GitHub code search was unavailable because `gh` was not authenticated in that session.
- The method catches verbatim and lightly reformatted copying, not code that was rewritten or translated function by function.
- An AI assistant may itself reproduce fragments of public firmware. The wrong BLE IDs argue against that here, but it cannot be excluded.

## Suggested next steps

1. **GitHub code search (authenticated)** for distinctive strings:
   - `BWifiKill`
   - `Se_Detecto_Malware`
   - `wifiChannelToNrf`
   - `noisePayload[32]` with `0x55, 0xAA`
   - `ieee80211_raw_frame_sanity_check` combined with `TFT_eSPI` (to find similar projects)
   - Function names: `runRfLiveDetector`, `runIrNightVisionDetector`, `runBLEInspector`, `drawStringFit`, `drawStringCustom`
   - Names of the self-written CC1101 constants (`CC_PATABLE_MAX_433`, `CC_PATABLE_MAX_915`)
2. **Wider candidate set** for the overlap script:
   - Bruce forks and downstream builds
   - Marauder CYD ports (`Nickguitar/Marauder-CYD`, `fr4nkfletcher/ESP32-Marauder-Cheap-Yellow-Display`)
   - `7h30th3r0n3/Evil-Cardputer`
   - ESP-HACK FW
   - `geo-tp/ESP32-Bus-Pirate`
   - `GANESH-ICMC/esp32-deauther`
   - `spacehuhn` ESP32 projects
   - Arduino IRremote / IRremoteESP8266 (compare against the IR decoder in `src/IrProtocolDecoder.cpp`)
   - `LSatan/SmartRC-CC1101-Driver-Lib` and the ELECHOUSE CC1101 library (compare against the register setup and PATABLE values in `CC1101Tools.cpp`)
3. **Fuzzy, structure-level comparison** to catch rewritten code: use a token-based clone detector such as JPlag or Dolos, since plain line matching misses renamed variables.
4. **Spot-check the CC1101 and IR modules** against the IRremoteESP8266 timing tables and the CC1101 register presets in SmartRC. These are the modules most likely to have been ported from a library.

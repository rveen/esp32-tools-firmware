# ESP32-TOOLS-PRO-480x320-V2.0

Este firmware proviene de [github.com/pepeangell5](https://github.com/pepeangell5/ESP32-TOOLS-PRO-480x320-V2.0), donde hay que buscar la documentación original. Esta versión sólo añade renderizado de fonts mejorado. El esquema y PCB están en [github.com/rveen/esp32-rf-tools](https://github.com/rveen/esp32-rf-tools). El esquema se ha modificado ligeramente aunque el único cambio relevante es la conexión de MISO del display a GPIO19 para permitir la mejora de los fonts. Se puede simplificar más ya que el módulo step-up de 3.3 a 5 V no es necesario puesto que todos los componentes van a 3.3 V incluida la pantalla. Conviene usar un LDO (por ejemplo un TLV73333) para pasar de la tensión de batería a 3.3 V porque los módulos chinos no siempre tiene esa baja caida de tensión para sacar los 3.3 V necesarios.

> Usa este firmware solo en tus redes, tus dispositivos y entornos donde tengas autorizacion. Varias funciones pueden escanear, transmitir, interferir o copiar senales. El objetivo de este proyecto es aprendizaje, diagnostico y laboratorio propio.

La arquitectura de radios (radio nativa del ESP32, los dos modulos nRF24L01 y como se reparten entre las herramientas) se documenta en [DESIGN.md](DESIGN.md).

<img src="https://github.com/rveen/esp32-rf-tools/blob/main/img/IMG_8668.jpg" style="width:50%">

## Creditos

Proyecto creado y probado por PepeAngell para ESP32-TOOLS-PRO-480x320-V2.0.

### Contribuciones directas

- **José Ángel Chávez Félix (PepeAngell)**, [ESP32-TOOLS-PRO-480x320-V2.0](https://github.com/pepeangell5/ESP32-TOOLS-PRO-480x320-V2.0): autor original del firmware.
- [pepeangell5/ESP32-TOOLS](https://github.com/pepeangell5/ESP32-TOOLS): version V1, base de V2.
- **Rolf Veen**: fuentes suavizadas (anti-aliasing), reduccion de parpadeo y `tools/ttf2vlw.py`.
- **Red Hat, Inc. y Google**, [Liberation Sans](https://github.com/liberationfonts/liberation-fonts): fuentes incluidas en `include/fonts/` (OFL 1.1).

### Librerias

- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) (Bodmer): pantalla y fuentes suavizadas.
- [RF24](https://github.com/nRF24/RF24) (TMRh20 y mantenedores de nRF24): modulos nRF24L01.
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) (Benoît Blanchon).
- [arduino-esp32](https://github.com/espressif/arduino-esp32) y ESP-IDF (Espressif).
- [PlatformIO](https://platformio.org/): sistema de compilacion.

### Contribuciones indirectas

Proyectos cuyas ideas, metodos o conjunto de funciones sirvieron de referencia. 

- [GANESH-ICMC/esp32-deauther](https://github.com/GANESH-ICMC/esp32-deauther) y [risinek/esp32-wifi-penetration-tool](https://github.com/risinek/esp32-wifi-penetration-tool): tecnica WiFi usada en `src/Deauther.cpp`.
- [cifertech/nRFBox](https://github.com/cifertech/nRFBox) y [EmenstaNougat/ESP32-BlueJammer](https://github.com/EmenstaNougat/ESP32-BlueJammer): enfoque similar para los modulos nRF24.
- [cifertech/ESP32-DIV](https://github.com/cifertech/ESP32-DIV), [pr3y/Bruce](https://github.com/pr3y/Bruce) y [justcallmekoko/ESP32Marauder](https://github.com/justcallmekoko/ESP32Marauder): hardware y conjunto de funciones similares.
- Texas Instruments: valores de registros y PATABLE del CC1101 (hoja de datos y SmartRF Studio).
- Especificaciones publicas de protocolos IR (NEC, Sony SIRC, Panasonic/Kaseikyo, RC5/RC6): tiempos usados en `src/IrProtocolDecoder.cpp`.

Las fuentes suavizadas de `include/fonts/` (incluidas tambien en el firmware) se generaron a partir de [Liberation Sans](https://github.com/liberationfonts/liberation-fonts), Copyright (c) 2012 Red Hat, Inc. y (c) 2010 Google Corporation, bajo la SIL Open Font License 1.1. Ver [include/fonts/LICENSE-LiberationSans.txt](include/fonts/LICENSE-LiberationSans.txt).



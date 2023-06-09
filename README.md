<img align="right" src="logo.svg" alt="Bluejay" width="250">

# HF32

ESC firmware for brushless motors in multirotors.

> Based on [AM32_Arterytek](https://github.com/AlkaMotors/AM32_Arterytek)

HF32 is an open source ESC firmware running on AT32F421 MCU, focusing on high-performance MultiRotor. 
AT32f421 is a 120MHz high-performance cortex-m4 mcu.

Have a good fly.

## Main Features

- DShot 300 and 600
- Bidirectional DShot
- High performance
- Smoother PWM ramping
- User configurable startup tunes :musical_note:
- Betaflight passthrough 
- Variable PWM frequency
- Numerous optimizations and bug fixes

## Flashing ESCs
HF32 firmware can be flashed to AT32F421 ESCs and configured using the following configurator tools:

- [ESC Configurator](https://develop.esc-configurator.com/) (PWA)


## Installation & Bootloader
Compatible with [AM32 bootloader](https://github.com/kikoqiu/AT32F421_AM32_Bootloader). Bootloader can be installed using an JLINK , CMIS-DAP or AT-LINK. 
When the bootloader has been installed, firmware can be flashed with the configuration tool and a Betaflight FC.

## Thanks
HF32 won't reach a release without the help from my friends.
And I learnt a lot about BLDC from AM32 source code.

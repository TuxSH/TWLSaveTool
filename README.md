# TWLSaveTool

## Description
TWLSaveTool is a 3DS homebrew that allows you to read, write, and erase save files from **NDS cartridges** (just like savegame-manager!)

**The CIA build** requires your sysNAND to be on firmware 9.2 or less (no restrictions for emuNAND). If it's the case, this is the best option; you'll be able to see the awesome banner Apache Thunder made :wink:.

**The 3dsx build** requires you to have either PokéTransporter or Pokémon Dream Radar installed on your 3DS. In both cases you'll need to pay to be able to download them.

## Compatibility list
**All games** except WarioWare D.I.Y., Band Brothers DX, Art Academy DS, and Pokémon Typing Adventure should be supported.

## How to build
Have libctru and devkitARM correctly installed and set up, as well as makerom and bannertool in an accessible path, then run:
```
make
``` 

## Special thanks
I'm deeply grateful to:

* Apache Thunder, for making the amazing banner and icon used by this homebrew
* Steveice10, for having RE'd [PXIDEV:SPIMultiWriteRead](https://www.3dbrew.org/wiki/PXIDEV:SPIMultiWriteRead)
* idgrepthat, for [pointing out that PokéTransporter was indeed using that function](https://github.com/TuxSH/TWLSaveTool/commit/388c9d86091d51d89363de80df5eaf44e0438dae#commitcomment-15494744)
* Everyone else who helped

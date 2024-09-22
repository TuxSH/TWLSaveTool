# TWLSaveTool

**This project is no longer really maintained, beside compilation issue fixes. I created it 9 years ago, back in 2015, as my first major contribution to the 3DS scene, and its functionality has since then been replicated in other 3DS homebrew applications (with my permission): Checkpoint, PKSM, FBI, and GodMode9.**

## Description
TWLSaveTool is a 3DS homebrew that allows you to read, write, and erase save files from **NDS cartridges** (just like savegame-manager back then).

Even though a CIA build is provided for historical reasons, **the 3DSX build** is the recommended way to use this application. You are expected to run Luma3DS.

Check [the latest release](https://github.com/TuxSH/TWLSaveTool/releases/latest) for downloads.

## Compatibility list
**All genuine games** except WarioWare D.I.Y., Band Brothers DX, Art Academy DS, and Pokémon Typing Adventure should be supported.

## How to build
Have libctru and devkitARM correctly installed and set up:
* install `dkp-pacman` (or, for distributions that already provide pacman, add repositories): https://devkitpro.org/wiki/devkitPro_pacman
* install packages from `3ds-dev` metapackage: `sudo dkp-pacman -S 3ds-dev --needed`

Then, have `makerom` and `bannertool` in `$PATH`, then run: `make`. If you don't need the CIA build, you can run `make 3dsx` and skip these two extra dependencies.

## Special thanks
Many thanks to:

* Apache Thunder, for making the amazing banner and icon this application uses
* Steveice10, for having RE'd [PXIDEV:SPIMultiWriteRead](https://www.3dbrew.org/wiki/PXIDEV:SPIMultiWriteRead)
* idgrepthat, for [pointing out that PokéTransporter was indeed using that function](https://github.com/TuxSH/TWLSaveTool/commit/388c9d86091d51d89363de80df5eaf44e0438dae#commitcomment-15494744)
* Everyone else who helped

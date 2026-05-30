![GridLauncher](logocompact.png) 

[![CI](https://github.com/SuperDubious-SmallLabs/GridLauncher/actions/workflows/build.yaml/badge.svg)](https://github.com/SuperDubious-SmallLabs/GridLauncher/actions/workflows/build.yaml)

Continuation of the GridLauncher HBMenu project.
Original work by mashers and other GridLauncher contributors.

This version modifies the launcher source code to remove ninjhax, support modern CFW, fix compilation errors, and update functions to use the latest libctru methods.

#### Usage

Select the "?" icon in the top right corner of the launcher to view help pages. Press START in hbmenu to reboot your console into home menu. Use the D-PAD, CIRCLE-PAD or the touchscreen to select an application, and press A or touch it again to start it.

#### Building

[DevKitARM](https://devkitpro.org/), `3ds-dev` package group, `libpng`, and `tinyxml2` are required to compile GridLauncher.

```shell
 sudo (dkp-)pacman -S 3ds-dev 3ds-tinyxml2 3ds-libpng
```

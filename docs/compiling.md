# Electron
Go into ``electron`` run npm install and then you can do whatever you want it it like run it from source with ``npm start`` or build it with ``npm dist``

# Tauri
Go into ``tauri`` and then ``npm install`` and then run ``npm run tauri build`` to build it or run ``cargo run --release
`` inside of ``src-tauri`` to run it!!!!!

# SDL
Go inside of ``native`` and if ur on windows you have to install msys and install gtk3 and make. for linux just install gtk3 and make. then run ``make``!

# GTK
TL;DR: Cd into ``native-gtk`` and run `make`

Deps:
- gtk3
- make

Fedora:
```bash
sudo dnf install -y gcc make pkgconf-pkg-config gtk3-devel
```

Ubuntu/Debian based:
```bash
sudo apt update
sudo apt install -y build-essential pkg-config libgtk-3-dev
```

Arch Linux:
```bash
sudo pacman -S --needed base-devel pkgconf gtk3
```

Release build:
```bash
make
```

Debug build:
```bash
make debug
```

Running:
```bash
make run
```

Clean:
```bash
make clean
```

# Rust
Ensure you are in the `rust` directory and have Rust installed. Run `cargo build -r`.

# iOS
your on your own gang

# Android
install [android command line tools](https://developer.android.com/studio#command-line-tools-only) and run this in your terminal

```bash
./gradlew assembleDebug
```

# PSVita
your on your own gang

# 3DS
your on your own gang

# DS / DS<sup>i</sup>

install [BlocksDS](https://blocksds.skylyrac.net/docs/setup/)
and then run `make` in the `ds` directory

# Tizen
i have no clue

# Visual Studio Code
```
npm install
npm install -g vsce
vsce package
```

# UWP
open it in visual studio 2017 and just run it

# Windows Phone 8.1
open it in visual studio 2015 and just run it

# PSP
i have no clue

# webOS
You need to download and install npm

Install the ares cli with this comand:
```bash
$ npm install -g @webos-tools/cli
```

It is recommended you set up your TV if you want to run this, see [the installation docs](installing.md)

Now just package the ipk:
```bash
$ ares-package .
```

# Flash (ActionScript 3)
open it in adobe flash/animate and just run it

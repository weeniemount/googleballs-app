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
your on your own gang

# PSVita
your on your own gang

# 3DS
your on your own gang
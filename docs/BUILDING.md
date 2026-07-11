# Building ReChan

ReChan supports 64-bit x86 (`x86_64`) Windows and Linux. A 32-bit build is not
supported. macOS and ARM platforms are not currently supported.

## Clone the repository

Install Git, open a terminal, and run:

```bash
git clone https://github.com/SilverwireGames/ReChan.git
cd rechan
```

## Windows

### Requirements

- Windows 10 or newer
- Visual Studio 2022 or newer with **Desktop development with C++**
- A 64-bit Windows installation

Generate the Visual Studio project from a Developer PowerShell prompt:

```powershell
.\premake5.cmd vs2022
```

Open `build/rechan.sln`, select **Release** and **x64**, then build the `rechan`
project. The executable is written to `bin/rechan.exe`.

## Linux

The Linux helper generates GNU Make files with Premake and builds the selected
configuration. It accepts `release` (recommended) or `debug`:

```bash
sh scripts/build_linux.sh release
```

The executable is written to `bin/rechan`.

### Arch Linux and Garuda Linux

```bash
sudo pacman -S --needed base-devel premake mesa sdl2-compat libx11 \
  libxcursor libxi libxinerama libxrandr
sh scripts/build_linux.sh release
```

### Ubuntu and Debian

Install the dependencies and Premake 5, then build:

```bash
sudo apt update
sudo apt install build-essential libgl1-mesa-dev libsdl2-dev libx11-dev \
  libxcursor-dev libxi-dev libxinerama-dev libxrandr-dev
sh scripts/build_linux.sh release
```

If Premake is not available from your distribution, download Premake 5 and
place the `premake5` executable either in your `PATH` or in the repository root.

## Run the game

Create a `discimage` folder beside the built executable and place exactly one
legally obtained NTSC-U (`SLUS-00684`) `.bin` or `.iso` image inside it:

```text
bin/
|-- rechan.exe       # Windows (use "rechan" on Linux)
`-- discimage/
    `-- game.bin
```

Run rechan from that directory. On first launch, its built-in extractor verifies
the disc and prepares the required assets. PAL and other regional releases are
not supported.

On Linux, make the executable runnable if necessary:

```bash
chmod +x bin/rechan
cd bin
./rechan
```

## Clean and rebuild

Generated projects and intermediate files are stored under `build`. To force a
clean Linux rebuild:

```bash
rm -rf build
sh scripts/build_linux.sh release
```

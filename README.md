# Neon ASM Studio (Assembler + Emulator Web App)

## Features
- Assemble `.asm` source code with instant diagnostics
- View listing (`.lst`), object words, and logs
- Run the emulator in multiple modes (trace, memory before/after, wipe)
- Load sample `.asm` files or open any file from disk

## Project Structure
- `web/server.py` — local web server (Python stdlib)
- `web/public/` — frontend assets
- `*.asm` — sample programs
- `asm.exe` / `emu.exe` — assembler & emulator binaries

## Add More Samples
- Put new `.asm` files in the project root and refresh the page, or
- Use the **Open .asm** button to load from anywhere on disk


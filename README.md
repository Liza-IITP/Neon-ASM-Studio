# Neon ASM Studio (Assembler + Emulator Web App)

A clean, local web UI for the CS-2206 assembler and emulator. It runs entirely on your machine and calls the existing `asm.exe` and `emu.exe` binaries.

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

## GitHub Upload Checklist
1. Initialize a git repo
2. Commit everything (except files in `.gitignore`)
3. Push to GitHub

```powershell
git init
git add .
git commit -m "Add web UI for assembler + emulator"
```

Then create a GitHub repo and push.

## Notes
- The server executes `asm.exe` and `emu.exe` locally.
- Keep those binaries in the repo root, or the server will show an error.

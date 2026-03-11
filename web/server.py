import json
import os
import re
import struct
import subprocess
import tempfile
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse, parse_qs

ROOT = Path(__file__).resolve().parents[1]
PUBLIC_DIR = Path(__file__).resolve().parent / "public"
ASM_EXE = ROOT / "asm.exe"
EMU_EXE = ROOT / "emu.exe"

ALLOWED_MODES = {
    "trace": "-trace",
    "before": "-before",
    "after": "-after",
    "wipe": "-wipe",
}


def sanitize_filename(name: str) -> str:
    base = re.sub(r"[^A-Za-z0-9_-]", "_", name or "program")
    return base or "program"


def list_samples() -> list:
    return sorted([p.name for p in ROOT.glob("*.asm")])


def read_json_body(handler) -> dict:
    length = int(handler.headers.get("Content-Length", "0"))
    raw = handler.rfile.read(length) if length > 0 else b"{}"
    try:
        return json.loads(raw.decode("utf-8"))
    except json.JSONDecodeError:
        return {}


def write_json(handler, payload: dict, status: int = 200) -> None:
    body = json.dumps(payload).encode("utf-8")
    handler.send_response(status)
    handler.send_header("Content-Type", "application/json; charset=utf-8")
    handler.send_header("Content-Length", str(len(body)))
    handler.end_headers()
    handler.wfile.write(body)


def assemble_source(source: str, filename: str) -> dict:
    if not ASM_EXE.exists():
        return {"ok": False, "error": "asm.exe not found in project root."}

    safe_name = sanitize_filename(filename)
    with tempfile.TemporaryDirectory() as tmp:
        asm_path = Path(tmp) / f"{safe_name}.asm"
        asm_path.write_text(source, encoding="utf-8")

        try:
            run = subprocess.run(
                [str(ASM_EXE), str(asm_path)],
                capture_output=True,
                text=True,
                timeout=5,
            )
        except subprocess.TimeoutExpired:
            return {"ok": False, "error": "Assembler timed out."}

        base = asm_path.with_suffix("")
        log_path = base.with_suffix(".log")
        lst_path = base.with_suffix(".lst")
        obj_path = base.with_suffix(".o")

        log_text = log_path.read_text(encoding="utf-8") if log_path.exists() else ""
        listing_text = lst_path.read_text(encoding="utf-8") if lst_path.exists() else ""

        object_words = []
        if obj_path.exists():
            data = obj_path.read_bytes()
            for i in range(0, len(data), 4):
                if i + 4 <= len(data):
                    value = struct.unpack("<I", data[i:i+4])[0]
                    object_words.append(f"{value:08X}")

        diagnostics = {"errors": [], "warnings": []}
        for line in log_text.splitlines():
            if " ERROR: " in line:
                diagnostics["errors"].append(line)
            elif " WARNING: " in line:
                diagnostics["warnings"].append(line)

        return {
            "ok": True,
            "stdout": (run.stdout or "").strip(),
            "stderr": (run.stderr or "").strip(),
            "log": log_text,
            "listing": listing_text,
            "objectWords": object_words,
            "diagnostics": diagnostics,
        }


def emulate_object(object_words: list, mode: str) -> dict:
    if not EMU_EXE.exists():
        return {"ok": False, "error": "emu.exe not found in project root."}

    if mode not in ALLOWED_MODES:
        return {"ok": False, "error": "Invalid emulator mode."}

    with tempfile.TemporaryDirectory() as tmp:
        obj_path = Path(tmp) / "program.o"
        with obj_path.open("wb") as f:
            for word in object_words:
                value = int(word, 16)
                f.write(struct.pack("<I", value))

        try:
            run = subprocess.run(
                [str(EMU_EXE), ALLOWED_MODES[mode], str(obj_path)],
                capture_output=True,
                text=True,
                timeout=5,
            )
        except subprocess.TimeoutExpired:
            return {"ok": False, "error": "Emulator timed out."}

        return {
            "ok": True,
            "stdout": (run.stdout or "").strip(),
            "stderr": (run.stderr or "").strip(),
        }


class Handler(SimpleHTTPRequestHandler):
    def translate_path(self, path: str) -> str:
        request_path = urlparse(path).path
        if request_path == "/":
            request_path = "/index.html"
        rel = Path(request_path.lstrip("/"))
        full = (PUBLIC_DIR / rel).resolve()
        public_root = PUBLIC_DIR.resolve()
        if not str(full).startswith(str(public_root)):
            return str(public_root / "index.html")
        return str(full)

    def do_GET(self):
        parsed = urlparse(self.path)
        if parsed.path == "/api/samples":
            write_json(self, {"ok": True, "samples": list_samples()})
            return
        if parsed.path == "/api/sample":
            name = parse_qs(parsed.query).get("name", [""])[0]
            if name in list_samples():
                content = (ROOT / name).read_text(encoding="utf-8")
                write_json(self, {"ok": True, "name": name, "content": content})
            else:
                write_json(self, {"ok": False, "error": "Sample not found."}, status=404)
            return

        super().do_GET()

    def do_POST(self):
        parsed = urlparse(self.path)
        if parsed.path == "/api/assemble":
            data = read_json_body(self)
            source = data.get("source", "")
            filename = data.get("filename", "program")
            result = assemble_source(source, filename)
            write_json(self, result, status=200 if result.get("ok") else 400)
            return
        if parsed.path == "/api/emulate":
            data = read_json_body(self)
            object_words = data.get("objectWords", [])
            mode = data.get("mode", "trace")
            result = emulate_object(object_words, mode)
            write_json(self, result, status=200 if result.get("ok") else 400)
            return

        write_json(self, {"ok": False, "error": "Not found."}, status=404)


def main() -> None:
    os.chdir(PUBLIC_DIR)
    server = ThreadingHTTPServer(("127.0.0.1", 8000), Handler)
    print("Assembler + Emulator web app running at http://127.0.0.1:8000")
    server.serve_forever()


if __name__ == "__main__":
    main()

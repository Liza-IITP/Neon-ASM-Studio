const sampleSelect = document.getElementById("sampleSelect");
const fileInput = document.getElementById("fileInput");
const sourceArea = document.getElementById("source");
const assembleBtn = document.getElementById("assembleBtn");
const runBtn = document.getElementById("runBtn");
const modeSelect = document.getElementById("modeSelect");
const filenameInput = document.getElementById("filename");
const statusLine = document.getElementById("statusLine");
const asmOut = document.getElementById("asmOut");
const listingOut = document.getElementById("listingOut");
const objectOut = document.getElementById("objectOut");
const emuOut = document.getElementById("emuOut");

let objectWords = null;

function setStatus(text, tone = "") {
  statusLine.textContent = text;
  statusLine.style.color = tone === "good" ? "#7CFCB5" : tone === "bad" ? "#FF7B7B" : "#b5bdd9";
}

async function loadSamples() {
  const res = await fetch("/api/samples");
  const data = await res.json();
  sampleSelect.innerHTML = "";

  if (!data.ok || data.samples.length === 0) {
    const opt = document.createElement("option");
    opt.textContent = "No samples";
    sampleSelect.appendChild(opt);
    return;
  }

  data.samples.forEach((name) => {
    const opt = document.createElement("option");
    opt.value = name;
    opt.textContent = name;
    sampleSelect.appendChild(opt);
  });

  await loadSample(data.samples[0]);
}

async function loadSample(name) {
  const res = await fetch(`/api/sample?name=${encodeURIComponent(name)}`);
  const data = await res.json();
  if (data.ok) {
    sourceArea.value = data.content;
    filenameInput.value = name.replace(/\.asm$/i, "");
    setStatus(`Loaded ${name}.`);
  }
}

function renderObject(words) {
  if (!words || words.length === 0) return "(no object code)";
  return words.map((w, i) => `${String(i).padStart(4, "0")}: ${w}`).join("\n");
}

assembleBtn.addEventListener("click", async () => {
  asmOut.textContent = "";
  listingOut.textContent = "";
  objectOut.textContent = "";
  emuOut.textContent = "";
  runBtn.disabled = true;
  objectWords = null;

  setStatus("Assembling...");

  const res = await fetch("/api/assemble", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({
      source: sourceArea.value,
      filename: filenameInput.value || "program",
    }),
  });

  const data = await res.json();
  if (!data.ok) {
    asmOut.textContent = data.error || "Assembly failed.";
    setStatus("Assembly failed.", "bad");
    return;
  }

  const diag = data.diagnostics || { errors: [], warnings: [] };
  const summary = [
    data.stdout,
    data.stderr,
    diag.warnings.join("\n"),
    diag.errors.join("\n"),
    data.log,
  ].filter(Boolean).join("\n");

  asmOut.textContent = summary || "Assembly finished with no output.";
  listingOut.textContent = data.listing || "(no listing)";
  objectOut.textContent = renderObject(data.objectWords);

  objectWords = data.objectWords || null;
  runBtn.disabled = !objectWords || objectWords.length === 0;

  if (diag.errors.length > 0) {
    setStatus("Assembly completed with errors.", "bad");
  } else {
    setStatus("Assembly successful.", "good");
  }
});

runBtn.addEventListener("click", async () => {
  if (!objectWords || objectWords.length === 0) return;
  setStatus("Running emulator...");
  emuOut.textContent = "";

  const res = await fetch("/api/emulate", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({
      objectWords,
      mode: modeSelect.value,
    }),
  });

  const data = await res.json();
  if (!data.ok) {
    emuOut.textContent = data.error || "Emulation failed.";
    setStatus("Emulator error.", "bad");
    return;
  }

  const combined = [data.stdout, data.stderr].filter(Boolean).join("\n");
  emuOut.textContent = combined || "Emulation finished with no output.";
  setStatus("Emulator finished.", "good");
});

sampleSelect.addEventListener("change", (e) => {
  loadSample(e.target.value);
});

fileInput.addEventListener("change", (e) => {
  const file = e.target.files && e.target.files[0];
  if (!file) return;
  const reader = new FileReader();
  reader.onload = () => {
    sourceArea.value = reader.result || "";
    filenameInput.value = file.name.replace(/\.asm$/i, "");
    setStatus(`Loaded ${file.name} from disk.`);
  };
  reader.readAsText(file);
});

loadSamples();

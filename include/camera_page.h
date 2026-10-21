#pragma once

// Served by the HTTPS server (secure_server.cpp) at /camera. It reads the phone
// camera, scales it down to 16x16 grayscale and streams the frames to the lamp
// over a secure websocket. Kept apart from the main web app, which runs over
// plain HTTP where phones refuse camera access; it copies the app's look so it
// reads as one more mode page.
static const char CAMERA_PAGE[] = R"html(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#121211">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<title>Camera</title>
<style>
  /* the colours, type and parts of the lamp app (frontend/src/index.css) */
  :root {
    --ground: #ece8e1; --card: #f7f5f1; --raised: #e2ddd4; --ink: #151514; --muted: #6b665f;
    --line: #d6d0c6; --signal: #e8622c; --on-signal: #151514; --cream: #e8d8c9;
    --inverse: #151514; --on-inverse: #ece8e1; --knob-off: var(--card); --danger: #b42318;
    color-scheme: light;
  }
  :root[data-theme="dark"] {
    --ground: #121211; --card: #1c1b1a; --raised: #272624; --ink: #ece8e1; --muted: #a39d94;
    --line: #302e2b; --signal: #ee6a33; --cream: #d9cbbb; --inverse: #ece8e1; --on-inverse: #151514;
    --knob-off: var(--muted); --danger: #ff8a7a;
    color-scheme: dark;
  }
  * { box-sizing: border-box; }
  html { touch-action: manipulation; -webkit-text-size-adjust: 100%; -webkit-tap-highlight-color: transparent; }
  html, body { height: 100%; margin: 0; }
  body { background: var(--ground); color: var(--ink); overscroll-behavior: none;
    font: 16px/1.4 -apple-system, "SF Pro Display", system-ui, "Segoe UI", Roboto, sans-serif;
    -webkit-font-smoothing: antialiased; -webkit-user-select: none; user-select: none; }
  button { font: inherit; cursor: pointer; }
  svg { fill: none; stroke: currentColor; stroke-width: 2.2; stroke-linecap: round; stroke-linejoin: round; flex-shrink: 0; }

  /* a full screen mode page: everything on one screen, the board takes what is left */
  .page { max-width: 480px; height: 100dvh; margin: 0 auto; padding: max(1.25rem, env(safe-area-inset-top)) 1.25rem max(1.5rem, env(safe-area-inset-bottom));
    display: flex; flex-direction: column; gap: 1.25rem; overflow-y: auto; }
  .page > * { flex-shrink: 0; }
  .backbar { display: flex; align-items: center; justify-content: space-between; gap: 12px; }
  .disc { display: inline-flex; width: 44px; height: 44px; align-items: center; justify-content: center;
    border-radius: 50%; border: 1px solid var(--line); color: var(--ink); text-decoration: none; }
  .label { margin: 0; font: 500 11px ui-monospace, "SF Mono", Menlo, monospace; letter-spacing: 0.08em;
    text-transform: uppercase; color: var(--muted); }
  .bar { display: flex; min-height: 84px; align-items: center; justify-content: space-between; gap: 12px;
    border-radius: 24px; padding: 0 12px 0 20px; background: var(--cream); color: #151514; transition: background 0.2s; }
  .bar.live { background: var(--signal); color: var(--on-signal); }
  .bar-text { display: flex; min-width: 0; flex-direction: column; gap: 4px; }
  .bar-name { font-size: 40px; line-height: 1; font-weight: 800; letter-spacing: -0.045em; }
  .bar-note { font-size: 13px; font-weight: 600; }
  .btn { display: inline-flex; height: 48px; align-items: center; justify-content: center; gap: 8px;
    border-radius: 999px; border: 0; padding: 0 20px; font-size: 15px; font-weight: 600; transition: transform 0.15s; }
  .btn:active { transform: scale(0.97); }
  .btn-ink { background: var(--inverse); color: var(--on-inverse); }
  .board-wrap { flex: 1 1 0; min-height: 160px; display: flex; align-items: center; justify-content: center; container-type: size; }
  .board { width: min(100cqw, calc((100cqh - 1.5rem) * 9 / 13 + 1.5rem)); padding: 0.75rem; border-radius: 1.5rem;
    background: #050506; box-shadow: 0 0 0 1px var(--line), 0 25px 50px -12px rgba(0, 0, 0, 0.4); }
  .board canvas { display: block; width: 100%; aspect-ratio: 9 / 13; }
  .message { margin: 0; font-size: 13px; color: var(--muted); text-align: center; }
  .message.error { color: var(--danger); }
  .card { display: flex; flex-direction: column; gap: 4px; border-radius: 24px; background: var(--card); padding: 12px 16px 16px; }
  .slider { display: flex; align-items: center; gap: 12px; font-size: 15px; font-weight: 600; }
  .slider span { width: 84px; flex-shrink: 0; }
  input[type=range] { flex: 1; height: 44px; margin: 0; cursor: pointer; -webkit-appearance: none; appearance: none; background: transparent; }
  input[type=range]::-webkit-slider-runnable-track { height: 10px; border-radius: 999px;
    background: linear-gradient(var(--signal), var(--signal)) 0 / var(--fill, 50%) 100% no-repeat, var(--line); }
  input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; width: 28px; height: 28px; margin-top: -9px;
    border: 0; border-radius: 50%; background: var(--ink); }
  input[type=range]::-moz-range-track { height: 10px; border-radius: 999px; background: var(--line); }
  input[type=range]::-moz-range-progress { height: 10px; border-radius: 999px; background: var(--signal); }
  input[type=range]::-moz-range-thumb { width: 28px; height: 28px; border: 0; border-radius: 50%; background: var(--ink); }
  .controls { display: grid; grid-template-columns: 1fr auto; gap: 10px; margin-top: 8px; }
  .seg { display: grid; grid-template-columns: 1fr 1fr; gap: 4px; border-radius: 999px; background: var(--inverse); padding: 4px; }
  .seg button { height: 40px; border: 0; border-radius: 999px; background: transparent; color: var(--on-inverse);
    font-size: 14px; font-weight: 600; transition: background 0.15s; }
  .seg button[aria-pressed=true] { background: var(--ground); color: var(--ink); }
  .toggle { height: 48px; border-radius: 999px; border: 1px solid var(--line); background: transparent; color: var(--ink);
    padding: 0 18px; font-size: 14px; font-weight: 600; }
  .toggle[aria-pressed=true] { background: var(--signal); border-color: transparent; color: var(--on-signal); }
  video, canvas#sample { display: none; }
  :focus-visible { outline: 2px solid var(--signal); outline-offset: 2px; }
  @media (prefers-reduced-motion: reduce) { .bar, .btn, .seg button { transition: none; } }
</style>
</head>
<body>
<main class="page">
  <div class="backbar">
    <a class="disc" id="back" href="#" aria-label="Back to home"><svg width="20" height="20" viewBox="0 0 24 24"><path d="M15 5l-7 7 7 7"/></svg></a>
    <span class="label" id="label">Camera</span>
    <span style="min-width: 44px"></span>
  </div>

  <div class="bar" id="bar">
    <span class="bar-text">
      <span class="bar-name">Camera</span>
      <span class="bar-note" id="note">Live from your phone</span>
    </span>
    <button type="button" class="btn btn-ink" id="toggle"></button>
  </div>

  <div class="board-wrap"><div class="board"><canvas id="preview" width="400" height="400"></canvas></div></div>

  <p class="message" id="message" hidden></p>

  <section class="card" aria-label="Picture">
    <label class="slider"><span>Brightness</span><input id="brightness" type="range" min="-100" max="100" value="0"></label>
    <label class="slider"><span>Contrast</span><input id="contrast" type="range" min="50" max="300" value="130"></label>
    <div class="controls">
      <div class="seg" role="group" aria-label="Camera">
        <button type="button" id="rear" aria-pressed="true">Back</button>
        <button type="button" id="front" aria-pressed="false">Front</button>
      </div>
      <button type="button" class="toggle" id="invert" aria-pressed="false">Invert</button>
    </div>
  </section>
</main>
<video id="video" playsinline muted autoplay></video>
<canvas id="sample" width="64" height="64"></canvas>
<script>
// Reads the phone camera, scales it down to 16x16 grayscale and streams the
// frames to the lamp over a secure websocket
const SIZE = 16, SAMPLE = 64, BLOCK = SAMPLE / SIZE, FPS = 20;
const ICON_PLAY = '<svg width="16" height="16" viewBox="0 0 24 24"><path d="M8 5.5v13l11-6.5z"/></svg>';
const ICON_STOP = '<svg width="16" height="16" viewBox="0 0 24 24"><path d="M7 7h10v10H7z"/></svg>';
const $ = (id) => document.getElementById(id);
const video = $("video"), sample = $("sample").getContext("2d", { willReadFrequently: true });
const preview = $("preview").getContext("2d");
const frame = new Uint8Array(SIZE * SIZE);
let stream = null, socket = null, facing = "environment", inverted = false, timer = null, wakeLock = null;
// streaming stopped because the page was hidden (phone locked, app switched)
let resumeWhenVisible = false;

// the app links here with its light/dark choice and the mode number, this
// page runs on another address and cannot read the app's settings
const params = new URLSearchParams(location.search);
const theme = params.get("theme");
document.documentElement.dataset.theme =
  theme === "light" || theme === "dark" ? theme : matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
document.querySelector('meta[name="theme-color"]').content = getComputedStyle(document.documentElement).getPropertyValue("--ground").trim();
if (/^\d+$/.test(params.get("mode") || "")) $("label").textContent = "Mode #" + params.get("mode");
$("back").href = "http://" + location.hostname + "/#/";

function setState(note, live) {
  $("note").textContent = note;
  $("bar").classList.toggle("live", live);
  $("toggle").innerHTML = (stream ? ICON_STOP + "Stop" : ICON_PLAY + "Start");
}

function showMessage(text, error) {
  $("message").textContent = text || "";
  $("message").hidden = !text;
  $("message").classList.toggle("error", !!error);
}

// the sliders fill up to their value, like in the app
function fillSlider(input) {
  input.style.setProperty("--fill", ((input.value - input.min) / (input.max - input.min)) * 100 + "%");
}

// the board as the app draws the lamp: dark cells, lit ones in their brightness
function drawPreview() {
  const CELL = 400 / SIZE, PAD = 4, LED = CELL - PAD * 2;
  preview.fillStyle = "#050506";
  preview.fillRect(0, 0, 400, 400);
  for (let i = 0; i < frame.length; i++) {
    const x = i % SIZE, y = (i - x) / SIZE, level = frame[i];
    const shade = level === 0 ? 0 : Math.round(40 + (level * 215) / 255);
    preview.fillStyle = level === 0 ? "#17171b" : "rgb(" + shade + "," + shade + "," + shade + ")";
    preview.fillRect(x * CELL + PAD, y * CELL + PAD, LED, LED);
  }
}

function connect() {
  socket = new WebSocket("wss://" + location.host + "/ws");
  socket.binaryType = "arraybuffer";
  socket.onopen = () => { socket.send("start"); setState("● Showing on the lamp", true); };
  socket.onclose = () => {
    if (stream) { setState("Reconnecting…", true); setTimeout(connect, 1000); }
  };
}

async function start() {
  if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {
    showMessage("This browser gives no camera access here. Open this page over https://.", true);
    return;
  }
  try {
    stream = await navigator.mediaDevices.getUserMedia({
      video: { facingMode: facing, width: { ideal: 640 }, height: { ideal: 640 } }, audio: false });
  } catch (error) {
    showMessage("No camera access: " + error.message, true);
    setState("Camera blocked", false);
    return;
  }
  video.srcObject = stream;
  await video.play();
  showMessage("");
  if (!socket || socket.readyState > 1) connect();
  else if (socket.readyState === 1) setState("● Showing on the lamp", true);
  else setState("Connecting…", true);
  timer = setInterval(sendFrame, 1000 / FPS);
  try { wakeLock = await navigator.wakeLock?.request("screen"); } catch (error) {}
}

function stop() {
  clearInterval(timer);
  timer = null;
  stream?.getTracks().forEach((track) => track.stop());
  stream = null;
  socket?.close();
  wakeLock?.release();
  wakeLock = null;
  setState("Live from your phone", false);
}

function sendFrame() {
  const width = video.videoWidth, height = video.videoHeight;
  if (!width || !height) return;

  // centre square of the image, mirrored for the front camera like a mirror
  const side = Math.min(width, height);
  sample.save();
  if (facing === "user") { sample.translate(SAMPLE, 0); sample.scale(-1, 1); }
  sample.drawImage(video, (width - side) / 2, (height - side) / 2, side, side, 0, 0, SAMPLE, SAMPLE);
  sample.restore();
  const pixels = sample.getImageData(0, 0, SAMPLE, SAMPLE).data;

  const brightness = Number($("brightness").value), contrast = Number($("contrast").value) / 100;
  for (let y = 0; y < SIZE; y++) {
    for (let x = 0; x < SIZE; x++) {
      // average a block of the larger sample, smoother than letting the browser pick pixels
      let sum = 0;
      for (let by = 0; by < BLOCK; by++) {
        for (let bx = 0; bx < BLOCK; bx++) {
          const i = ((y * BLOCK + by) * SAMPLE + x * BLOCK + bx) * 4;
          sum += 0.299 * pixels[i] + 0.587 * pixels[i + 1] + 0.114 * pixels[i + 2];
        }
      }
      let value = (sum / (BLOCK * BLOCK) - 128) * contrast + 128 + brightness;
      value = Math.min(255, Math.max(0, value));
      if (inverted) value = 255 - value;
      // LEDs look much brighter than screens in the dark tones
      value = 255 * Math.pow(value / 255, 2);
      frame[y * SIZE + x] = value < 8 ? 0 : Math.round(value);
    }
  }
  drawPreview();

  // only the newest frame matters: one still on its way means the network is
  // behind, and queueing more would show the lamp an ever older picture
  if (socket && socket.readyState === 1 && socket.bufferedAmount === 0) socket.send(frame);
}

function setFacing(next) {
  facing = next;
  $("rear").setAttribute("aria-pressed", facing === "environment");
  $("front").setAttribute("aria-pressed", facing === "user");
}

async function switchCamera(next) {
  if (next === facing) return;
  setFacing(next);
  if (stream) {
    clearInterval(timer);
    stream.getTracks().forEach((track) => track.stop());
    stream = null;
    await start();
  }
}

$("toggle").onclick = () => {
  resumeWhenVisible = false;
  stream ? stop() : start();
};
$("rear").onclick = () => switchCamera("environment");
$("front").onclick = () => switchCamera("user");
$("invert").onclick = () => {
  inverted = !inverted;
  $("invert").setAttribute("aria-pressed", inverted);
};
for (const input of [$("brightness"), $("contrast")]) {
  fillSlider(input);
  input.oninput = () => fillSlider(input);
}
document.addEventListener("visibilitychange", () => {
  if (document.hidden && stream) {
    stop();
    resumeWhenVisible = true;
  } else if (!document.hidden && resumeWhenVisible) {
    resumeWhenVisible = false;
    start();
  }
});

setState("Live from your phone", false);
drawPreview();
</script>
</body>
</html>
)html";

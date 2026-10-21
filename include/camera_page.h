#pragma once

// Served by the HTTPS server (secure_server.cpp) at /camera. It reads the phone
// camera, scales it down to 16x16 grayscale and streams the frames to the lamp
// over a secure websocket. Kept apart from the main web app, which runs over
// plain HTTP where phones refuse camera access.
static const char CAMERA_PAGE[] = R"html(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#0b0b0d">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<title>Lamp camera</title>
<style>
  :root { color-scheme: dark; --bg: #0b0b0d; --surface: #16161a; --line: #2a2a31; --fg: #f3f3f5; --muted: #8e8e98; --accent: #f5b547; }
  * { box-sizing: border-box; }
  html { touch-action: manipulation; -webkit-text-size-adjust: 100%; }
  body { margin: 0; min-height: 100dvh; background: var(--bg); color: var(--fg);
    font: 16px/1.4 system-ui, -apple-system, sans-serif; overscroll-behavior: none;
    padding: max(16px, env(safe-area-inset-top)) 16px max(16px, env(safe-area-inset-bottom)); }
  main { max-width: 420px; margin: 0 auto; display: flex; flex-direction: column; gap: 16px; }
  header { display: flex; justify-content: space-between; align-items: center; }
  h1 { font-size: 17px; margin: 0; }
  a { color: var(--muted); font-size: 14px; }
  #status { font-size: 13px; color: var(--muted); display: flex; align-items: center; gap: 6px; }
  #status::before { content: ""; width: 8px; height: 8px; border-radius: 50%; background: var(--muted); }
  #status.live::before { background: #34d399; }
  #status.error::before { background: #f87171; }
  canvas#preview { width: 100%; aspect-ratio: 1; border-radius: 24px; background: #050506;
    image-rendering: pixelated; border: 1px solid var(--line); }
  video, canvas#sample { display: none; }
  .card { background: var(--surface); border: 1px solid var(--line); border-radius: 20px; padding: 16px;
    display: flex; flex-direction: column; gap: 14px; }
  label { display: flex; align-items: center; gap: 12px; font-size: 14px; color: var(--muted); }
  label span { width: 84px; flex-shrink: 0; }
  input[type=range] { flex: 1; accent-color: var(--accent); }
  .row { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
  button { font: inherit; font-size: 15px; font-weight: 600; border-radius: 14px; padding: 13px;
    border: 1px solid var(--line); background: #222228; color: var(--fg); cursor: pointer; }
  button.primary { background: var(--accent); color: #1a1205; border-color: transparent; }
  button[aria-pressed=true] { border-color: var(--accent); color: var(--accent); }
  #message { font-size: 14px; color: var(--muted); margin: 0; }
</style>
</head>
<body>
<main>
  <header>
    <h1>Lamp camera</h1>
    <div id="status">Not streaming</div>
  </header>
  <canvas id="preview" width="16" height="16"></canvas>
  <p id="message">Tap Start to show your camera on the lamp.</p>
  <button id="toggle" class="primary">Start camera</button>
  <div class="card">
    <label><span>Brightness</span><input id="brightness" type="range" min="-100" max="100" value="0"></label>
    <label><span>Contrast</span><input id="contrast" type="range" min="50" max="300" value="130"></label>
    <div class="row">
      <button id="flip">Front camera</button>
      <button id="invert" aria-pressed="false">Invert</button>
    </div>
  </div>
  <a href="#" id="back">Back to the lamp app</a>
</main>
<video id="video" playsinline muted autoplay></video>
<canvas id="sample" width="64" height="64"></canvas>
<script>
const SIZE = 16, SAMPLE = 64, BLOCK = SAMPLE / SIZE, FPS = 15;
const $ = (id) => document.getElementById(id);
const video = $("video"), sample = $("sample").getContext("2d", { willReadFrequently: true });
const preview = $("preview").getContext("2d");
const frame = new Uint8Array(SIZE * SIZE);
let stream = null, socket = null, facing = "environment", inverted = false, timer = null, wakeLock = null;

$("back").href = "http://" + location.hostname + "/";

function setStatus(text, state) {
  $("status").textContent = text;
  $("status").className = state || "";
}

function connect() {
  socket = new WebSocket("wss://" + location.host + "/ws");
  socket.binaryType = "arraybuffer";
  socket.onopen = () => { socket.send("start"); setStatus("Live on the lamp", "live"); };
  socket.onclose = () => {
    if (stream) { setStatus("Reconnecting…", "error"); setTimeout(connect, 1000); }
  };
}

async function start() {
  if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {
    $("message").textContent = "This browser gives no camera access here. Open this page over https://.";
    return;
  }
  try {
    stream = await navigator.mediaDevices.getUserMedia({
      video: { facingMode: facing, width: { ideal: 640 }, height: { ideal: 640 } }, audio: false });
  } catch (error) {
    $("message").textContent = "No camera access: " + error.message;
    setStatus("Camera blocked", "error");
    return;
  }
  video.srcObject = stream;
  await video.play();
  if (!socket || socket.readyState > 1) connect();
  timer = setInterval(sendFrame, 1000 / FPS);
  $("toggle").textContent = "Stop camera";
  $("toggle").classList.remove("primary");
  $("message").textContent = "Point your camera at something. The lamp shows it live.";
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
  $("toggle").textContent = "Start camera";
  $("toggle").classList.add("primary");
  setStatus("Not streaming");
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
  const image = preview.createImageData(SIZE, SIZE);
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
      const level = value < 8 ? 0 : Math.round(value);
      frame[y * SIZE + x] = level;
      const p = (y * SIZE + x) * 4;
      image.data[p] = 255; image.data[p + 1] = 246; image.data[p + 2] = 230;
      image.data[p + 3] = level === 0 ? 18 : 40 + level * 215 / 255;
    }
  }
  preview.putImageData(image, 0, 0);

  if (socket && socket.readyState === 1 && socket.bufferedAmount < 1024) socket.send(frame);
}

$("toggle").onclick = () => (stream ? stop() : start());
$("flip").onclick = async () => {
  facing = facing === "environment" ? "user" : "environment";
  $("flip").textContent = facing === "user" ? "Back camera" : "Front camera";
  if (stream) {
    clearInterval(timer);
    stream.getTracks().forEach((track) => track.stop());
    stream = null;
    await start();
  }
};
$("invert").onclick = () => {
  inverted = !inverted;
  $("invert").setAttribute("aria-pressed", inverted);
};
document.addEventListener("visibilitychange", () => { if (document.hidden && stream) stop(); });
</script>
</body>
</html>
)html";

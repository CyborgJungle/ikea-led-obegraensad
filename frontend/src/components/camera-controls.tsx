import { type Component, createSignal, For, Show } from "solid-js";

// the camera page runs on the lamp's HTTPS server, phones only allow the camera there
const host = window.location.hostname;
const CAMERA_URL = `https://${host}/camera`;
const CERTIFICATE_URL = `http://${host}/lamp-ca.cer`;

const SETUP_STEPS = [
  "Tap “Install certificate”, then Allow and Close.",
  "Settings › General › VPN & Device Management: open the lamp profile and tap Install.",
  "Settings › General › About › Certificate Trust Settings: switch the lamp authority on.",
  "Tap “Open camera”.",
];

export const CameraControls: Component = () => {
  const [showSetup, setShowSetup] = createSignal(false);

  return (
    <div class="space-y-3">
      <a href={CAMERA_URL} class="btn btn-accent w-full py-3">
        <i class="fa-solid fa-camera" />
        Open camera
      </a>
      <button
        type="button"
        class="btn btn-ghost w-full"
        aria-expanded={showSetup()}
        onClick={() => setShowSetup(!showSetup())}
      >
        <i class={`fa-solid ${showSetup() ? "fa-chevron-up" : "fa-shield-halved"}`} />
        First time on this phone?
      </button>
      <Show when={showSetup()}>
        <div class="space-y-3 rounded-xl border border-line bg-canvas/40 p-3 text-sm">
          <p class="text-muted">
            Phones only allow the camera on secure pages. Trust the lamp once (iPhone):
          </p>
          <ol class="list-decimal space-y-1.5 pl-5">
            <For each={SETUP_STEPS}>{(step) => <li>{step}</li>}</For>
          </ol>
          <a href={CERTIFICATE_URL} class="btn w-full">
            <i class="fa-solid fa-certificate" />
            Install certificate
          </a>
        </div>
      </Show>
    </div>
  );
};

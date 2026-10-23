import { type Component, createSignal, For, onMount, Show } from "solid-js";

import { Icon } from "../components/icons";
import { Section, Slider } from "../components/ui";
import { useStore } from "../contexts/store";
import { useToast } from "../contexts/toast";
import { cameraUrl, certificateUrl, getConfig, getInfo, saveConfig } from "../lib/api";
import type { PanelProps } from "./shared";

const CAMERA_STEPS = [
  "Tap “Install certificate”, then Allow and Close.",
  "Settings › General › VPN & Device Management: open the lamp profile, tap Install.",
  "Settings › General › About › Certificate Trust Settings: switch the lamp authority on.",
  "Come back and tap “Open camera”.",
];

// Camera: the camera page runs on the lamp's HTTPS server, phones only allow the camera there
export const CameraPanel: Component<PanelProps> = (props) => {
  const [setup, setSetup] = createSignal(false);
  return (
    <>
      {/* replaces this page, like every move in the app: no swipe back to it */}
      <a
        href={cameraUrl(props.plugin.id)}
        class="btn btn-signal h-16 text-lg no-underline"
        onClick={(event) => {
          event.preventDefault();
          window.location.replace(event.currentTarget.href);
        }}
      >
        <Icon name="camera" size={22} />
        Open camera
      </a>
      <button type="button" class="btn btn-line" onClick={() => setSetup(!setup())}>
        First time on this phone?
        <Icon name={setup() ? "chevronDown" : "chevronRight"} size={16} />
      </button>
      <Show when={setup()}>
        <section class="card flex flex-col gap-3 text-[15px]">
          <p class="m-0 text-muted">
            Phones only allow the camera on secure pages. Trust the lamp once:
          </p>
          <ol class="m-0 flex flex-col gap-2 pl-5">
            <For each={CAMERA_STEPS}>{(step) => <li>{step}</li>}</For>
          </ol>
          <a href={certificateUrl()} class="btn btn-ink no-underline">
            Install certificate
          </a>
        </section>
      </Show>
    </>
  );
};

// Game of Life: time between generations
export const LifePanel: Component<PanelProps> = () => {
  const [store, actions] = useStore();
  return (
    <Section
      title="Generation time"
      aside={<span class="text-[13px] font-semibold tabular-nums">{store.GOLDelay} ms</span>}
    >
      <Slider
        label="Time between generations"
        min={1}
        max={4000}
        value={store.GOLDelay}
        onInput={(value) => actions.setGOLDelay(value)}
        onCommit={(value) => actions.send(JSON.stringify({ event: "goldelay", delay: value }))}
      />
    </Section>
  );
};

const useLampIp = () => {
  const [ip, setIp] = createSignal(window.location.hostname);
  onMount(async () => {
    try {
      setIp((await getInfo()).ipAddress);
    } catch {
      // the hostname will do
    }
  });
  return ip;
};

// Art-Net: listens to one DMX universe, one channel per LED
export const ArtNetPanel: Component<PanelProps> = () => {
  const [store, actions] = useStore();
  const ip = useLampIp();
  return (
    <>
      <Section
        title="Universe"
        aside={<span class="text-[13px] font-semibold tabular-nums">{store.artnetUniverse}</span>}
      >
        <Slider
          label="Art-Net universe"
          min={0}
          max={255}
          value={store.artnetUniverse}
          onInput={(value) => actions.setArtnetUniverse(value)}
          onCommit={(value) => actions.send(JSON.stringify({ event: "artnet", universe: value }))}
        />
      </Section>
      <p class="m-0 text-[15px] text-muted">
        Send Art-Net DMX from a lighting desk or app (QLC+, Resolume) to{" "}
        <span class="font-mono text-ink">{ip()}</span>, port 6454. Channel 1–256 are the LEDs, row
        by row.
      </p>
    </>
  );
};

// Clouds: where its data comes from, with the credit EUMETSAT asks for
export const CloudsPanel: Component<PanelProps> = () => (
  <p class="m-0 text-[13px] text-muted">
    Every 10 minutes the lamp fetches the newest infrared satellite image of the sky around
    Amsterdam: where it is cloudy the texture thickens, and it drifts with the real wind. Meteosat
    data © EUMETSAT.
  </p>
);

// DDP: streams frames from LED software like xLights or LedFx
export const DdpPanel: Component<PanelProps> = () => {
  const ip = useLampIp();
  return (
    <p class="m-0 text-[15px] text-muted">
      Stream pixels with DDP from LED software like xLights or LedFx to{" "}
      <span class="font-mono text-ink">{ip()}</span>, port 4048. The lamp shows each pixel's
      brightness.
    </p>
  );
};

// Weather: the city it shows
export const WeatherPanel: Component<PanelProps> = () => {
  const { toast } = useToast();
  const [location, setLocation] = createSignal("");
  onMount(async () => {
    try {
      setLocation((await getConfig()).weatherLocation ?? "");
    } catch {
      // left empty
    }
  });
  const save = async (event: SubmitEvent) => {
    event.preventDefault();
    try {
      await saveConfig({ weatherLocation: location().trim() });
      toast("Location saved", 1500);
    } catch {
      toast("Could not reach the lamp", 2000);
    }
  };
  return (
    <form class="flex flex-col gap-3" onSubmit={save}>
      <Section title="Location">
        <input
          class="input"
          placeholder="City, e.g. Rotterdam"
          value={location()}
          onInput={(e) => setLocation(e.currentTarget.value)}
        />
      </Section>
      <button type="submit" class="btn btn-ink" disabled={!location().trim()}>
        Save location
      </button>
    </form>
  );
};

// the clock modes: their time comes from the timezone in Settings
export const ClockPanel: Component<PanelProps> = () => (
  <a href="#/settings" class="card flex items-center justify-between text-ink no-underline">
    <span class="flex flex-col">
      <span class="text-[15px] font-semibold">Time zone & time server</span>
      <span class="text-[13px] text-muted">In Settings</span>
    </span>
    <Icon name="chevronRight" />
  </a>
);

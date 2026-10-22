import { type Component, createSignal, For, onCleanup, onMount, Show } from "solid-js";

import { Icon, IconButton } from "../components/icons";
import { formatUptime, PageTitle, Section, Segmented, Switch } from "../components/ui";
import { useStore } from "../contexts/store";
import { useToast } from "../contexts/toast";
import {
  certificateUrl,
  type DeviceInfo,
  getConfig,
  getInfo,
  resetConfig,
  saveConfig,
} from "../lib/api";
import { type Appearance, appearance, setAppearance } from "../lib/theme";
import { viewportInfo } from "../lib/viewport";

// POSIX TZ strings, the device cannot resolve IANA names like "Europe/Berlin"
const TIMEZONES = [
  { label: "UTC", value: "UTC0" },
  { label: "United Kingdom / Ireland", value: "GMT0BST,M3.5.0/1,M10.5.0" },
  { label: "Central Europe (Amsterdam, Berlin)", value: "CET-1CEST,M3.5.0,M10.5.0/3" },
  { label: "Eastern Europe (Athens, Helsinki)", value: "EET-2EEST,M3.5.0/3,M10.5.0/4" },
  { label: "India", value: "IST-5:30" },
  { label: "Japan", value: "JST-9" },
  { label: "Australia East (Sydney)", value: "AEST-10AEDT,M10.1.0,M4.1.0/3" },
  { label: "Brazil (São Paulo)", value: "<-03>3" },
  { label: "US Eastern", value: "EST5EDT,M3.2.0,M11.1.0" },
  { label: "US Central", value: "CST6CDT,M3.2.0,M11.1.0" },
  { label: "US Mountain", value: "MST7MDT,M3.2.0,M11.1.0" },
  { label: "US Pacific", value: "PST8PDT,M3.2.0,M11.1.0" },
];
const CUSTOM = "custom";

const signalLabel = (rssi: number) =>
  rssi >= -55 ? "Excellent" : rssi >= -67 ? "Good" : rssi >= -75 ? "Fair" : "Weak";

const kb = (bytes?: number) => (bytes === undefined ? "–" : `${Math.round(bytes / 1024)} KB`);

export const Settings: Component = () => {
  const [store, actions] = useStore();
  const { toast } = useToast();

  const [info, setInfo] = createSignal<DeviceInfo | null>(null);
  const [location, setLocation] = createSignal("");
  const [ntp, setNtp] = createSignal("");
  const [tz, setTz] = createSignal("");
  const [customTz, setCustomTz] = createSignal(false);
  const tzChoice = () =>
    customTz() || !TIMEZONES.some((zone) => zone.value === tz()) ? CUSTOM : tz();

  const loadInfo = async () => {
    try {
      setInfo(await getInfo());
    } catch {
      // keeps the last values
    }
  };

  const loadConfig = async () => {
    try {
      const config = await getConfig();
      setLocation(config.weatherLocation ?? "");
      setNtp(config.ntpServer ?? "");
      setTz(config.tzInfo ?? "");
      setCustomTz(false);
    } catch {
      toast("Could not load the settings", 2000);
    }
  };

  onMount(async () => {
    // one request at a time, the lamp answers parallel requests slowly
    await loadConfig();
    await loadInfo();
    const timer = setInterval(loadInfo, 10000);
    onCleanup(() => clearInterval(timer));
  });

  const saveTime = async (event: SubmitEvent) => {
    event.preventDefault();
    if (!ntp().trim() || !tz().trim()) {
      toast("Time zone and time server are needed", 2000);
      return;
    }
    try {
      await saveConfig({
        weatherLocation: location().trim(),
        ntpServer: ntp().trim(),
        tzInfo: tz().trim(),
      });
      toast("Saved, the clock re-syncs", 1500);
    } catch {
      toast("Could not reach the lamp", 2000);
    }
  };

  const rotate = (right: boolean) => {
    actions.setRotation(((store.rotation || 0) + (right ? 1 : -1) + 4) % 4);
    actions.send(JSON.stringify({ event: "rotate", direction: right ? "right" : "left" }));
  };

  const reset = async () => {
    if (!window.confirm("Reset all settings on the lamp to their defaults?")) return;
    try {
      await resetConfig();
      await loadConfig();
      toast("Settings reset", 1500);
    } catch {
      toast("Could not reach the lamp", 2000);
    }
  };

  // "785/844 · gap 59 · 47/34": viewport/screen height, the gap made up for,
  // safe area top/bottom as iOS reports them; "web" outside a home screen app
  const screenLine = () => {
    const v = viewportInfo();
    return `${v.viewport}/${v.screen} · gap ${v.gap} · ${v.safeTop}/${v.safeBottom}${v.standalone ? "" : " · web"}`;
  };

  const stats = () => {
    const device = info();
    if (!device) return [];
    return [
      { label: "Uptime", value: formatUptime(device.uptime) },
      { label: "Restart", value: device.resetReason },
      { label: "Memory", value: kb(device.freeHeap) },
      { label: "Lowest", value: kb(device.minFreeHeap) },
      { label: "IP address", value: device.ipAddress },
      { label: "MAC", value: device.macAddress },
      // temporary: what iOS gives this app, while the short viewport is sorted out
      { label: "Screen", value: screenLine() },
    ];
  };

  return (
    <main class="page animate-fade-in">
      <PageTitle title="Settings" sub="Lamp & app" />

      <Section title="Appearance">
        <Segmented<Appearance>
          label="Appearance"
          value={appearance()}
          onChange={setAppearance}
          options={[
            { value: "auto", label: "Auto" },
            { value: "light", label: "Light" },
            { value: "dark", label: "Dark" },
          ]}
        />
        <p class="m-0 text-[13px] text-muted">
          Auto follows the light or dark setting of this phone.
        </p>
      </Section>

      <Section title="Lamp">
        <div class="card flex flex-col gap-2">
          <div class="row">
            <span class="flex flex-col">
              <span class="text-[15px] font-semibold">Rotation</span>
              <span class="text-[13px] text-muted">Turn the picture to how the lamp hangs</span>
            </span>
            <div class="flex items-center gap-2">
              <IconButton name="rotateLeft" label="Rotate left" onClick={() => rotate(false)} />
              <span class="w-11 text-center text-[17px] font-semibold tabular-nums">
                {[0, 90, 180, 270][store.rotation || 0]}°
              </span>
              <IconButton name="rotateRight" label="Rotate right" onClick={() => rotate(true)} />
            </div>
          </div>
          <div class="border-t border-line" />
          <Switch
            label="Live preview"
            hint="Mirror the lamp in this app"
            checked={store.livePreview}
            onChange={actions.setLivePreview}
          />
        </div>
      </Section>

      <Show when={info()}>
        {(device) => (
          <Section title="Device">
            <div class="grid grid-cols-[auto_1fr] items-center gap-3">
              <div class="flex size-[132px] flex-col items-center justify-center rounded-full bg-inverse text-on-inverse">
                <Icon name="wifi" size={18} />
                <span class="numeral text-[40px]">{device().rssi}</span>
                <span class="text-[12px] opacity-75">dBm · {signalLabel(device().rssi)}</span>
              </div>
              <dl class="m-0 grid grid-cols-2 gap-2">
                <For each={stats().slice(0, 4)}>
                  {(stat) => (
                    <div class="rounded-[18px] bg-card px-3 py-2">
                      <dt class="label text-[10px]">{stat.label}</dt>
                      <dd class="m-0 truncate text-[15px] font-semibold">{stat.value}</dd>
                    </div>
                  )}
                </For>
              </dl>
            </div>
            <dl class="card m-0 flex flex-col gap-1 py-3">
              <For each={stats().slice(4)}>
                {(stat) => (
                  <div class="flex justify-between gap-3 text-[14px]">
                    <dt class="text-muted">{stat.label}</dt>
                    <dd class="m-0 font-mono text-[13px]">{stat.value}</dd>
                  </div>
                )}
              </For>
            </dl>
          </Section>
        )}
      </Show>

      <form class="flex flex-col gap-5" onSubmit={saveTime}>
        <Section title="Time & weather">
          <div class="card flex flex-col gap-3">
            <label class="flex flex-col gap-1.5">
              <span class="text-[15px] font-semibold">Time zone</span>
              <select
                class="input"
                value={tzChoice()}
                onChange={(e) => {
                  if (e.currentTarget.value === CUSTOM) {
                    setCustomTz(true);
                  } else {
                    setCustomTz(false);
                    setTz(e.currentTarget.value);
                  }
                }}
              >
                <For each={TIMEZONES}>
                  {(zone) => <option value={zone.value}>{zone.label}</option>}
                </For>
                <option value={CUSTOM}>Custom POSIX string…</option>
              </select>
            </label>
            <Show when={tzChoice() === CUSTOM}>
              <input
                class="input font-mono"
                aria-label="POSIX time zone"
                placeholder="CET-1CEST,M3.5.0,M10.5.0/3"
                value={tz()}
                onInput={(e) => setTz(e.currentTarget.value)}
              />
            </Show>
            <label class="flex flex-col gap-1.5">
              <span class="text-[15px] font-semibold">Time server</span>
              <input
                class="input"
                placeholder="pool.ntp.org"
                value={ntp()}
                onInput={(e) => setNtp(e.currentTarget.value)}
              />
            </label>
            <label class="flex flex-col gap-1.5">
              <span class="text-[15px] font-semibold">Weather location</span>
              <input
                class="input"
                placeholder="Rotterdam"
                value={location()}
                onInput={(e) => setLocation(e.currentTarget.value)}
              />
            </label>
            <button type="submit" class="btn btn-ink">
              Save
            </button>
          </div>
        </Section>
      </form>

      <Section title="Camera">
        <a
          href={certificateUrl()}
          class="card flex items-center justify-between text-ink no-underline"
        >
          <span class="flex flex-col">
            <span class="text-[15px] font-semibold">Install the lamp certificate</span>
            <span class="text-[13px] text-muted">Once per phone, for the live camera</span>
          </span>
          <Icon name="chevronRight" />
        </a>
      </Section>

      <button type="button" class="btn btn-line text-danger" onClick={reset}>
        Reset lamp settings
      </button>
    </main>
  );
};

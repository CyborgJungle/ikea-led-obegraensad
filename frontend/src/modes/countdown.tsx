import { type Component, createSignal, For, onMount } from "solid-js";

import { Icon } from "../components/icons";
import { Section } from "../components/ui";
import { useToast } from "../contexts/toast";
import { getConfig, saveConfig } from "../lib/api";
import type { PanelProps } from "./shared";

// must match Config::COUNTDOWN_MAX_SECONDS on the device
const MAX_SECONDS = 5999;
const PRESETS = [10, 30, 60, 300];

const formatClock = (seconds: number) =>
  `${Math.floor(seconds / 60)}:${String(seconds % 60).padStart(2, "0")}`;

const formatPreset = (seconds: number) =>
  seconds < 60 ? `${seconds} s` : `${Math.floor(seconds / 60)} min`;

// The lamp shows the duration and waits; Start counts down, at zero rings ripple,
// then it shows the duration again for the next start.
export const CountdownPanel: Component<PanelProps> = (props) => {
  const { toast } = useToast();
  const [seconds, setSeconds] = createSignal(60);
  const [sending, setSending] = createSignal(false);
  const valid = () => Number.isInteger(seconds()) && seconds() >= 1 && seconds() <= MAX_SECONDS;

  onMount(async () => {
    try {
      setSeconds((await getConfig()).countdownSeconds ?? 60);
    } catch {
      // keeps the default
    }
  });

  const send = async (body: Record<string, number | boolean>, done: string) => {
    setSending(true);
    try {
      await saveConfig(body);
      if (!props.active) props.activate();
      toast(done, 1500);
    } catch {
      toast("Could not reach the lamp", 2000);
    } finally {
      setSending(false);
    }
  };

  const setDuration = (value: number) => {
    setSeconds(value);
    if (valid()) send({ countdownSeconds: value }, `Set to ${formatClock(value)}`);
  };

  return (
    <>
      <section class="card flex flex-col items-center gap-3 py-6">
        <span class="label">Counts down from</span>
        <span class="numeral text-[88px]">{valid() ? formatClock(seconds()) : "–:––"}</span>
        <label class="flex items-center gap-2">
          <input
            type="number"
            inputmode="numeric"
            min="1"
            max={MAX_SECONDS}
            aria-label="Seconds"
            value={seconds()}
            onInput={(e) => setSeconds(e.currentTarget.valueAsNumber)}
            onChange={(e) => setDuration(e.currentTarget.valueAsNumber)}
            class="input w-28 text-center tabular-nums"
          />
          <span class="text-[15px] text-muted">seconds</span>
        </label>
      </section>

      <Section title="Quick">
        <div class="grid grid-cols-4 gap-2">
          <For each={PRESETS}>
            {(preset) => (
              <button
                type="button"
                class={`btn px-0 ${seconds() === preset ? "btn-ink" : "btn-line"}`}
                disabled={sending()}
                onClick={() => setDuration(preset)}
              >
                {formatPreset(preset)}
              </button>
            )}
          </For>
        </div>
      </Section>

      <div class="grid grid-cols-[1fr_auto] items-center gap-4 border-t border-line pt-5">
        <button
          type="button"
          class="btn btn-signal h-16 text-lg"
          disabled={!valid() || sending()}
          onClick={() =>
            send({ countdownSeconds: seconds(), countdownStart: true }, "Countdown started")
          }
        >
          <Icon name="play" size={20} />
          Start
        </button>
        <button
          type="button"
          class="disc size-16"
          aria-label="Reset"
          disabled={sending()}
          onClick={() => send({ countdownReset: true }, "Countdown reset")}
        >
          <Icon name="rotateLeft" size={24} />
        </button>
      </div>
    </>
  );
};

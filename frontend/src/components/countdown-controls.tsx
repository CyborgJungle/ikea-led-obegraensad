import { type Component, createSignal, For, onMount } from "solid-js";

import { useToast } from "../contexts/toast";

const API_URL = import.meta.env.PROD
  ? `http://${window.location.host}/`
  : import.meta.env.VITE_BASE_URL;

// must match Config::COUNTDOWN_MAX_SECONDS on the device
const MAX_SECONDS = 5999;

const PRESETS = [10, 30, 60, 300];

const formatDuration = (seconds: number) => {
  if (seconds < 60) return `${seconds} s`;
  const minutes = Math.floor(seconds / 60);
  return seconds % 60 === 0
    ? `${minutes} min`
    : `${minutes}:${String(seconds % 60).padStart(2, "0")} min`;
};

export const CountdownControls: Component = () => {
  const { toast } = useToast();
  const [seconds, setSeconds] = createSignal(60);
  const [isSending, setIsSending] = createSignal(false);

  const isValid = () => Number.isInteger(seconds()) && seconds() >= 1 && seconds() <= MAX_SECONDS;

  onMount(async () => {
    try {
      const response = await fetch(`${API_URL}api/config`);
      if (response.ok) {
        setSeconds((await response.json()).countdownSeconds ?? 60);
      }
    } catch (error) {
      console.error("Failed to load countdown:", error);
    }
  });

  // the lamp shows a new duration and waits; it only counts down after a start
  const send = async (body: Record<string, number | boolean>, done: string) => {
    setIsSending(true);
    try {
      const response = await fetch(`${API_URL}api/config`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(body),
      });
      if (!response.ok) {
        throw new Error(`HTTP ${response.status}`);
      }
      toast(done, 1500);
    } catch (error) {
      console.error("Failed to update countdown:", error);
      toast("Failed to reach the lamp", 2000);
    } finally {
      setIsSending(false);
    }
  };

  const setDuration = (value: number) => {
    setSeconds(value);
    if (isValid()) {
      send({ countdownSeconds: value }, `Set to ${formatDuration(value)}`);
    }
  };

  const start = () => {
    if (isValid()) {
      send(
        { countdownSeconds: seconds(), countdownStart: true },
        `Countdown started: ${formatDuration(seconds())}`,
      );
    }
  };

  return (
    <form
      class="space-y-3"
      onSubmit={(event) => {
        event.preventDefault();
        start();
      }}
    >
      <div class="flex items-center gap-2">
        <input
          type="number"
          inputmode="numeric"
          min="1"
          max={MAX_SECONDS}
          aria-label="Countdown in seconds"
          value={seconds()}
          onInput={(e) => setSeconds(e.currentTarget.valueAsNumber)}
          onChange={(e) => setDuration(e.currentTarget.valueAsNumber)}
          class="input"
        />
        <span class="shrink-0 text-sm text-muted">seconds</span>
      </div>
      <div class="grid grid-cols-4 gap-2">
        <For each={PRESETS}>
          {(preset) => (
            <button
              type="button"
              class="btn px-0 py-2 text-xs"
              disabled={isSending()}
              onClick={() => setDuration(preset)}
            >
              {formatDuration(preset)}
            </button>
          )}
        </For>
      </div>
      <div class="grid grid-cols-[1fr_auto] gap-2">
        <button type="submit" class="btn btn-accent" disabled={!isValid() || isSending()}>
          <i class="fa-solid fa-play" />
          {isValid() ? `Start ${formatDuration(seconds())}` : `Enter 1 to ${MAX_SECONDS} s`}
        </button>
        <button
          type="button"
          class="btn"
          disabled={isSending()}
          onClick={() => send({ countdownReset: true }, "Countdown reset")}
        >
          <i class="fa-solid fa-rotate-left" />
          Reset
        </button>
      </div>
    </form>
  );
};

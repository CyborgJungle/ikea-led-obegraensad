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

  // the device restarts the countdown on every request, also with the same duration
  const start = async (value: number) => {
    setSeconds(value);
    if (!isValid()) {
      return;
    }
    setIsSending(true);
    try {
      const response = await fetch(`${API_URL}api/config`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ countdownSeconds: value }),
      });
      if (!response.ok) {
        throw new Error(`HTTP ${response.status}`);
      }
      toast(`Countdown started: ${formatDuration(value)}`, 1500);
    } catch (error) {
      console.error("Failed to start countdown:", error);
      toast("Failed to start countdown", 2000);
    } finally {
      setIsSending(false);
    }
  };

  return (
    <form
      class="space-y-3"
      onSubmit={(event) => {
        event.preventDefault();
        start(seconds());
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
              onClick={() => start(preset)}
            >
              {formatDuration(preset)}
            </button>
          )}
        </For>
      </div>
      <button type="submit" class="btn btn-accent w-full" disabled={!isValid() || isSending()}>
        <i class="fa-solid fa-play" />
        {isValid() ? `Start ${formatDuration(seconds())}` : `Enter 1 to ${MAX_SECONDS} seconds`}
      </button>
    </form>
  );
};

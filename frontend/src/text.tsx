import { type Component, createMemo, createSignal, onMount, Show } from "solid-js";

import { Layout } from "./components/layout/layout";
import { LedMatrix } from "./components/led-matrix";
import { useStore } from "./contexts/store";
import { useToast } from "./contexts/toast";
import { rotateArray } from "./helpers";

const API_URL = import.meta.env.PROD
  ? `http://${window.location.host}/`
  : import.meta.env.VITE_BASE_URL;

// must match Config::SCROLL_TEXT_MAX_LENGTH and SCROLL_SPEED_MIN/MAX on the device
const MAX_LENGTH = 200;
const SPEED_MIN = 20;
const SPEED_MAX = 250;

// the device stores milliseconds per pixel, the slider shows "faster" to the right
const sliderToDelay = (value: number) => SPEED_MIN + SPEED_MAX - value;

const TextPage: Component = () => {
  const [store, actions] = useStore();
  const { toast } = useToast();

  const [text, setText] = createSignal("");
  const [delay, setDelay] = createSignal(60);
  const [isLoading, setIsLoading] = createSignal(true);
  const [isSending, setIsSending] = createSignal(false);

  const textPlugin = createMemo(() => store.plugins.find((plugin) => plugin.name === "Text"));
  const rotatedMatrix = createMemo(() => rotateArray(store.indexMatrix, store.rotation));

  onMount(async () => {
    try {
      const response = await fetch(`${API_URL}api/config`);
      if (response.ok) {
        const data = await response.json();
        setText(data.scrollText ?? "");
        setDelay(data.scrollSpeed ?? 60);
      }
    } catch (error) {
      console.error("Failed to load text:", error);
      toast("Failed to load current text", 2000);
    } finally {
      setIsLoading(false);
    }
  });

  const handleSubmit = async (event: SubmitEvent) => {
    event.preventDefault();
    const plugin = textPlugin();
    if (!plugin) {
      toast("The lamp firmware has no Text mode yet, update it first", 3000);
      return;
    }

    setIsSending(true);
    try {
      const response = await fetch(`${API_URL}api/config`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ scrollText: text(), scrollSpeed: delay() }),
      });
      if (!response.ok) {
        throw new Error(`HTTP ${response.status}`);
      }

      if (store.plugin !== plugin.id || store.isActiveScheduler) {
        actions.send(JSON.stringify({ event: "plugin", plugin: plugin.id }));
      }
      toast("Text sent to the lamp", 1500);
    } catch (error) {
      console.error("Failed to send text:", error);
      toast("Failed to send text", 2000);
    } finally {
      setIsSending(false);
    }
  };

  return (
    <Layout
      content={
        <div class="flex min-h-full items-center justify-center p-4 lg:p-8">
          <div class="w-[min(100%,700px,(100dvh-16rem)*9/13)] max-lg:w-[min(100%,55dvh*9/13)]">
            <LedMatrix
              disabled
              data={store.leds || []}
              indexData={rotatedMatrix()}
              brightness={store.brightness ?? store.baseBrightness ?? 255}
            />
          </div>
        </div>
      }
      sidebar={
        <form class="space-y-5" onSubmit={handleSubmit}>
          <div class="space-y-3">
            <h3 class="section-title">Scrolling text</h3>
            <textarea
              id="scroll-text"
              rows={3}
              maxLength={MAX_LENGTH}
              placeholder="Type something to scroll…"
              value={text()}
              disabled={isLoading()}
              onInput={(e) => setText(e.currentTarget.value)}
              class="input resize-none"
            />
            <p class="flex justify-between text-xs text-muted">
              <span>Letters, numbers, symbols and umlauts</span>
              <span>
                {text().length}/{MAX_LENGTH}
              </span>
            </p>
          </div>

          <div class="space-y-3">
            <label class="section-title block" for="scroll-speed">
              Speed
            </label>
            <div class="flex items-center gap-3 text-xs text-muted">
              <i class="fa-solid fa-person-walking" />
              <input
                id="scroll-speed"
                type="range"
                min={SPEED_MIN}
                max={SPEED_MAX}
                value={sliderToDelay(delay())}
                disabled={isLoading()}
                onInput={(e) => setDelay(sliderToDelay(Number(e.currentTarget.value)))}
                class="w-full"
              />
              <i class="fa-solid fa-person-running" />
            </div>
          </div>

          <button
            type="submit"
            disabled={isLoading() || isSending()}
            class="btn btn-accent w-full py-3"
          >
            <i class="fa-solid fa-paper-plane" />
            {isSending() ? "Sending…" : "Show on lamp"}
          </button>

          <Show when={!isLoading() && !textPlugin()}>
            <p class="text-xs text-muted">
              Text mode is missing on the lamp. Install the latest firmware via Firmware Update.
            </p>
          </Show>
        </form>
      }
    />
  );
};

export default TextPage;

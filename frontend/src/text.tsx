import { type Component, createMemo, createSignal, For, onMount, Show } from "solid-js";

import { Layout } from "./components/layout/layout";
import { LedMatrix } from "./components/led-matrix";
import { useStore } from "./contexts/store";
import { useToast } from "./contexts/toast";
import { rotateArray } from "./helpers";

const API_URL = import.meta.env.PROD
  ? `http://${window.location.host}/`
  : import.meta.env.VITE_BASE_URL;

// must match the limits in config.h on the device
const SCROLL_MAX_LENGTH = 200;
const BOARD_MAX_LENGTH = 500;
const SPEED_MIN = 20;
const SPEED_MAX = 250;

type Mode = "scroll" | "board";

const MODES: { value: Mode; label: string; icon: string; plugin: string }[] = [
  { value: "scroll", label: "Scroll", icon: "fa-arrow-left", plugin: "Text" },
  { value: "board", label: "Board", icon: "fa-list", plugin: "Board" },
];

// values of Config::ScrollSize
const SIZES = [
  { value: 0, label: "Small" },
  { value: 1, label: "Medium" },
  { value: 2, label: "Large" },
];

// the device stores milliseconds per pixel, the slider shows "faster" to the right
const sliderToDelay = (value: number) => SPEED_MIN + SPEED_MAX - value;

// the lamp fonts have no ellipsis glyph; quotes and dashes it maps itself
const normalize = (text: string) => text.replace(/…/g, "...");

const SpeedSlider: Component<{
  id: string;
  label: string;
  delay: number;
  disabled: boolean;
  onChange: (delay: number) => void;
}> = (props) => (
  <div class="space-y-3">
    <label class="section-title block" for={props.id}>
      {props.label}
    </label>
    <div class="flex items-center gap-3 text-xs text-muted">
      <i class="fa-solid fa-person-walking" />
      <input
        id={props.id}
        type="range"
        min={SPEED_MIN}
        max={SPEED_MAX}
        value={sliderToDelay(props.delay)}
        disabled={props.disabled}
        onInput={(e) => props.onChange(sliderToDelay(Number(e.currentTarget.value)))}
        class="w-full"
      />
      <i class="fa-solid fa-person-running" />
    </div>
  </div>
);

const TextPage: Component = () => {
  const [store, actions] = useStore();
  const { toast } = useToast();

  const [mode, setMode] = createSignal<Mode>("scroll");
  const [scrollText, setScrollText] = createSignal("");
  const [scrollSpeed, setScrollSpeed] = createSignal(60);
  const [scrollSize, setScrollSize] = createSignal(2);
  const [boardText, setBoardText] = createSignal("");
  const [boardSpeed, setBoardSpeed] = createSignal(120);
  const [isLoading, setIsLoading] = createSignal(true);
  const [isSending, setIsSending] = createSignal(false);

  const pluginFor = (value: Mode) => {
    const name = MODES.find((item) => item.value === value)?.plugin;
    return store.plugins.find((plugin) => plugin.name === name);
  };
  const rotatedMatrix = createMemo(() => rotateArray(store.indexMatrix, store.rotation));

  onMount(async () => {
    if (pluginFor("board") && store.plugin === pluginFor("board")?.id) {
      setMode("board");
    }
    try {
      const response = await fetch(`${API_URL}api/config`);
      if (response.ok) {
        const data = await response.json();
        setScrollText(data.scrollText ?? "");
        setScrollSpeed(data.scrollSpeed ?? 60);
        setScrollSize(data.scrollSize ?? 2);
        setBoardText(data.boardText ?? "");
        setBoardSpeed(data.boardSpeed ?? 120);
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
    const plugin = pluginFor(mode());
    if (!plugin) {
      toast("The lamp firmware lacks this mode, update it first", 3000);
      return;
    }

    const body =
      mode() === "scroll"
        ? {
            scrollText: normalize(scrollText()),
            scrollSpeed: scrollSpeed(),
            scrollSize: scrollSize(),
          }
        : { boardText: normalize(boardText()), boardSpeed: boardSpeed() };

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
          <div class="segmented w-full" role="group" aria-label="Text mode">
            <For each={MODES}>
              {(item) => (
                <button
                  type="button"
                  class="flex-1"
                  aria-pressed={mode() === item.value}
                  onClick={() => setMode(item.value)}
                >
                  <i class={`fa-solid ${item.icon} mr-1.5 text-xs`} />
                  {item.label}
                </button>
              )}
            </For>
          </div>

          <Show
            when={mode() === "scroll"}
            fallback={
              <>
                <div class="space-y-3">
                  <h3 class="section-title">Board text</h3>
                  <textarea
                    id="board-text"
                    rows={6}
                    maxLength={BOARD_MAX_LENGTH}
                    placeholder={"Menu\nCoffee 3\nTea 2,50"}
                    value={boardText()}
                    disabled={isLoading()}
                    onInput={(e) => setBoardText(e.currentTarget.value)}
                    class="input resize-none"
                  />
                  <p class="flex justify-between gap-3 text-xs text-muted">
                    <span>
                      Stands still in the largest size that fits. Longer text scrolls up like a
                      reader. Enter starts a new line.
                    </span>
                    <span class="shrink-0">
                      {boardText().length}/{BOARD_MAX_LENGTH}
                    </span>
                  </p>
                </div>
                <SpeedSlider
                  id="board-speed"
                  label="Reader speed"
                  delay={boardSpeed()}
                  disabled={isLoading()}
                  onChange={setBoardSpeed}
                />
              </>
            }
          >
            <div class="space-y-3">
              <h3 class="section-title">Scrolling text</h3>
              <textarea
                id="scroll-text"
                rows={3}
                maxLength={SCROLL_MAX_LENGTH}
                placeholder="Type something to scroll…"
                value={scrollText()}
                disabled={isLoading()}
                onInput={(e) => setScrollText(e.currentTarget.value)}
                class="input resize-none"
              />
              <p class="flex justify-between text-xs text-muted">
                <span>Letters, numbers, €, accents and umlauts</span>
                <span>
                  {scrollText().length}/{SCROLL_MAX_LENGTH}
                </span>
              </p>
            </div>

            <div class="space-y-3">
              <h3 class="section-title">Size</h3>
              <div class="segmented w-full" role="group" aria-label="Text size">
                <For each={SIZES}>
                  {(size) => (
                    <button
                      type="button"
                      class="flex-1"
                      aria-pressed={scrollSize() === size.value}
                      disabled={isLoading()}
                      onClick={() => setScrollSize(size.value)}
                    >
                      {size.label}
                    </button>
                  )}
                </For>
              </div>
            </div>

            <SpeedSlider
              id="scroll-speed"
              label="Speed"
              delay={scrollSpeed()}
              disabled={isLoading()}
              onChange={setScrollSpeed}
            />
          </Show>

          <button
            type="submit"
            disabled={isLoading() || isSending()}
            class="btn btn-accent w-full py-3"
          >
            <i class="fa-solid fa-paper-plane" />
            {isSending() ? "Sending…" : "Show on lamp"}
          </button>

          <Show when={!isLoading() && !pluginFor(mode())}>
            <p class="text-xs text-muted">
              This mode is missing on the lamp. Install the latest firmware via Firmware Update.
            </p>
          </Show>
        </form>
      }
    />
  );
};

export default TextPage;

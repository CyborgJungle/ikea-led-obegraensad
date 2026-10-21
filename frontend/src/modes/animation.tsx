import {
  type Component,
  createEffect,
  createSignal,
  For,
  onCleanup,
  onMount,
  Show,
} from "solid-js";

import { Icon, IconButton } from "../components/icons";
import { LedMatrix } from "../components/led-matrix";
import { Section, Slider } from "../components/ui";
import { useStore } from "../contexts/store";
import { useToast } from "../contexts/toast";
import {
  DEFAULT_FRAME_DELAY_MS,
  HISTORY_DEBOUNCE_MS,
  INITIAL_HISTORY_SAVE_DELAY_MS,
  MAX_FRAME_DELAY_MS,
  MIN_FRAME_DELAY_MS,
} from "../creator/constants";
import { useAnimation } from "../creator/hooks/useAnimation";
import { useAnimationStorage } from "../creator/hooks/useAnimationStorage";
import { useHistory } from "../creator/hooks/useHistory";
import type { FrameSignals } from "../creator/types";
import { createNewScreen, focusFrame, scrollTimelineToFrame } from "../creator/utils/helpers";
import { createKeyboardHandler } from "../creator/utils/keyboard";
import { matrixToHexArray } from "../helpers";
import type { PanelProps } from "./shared";

// Animation: draw frames on the phone, play them back, upload them to the lamp.
// Frames are kept on this phone (local storage) between visits.
export const AnimationPanel: Component<PanelProps> = (props) => {
  const [store, actions] = useStore();
  const { toast } = useToast();

  const [frames, setFrames] = createSignal<FrameSignals>([]);
  const [delayMs, setDelayMs] = createSignal(DEFAULT_FRAME_DELAY_MS);
  const [focused, setFocused] = createSignal(0);

  const { saveToLocalStorage, loadFromLocalStorage, hasLoadedFromStorage } =
    useAnimationStorage(createNewScreen);
  const { saveToHistory, undo, redo, canUndo, canRedo, isUndoRedoing } = useHistory<number[][]>({
    onUndo: () => toast("Undo", 800),
    onRedo: () => toast("Redo", 800),
  });
  const { isPlaying, currentFrame, togglePlay } = useAnimation(frames, delayMs);

  let timelineRef: HTMLDivElement | undefined;
  const scrollToFrame = (index: number) =>
    scrollTimelineToFrame(timelineRef, index, frames().length);
  const focusOn = (index: number, delay?: number) =>
    focusFrame(setFocused, scrollToFrame, index, delay);

  const focusedFrame = () => frames()[focused()] ?? null;

  const addFrame = () => {
    setFrames((list) => {
      const last = list.length > 0 ? list[list.length - 1][0]() : undefined;
      return [...list, createNewScreen(last ? [...last] : undefined)];
    });
    focusOn(frames().length - 1);
  };

  const removeFrame = (index: number) => {
    const total = frames().length;
    if (total === 0) return;
    setFrames((list) => list.filter((_, i) => i !== index));
    if (total > 1) focusOn(Math.min(index, total - 2), 0);
  };

  const duplicateFrame = (index: number) => {
    setFrames((list) => {
      const copy = [...list];
      copy.splice(index + 1, 0, createNewScreen([...list[index][0]()]));
      return copy;
    });
    focusOn(index + 1);
  };

  const clearFrame = (index: number) => frames()[index]?.[1](new Array(256).fill(0));

  const moveFrame = (index: number, step: -1 | 1) => {
    const target = index + step;
    if (target < 0 || target >= frames().length) return;
    setFrames((list) => {
      const copy = [...list];
      [copy[index], copy[target]] = [copy[target], copy[index]];
      return copy;
    });
    focusOn(target);
  };

  const restore = (state: number[][] | null | undefined) => {
    if (!state) return;
    setFrames(state.map((frame) => createNewScreen(frame)) as FrameSignals);
    if (focused() >= state.length) setFocused(Math.max(0, state.length - 1));
  };

  const upload = () => {
    const screens = frames().map(([screen]) => screen());
    actions.send(
      JSON.stringify({
        event: "upload",
        screens: screens.length,
        frameDelay: delayMs(),
        data: screens.map((screen) => matrixToHexArray(screen.map((s) => (s > 0 ? 1 : 0)))),
      }),
    );
    toast("Animation sent to the lamp", 2000);
  };

  // the upload goes to the Animation mode, so switch to it first
  const uploadToLamp = () => {
    if (props.active) {
      upload();
    } else {
      props.activate();
      setTimeout(upload, 500);
    }
  };

  const exportJson = () => {
    const data = { version: 1, frames: frames().map(([screen]) => screen()) };
    const link = document.createElement("a");
    link.href = `data:application/json;charset=utf-8,${encodeURIComponent(JSON.stringify(data, null, 2))}`;
    link.download = "animation.json";
    link.click();
    toast("Exported animation.json", 1500);
  };

  const importJson = () => {
    const input = document.createElement("input");
    input.type = "file";
    input.accept = ".json,application/json";
    input.onchange = () => {
      const file = input.files?.[0];
      if (!file) return;
      const reader = new FileReader();
      reader.onload = () => {
        try {
          const data = JSON.parse(reader.result as string);
          if (!Array.isArray(data.frames)) throw new Error("no frames");
          setFrames(data.frames.map((frame: number[]) => createNewScreen(frame)) as FrameSignals);
          focusOn(data.frames.length - 1);
          toast(`Loaded ${data.frames.length} frames`, 1500);
        } catch {
          toast("That is not an animation file", 2000);
        }
      };
      reader.readAsText(file);
    };
    input.click();
  };

  const handleKeyDown = createKeyboardHandler(
    {
      onUndo: () => restore(undo()),
      onRedo: () => restore(redo()),
      onTogglePlay: togglePlay,
      onPreviousFrame: () => focused() > 0 && focusOn(focused() - 1, 0),
      onNextFrame: () => focused() < frames().length - 1 && focusOn(focused() + 1, 0),
      onDeleteFrame: () => frames().length > 0 && removeFrame(focused()),
      onDuplicateFrame: () => frames().length > 0 && duplicateFrame(focused()),
    },
    { hasFrames: () => frames().length > 0, isPlaying },
  );

  onMount(() => {
    const loaded = loadFromLocalStorage();
    if (loaded) {
      setFrames(loaded.frames);
      if (loaded.animationDelayMs) setDelayMs(loaded.animationDelayMs);
    }
    setTimeout(() => {
      if (frames().length > 0) saveToHistory(frames().map(([screen]) => [...screen()]));
    }, INITIAL_HISTORY_SAVE_DELAY_MS);
    window.addEventListener("keydown", handleKeyDown);
    onCleanup(() => window.removeEventListener("keydown", handleKeyDown));
  });

  // history and local storage follow every drawing change
  createEffect(() => {
    const list = frames();
    for (const [screen] of list) screen();
    if (hasLoadedFromStorage() && list.length > 0) {
      const timer = setTimeout(() => {
        if (!isUndoRedoing()) saveToHistory(list.map(([screen]) => [...screen()]));
      }, HISTORY_DEBOUNCE_MS);
      onCleanup(() => clearTimeout(timer));
    }
  });

  createEffect(() => {
    if (!hasLoadedFromStorage()) return;
    saveToLocalStorage(
      frames().map(([screen]) => screen()),
      delayMs(),
    );
  });

  return (
    <Show
      when={frames().length > 0}
      fallback={
        <section class="card flex flex-col items-center gap-4 py-10 text-center">
          <span class="text-[22px] font-bold tracking-[-0.02em]">Make an animation</span>
          <p class="m-0 max-w-[260px] text-[15px] text-muted">
            Draw frame by frame, play it here, then send it to the lamp.
          </p>
          <div class="flex gap-2">
            <button type="button" class="btn btn-signal" onClick={addFrame}>
              <Icon name="plus" size={18} />
              First frame
            </button>
            <button type="button" class="btn btn-line" onClick={importJson}>
              Import
            </button>
          </div>
        </section>
      }
    >
      <Show
        when={!isPlaying()}
        fallback={
          <LedMatrix
            class="w-[min(100%,44dvh*9/13)]"
            data={currentFrame()}
            indexData={store.indexMatrix}
            brightness={255}
          />
        }
      >
        <Show when={focusedFrame()}>
          {(frame) => (
            <LedMatrix
              class="w-[min(100%,44dvh*9/13)]"
              data={frame()[0]()}
              indexData={store.indexMatrix}
              brightness={255}
              onSetLed={(data) => {
                const pixels = [...frame()[0]()];
                pixels[data.index] = Number(data.status);
                frame()[1](pixels);
              }}
              onSetMatrix={(data) => frame()[1]([...data])}
            />
          )}
        </Show>
      </Show>

      <div class="flex items-center justify-between gap-2">
        <IconButton
          name="undo"
          label="Undo"
          disabled={!canUndo() || isPlaying()}
          onClick={() => restore(undo())}
        />
        <IconButton
          name="redo"
          label="Redo"
          disabled={!canRedo() || isPlaying()}
          onClick={() => restore(redo())}
        />
        <button type="button" class="btn btn-ink flex-1" onClick={togglePlay}>
          <Icon name={isPlaying() ? "stop" : "play"} size={16} />
          {isPlaying() ? "Stop" : "Play"}
        </button>
        <IconButton name="plus" label="Add frame" disabled={isPlaying()} onClick={addFrame} />
      </div>

      <Section title={`Frame ${focused() + 1} of ${frames().length}`}>
        <div class="flex justify-between">
          <IconButton
            name="arrowLeft"
            label="Move frame left"
            disabled={isPlaying() || focused() === 0}
            onClick={() => moveFrame(focused(), -1)}
          />
          <IconButton
            name="arrowRight"
            label="Move frame right"
            disabled={isPlaying() || focused() === frames().length - 1}
            onClick={() => moveFrame(focused(), 1)}
          />
          <IconButton
            name="copy"
            label="Duplicate frame"
            disabled={isPlaying()}
            onClick={() => duplicateFrame(focused())}
          />
          <IconButton
            name="eraser"
            label="Clear frame"
            disabled={isPlaying()}
            onClick={() => clearFrame(focused())}
          />
          <IconButton
            name="trash"
            label="Delete frame"
            disabled={isPlaying()}
            onClick={() => removeFrame(focused())}
          />
        </div>
        <div ref={timelineRef} class="-mx-5 flex gap-2 overflow-x-auto px-5 pb-1">
          <For each={frames()}>
            {([screen], index) => (
              <button
                type="button"
                onClick={() => setFocused(index())}
                aria-label={`Frame ${index() + 1}`}
                aria-current={focused() === index() ? "true" : undefined}
                class={`flex shrink-0 flex-col items-center gap-1 rounded-2xl p-1.5 transition ${
                  focused() === index() ? "bg-signal text-on-signal" : "bg-card text-muted"
                }`}
              >
                <span class="grid h-[46px] w-8 grid-cols-16 overflow-hidden rounded-md bg-[#050506]">
                  <For each={screen()}>
                    {(pixel) => <span class={pixel ? "bg-white" : "bg-white/5"} />}
                  </For>
                </span>
                <span class="text-[11px] font-semibold tabular-nums">{index() + 1}</span>
              </button>
            )}
          </For>
        </div>
      </Section>

      <Section
        title="Frame time"
        aside={<span class="text-[13px] font-semibold tabular-nums">{delayMs()} ms</span>}
      >
        <Slider
          label="Frame time"
          min={MIN_FRAME_DELAY_MS}
          max={MAX_FRAME_DELAY_MS}
          step={10}
          value={delayMs()}
          onInput={setDelayMs}
        />
      </Section>

      <button
        type="button"
        class="btn btn-signal h-14 text-base"
        disabled={isPlaying()}
        onClick={uploadToLamp}
      >
        <Icon name="upload" size={18} />
        Send to lamp · {frames().length} frames
      </button>
      <div class="grid grid-cols-2 gap-2">
        <button type="button" class="btn btn-line" onClick={importJson}>
          <Icon name="download" size={16} />
          Import
        </button>
        <button type="button" class="btn btn-line" onClick={exportJson}>
          <Icon name="upload" size={16} />
          Export
        </button>
      </div>
    </Show>
  );
};

import { type Component, createMemo, For, Show } from "solid-js";

import { Icon, type IconName } from "../components/icons";
import { LedMatrix } from "../components/led-matrix";
import { useStore } from "../contexts/store";
import { useToast } from "../contexts/toast";
import { loadImageAndGetDataArray, rotateArray } from "../helpers";
import type { PanelProps } from "./shared";

// Draw: paint on the board with a finger, the lamp follows LED by LED
export const DrawPanel: Component<PanelProps> = (props) => {
  const [store, actions] = useStore();
  const { toast } = useToast();
  const rotatedMatrix = createMemo(() => rotateArray(store.indexMatrix, store.rotation));
  const canDraw = () => props.active && !store.isActiveScheduler;

  const send = (event: string, data?: Record<string, unknown>) =>
    actions.send(JSON.stringify({ event, ...data }));

  const tools: { label: string; icon: IconName; run: () => void }[] = [
    {
      label: "Image",
      icon: "image",
      run: () =>
        loadImageAndGetDataArray((data) => {
          actions.setLeds(store.indexMatrix.map((index) => (data[index] ? 255 : 0)));
          send("screen", { data });
        }),
    },
    {
      label: "Clear",
      icon: "eraser",
      run: () => {
        actions.setLeds(new Array(256).fill(0));
        send("clear");
        toast("Board cleared", 1000);
      },
    },
    {
      label: "Save",
      icon: "save",
      run: () => {
        send("persist");
        toast("Drawing saved on the lamp", 1500);
      },
    },
    {
      label: "Load",
      icon: "folder",
      run: () => {
        send("load");
        toast("Saved drawing loaded", 1500);
      },
    },
  ];

  return (
    <>
      {/* the board takes the height that is left, so the tools stay above the tab bar */}
      <div class="flex min-h-[200px] flex-1 items-center justify-center [container-type:size]">
        <LedMatrix
          class="w-[min(100cqw,calc((100cqh_-_1.5rem)*9/13_+_1.5rem))]"
          disabled={!canDraw()}
          data={store.leds || []}
          indexData={rotatedMatrix()}
          brightness={store.brightness ?? store.baseBrightness ?? 255}
          onSetLed={(data) => send("led", data)}
          onSetMatrix={(data) => actions.setLeds([...data])}
        />
      </div>
      <Show
        when={canDraw()}
        fallback={
          <button type="button" class="btn btn-signal h-14 text-base" onClick={props.activate}>
            <Icon name="play" size={18} />
            Draw on the lamp
          </button>
        }
      >
        <p class="m-0 text-center text-[13px] text-muted">Tap or drag to switch LEDs on and off</p>
      </Show>
      <div class="grid grid-cols-4 gap-2">
        <For each={tools}>
          {(tool) => (
            <button
              type="button"
              disabled={!canDraw()}
              onClick={tool.run}
              class="flex flex-col items-center gap-1.5 rounded-[20px] bg-card py-3 text-[13px] font-semibold text-ink transition active:scale-95 disabled:opacity-40"
            >
              <Icon name={tool.icon} size={22} />
              {tool.label}
            </button>
          )}
        </For>
      </div>
    </>
  );
};

import { type Component, createMemo, Show } from "solid-js";

import { Icon } from "../components/icons";
import { LedMatrix } from "../components/led-matrix";
import { PageTitle, Section, Slider } from "../components/ui";
import { useStore } from "../contexts/store";
import { useToast } from "../contexts/toast";
import { rotateArray } from "../helpers";
import { stopSchedule } from "../lib/api";
import { modeLabel, modeNote } from "../lib/modes";
import { navigate } from "../lib/router";

const percent = (value: number) => Math.round((value / 255) * 100);

export const LivePreview: Component<{ class?: string }> = (props) => {
  const [store] = useStore();
  const rotatedMatrix = createMemo(() => rotateArray(store.indexMatrix, store.rotation));
  return (
    <div class="relative">
      <LedMatrix
        disabled
        class={props.class}
        data={store.leds || []}
        indexData={rotatedMatrix()}
        brightness={store.brightness ?? store.baseBrightness ?? 255}
      />
      <Show when={!store.livePreview}>
        <div class="absolute inset-0 mx-auto flex flex-col items-center justify-center gap-2 rounded-3xl text-center text-[13px] font-semibold text-[#a39d94]">
          Live preview is off
          <a href="#/settings" class="text-[#ece8e1]">
            Turn on in Settings
          </a>
        </div>
      </Show>
    </div>
  );
};

export const Home: Component = () => {
  const [store, actions] = useStore();
  const { toast } = useToast();
  const active = () => store.plugins.find((plugin) => plugin.id === store.plugin);
  const activeName = () => active()?.name ?? "";

  const handleStopRoutine = async () => {
    try {
      await stopSchedule();
      toast("Routine stopped", 1500);
    } catch {
      toast("Could not reach the lamp", 2000);
    }
  };

  return (
    <main class="page animate-fade-in">
      <PageTitle
        title="Obegränsad"
        sub={store.isActiveScheduler ? "Routine running" : "Live"}
        aside={
          <span class="mt-1 flex items-center gap-2 rounded-full bg-card px-3 py-2 font-mono text-[11px] tracking-[0.08em] text-muted uppercase">
            <span class="size-2 rounded-full bg-signal" />
            {store.plugins.length} modes
          </span>
        }
      />

      {/* the board takes the height that is left, so home fits on one screen;
          the width keeps the 9:13 board (plus its 0.75rem frame) inside it */}
      <div class="flex min-h-[200px] flex-1 items-center justify-center [container-type:size]">
        <LivePreview class="w-[min(100cqw,calc((100cqh_-_1.5rem)*9/13_+_1.5rem))]" />
      </div>

      <Section title="Now showing">
        <Show
          when={!store.isActiveScheduler}
          fallback={
            <div class="bar bg-slate text-on-slate">
              <span class="flex flex-col gap-1">
                <span class="bar-name">Routine</span>
                <span class="text-[13px]">
                  {active() ? `${modeLabel(activeName())} · ` : ""}
                  {store.schedule.length} modes in turn
                </span>
              </span>
              <button type="button" class="btn btn-ink shrink-0" onClick={handleStopRoutine}>
                <Icon name="stop" size={16} />
                Stop
              </button>
            </div>
          }
        >
          <div class="bar bg-signal pr-3 text-on-signal">
            <a
              href={active() ? `#/mode/${store.plugin}` : "#/modes"}
              class="flex min-w-0 flex-1 flex-col gap-1 text-inherit no-underline"
            >
              <span class="bar-name truncate">{active() ? modeLabel(activeName()) : "…"}</span>
              <span class="flex items-center gap-1 text-[13px] font-semibold">
                {active() ? modeNote(activeName()) || "Settings" : "Pick a mode"}
                <Icon name="chevronRight" size={14} />
              </span>
            </a>
            <button
              type="button"
              class="flex size-14 shrink-0 items-center justify-center rounded-full bg-inverse text-on-inverse transition active:scale-95"
              aria-label="Choose a mode"
              onClick={() => navigate("/modes")}
            >
              <Icon name="chevronDown" size={22} />
            </button>
          </div>
        </Show>
      </Section>

      <section class="card flex flex-col gap-2">
        <div class="flex items-end justify-between gap-4">
          <div>
            <h2 class="text-[15px] font-semibold">Brightness</h2>
            <Show when={store.brightnessSchedule.active}>
              <p class="label mt-1">Night schedule · now {percent(store.brightness)}%</p>
            </Show>
          </div>
          <div class="numeral text-[56px]">
            {percent(store.baseBrightness)}
            <span class="text-[22px] tracking-normal">%</span>
          </div>
        </div>
        <Slider
          label="Brightness"
          min={0}
          max={255}
          value={store.baseBrightness}
          onInput={(value) => actions.setBaseBrightness(value)}
          onCommit={(value) =>
            actions.send(JSON.stringify({ event: "brightness", brightness: value }))
          }
        />
      </section>
    </main>
  );
};

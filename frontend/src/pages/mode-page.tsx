import { type Component, Show } from "solid-js";
import { Dynamic } from "solid-js/web";

import { Icon, IconButton } from "../components/icons";
import { BackBar } from "../components/ui";
import { useStore } from "../contexts/store";
import { useToast } from "../contexts/toast";
import { modeKey, modeLabel, modeNote } from "../lib/modes";
import { AnimationPanel } from "../modes/animation";
import { CountdownPanel } from "../modes/countdown";
import { DrawPanel } from "../modes/draw";
import type { PanelProps } from "../modes/shared";
import {
  ArtNetPanel,
  CameraPanel,
  ClockPanel,
  DdpPanel,
  LifePanel,
  WeatherPanel,
} from "../modes/simple";
import { BoardPanel, TextPanel } from "../modes/text";
import { LivePreview } from "./home";

// settings per mode; ownBoard: the panel shows its own (editable) LED board
const PANELS: Record<string, { panel: Component<PanelProps>; ownBoard?: boolean }> = {
  Draw: { panel: DrawPanel, ownBoard: true },
  Animation: { panel: AnimationPanel, ownBoard: true },
  Text: { panel: TextPanel },
  Board: { panel: BoardPanel },
  Countdown: { panel: CountdownPanel },
  Camera: { panel: CameraPanel },
  GameOfLife: { panel: LifePanel },
  ArtNet: { panel: ArtNetPanel },
  DDP: { panel: DdpPanel },
  Weather: { panel: WeatherPanel },
  BigClock: { panel: ClockPanel },
  Clock: { panel: ClockPanel },
  PongClock: { panel: ClockPanel },
  TickingClock: { panel: ClockPanel },
};

// A mode, full screen: its name, the board and its settings. Back goes home.
export const ModePage: Component<{ id: number }> = (props) => {
  const [store, actions] = useStore();
  const { toast } = useToast();

  const plugin = () => store.plugins.find((p) => p.id === props.id);
  const active = () => store.plugin === props.id && !store.isActiveScheduler;
  const isDefault = () => store.persistedPlugin === props.id;
  const name = () => plugin()?.name ?? "";
  const entry = () => PANELS[modeKey(name())];

  const activate = () => actions.send(JSON.stringify({ event: "plugin", plugin: props.id }));

  const makeDefault = () => {
    actions.send(JSON.stringify({ event: "persist-plugin" }));
    toast(`${modeLabel(name())} starts when the lamp turns on`, 2000);
  };

  return (
    <main class="page page-full animate-page-in">
      <BackBar
        label={`Mode #${props.id}`}
        aside={
          <IconButton
            name="star"
            label={isDefault() ? "Starts with the lamp" : "Start with this mode"}
            pressed={isDefault()}
            disabled={!active() || isDefault()}
            class={isDefault() ? "border-signal bg-signal text-on-signal disabled:opacity-100" : ""}
            onClick={makeDefault}
          />
        }
      />

      <Show when={plugin()} fallback={<p class="text-muted">Loading modes…</p>}>
        {(mode) => (
          <>
            <div class={`bar ${active() ? "bg-signal text-on-signal" : "bg-cream text-[#151514]"}`}>
              <span class="flex min-w-0 flex-col gap-1">
                <span class="bar-name truncate">{modeLabel(mode().name)}</span>
                <span class="text-[13px] font-semibold">
                  {active() ? "● Showing on the lamp" : modeNote(mode().name)}
                </span>
              </span>
              <Show when={!active()}>
                <button type="button" class="btn btn-ink shrink-0" onClick={activate}>
                  <Icon name="play" size={16} />
                  Show
                </button>
              </Show>
            </div>

            <Show when={!entry()?.ownBoard}>
              {/* gives way to the settings below it, like on the home page */}
              <div class="flex max-h-[40dvh] min-h-[180px] flex-1 items-center justify-center [container-type:size]">
                <LivePreview class="w-[min(100cqw,calc((100cqh_-_1.5rem)*9/13_+_1.5rem))]" />
              </div>
            </Show>

            <Show
              when={entry()}
              fallback={
                <p class="m-0 text-center text-[15px] text-muted">
                  Nothing to set here, it runs by itself.
                </p>
              }
            >
              {(found) => (
                <Dynamic
                  component={found().panel}
                  plugin={mode()}
                  active={active()}
                  activate={activate}
                />
              )}
            </Show>
          </>
        )}
      </Show>
    </main>
  );
};

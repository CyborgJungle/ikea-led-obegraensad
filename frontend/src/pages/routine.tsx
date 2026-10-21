import { type Component, createEffect, createSignal, For, onMount, Show } from "solid-js";

import { Icon, IconButton } from "../components/icons";
import { PageTitle, Section, Slider, Switch } from "../components/ui";
import { useStore } from "../contexts/store";
import { useToast } from "../contexts/toast";
import { clearSchedule, getConfig, saveConfig, startSchedule, stopSchedule } from "../lib/api";
import { modeLabel } from "../lib/modes";
import type { BrightnessSchedule, ScheduleItem } from "../types";

const formatDuration = (seconds: number) =>
  seconds >= 3600
    ? `${Math.floor(seconds / 3600)}h ${Math.round((seconds % 3600) / 60)}m`
    : seconds >= 60
      ? `${Math.floor(seconds / 60)}m ${seconds % 60 ? `${seconds % 60}s` : ""}`
      : `${seconds}s`;

const TimeRange: Component<{
  from: string;
  to: string;
  onChange: (from: string, to: string) => void;
}> = (props) => (
  <div class="grid grid-cols-2 gap-3">
    <label class="flex flex-col gap-1.5">
      <span class="label">From</span>
      <input
        type="time"
        class="input tabular-nums"
        value={props.from}
        onChange={(e) => props.onChange(e.currentTarget.value, props.to)}
      />
    </label>
    <label class="flex flex-col gap-1.5">
      <span class="label">To</span>
      <input
        type="time"
        class="input tabular-nums"
        value={props.to}
        onChange={(e) => props.onChange(props.from, e.currentTarget.value)}
      />
    </label>
  </div>
);

export const Routine: Component = () => {
  const [store, actions] = useStore();
  const { toast } = useToast();

  // edited here, sent on Start; follows the lamp until changed
  const [items, setItems] = createSignal<ScheduleItem[]>([]);
  const [edited, setEdited] = createSignal(false);
  createEffect(() => {
    const fromLamp = store.schedule;
    if (!edited()) setItems(fromLamp.map((item) => ({ ...item })));
  });

  const [hoursEnabled, setHoursEnabled] = createSignal(false);
  const [hoursFrom, setHoursFrom] = createSignal("08:00");
  const [hoursTo, setHoursTo] = createSignal("23:00");
  const [autoStart, setAutoStart] = createSignal(false);

  onMount(async () => {
    try {
      const config = await getConfig();
      setHoursEnabled(config.scheduleHoursEnabled ?? false);
      setHoursFrom(config.scheduleFrom ?? "08:00");
      setHoursTo(config.scheduleTo ?? "23:00");
      setAutoStart(config.autoStartSchedule ?? false);
    } catch {
      toast("Could not load the routine settings", 2000);
    }
  });

  const update = (index: number, changes: Partial<ScheduleItem>) => {
    setEdited(true);
    setItems((list) => list.map((item, i) => (i === index ? { ...item, ...changes } : item)));
  };

  const move = (index: number, step: -1 | 1) => {
    const target = index + step;
    if (target < 0 || target >= items().length) return;
    setEdited(true);
    setItems((list) => {
      const copy = [...list];
      [copy[index], copy[target]] = [copy[target], copy[index]];
      return copy;
    });
  };

  const add = () => {
    setEdited(true);
    setItems((list) => [...list, { pluginId: store.plugin, duration: 60 }]);
  };

  const remove = (index: number) => {
    setEdited(true);
    setItems((list) => list.filter((_, i) => i !== index));
  };

  const total = () => items().reduce((sum, item) => sum + (item.duration || 0), 0);
  const valid = () => items().length > 0 && items().every((item) => item.duration >= 1);

  const run = async (action: () => Promise<unknown>, done: string) => {
    try {
      await action();
      setEdited(false);
      toast(done, 1500);
    } catch {
      toast("Could not reach the lamp", 2000);
    }
  };

  const saveHours = async (enabled: boolean, from: string, to: string) => {
    if (from === to) {
      toast("From and to need to differ", 2000);
      return;
    }
    setHoursEnabled(enabled);
    setHoursFrom(from);
    setHoursTo(to);
    await run(
      () => saveConfig({ scheduleHoursEnabled: enabled, scheduleFrom: from, scheduleTo: to }),
      enabled ? `Runs from ${from} to ${to}` : "Runs any time",
    );
  };

  const sendLights = (changes: Partial<BrightnessSchedule>) => {
    const next = { ...store.brightnessSchedule, ...changes };
    if (next.startTime === next.endTime) {
      toast("From and to need to differ", 2000);
      return;
    }
    actions.setBrightnessSchedule(next);
    actions.send(JSON.stringify({ event: "brightness-schedule", ...next }));
  };

  return (
    <main class="page animate-fade-in">
      <PageTitle title="Routine" sub="Modes in turn" />

      <Show when={store.isActiveScheduler}>
        <div class="bar bg-slate text-on-slate">
          <span class="flex flex-col gap-1">
            <span class="bar-name">Running</span>
            <span class="text-[13px]">{formatDuration(total())} per round</span>
          </span>
          <button
            type="button"
            class="btn btn-ink shrink-0"
            onClick={() => run(stopSchedule, "Routine stopped")}
          >
            <Icon name="stop" size={16} />
            Stop
          </button>
        </div>
      </Show>

      <Section
        title="Sequence"
        aside={
          <span class="text-[13px] font-semibold text-muted">{formatDuration(total())} round</span>
        }
      >
        <ol class="m-0 flex list-none flex-col gap-2 p-0">
          <For
            each={items()}
            fallback={
              <li class="card text-center text-[15px] text-muted">No modes yet. Add the first.</li>
            }
          >
            {(item, index) => (
              <li class="card flex flex-col gap-3 p-3">
                <div class="flex items-center gap-2">
                  <span class="flex size-9 shrink-0 items-center justify-center rounded-full bg-inverse font-mono text-[13px] text-on-inverse">
                    {index() + 1}
                  </span>
                  <select
                    aria-label={`Mode ${index() + 1}`}
                    class="input h-11 min-w-0 flex-1 text-[17px] font-semibold"
                    value={item.pluginId}
                    onChange={(e) => update(index(), { pluginId: Number(e.currentTarget.value) })}
                  >
                    <For each={store.plugins}>
                      {(plugin) => <option value={plugin.id}>{modeLabel(plugin.name)}</option>}
                    </For>
                  </select>
                  <IconButton name="trash" label="Remove" onClick={() => remove(index())} />
                </div>
                <div class="flex items-center gap-2">
                  <input
                    type="number"
                    inputmode="numeric"
                    min="1"
                    aria-label="Seconds"
                    class="input h-11 w-24 text-center tabular-nums"
                    value={item.duration}
                    onInput={(e) =>
                      update(index(), { duration: e.currentTarget.valueAsNumber || 0 })
                    }
                  />
                  <span class="flex-1 text-[14px] text-muted">
                    seconds · {formatDuration(item.duration || 0)}
                  </span>
                  <IconButton
                    name="arrowUp"
                    label="Earlier"
                    disabled={index() === 0}
                    onClick={() => move(index(), -1)}
                  />
                  <IconButton
                    name="arrowDown"
                    label="Later"
                    disabled={index() === items().length - 1}
                    onClick={() => move(index(), 1)}
                  />
                </div>
              </li>
            )}
          </For>
        </ol>
        <button type="button" class="btn btn-line" onClick={add}>
          <Icon name="plus" size={18} />
          Add mode
        </button>
        <div class="grid grid-cols-[1fr_auto] gap-2">
          <button
            type="button"
            class="btn btn-signal h-14 text-base"
            disabled={!valid()}
            onClick={() => run(() => startSchedule(items()), "Routine started")}
          >
            <Icon name="play" size={18} />
            {store.isActiveScheduler && !edited() ? "Restart" : "Start"}
          </button>
          <button
            type="button"
            class="btn btn-line h-14"
            onClick={() =>
              run(async () => {
                await clearSchedule();
                setItems([]);
              }, "Routine cleared")
            }
          >
            Clear
          </button>
        </div>
      </Section>

      <Section title="When it runs">
        <div class="card flex flex-col gap-2">
          <Switch
            label="Only between set times"
            hint="Starts and stops by itself, every day"
            checked={hoursEnabled()}
            onChange={(on) => saveHours(on, hoursFrom(), hoursTo())}
          />
          <Show when={hoursEnabled()}>
            <TimeRange
              from={hoursFrom()}
              to={hoursTo()}
              onChange={(from, to) => saveHours(true, from, to)}
            />
          </Show>
          <div class="my-1 border-t border-line" />
          <Switch
            label="Start when the lamp turns on"
            checked={autoStart()}
            onChange={(on) => {
              setAutoStart(on);
              run(
                () => saveConfig({ autoStartSchedule: on }),
                on ? "Starts at power-up" : "Waits for you",
              );
            }}
          />
        </div>
      </Section>

      <Section title="Lamp on & off">
        <div class="card flex flex-col gap-2">
          <Switch
            label="Dim or switch off at set times"
            hint={store.brightnessSchedule.active ? "Active right now" : "Night mode"}
            checked={store.brightnessSchedule.enabled}
            onChange={(on) => sendLights({ enabled: on })}
          />
          <Show when={store.brightnessSchedule.enabled}>
            <TimeRange
              from={store.brightnessSchedule.startTime}
              to={store.brightnessSchedule.endTime}
              onChange={(from, to) => sendLights({ startTime: from, endTime: to })}
            />
            <div class="mt-2 flex items-baseline justify-between">
              <span class="text-[15px] font-semibold">Brightness then</span>
              <span class="text-[15px] font-semibold tabular-nums">
                {store.brightnessSchedule.brightness === 0
                  ? "Off"
                  : `${Math.round((store.brightnessSchedule.brightness / 255) * 100)}%`}
              </span>
            </div>
            <Slider
              label="Brightness during these hours"
              min={0}
              max={255}
              value={store.brightnessSchedule.brightness}
              onInput={(value) =>
                actions.setBrightnessSchedule({ ...store.brightnessSchedule, brightness: value })
              }
              onCommit={(value) => sendLights({ brightness: value })}
            />
          </Show>
        </div>
      </Section>
    </main>
  );
};

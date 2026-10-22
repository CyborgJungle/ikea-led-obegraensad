import { type Component, For, type JSX, Show } from "solid-js";

// Two-line title, the second line muted: "Obegränsad / Connected"
export const PageTitle: Component<{ title: string; sub?: JSX.Element; aside?: JSX.Element }> = (
  props,
) => (
  <header class="flex items-start justify-between gap-4">
    <h1 class="title">
      {props.title}
      <span class="title-sub">{props.sub}</span>
    </h1>
    <Show when={props.aside}>{props.aside}</Show>
  </header>
);

export const Section: Component<{ title: string; aside?: JSX.Element; children: JSX.Element }> = (
  props,
) => (
  <section class="flex flex-col gap-3">
    <div class="flex items-baseline justify-between gap-3">
      <h2 class="label">{props.title}</h2>
      {props.aside}
    </div>
    {props.children}
  </section>
);

// range input whose track fills up to the value (see index.css)
export const Slider: Component<{
  label: string;
  min: number;
  max: number;
  step?: number;
  value: number;
  onInput: (value: number) => void;
  onCommit?: (value: number) => void;
}> = (props) => {
  const fill = () => `${((props.value - props.min) / (props.max - props.min)) * 100}%`;
  return (
    <input
      type="range"
      aria-label={props.label}
      min={props.min}
      max={props.max}
      step={props.step ?? 1}
      value={props.value}
      style={{ "--fill": fill() }}
      onInput={(e) => props.onInput(Number(e.currentTarget.value))}
      onChange={(e) => props.onCommit?.(Number(e.currentTarget.value))}
    />
  );
};

export const Segmented = <T extends string | number>(props: {
  label: string;
  options: { value: T; label: string }[];
  value: T;
  onChange: (value: T) => void;
}) => (
  <div
    class="seg"
    role="group"
    aria-label={props.label}
    style={{ "grid-template-columns": `repeat(${props.options.length}, minmax(0, 1fr))` }}
  >
    <For each={props.options}>
      {(option) => (
        <button
          type="button"
          aria-pressed={props.value === option.value}
          onClick={() => props.onChange(option.value)}
        >
          {option.label}
        </button>
      )}
    </For>
  </div>
);

export const Switch: Component<{
  label: string;
  hint?: string;
  checked: boolean;
  onChange: (checked: boolean) => void;
}> = (props) => (
  <label class="row cursor-pointer">
    <span class="flex flex-col">
      <span class="text-[15px] font-semibold">{props.label}</span>
      <Show when={props.hint}>
        <span class="text-[13px] text-muted">{props.hint}</span>
      </Show>
    </span>
    <input
      type="checkbox"
      class="switch"
      checked={props.checked}
      onChange={(e) => props.onChange(e.currentTarget.checked)}
    />
  </label>
);

// "2d 4h", "3h 12m", "45m"
export const formatUptime = (seconds: number) => {
  const days = Math.floor(seconds / 86400);
  const hours = Math.floor((seconds % 86400) / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  if (days > 0) return `${days}d ${hours}h`;
  if (hours > 0) return `${hours}h ${minutes}m`;
  return `${minutes}m`;
};

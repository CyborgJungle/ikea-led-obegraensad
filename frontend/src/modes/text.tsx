import { type Component, createSignal, onMount } from "solid-js";

import { Icon } from "../components/icons";
import { Section, Segmented, Slider } from "../components/ui";
import { useToast } from "../contexts/toast";
import { getConfig, saveConfig } from "../lib/api";
import { normalizeText, type PanelProps, SPEED_MAX, SPEED_MIN, sliderToDelay } from "./shared";

// must match the limits in config.h on the device
const SCROLL_MAX_LENGTH = 200;
const BOARD_MAX_LENGTH = 500;

const useSend = (props: PanelProps) => {
  const { toast } = useToast();
  const [sending, setSending] = createSignal(false);
  return {
    sending,
    send: async (changes: Record<string, unknown>) => {
      setSending(true);
      try {
        await saveConfig(changes);
        if (!props.active) props.activate();
        toast("Showing on the lamp", 1500);
      } catch {
        toast("Could not reach the lamp", 2000);
      } finally {
        setSending(false);
      }
    },
  };
};

const SpeedSlider: Component<{ label: string; delay: number; onChange: (d: number) => void }> = (
  props,
) => (
  <div class="flex flex-col gap-1">
    <div class="flex justify-between text-[13px] font-semibold">
      <span>{props.label}</span>
      <span class="text-muted tabular-nums">{props.delay} ms</span>
    </div>
    <Slider
      label={props.label}
      min={SPEED_MIN}
      max={SPEED_MAX}
      value={sliderToDelay(props.delay)}
      onInput={(value) => props.onChange(sliderToDelay(value))}
    />
    <div class="flex justify-between text-[12px] text-faint">
      <span>Slow</span>
      <span>Fast</span>
    </div>
  </div>
);

// Text: scrolls endlessly in one of three sizes
export const TextPanel: Component<PanelProps> = (props) => {
  const [text, setText] = createSignal("");
  const [size, setSize] = createSignal(2);
  const [speed, setSpeed] = createSignal(60);
  const [loading, setLoading] = createSignal(true);
  const { send, sending } = useSend(props);

  onMount(async () => {
    try {
      const config = await getConfig();
      setText(config.scrollText ?? "");
      setSize(config.scrollSize ?? 2);
      setSpeed(config.scrollSpeed ?? 60);
    } finally {
      setLoading(false);
    }
  });

  return (
    <>
      <label class="card flex flex-col gap-2 bg-signal text-on-signal">
        <span class="label text-on-signal">Your text</span>
        <textarea
          rows={2}
          maxLength={SCROLL_MAX_LENGTH}
          placeholder="Type something…"
          value={text()}
          disabled={loading()}
          onInput={(e) => setText(e.currentTarget.value)}
          class="w-full resize-none border-0 bg-transparent p-0 text-[28px] leading-tight font-semibold tracking-[-0.02em] text-inherit outline-none placeholder:text-black/40"
        />
        <span class="text-[12px] tabular-nums">
          {text().length} / {SCROLL_MAX_LENGTH} · letters, €, accents and umlauts
        </span>
      </label>

      <Section title="Size">
        <Segmented
          label="Text size"
          value={size()}
          onChange={setSize}
          options={[
            { value: 0, label: "Small" },
            { value: 1, label: "Medium" },
            { value: 2, label: "Large" },
          ]}
        />
      </Section>

      <SpeedSlider label="Speed" delay={speed()} onChange={setSpeed} />

      <button
        type="button"
        class="btn btn-ink h-14 text-base"
        disabled={loading() || sending()}
        onClick={() =>
          send({ scrollText: normalizeText(text()), scrollSize: size(), scrollSpeed: speed() })
        }
      >
        <Icon name="upload" size={18} />
        {sending() ? "Sending…" : "Show on lamp"}
      </button>
    </>
  );
};

// Board: stands still in the largest size that fits, longer text reads like a scroll
export const BoardPanel: Component<PanelProps> = (props) => {
  const [text, setText] = createSignal("");
  const [speed, setSpeed] = createSignal(120);
  const [loading, setLoading] = createSignal(true);
  const { send, sending } = useSend(props);

  onMount(async () => {
    try {
      const config = await getConfig();
      setText(config.boardText ?? "");
      setSpeed(config.boardSpeed ?? 120);
    } finally {
      setLoading(false);
    }
  });

  return (
    <>
      <label class="card flex flex-col gap-2 bg-cream text-[#151514]">
        <span class="label text-[#151514]">Board text</span>
        <textarea
          rows={5}
          maxLength={BOARD_MAX_LENGTH}
          placeholder={"Menu\nCoffee 3\nTea 2,50"}
          value={text()}
          disabled={loading()}
          onInput={(e) => setText(e.currentTarget.value)}
          class="w-full resize-none border-0 bg-transparent p-0 text-[22px] leading-snug font-semibold text-inherit outline-none placeholder:text-black/35"
        />
        <span class="text-[12px] tabular-nums">
          {text().length} / {BOARD_MAX_LENGTH} · Enter starts a new line
        </span>
      </label>
      <p class="m-0 text-[14px] text-muted">
        Stands still in the largest size that fits. Longer text scrolls up line by line, like a
        reader.
      </p>

      <SpeedSlider label="Reader speed" delay={speed()} onChange={setSpeed} />

      <button
        type="button"
        class="btn btn-ink h-14 text-base"
        disabled={loading() || sending()}
        onClick={() => send({ boardText: normalizeText(text()), boardSpeed: speed() })}
      >
        <Icon name="upload" size={18} />
        {sending() ? "Sending…" : "Show on lamp"}
      </button>
    </>
  );
};

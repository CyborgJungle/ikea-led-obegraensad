import { type Component, createSignal, onMount } from "solid-js";

import { Section, Slider } from "../components/ui";
import { useToast } from "../contexts/toast";
import { getConfig, saveConfig } from "../lib/api";

// must match Config::MODE_SPEED_MIN/MAX on the device, percent of the normal pace
const SPEED_MIN = 25;
const SPEED_MAX = 400;

// The slider is logarithmic with 1x in the middle: slowing down to 0.25x gets
// as much room as speeding up to 4x. Positions 0..100.
const toPosition = (speed: number) =>
  (Math.log(speed / SPEED_MIN) / Math.log(SPEED_MAX / SPEED_MIN)) * 100;
const toSpeed = (position: number) => {
  const raw = SPEED_MIN * (SPEED_MAX / SPEED_MIN) ** (position / 100);
  // round values: 0.25x 0.3x .. 0.95x 1x 1.1x .. 4x
  const step = raw < 100 ? 5 : 10;
  return Math.min(SPEED_MAX, Math.max(SPEED_MIN, Math.round(raw / step) * step));
};
// "0.25×", "1×", "1.5×"
export const formatSpeed = (speed: number) => `${Number((speed / 100).toFixed(2))}×`;

// How fast a mode runs, saved on the lamp per mode when the slider is let go.
// The lamp runs the mode's clock at it, so the mode moves the same way.
export const SpeedPanel: Component<{ modeId: number }> = (props) => {
  const { toast } = useToast();
  const [speed, setSpeed] = createSignal(100);
  onMount(async () => {
    try {
      setSpeed((await getConfig()).modeSpeed?.[props.modeId] ?? 100);
    } catch {
      // the normal pace will do
    }
  });
  const save = async (position: number) => {
    try {
      await saveConfig({ modeSpeed: { [props.modeId]: toSpeed(position) } });
    } catch {
      toast("Could not reach the lamp", 2000);
    }
  };
  return (
    <Section
      title="Speed"
      aside={<span class="text-[13px] font-semibold tabular-nums">{formatSpeed(speed())}</span>}
    >
      <Slider
        label="Speed"
        min={0}
        max={100}
        step={0.5}
        value={toPosition(speed())}
        onInput={(position) => setSpeed(toSpeed(position))}
        onCommit={save}
      />
      <div class="flex justify-between text-[12px] text-faint">
        <span>Slow</span>
        <span>Fast</span>
      </div>
    </Section>
  );
};

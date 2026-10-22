import { type Component, createSignal, For, onCleanup, onMount, Show } from "solid-js";

// orange at the top, through pink, to deep blue at the bottom of the stack
const RINGS = [
  "#2e3f9e",
  "#4a5fc4",
  "#6e7fd8",
  "#9a8bdb",
  "#c892cf",
  "#e39ab5",
  "#efa083",
  "#f08a4b",
  "#ee6a33",
];

const SLOW_AFTER = 8000;

// Shown while the app connects to the lamp: a stack of rings that rises and
// settles like a wave, after the ring image of the design inspiration
export const Splash: Component<{ leaving: boolean }> = (props) => {
  const [slow, setSlow] = createSignal(false);
  onMount(() => {
    const timer = setTimeout(() => setSlow(true), SLOW_AFTER);
    onCleanup(() => clearTimeout(timer));
  });

  return (
    <div
      role="status"
      aria-label="Connecting to the lamp"
      class="fixed inset-x-0 top-0 z-50 flex flex-col items-center justify-center bg-[#0b0b0d] transition-opacity duration-500"
      style={{
        // down to the screen bottom, also where iOS stops the viewport short
        bottom: "calc(0px - var(--viewport-gap))",
        opacity: props.leaving ? 0 : 1,
        "pointer-events": props.leaving ? "none" : "auto",
      }}
    >
      <style>{`
        @keyframes ring-wave {
          0%, 100% { transform: translateY(0) scale(1); }
          50% { transform: translateY(-18px) scale(1.04); }
        }
        @keyframes ring-glow {
          0%, 100% { filter: saturate(1) brightness(1); }
          50% { filter: saturate(1.25) brightness(1.15); }
        }
        @media (prefers-reduced-motion: reduce) {
          .ring { animation: none !important; }
        }
      `}</style>
      <div
        class="relative h-[330px] w-[240px]"
        style={{ animation: "ring-glow 3s ease-in-out infinite" }}
      >
        <For each={RINGS}>
          {(color, index) => (
            <div
              class="ring absolute left-0 h-[104px] w-[240px] rounded-[50%]"
              style={{
                // later rings sit higher and in front: blue at the bottom, orange on top
                top: `${216 - index() * 12}px`,
                border: `26px solid ${color}`,
                "box-shadow": `0 6px 18px rgba(0, 0, 0, 0.35), inset 0 -4px 10px rgba(0, 0, 0, 0.25)`,
                animation: `ring-wave 2.4s ease-in-out ${index() * 0.12}s infinite`,
              }}
            />
          )}
        </For>
      </div>
      <div
        class="absolute inset-x-0 text-center"
        style={{ bottom: "max(3rem, var(--safe-bottom))" }}
      >
        <div class="text-[22px] font-bold tracking-[-0.02em] text-[#ece8e1]">Obegränsad</div>
        <div class="mt-1 font-mono text-[11px] tracking-[0.08em] text-[#a39d94] uppercase">
          <Show when={slow()} fallback="Connecting to the lamp">
            Still looking — same WiFi as the lamp?
          </Show>
        </div>
      </div>
    </div>
  );
};

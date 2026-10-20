import { type Component, createSignal, onMount, Show } from "solid-js";

import { useStore } from "../contexts/store";
import { useToast } from "../contexts/toast";

export const TextControls: Component = () => {
  const [store] = useStore();
  const { toast } = useToast();
  const [text, setText] = createSignal("Hello!");
  const [size, setSize] = createSignal(1);
  const [delay, setDelay] = createSignal(60);
  const [busy, setBusy] = createSignal(false);
  const [error, setError] = createSignal("");
  const [ready, setReady] = createSignal(false);

  const applySettings = (body: { text: string; size: number; delay: number }) => {
    setText(body.text);
    setSize(body.size);
    setDelay(body.delay);
  };

  const request = async (method: "GET" | "POST" | "DELETE") => {
    const response = await fetch("/api/text", {
      method,
      ...(method === "POST"
        ? {
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ text: text(), size: size(), delay: delay() }),
          }
        : {}),
      signal: AbortSignal.timeout(10000),
    });
    if (response.status === 404) throw new Error("Install the firmware with Text controls first.");
    const body = await response.json();
    if (!response.ok) throw new Error(body.message || "The display could not update the text.");
    return body;
  };

  onMount(async () => {
    try {
      applySettings(await request("GET"));
      setReady(true);
    } catch (cause) {
      setError(cause instanceof Error ? cause.message : "Could not read text settings.");
    }
  });

  const send = async (method: "POST" | "DELETE") => {
    if (busy()) return;
    if (method === "POST" && (!text().trim() || !/^[\x20-\x7e]{1,120}$/.test(text()))) {
      setError(
        "Enter 1–120 characters using letters, numbers and punctuation. Emoji and accents are not supported yet.",
      );
      return;
    }
    setBusy(true);
    setError("");
    try {
      applySettings(await request(method));
      toast(method === "POST" ? "Text is scrolling" : "Text stopped", 1500);
    } catch (cause) {
      setError(cause instanceof Error ? cause.message : "Could not update the display.");
    } finally {
      setBusy(false);
    }
  };

  return (
    <section class="space-y-3" aria-labelledby="scrolling-text-title">
      <h3 id="scrolling-text-title" class="section-title">
        Scrolling text
      </h3>
      <form
        class="space-y-3"
        onSubmit={(event) => {
          event.preventDefault();
          void send("POST");
        }}
      >
        <div class="space-y-1.5">
          <label for="scrolling-text-message" class="text-xs text-muted">
            Message
          </label>
          <input
            id="scrolling-text-message"
            class="input w-full"
            type="text"
            value={text()}
            maxLength={120}
            required
            disabled={busy() || !ready()}
            onInput={(event) => setText(event.currentTarget.value)}
            aria-describedby="scrolling-text-help"
            placeholder="Type your message"
          />
          <p id="scrolling-text-help" class="text-xs text-muted">
            Up to 120 letters, numbers or punctuation marks.
          </p>
        </div>
        <div class="space-y-1.5">
          <label for="scrolling-text-size" class="text-xs text-muted">
            Text size
          </label>
          <select
            id="scrolling-text-size"
            class="input w-full"
            value={size()}
            disabled={busy() || !ready()}
            onChange={(event) => setSize(Number(event.currentTarget.value))}
          >
            <option value={1}>Small · 5 × 7 pixels</option>
            <option value={2}>Large · 10 × 14 pixels</option>
          </select>
        </div>
        <div class="space-y-1.5">
          <label for="scrolling-text-speed" class="text-xs text-muted">
            Scroll speed
          </label>
          <select
            id="scrolling-text-speed"
            class="input w-full"
            value={delay()}
            disabled={busy() || !ready()}
            onChange={(event) => setDelay(Number(event.currentTarget.value))}
          >
            <option value={120}>Slow</option>
            <option value={60}>Normal</option>
            <option value={30}>Fast</option>
          </select>
        </div>
        <p class="text-xs text-muted">
          Repeats until stopped or you choose another mode. Text resets on reboot. Showing text
          stops the animation scheduler.
        </p>
        <Show when={error()}>
          <p role="alert" class="text-sm text-red-400">
            {error()}
          </p>
        </Show>
        <div class="grid grid-cols-2 gap-2">
          <button
            type="submit"
            class="btn"
            disabled={busy() || !ready() || !text().trim() || store.connectionState() !== 1}
          >
            <i class="fa-solid fa-play" />
            {busy() ? "Please wait…" : "Show text"}
          </button>
          <button
            type="button"
            class="btn"
            disabled={busy() || !ready() || store.connectionState() !== 1}
            onClick={() => void send("DELETE")}
          >
            <i class="fa-solid fa-stop" />
            Stop text
          </button>
        </div>
      </form>
    </section>
  );
};

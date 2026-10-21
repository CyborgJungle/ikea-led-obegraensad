import { createEffect, createSignal, Match, onCleanup, Show, Switch } from "solid-js";
import { render } from "solid-js/web";

import "./index.css";
import "./lib/theme";

import { Splash } from "./components/splash";
import { TabBar } from "./components/tab-bar";
import { StoreProvider, useStore } from "./contexts/store";
import { route } from "./lib/router";
import { Home } from "./pages/home";
import { ModePage } from "./pages/mode-page";
import { ModePicker } from "./pages/mode-picker";
import { Routine } from "./pages/routine";
import { Settings } from "./pages/settings";

// the start-up animation stays at least this long, so it never just flickers
const MIN_SPLASH = 1400;

const App = () => {
  const [store] = useStore();
  const ready = () => store.connectionState() === WebSocket.OPEN && store.plugins.length > 0;

  const [minimumShown, setMinimumShown] = createSignal(false);
  const [splashGone, setSplashGone] = createSignal(false);
  const timer = setTimeout(() => setMinimumShown(true), MIN_SPLASH);
  onCleanup(() => clearTimeout(timer));

  const leaving = () => ready() && minimumShown();
  createEffect(() => {
    if (leaving()) setTimeout(() => setSplashGone(true), 500);
  });

  const modeId = () => {
    const current = route();
    return current.page === "mode" ? current.id : undefined;
  };

  // pages without the tab bar: a mode and the mode picker
  const fullScreen = () => route().page === "mode" || route().page === "modes";

  return (
    <>
      <Show when={!splashGone()}>
        <Splash leaving={leaving()} />
      </Show>
      <Show when={store.plugins.length > 0}>
        <Switch fallback={<Home />}>
          <Match when={route().page === "modes"}>
            <ModePicker />
          </Match>
          <Match when={modeId()} keyed>
            {(id) => <ModePage id={id} />}
          </Match>
          <Match when={route().page === "routine"}>
            <Routine />
          </Match>
          <Match when={route().page === "settings"}>
            <Settings />
          </Match>
        </Switch>
        <Show when={!fullScreen()}>
          <TabBar />
        </Show>
        {/* lost the lamp after start-up: a thin bar instead of the splash */}
        <Show when={splashGone() && store.connectionState() !== WebSocket.OPEN}>
          <div
            role="status"
            class="fixed inset-x-0 z-40 mx-auto w-fit rounded-full bg-inverse px-4 py-2 font-mono text-[11px] tracking-[0.08em] text-on-inverse uppercase"
            style={{ top: "max(0.75rem, env(safe-area-inset-top))" }}
          >
            Reconnecting to the lamp…
          </div>
        </Show>
      </Show>
    </>
  );
};

render(
  () => (
    <StoreProvider>
      <App />
    </StoreProvider>
  ),
  document.getElementById("app") as HTMLElement,
);

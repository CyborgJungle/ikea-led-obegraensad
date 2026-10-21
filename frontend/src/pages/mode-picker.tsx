import { type Component, createMemo, For, Show } from "solid-js";

import { Icon } from "../components/icons";
import { useStore } from "../contexts/store";
import { GROUPS, modeGroup, modeLabel, modeNote } from "../lib/modes";
import { navigate } from "../lib/router";

// Every mode as a big colour bar, after the colour card of the design
// inspiration. Picking one switches the lamp and opens that mode's page.
export const ModePicker: Component = () => {
  const [store, actions] = useStore();

  const groups = createMemo(() => {
    let index = 0;
    return GROUPS.map((group) => ({
      ...group,
      modes: store.plugins
        .filter((plugin) => modeGroup(plugin.name) === group.id)
        .map((plugin) => ({ ...plugin, tone: index++ % 2 === 0 ? "cream" : "slate" })),
    })).filter((group) => group.modes.length > 0);
  });

  const pick = (id: number) => {
    if (id !== store.plugin || store.isActiveScheduler) {
      actions.send(JSON.stringify({ event: "plugin", plugin: id }));
    }
    // replace the picker in history: back from the mode page goes home
    navigate(`/mode/${id}`, true);
  };

  return (
    <main
      class="page page-full animate-page-in gap-6 bg-[#151514] text-[#ece8e1]"
      style={{ "max-width": "none", "padding-left": "1rem", "padding-right": "1rem" }}
    >
      <div class="mx-auto flex w-full max-w-[480px] flex-col gap-6">
        <header class="flex items-start justify-between gap-4 px-1">
          <div class="flex items-end gap-3">
            <span class="numeral text-[72px] font-bold text-[#d9cbbb]">{store.plugins.length}</span>
            <h1 class="title">
              Modes
              <span class="block text-[#8a847b]">Pick one</span>
            </h1>
          </div>
          <button
            type="button"
            class="disc border-[#3a3936] text-[#ece8e1]"
            aria-label="Close"
            onClick={() => navigate("/", true)}
          >
            <Icon name="close" />
          </button>
        </header>

        <For each={groups()}>
          {(group) => (
            <section class="flex flex-col gap-2.5" aria-label={group.label}>
              <h2 class="label px-1 text-[#a39d94]">{group.label}</h2>
              <ul class="m-0 flex list-none flex-col gap-2.5 p-0">
                <For each={group.modes}>
                  {(mode) => {
                    const current = () => mode.id === store.plugin && !store.isActiveScheduler;
                    return (
                      <li>
                        <button
                          type="button"
                          aria-current={current() ? "true" : undefined}
                          onClick={() => pick(mode.id)}
                          class={`bar w-full border-0 ${
                            current()
                              ? "bg-[#e8622c] text-[#151514]"
                              : mode.tone === "cream"
                                ? "bg-[#e8d8c9] text-[#151514]"
                                : "bg-[#4b607f] text-[#f7f5f1]"
                          }`}
                        >
                          <span class="bar-name truncate">{modeLabel(mode.name)}</span>
                          <span class="flex shrink-0 flex-col items-end gap-1">
                            <span class="font-mono text-[13px]">#{mode.id}</span>
                            <span class="text-[12px]">
                              <Show when={current()} fallback={modeNote(mode.name)}>
                                ● Showing now
                              </Show>
                            </span>
                          </span>
                        </button>
                      </li>
                    );
                  }}
                </For>
              </ul>
            </section>
          )}
        </For>
      </div>
    </main>
  );
};

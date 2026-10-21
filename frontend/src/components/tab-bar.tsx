import { type Component, createSignal, For, Show } from "solid-js";

import { route } from "../lib/router";
import { Icon, type IconName } from "./icons";

// a field that brings up the keyboard has focus; the keyboard would push the
// bar up the screen, so it steps aside like it does in an app
const typesText = (element: EventTarget | null) =>
  element instanceof HTMLTextAreaElement ||
  element instanceof HTMLSelectElement ||
  (element instanceof HTMLInputElement &&
    !["range", "checkbox", "radio", "button", "submit", "file", "color"].includes(element.type));
const [typing, setTyping] = createSignal(typesText(document.activeElement));
document.addEventListener("focusin", (event) => setTyping(typesText(event.target)));
document.addEventListener("focusout", (event) => {
  if (!typesText(event.relatedTarget)) setTyping(false);
});

const TABS: {
  page: "home" | "routine" | "settings";
  href: string;
  label: string;
  icon: IconName;
}[] = [
  { page: "home", href: "#/", label: "Home", icon: "home" },
  { page: "routine", href: "#/routine", label: "Routine", icon: "routine" },
  { page: "settings", href: "#/settings", label: "Settings", icon: "settings" },
];

// the black pill at the bottom; mode pages hide it and show a back button instead
export const TabBar: Component = () => (
  <Show when={!typing()}>
    {/* scrolled content fades out under the bar instead of showing around it */}
    <div
      aria-hidden="true"
      class="pointer-events-none fixed inset-x-0 bottom-0 z-20 bg-linear-to-t from-ground from-60% to-transparent"
      style={{ height: "calc(var(--tab-bar-top) + 1.25rem)" }}
    />
    <nav
      aria-label="Main"
      class="fixed inset-x-0 z-30 mx-auto grid max-w-[440px] grid-cols-3 gap-1 rounded-full bg-inverse p-1.5 shadow-lg shadow-black/20"
      style={{
        bottom: "var(--tab-bar-bottom)",
        width: "calc(100% - 2.5rem)",
      }}
    >
      <For each={TABS}>
        {(tab) => (
          <a
            href={tab.href}
            aria-current={route().page === tab.page ? "page" : undefined}
            class={`flex h-12 items-center justify-center gap-2 rounded-full text-[15px] font-semibold no-underline transition ${
              route().page === tab.page ? "bg-ground text-ink" : "text-on-inverse"
            }`}
          >
            <Icon name={tab.icon} size={18} />
            {tab.label}
          </a>
        )}
      </For>
    </nav>
  </Show>
);

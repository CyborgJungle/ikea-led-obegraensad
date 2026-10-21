import { type Component, For } from "solid-js";

import { route } from "../lib/router";
import { Icon, type IconName } from "./icons";

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
  <nav
    aria-label="Main"
    class="fixed inset-x-0 z-30 mx-auto grid max-w-[440px] grid-cols-3 gap-1 rounded-full bg-inverse p-1.5 shadow-lg shadow-black/20"
    style={{
      bottom: "max(1rem, env(safe-area-inset-bottom))",
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
);

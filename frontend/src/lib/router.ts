import { createSignal } from "solid-js";

// Hash routes, so the lamp only has to serve one page:
// #/  #/modes  #/mode  #/mode/<id>  #/routine  #/settings
// #/mode is the Mode tab: the mode the lamp shows now
export type Route =
  | { page: "home" }
  | { page: "modes" }
  | { page: "mode"; id?: number }
  | { page: "routine" }
  | { page: "settings" };

const parse = (hash: string): Route => {
  const path = hash.replace(/^#/, "") || "/";
  const mode = path.match(/^\/mode(?:\/(\d+))?$/);
  if (mode) return { page: "mode", id: mode[1] ? Number(mode[1]) : undefined };
  if (path === "/modes") return { page: "modes" };
  if (path === "/routine") return { page: "routine" };
  if (path === "/settings") return { page: "settings" };
  return { page: "home" };
};

const [route, setRoute] = createSignal<Route>(parse(window.location.hash));
window.addEventListener("hashchange", () => setRoute(parse(window.location.hash)));

export { route };

// Every move replaces the current history entry instead of adding one: the
// history stays one page long, so the edge swipe of a home screen app has
// nothing to go back to. The app has its own back buttons.
export const navigate = (path: string) => window.location.replace(`#${path}`);

// links within the app (href="#/...") replace too
document.addEventListener("click", (event) => {
  const link = (event.target as Element | null)?.closest?.('a[href^="#"]');
  if (!link || event.defaultPrevented || event.metaKey || event.ctrlKey || event.shiftKey) return;
  event.preventDefault();
  navigate((link.getAttribute("href") ?? "#/").slice(1));
});

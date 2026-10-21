import { createSignal } from "solid-js";

// Hash routes, so the lamp only has to serve one page:
// #/  #/modes  #/mode/<id>  #/routine  #/settings
export type Route =
  | { page: "home" }
  | { page: "modes" }
  | { page: "mode"; id: number }
  | { page: "routine" }
  | { page: "settings" };

const parse = (hash: string): Route => {
  const path = hash.replace(/^#/, "") || "/";
  const mode = path.match(/^\/mode\/(\d+)$/);
  if (mode) return { page: "mode", id: Number(mode[1]) };
  if (path === "/modes") return { page: "modes" };
  if (path === "/routine") return { page: "routine" };
  if (path === "/settings") return { page: "settings" };
  return { page: "home" };
};

const [route, setRoute] = createSignal<Route>(parse(window.location.hash));
window.addEventListener("hashchange", () => setRoute(parse(window.location.hash)));

export { route };

// replace: no history entry, so Back skips it (picker → mode page → home)
export const navigate = (path: string, replace = false) => {
  if (replace) {
    window.location.replace(`#${path}`);
  } else {
    window.location.hash = path;
  }
};

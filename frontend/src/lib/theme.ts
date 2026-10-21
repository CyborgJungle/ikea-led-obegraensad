import { createSignal } from "solid-js";

// "auto" follows the phone's light/dark setting
export type Appearance = "auto" | "light" | "dark";

const STORAGE_KEY = "appearance";
const media = window.matchMedia("(prefers-color-scheme: dark)");

const read = (): Appearance => {
  try {
    const stored = localStorage.getItem(STORAGE_KEY);
    return stored === "light" || stored === "dark" ? stored : "auto";
  } catch {
    return "auto";
  }
};

const [appearance, setAppearanceSignal] = createSignal<Appearance>(read());

const apply = () => {
  const dark = appearance() === "dark" || (appearance() === "auto" && media.matches);
  document.documentElement.dataset.theme = dark ? "dark" : "light";
  // the status bar of the home screen app follows the page colour
  document
    .querySelector('meta[name="theme-color"]')
    ?.setAttribute("content", dark ? "#121211" : "#ece8e1");
};

media.addEventListener("change", apply);
apply();

export { appearance };

export const setAppearance = (value: Appearance) => {
  setAppearanceSignal(value);
  try {
    localStorage.setItem(STORAGE_KEY, value);
  } catch {
    // only kept for this visit
  }
  apply();
};

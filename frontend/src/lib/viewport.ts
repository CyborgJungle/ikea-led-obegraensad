import { createSignal } from "solid-js";

// iOS can give a home screen web app a layout viewport shorter than the
// screen: on an iPhone 13 the start-up screen (fixed, inset 0) ended 59 pt
// above the bottom edge over a bare strip, and the tab bar floated that much
// too high. It comes and goes, so it is measured instead of assumed: the gap
// goes into --viewport-gap, pages grow by it and the parts anchored to the
// bottom move down by it (index.css, tab-bar.tsx, splash.tsx).

export interface ViewportInfo {
  standalone: boolean;
  viewport: number; // window.innerHeight
  screen: number; // screen height in this orientation
  gap: number;
  safeTop: number;
  safeBottom: number;
}

// the home bar inset of Face ID iPhones in portrait; iOS reports none when
// the viewport stops above the home bar, but the home indicator is still there
const HOME_BAR_INSET = 34;

// iOS only: an installed app on a computer runs in a window smaller than the
// screen, which is not this gap
const standalone = (navigator as Navigator & { standalone?: boolean }).standalone === true;
// larger than any status bar plus home bar: something else, leave it alone
const MAX_GAP = 120;

// reads env() through a hidden element, JavaScript has no other way to them
const probe = document.createElement("div");
probe.setAttribute("aria-hidden", "true");
probe.style.cssText =
  "position:fixed;top:0;left:0;visibility:hidden;pointer-events:none;" +
  "padding-top:env(safe-area-inset-top,0px);padding-bottom:env(safe-area-inset-bottom,0px)";
document.body.append(probe);

const [viewportInfo, setViewportInfo] = createSignal<ViewportInfo>({
  standalone,
  viewport: window.innerHeight,
  screen: window.screen.height,
  gap: 0,
  safeTop: 0,
  safeBottom: 0,
});

const measure = () => {
  const root = document.documentElement.style;
  const portrait = window.matchMedia("(orientation: portrait)").matches;
  const { width, height } = window.screen;
  const screenHeight = portrait ? Math.max(width, height) : Math.min(width, height);
  const probeStyle = getComputedStyle(probe);
  const safeTop = parseFloat(probeStyle.paddingTop) || 0;
  const safeBottom = parseFloat(probeStyle.paddingBottom) || 0;

  // only a home screen app fills the screen, a browser tab has its own bars
  const short = Math.round(screenHeight - window.innerHeight);
  const gap = standalone && short > 1 && short <= MAX_GAP ? short : 0;

  root.setProperty("--viewport-gap", `${gap}px`);
  if (gap > 0 && safeBottom === 0) {
    root.setProperty("--safe-bottom", `${HOME_BAR_INSET}px`);
  } else {
    root.removeProperty("--safe-bottom");
  }
  setViewportInfo({
    standalone,
    viewport: window.innerHeight,
    screen: screenHeight,
    gap,
    safeTop,
    safeBottom,
  });
};

measure();
window.addEventListener("resize", measure);
window.addEventListener("orientationchange", () => setTimeout(measure, 300));
// iOS settles the viewport of a home screen app a moment after launch
window.addEventListener("load", measure);
setTimeout(measure, 500);

export { viewportInfo };

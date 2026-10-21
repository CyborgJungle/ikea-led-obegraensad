import type { Component, JSX } from "solid-js";

// Stroke icons drawn inline: the lamp has no internet connection to fetch an icon font
const PATHS = {
  back: "M15 5l-7 7 7 7",
  chevronDown: "M6 9l6 6 6-6",
  chevronRight: "M9 5l7 7-7 7",
  close: "M6 6l12 12M18 6L6 18",
  plus: "M12 5v14M5 12h14",
  check: "M5 12.5l4.5 4.5L19 7",
  play: "M8 5.5v13l11-6.5z",
  stop: "M7 7h10v10H7z",
  pause: "M8 5v14M16 5v14",
  rotateLeft: "M4 12a8 8 0 1 0 2.5-5.8M4 4v5h5",
  rotateRight: "M20 12a8 8 0 1 1-2.5-5.8M20 4v5h-5",
  star: "M12 4l2.4 5 5.6.8-4 3.9 1 5.5L12 16.6 7 19.2l1-5.5-4-3.9 5.6-.8z",
  trash: "M5 7h14M10 11v6M14 11v6M7 7l1 12h8l1-12M9 7V4h6v3",
  undo: "M9 14L4 9l5-5M4 9h10a6 6 0 0 1 0 12h-3",
  redo: "M15 14l5-5-5-5M20 9H10a6 6 0 0 0 0 12h3",
  copy: "M9 9h10v10H9zM5 15V5h10",
  eraser: "M6 18h12M8 14l7-7 4 4-7 7H8l-3-3z",
  image: "M4 5h16v14H4zM4 15l4-4 4 4 3-3 5 5M15 9.5a.5.5 0 1 0 0-1",
  save: "M5 5h11l3 3v11H5zM8 5v5h7V5M8 19v-5h8v5",
  folder: "M4 7h6l2 2h8v10H4z",
  upload: "M12 16V5M7 10l5-5 5 5M5 19h14",
  download: "M12 5v11M7 11l5 5 5-5M5 19h14",
  camera: "M4 8h3l2-3h6l2 3h3v11H4zM12 17a4 4 0 1 0 0-8 4 4 0 0 0 0 8",
  arrowLeft: "M19 12H5M11 6l-6 6 6 6",
  arrowRight: "M5 12h14M13 6l6 6-6 6",
  arrowUp: "M12 19V5M6 11l6-6 6 6",
  arrowDown: "M12 5v14M6 13l6 6 6-6",
  external: "M7 17L17 7M9 7h8v8",
  home: "M4 11l8-7 8 7v9h-5v-6H9v6H4z",
  routine: "M4 12a8 8 0 0 1 14-5.3M20 12a8 8 0 0 1-14 5.3M18 3v4h-4M6 21v-4h4",
  settings: "M4 7h10M18 7h2M4 17h2M10 17h10M16 4v6M8 14v6",
  sun: "M12 7a5 5 0 1 0 0 10 5 5 0 0 0 0-10M12 2v2M12 20v2M2 12h2M20 12h2M5 5l1.5 1.5M17.5 17.5L19 19M5 19l1.5-1.5M17.5 6.5L19 5",
  wifi: "M3 9a13 13 0 0 1 18 0M6 12.5a8.5 8.5 0 0 1 12 0M9 16a4 4 0 0 1 6 0M12 19.5h.01",
} as const;

export type IconName = keyof typeof PATHS;

export const Icon: Component<{ name: IconName; size?: number; class?: string }> = (props) => (
  <svg
    width={props.size ?? 20}
    height={props.size ?? 20}
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    stroke-width="2.2"
    stroke-linecap="round"
    stroke-linejoin="round"
    class={props.class}
    aria-hidden="true"
  >
    <path d={PATHS[props.name]} />
  </svg>
);

// for icons that sit inside a disc button and need a label for screen readers
export const IconButton: Component<{
  name: IconName;
  label: string;
  onClick?: JSX.EventHandler<HTMLButtonElement, MouseEvent>;
  class?: string;
  disabled?: boolean;
  pressed?: boolean;
}> = (props) => (
  <button
    type="button"
    class={`disc ${props.class ?? ""}`}
    aria-label={props.label}
    aria-pressed={props.pressed}
    disabled={props.disabled}
    onClick={props.onClick}
  >
    <Icon name={props.name} />
  </button>
);

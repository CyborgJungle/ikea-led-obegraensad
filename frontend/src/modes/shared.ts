// what every mode panel gets from the mode page
export interface PanelProps {
  plugin: { id: number; name: string };
  // the lamp shows this mode right now
  active: boolean;
  // switch the lamp to this mode
  activate: () => void;
}

// limits of Config::SCROLL_SPEED_MIN/MAX on the device, in ms per pixel
export const SPEED_MIN = 20;
export const SPEED_MAX = 250;

// sliders show "faster" to the right, the device stores a delay
export const sliderToDelay = (value: number) => SPEED_MIN + SPEED_MAX - value;

// the lamp fonts have no ellipsis glyph; quotes and dashes it maps itself
export const normalizeText = (text: string) => text.replace(/…/g, "...");

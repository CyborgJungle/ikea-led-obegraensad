// How the app presents the lamp's plugins: a readable name, a group for the
// mode picker and a short line. Plugins missing here still show up, under "More".

export type ModeGroup = "create" | "time" | "ambient" | "games" | "connect" | "more";

export const GROUPS: { id: ModeGroup; label: string }[] = [
  { id: "create", label: "Make your own" },
  { id: "time", label: "Time & weather" },
  { id: "ambient", label: "Ambient" },
  { id: "games", label: "Games" },
  { id: "connect", label: "Connect" },
  { id: "more", label: "More" },
];

const INFO: Record<string, { label?: string; group: ModeGroup; note: string }> = {
  Draw: { group: "create", note: "Draw with your finger" },
  Text: { group: "create", note: "Scrolling text" },
  Board: { group: "create", note: "Menu board & reader" },
  Animation: { group: "create", note: "Frame by frame" },
  Camera: { group: "create", note: "Live from your phone" },
  Countdown: { label: "Timer", group: "create", note: "Countdown" },
  BigClock: { label: "Big Clock", group: "time", note: "Large digits" },
  Clock: { group: "time", note: "Hours and minutes" },
  PongClock: { label: "Pong Clock", group: "time", note: "Pong keeps the time" },
  TickingClock: { label: "Ticking", group: "time", note: "Clock with seconds" },
  Weather: { group: "time", note: "Now, from wttr.in" },
  Stars: { group: "ambient", note: "Twinkling" },
  Lines: { group: "ambient", note: "Sweeping lines" },
  Circle: { group: "ambient", note: "Growing circles" },
  Rain: { group: "ambient", note: "Falling drops" },
  MatrixRain: { label: "Matrix", group: "ambient", note: "Digital rain" },
  Firework: { group: "ambient", note: "Bursts" },
  Blobs: { group: "ambient", note: "Morphing blobs" },
  Spiral: { group: "ambient", note: "Spinning" },
  Wave: { group: "ambient", note: "Sine wave" },
  Checkerboard: { label: "Checker", group: "ambient", note: "Pattern" },
  Radar: { group: "ambient", note: "Sweep" },
  Bubbles: { group: "ambient", note: "Rising" },
  Comet: { group: "ambient", note: "Trails" },
  Fireflies: { group: "ambient", note: "Drifting lights" },
  MeteorShower: { label: "Meteors", group: "ambient", note: "Shower" },
  Scanlines: { group: "ambient", note: "Retro screen" },
  SparkleField: { label: "Sparkle", group: "ambient", note: "Glitter" },
  WaveBars: { label: "Bars", group: "ambient", note: "Equaliser" },
  Spotlight: { group: "ambient", note: "Roaming light" },
  Face: { group: "ambient", note: "Expressions" },
  FallingSand: { label: "Sand", group: "ambient", note: "Falling sand" },
  BouncingBall: { label: "Ball", group: "ambient", note: "Bouncing" },
  AutoWalker: { label: "Walker", group: "ambient", note: "Wandering" },
  GameOfLife: { label: "Life", group: "ambient", note: "Game of Life" },
  Breakout: { group: "games", note: "Plays itself" },
  Snake: { group: "games", note: "Plays itself" },
  Tetris: { group: "games", note: "Plays itself" },
  FlappyBird: { label: "Flappy", group: "games", note: "Plays itself" },
  SpaceInvaders: { label: "Invaders", group: "games", note: "Plays itself" },
  BigPong: { label: "Pong", group: "games", note: "Plays itself" },
  Tron: { group: "games", note: "Light cycles" },
  DDP: { group: "connect", note: "Stream pixels, port 4048" },
  ArtNet: { label: "Art-Net", group: "connect", note: "Lighting desks, port 6454" },
};

// the lamp names some plugins with spaces ("Meteor Shower", "Big Clock"),
// others without ("GameOfLife"); the tables here use the name without spaces
export const modeKey = (name: string) => name.replace(/\s+/g, "");

// "MeteorShower" → "Meteor Shower" for plugins without a label above
const split = (name: string) => name.replace(/([a-z])([A-Z])/g, "$1 $2");

export const modeLabel = (name: string) => INFO[modeKey(name)]?.label ?? split(name);
export const modeNote = (name: string) => INFO[modeKey(name)]?.note ?? "";
export const modeGroup = (name: string): ModeGroup => INFO[modeKey(name)]?.group ?? "more";

// REST calls to the lamp. The websocket (contexts/store.tsx) carries the live
// state; these cover configuration, device info and the plugin schedule.

export const API_URL = import.meta.env.PROD
  ? `http://${window.location.host}/`
  : import.meta.env.VITE_BASE_URL;

export interface DeviceConfig {
  weatherLocation: string;
  ntpServer: string;
  tzInfo: string;
  autoStartSchedule: boolean;
  scrollText: string;
  scrollSpeed: number;
  scrollSize: number;
  boardText: string;
  boardSpeed: number;
  // per mode id, percent of its normal pace; only modes not at 100 are listed,
  // missing on firmware from before it
  modeSpeed?: Record<string, number>;
  countdownSeconds: number;
  scheduleHoursEnabled: boolean;
  scheduleFrom: string;
  scheduleTo: string;
}

export interface DeviceInfo {
  ipAddress: string;
  macAddress: string;
  rssi: number;
  uptime: number;
  freeHeap: number;
  minFreeHeap?: number;
  maxAllocHeap?: number;
  resetReason: string;
}

const request = async (path: string, init?: RequestInit) => {
  const response = await fetch(`${API_URL}${path}`, init);
  if (!response.ok) {
    throw new Error(`${path}: HTTP ${response.status}`);
  }
  return response;
};

export const getConfig = async (): Promise<DeviceConfig> => (await request("api/config")).json();

// partial updates: the lamp only changes the keys it is sent
export const saveConfig = (changes: Partial<DeviceConfig> & Record<string, unknown>) =>
  request("api/config", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(changes),
  });

export const resetConfig = () => request("api/config/reset", { method: "POST" });

export const getInfo = async (): Promise<DeviceInfo> => (await request("api/info")).json();

export const startSchedule = (items: { pluginId: number; duration: number }[]) =>
  request("api/schedule", {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: `schedule=${encodeURIComponent(JSON.stringify(items))}`,
  });

export const stopSchedule = () => request("api/schedule/stop");

export const clearSchedule = () => request("api/schedule/clear");

// the HTTPS camera page and the certificate phones install once to trust it
// the camera page runs on another address and cannot read this app's
// settings, so it is told the light/dark choice and the mode number
export const cameraUrl = (modeId: number) =>
  `https://${window.location.hostname}/camera?mode=${modeId}&theme=${document.documentElement.dataset.theme ?? ""}`;
export const certificateUrl = () => `http://${window.location.hostname}/lamp-ca.cer`;

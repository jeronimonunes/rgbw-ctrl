import {DeviceState} from "./model";

const BASE_URL = "";
const GET_STATE_URL = `${BASE_URL}/state`;
const SET_BRIGHTNESS_URL = `${BASE_URL}/output/brightness`;
const SET_COLOR_URL = `${BASE_URL}/output/color`;
const SET_BLUETOOTH_STATE_URL = `${BASE_URL}/bluetooth`;
const SYSTEM_RESTART_URL = `${BASE_URL}/system/restart`;
const SYSTEM_RESET_URL = `${BASE_URL}/system/reset`;

export async function getState(): Promise<DeviceState> {
  const response = await fetch(GET_STATE_URL);
  if (!response.ok) throw new Error("Failed to fetch device state");
  return response.json();
}

export async function setBrightness(value: number): Promise<void> {
  if (value < 0 || value > 255) {
    throw new Error("Brightness value must be between 0 and 255");
  }
  const response = await fetch(`${SET_BRIGHTNESS_URL}?value=${value}`);
  if (!response.ok) throw new Error("Failed to set brightness");
}

export async function setColor({r, g, b, w}: {
  r?: number;
  g?: number;
  b?: number;
  w?: number;
}): Promise<void> {
  const params = new URLSearchParams();
  if (r !== undefined) params.append("r", r.toString());
  if (g !== undefined) params.append("g", g.toString());
  if (b !== undefined) params.append("b", b.toString());
  if (w !== undefined) params.append("w", w.toString());
  const response = await fetch(`${SET_COLOR_URL}?${params.toString()}`);
  if (!response.ok) throw new Error("Failed to set color");
}

export async function setBluetoothState(enabled: boolean): Promise<void> {
  const state = enabled ? "on" : "off";
  const response = await fetch(`${SET_BLUETOOTH_STATE_URL}?state=${state}`);
  if (!response.ok) throw new Error("Failed to change Bluetooth state");
}

export async function restartSystem(): Promise<void> {
  const response = await fetch(SYSTEM_RESTART_URL);
  if (!response.ok) throw new Error("Failed to restart device");
}

export async function resetSystem(): Promise<void> {
  const response = await fetch(SYSTEM_RESET_URL);
  if (!response.ok) throw new Error("Failed to reset device");
}

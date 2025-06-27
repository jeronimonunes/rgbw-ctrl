export const ESP_NOW_DEVICE_NAME_MAX_LENGTH = 23;
export const ESP_NOW_DEVICE_NAME_TOTAL_LENGTH = ESP_NOW_DEVICE_NAME_MAX_LENGTH + 1; // +1 for null terminator
export const ESP_NOW_DEVICE_MAC_LENGTH = 6;
export const ESP_NOW_DEVICE_LENGTH = ESP_NOW_DEVICE_NAME_TOTAL_LENGTH + ESP_NOW_DEVICE_MAC_LENGTH;
export const ESP_NOW_MAX_DEVICES_PER_MESSAGE = 15;

export interface EspNowDevice {
  name: string;
  address: string
}

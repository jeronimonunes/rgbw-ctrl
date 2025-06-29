export enum AlexaIntegrationMode {
  OFF = 0,
  RGBW_DEVICE = 1,
  RGB_DEVICE = 2,
  MULTI_DEVICE = 3
}

export interface AlexaIntegrationSettings {
  integrationMode: AlexaIntegrationMode;
  rDeviceName: string;
  gDeviceName: string;
  bDeviceName: string;
  wDeviceName: string;
}

export const ALEXA_MAX_DEVICE_NAME_LENGTH = 32;
export const ALEXA_SETTINGS_TOTAL_LENGTH = 1 + (4 * ALEXA_MAX_DEVICE_NAME_LENGTH);

export function alexaModeToString(mode: AlexaIntegrationMode): string {
  switch (mode) {
    case AlexaIntegrationMode.OFF:
      return 'Off';
    case AlexaIntegrationMode.RGBW_DEVICE:
      return 'RGBW Device';
    case AlexaIntegrationMode.RGB_DEVICE:
      return 'RGB Device';
    case AlexaIntegrationMode.MULTI_DEVICE:
      return 'Multi Device';
    default:
      return 'Unknown';
  }
}

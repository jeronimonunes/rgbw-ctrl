import {LightState, LIGHT_STATE_BYTE_SIZE} from './light.model';

export interface OutputState {

  values: [LightState, LightState, LightState, LightState];

}

export const OUTPUT_STATE_BYTE_SIZE = LIGHT_STATE_BYTE_SIZE * 4;

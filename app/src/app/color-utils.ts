const GAMMA = 2.2;

/**
 * Maps a linear brightness value [0–255] to a perceptually correct PWM value [0–255].
 */
export function perceptualMap(linearValue: number): number {
  const clamped = Math.min(Math.max(linearValue, 0), 255);
  const normalized = clamped / 255;
  const perceptual = Math.pow(normalized, GAMMA);
  return Math.round(perceptual * 255);
}

/**
 * Inverse of perceptualMap — maps perceptual PWM [0–255] to linear brightness [0–255].
 */
export function inversePerceptualMap(pwmValue: number): number {
  const clamped = Math.min(Math.max(pwmValue, 0), 255);
  const normalized = clamped / 255;
  const linear = Math.pow(normalized, 1 / GAMMA);
  return Math.round(linear * 255);
}

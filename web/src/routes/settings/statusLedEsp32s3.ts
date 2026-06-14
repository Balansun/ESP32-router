import type { StatusLedMode } from "../../api/types";

export function isEsp32s3Chip(chipModel: string | undefined): boolean {
  return Boolean(chipModel && /s3/i.test(chipModel));
}

/** WROOM factory preset still stored on many S3 boards; firmware maps this to onboard RGB. */
export function isLegacyWroomDualGpioPreset(
  mode: StatusLedMode | undefined,
  gpioActivity: number | undefined,
  gpioRegulation: number | undefined,
): boolean {
  return (
    (mode ?? "dual_gpio") === "dual_gpio" &&
    (gpioActivity ?? 18) === 18 &&
    (gpioRegulation ?? 19) === 19
  );
}

export function resolveStatusLedModeForEsp32s3(
  mode: StatusLedMode,
  gpioActivity: number,
  gpioRegulation: number,
  esp32s3: boolean,
): StatusLedMode {
  if (!esp32s3) return mode;
  if (isLegacyWroomDualGpioPreset(mode, gpioActivity, gpioRegulation)) {
    return "rgb_ws2812";
  }
  return mode;
}

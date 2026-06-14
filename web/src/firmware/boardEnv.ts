export type BoardEnv = "wroom32" | "esp32s3";

/** Map ESP chip model string to PlatformIO release artifact env. */
export function pickBoardEnv(chipModel: string | undefined): BoardEnv {
  if (chipModel && /s3/i.test(chipModel)) return "esp32s3";
  return "wroom32";
}

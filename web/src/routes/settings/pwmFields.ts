import type { AppStrings } from "../../i18n";
import type { SelectOption } from "./formRows";

/** Matches firmware `balansun_pwm_logic_parse_mode`. */
export const PWM_MODES = ["off", "follow_triac", "independent"] as const;
export type PwmMode = (typeof PWM_MODES)[number];

/** Matches firmware `balansun_pwm_logic_is_allowed_gpio` plus -1 (disabled). */
export const PWM_GPIO_CHOICES = [-1, 4, 5, 14, 16, 17, 21, 25] as const;

export function normalizePwmMode(value: string | undefined): PwmMode {
  if (value && (PWM_MODES as readonly string[]).includes(value)) {
    return value as PwmMode;
  }
  return "off";
}

export function normalizePwmGpio(value: number | undefined): number {
  if (value !== undefined && (PWM_GPIO_CHOICES as readonly number[]).includes(value)) {
    return value;
  }
  return -1;
}

export function pwmModeOptions(T: AppStrings["settings"]): SelectOption[] {
  return [
    { value: "off", label: T.pwmModeOff },
    { value: "follow_triac", label: T.pwmModeFollowTriac },
    { value: "independent", label: T.pwmModeIndependent },
  ];
}

export function pwmGpioOptions(T: AppStrings["settings"]): SelectOption[] {
  return PWM_GPIO_CHOICES.map((gpio) => ({
    value: String(gpio),
    label: gpio < 0 ? T.pwmGpioOff : `GPIO ${gpio}`,
  }));
}

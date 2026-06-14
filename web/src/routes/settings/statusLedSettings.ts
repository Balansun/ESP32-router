import { h } from "../../utils/dom";
import { api } from "../../api/client";
import { toast } from "../../components/Toast";
import type { AppStrings } from "../../i18n/locales/en";
import type { RouterConfig, StatusLedColor, StatusLedMode, StatusLedTestRole } from "../../api/types";
import { numberRow } from "./formRows";
import { wrapSwitchWithHelp } from "../../components/FieldHelp";
import { settingsSection } from "./cards/section";
import {
  isLegacyWroomDualGpioPreset,
  resolveStatusLedModeForEsp32s3,
} from "./statusLedEsp32s3";

const H = { helpScope: "settings" as const };

function rgbToHex(rgb: StatusLedColor | undefined, fallback: string): string {
  if (!rgb) return fallback;
  const hex = (n: number) => n.toString(16).padStart(2, "0");
  return `#${hex(rgb[0])}${hex(rgb[1])}${hex(rgb[2])}`;
}

function hexToRgb(hex: string): StatusLedColor {
  const v = hex.replace("#", "").trim();
  if (v.length !== 6) return [0, 0, 0];
  const n = parseInt(v, 16);
  return [(n >> 16) & 255, (n >> 8) & 255, n & 255];
}

function wireColorPicker(picker: HTMLInputElement, onChange: (rgb: StatusLedColor) => void): void {
  const sync = () => onChange(hexToRgb(picker.value));
  picker.addEventListener("input", sync);
  picker.addEventListener("change", sync);
  sync();
}

export interface StatusLedPageOptions {
  esp32s3?: boolean;
  chipModel?: string;
}

export interface StatusLedPageHandle {
  pageRoot: HTMLElement;
  cards: {
    overview: HTMLElement;
    hardware: HTMLElement;
    colors: HTMLElement;
    preview: HTMLElement;
  };
  readMode(): StatusLedMode;
  collectPatch(): Partial<RouterConfig>;
  setDisabledByMode(): void;
}

export function buildStatusLedPage(
  cfg: RouterConfig,
  T: AppStrings,
  signal: AbortSignal,
  options: StatusLedPageOptions = {},
): StatusLedPageHandle {
  const esp32s3 = options.esp32s3 ?? false;
  const ext = cfg as RouterConfig & {
    status_led_mode?: StatusLedMode;
    status_led_gpio_activity?: number;
    status_led_gpio_regulation?: number;
    status_led_rgb_gpio?: number;
    status_led_active_low?: boolean;
    status_led_color_activity?: StatusLedColor;
    status_led_color_regulation?: StatusLedColor;
    status_led_color_reboot?: StatusLedColor;
    status_led_color_ap?: StatusLedColor;
  };

  const modeSelect = h("select", { class: "input" }) as HTMLSelectElement;
  for (const [value, label] of [
    ["off", T.settings.statusLedModeOff],
    ["dual_gpio", T.settings.statusLedModeDual],
    ["rgb_ws2812", T.settings.statusLedModeRgb],
  ] as const) {
    modeSelect.append(h("option", { value }, label));
  }
  let initialMode = ext.status_led_mode ?? "dual_gpio";
  if (
    esp32s3 &&
    isLegacyWroomDualGpioPreset(initialMode, ext.status_led_gpio_activity, ext.status_led_gpio_regulation)
  ) {
    initialMode = "rgb_ws2812";
  }
  modeSelect.value = initialMode;

  const gpioActivity = numberRow(
    "status_led_gpio_activity",
    T.settings.statusLedGpioActivity,
    ext.status_led_gpio_activity ?? 18,
    "-1 = off",
    undefined,
    { ...H, helpKey: "status_led_gpio_activity" },
  );
  const gpioRegulation = numberRow(
    "status_led_gpio_regulation",
    T.settings.statusLedGpioRegulation,
    ext.status_led_gpio_regulation ?? 19,
    "-1 = off",
    undefined,
    { ...H, helpKey: "status_led_gpio_regulation" },
  );
  const rgbGpio = numberRow(
    "status_led_rgb_gpio",
    T.settings.statusLedRgbGpio,
    ext.status_led_rgb_gpio ?? -1,
    T.settings.statusLedRgbGpioHint,
    undefined,
    { ...H, helpKey: "status_led_rgb_gpio" },
  );
  const activeLowInput = h("input", {
    type: "checkbox",
    checked: !!ext.status_led_active_low,
  }) as HTMLInputElement;
  const activeLowRow = wrapSwitchWithHelp(
    h("label", { class: "switch" }, activeLowInput, h("span", {}, T.settings.statusLedActiveLow)),
    "settings",
    "status_led_active_low",
  );

  const activityLabel = esp32s3 ? T.settings.statusLedColorActivityS3 : T.settings.statusLedColorActivity;
  const regulationLabel = esp32s3
    ? T.settings.statusLedColorRegulationS3
    : T.settings.statusLedColorRegulation;

  const colorActivityPicker = h("input", {
    type: "color",
    value: rgbToHex(ext.status_led_color_activity, "#ffb400"),
    class: "input input--color",
  }) as HTMLInputElement;
  const colorRegulationPicker = h("input", {
    type: "color",
    value: rgbToHex(ext.status_led_color_regulation, "#00ff00"),
    class: "input input--color",
  }) as HTMLInputElement;
  const colorRebootPicker = h("input", {
    type: "color",
    value: rgbToHex(ext.status_led_color_reboot, "#ff0000"),
    class: "input input--color",
  }) as HTMLInputElement;
  const colorApPicker = h("input", {
    type: "color",
    value: rgbToHex(ext.status_led_color_ap, "#6600ff"),
    class: "input input--color",
  }) as HTMLInputElement;

  const swatchActivity = h("span", {
    class: "status-led-swatch",
    style: `background:${colorActivityPicker.value}`,
  });
  const swatchRegulation = h("span", {
    class: "status-led-swatch",
    style: `background:${colorRegulationPicker.value}`,
  });
  const swatchReboot = h("span", {
    class: "status-led-swatch",
    style: `background:${colorRebootPicker.value}`,
  });
  const swatchAp = h("span", {
    class: "status-led-swatch",
    style: `background:${colorApPicker.value}`,
  });

  function updateSwatches(): void {
    swatchActivity.style.background = colorActivityPicker.value;
    swatchRegulation.style.background = colorRegulationPicker.value;
    swatchReboot.style.background = colorRebootPicker.value;
    swatchAp.style.background = colorApPicker.value;
  }

  wireColorPicker(colorActivityPicker, () => updateSwatches());
  wireColorPicker(colorRegulationPicker, () => updateSwatches());
  wireColorPicker(colorRebootPicker, () => updateSwatches());
  wireColorPicker(colorApPicker, () => updateSwatches());

  function readPickerColors(): {
    activity: StatusLedColor;
    regulation: StatusLedColor;
    reboot: StatusLedColor;
    ap: StatusLedColor;
  } {
    return {
      activity: hexToRgb(colorActivityPicker.value),
      regulation: hexToRgb(colorRegulationPicker.value),
      reboot: hexToRgb(colorRebootPicker.value),
      ap: hexToRgb(colorApPicker.value),
    };
  }

  const dualRows = h(
    "div",
    { class: "status-led-dual" },
    gpioActivity.el,
    gpioRegulation.el,
    h("div", { class: "field" }, activeLowRow),
  );
  const rgbRows = h("div", { class: "status-led-rgb" }, rgbGpio.el);

  const colorsDualHint = h("p", {
    class: "field__hint status-led-colors-dual-hint",
    hidden: true,
  }, T.settings.statusLedAdvancedHint);

  const colorsS3Hint = esp32s3
    ? h("p", { class: "field__hint" }, T.settings.statusLedS3ColorsHint)
    : null;

  const colorGrid = h(
    "div",
    { class: "status-led-color-grid" },
    h(
      "div",
      { class: "field status-led-color-grid__cell" },
      h("label", {}, activityLabel),
      h("div", { class: "status-led-color-grid__row" }, colorActivityPicker, swatchActivity),
    ),
    h(
      "div",
      { class: "field status-led-color-grid__cell" },
      h("label", {}, regulationLabel),
      h("div", { class: "status-led-color-grid__row" }, colorRegulationPicker, swatchRegulation),
    ),
    h(
      "div",
      { class: "field status-led-color-grid__cell" },
      wrapSwitchWithHelp(h("label", {}, T.settings.statusLedColorReboot), "settings", "status_led_color_reboot"),
      h("div", { class: "status-led-color-grid__row" }, colorRebootPicker, swatchReboot),
    ),
    h(
      "div",
      { class: "field status-led-color-grid__cell" },
      wrapSwitchWithHelp(h("label", {}, T.settings.statusLedColorAp), "settings", "status_led_color_ap"),
      h("div", { class: "status-led-color-grid__row" }, colorApPicker, swatchAp),
    ),
  );

  const testActivityBtn = h("button", { type: "button", class: "btn btn--secondary" }, T.settings.statusLedTestActivity);
  const testRegulationBtn = h(
    "button",
    { type: "button", class: "btn btn--secondary" },
    T.settings.statusLedTestRegulation,
  );
  const testBothBtn = h("button", { type: "button", class: "btn btn--secondary" }, T.settings.statusLedTestBoth);
  const testRebootBtn = h("button", { type: "button", class: "btn btn--secondary" }, T.settings.statusLedTestReboot);
  const testApBtn = h("button", { type: "button", class: "btn btn--secondary" }, T.settings.statusLedTestAp);
  const testHint = h("p", { class: "field__hint" }, T.settings.statusLedTestHint);
  const testButtons = [testActivityBtn, testRegulationBtn, testBothBtn, testRebootBtn, testApBtn];

  const overviewParts: HTMLElement[] = [
    h("p", { class: "field__hint" }, T.settings.ledPageLead),
  ];
  if (esp32s3) {
    overviewParts.push(
      h("p", { class: "status-led-chip-badge" }, T.settings.ledChipBadgeS3),
    );
  } else if (options.chipModel?.trim()) {
    overviewParts.push(h("p", { class: "field__hint" }, options.chipModel.trim()));
  }
  overviewParts.push(
    h("h3", { class: "field__hint" }, T.settings.ledBehaviorTitle),
    h(
      "ul",
      { class: "status-led-behavior-list field__hint" },
      h("li", {}, T.settings.ledBehaviorFlash),
      h("li", {}, T.settings.ledBehaviorPriority),
      h("li", {}, T.settings.ledBehaviorOff),
    ),
  );

  const overviewCard = settingsSection(T.settings.ledCardOverview, ...overviewParts);

  const hardwareCard = settingsSection(
    T.settings.ledCardHardware,
    h("div", { class: "field" }, wrapSwitchWithHelp(h("label", {}, T.settings.statusLedMode), "settings", "status_led_mode"), modeSelect),
    dualRows,
    rgbRows,
  );

  const colorsCardParts: HTMLElement[] = [colorGrid];
  if (colorsS3Hint) colorsCardParts.push(colorsS3Hint);
  colorsCardParts.push(colorsDualHint);
  const colorsCard = settingsSection(T.settings.ledCardColors, ...colorsCardParts);

  const previewCard = settingsSection(
    T.settings.ledCardPreview,
    h(
      "div",
      { class: "field status-led-test-row" },
      h("div", { class: "btn-row" }, ...testButtons),
      testHint,
    ),
  );

  function setDisabledByMode(): void {
    const mode = modeSelect.value as StatusLedMode;
    const off = mode === "off";
    dualRows.hidden = esp32s3 || mode !== "dual_gpio";
    rgbRows.hidden = mode !== "rgb_ws2812" && !esp32s3;
    colorsDualHint.hidden = esp32s3 || mode === "rgb_ws2812" || mode === "off";
    for (const btn of testButtons) {
      btn.disabled = off;
    }
  }
  modeSelect.addEventListener("change", setDisabledByMode);
  setDisabledByMode();

  async function runTest(role: StatusLedTestRole): Promise<void> {
    if (modeSelect.value === "off") {
      toast(T.settings.statusLedTestOff, "error");
      return;
    }
    const body = buildTestRequest(role, collectPatch());
    for (const btn of testButtons) {
      btn.disabled = true;
    }
    try {
      await api.postStatusLedTest(body, { signal });
      toast(T.settings.statusLedTestOk.replace("{seconds}", "5"), "success");
    } catch (e) {
      const msg = e instanceof Error ? e.message : String(e);
      toast(T.settings.statusLedTestFailed.replace("{message}", msg), "error");
    } finally {
      setDisabledByMode();
    }
  }

  testActivityBtn.addEventListener("click", () => void runTest("activity"));
  testRegulationBtn.addEventListener("click", () => void runTest("regulation"));
  testBothBtn.addEventListener("click", () => void runTest("both"));
  testRebootBtn.addEventListener("click", () => void runTest("reboot"));
  testApBtn.addEventListener("click", () => void runTest("ap"));

  function readMode(): StatusLedMode {
    return modeSelect.value as StatusLedMode;
  }

  function effectiveMode(): StatusLedMode {
    const gpioA = parseInt(gpioActivity.ref.read(), 10);
    const gpioR = parseInt(gpioRegulation.ref.read(), 10);
    return resolveStatusLedModeForEsp32s3(readMode(), gpioA, gpioR, esp32s3);
  }

  function buildTestRequest(role: StatusLedTestRole, patch: Partial<RouterConfig>) {
    const colors = readPickerColors();
    const mode = effectiveMode();
    const body: import("../../api/types").StatusLedTestRequest = {
      role,
      duration_ms: 5000,
      ...patch,
      status_led_color_reboot: colors.reboot,
      status_led_color_ap: colors.ap,
    };
    if (esp32s3 || mode === "rgb_ws2812") {
      body.status_led_mode = "rgb_ws2812";
      body.status_led_color_activity = colors.activity;
      body.status_led_color_regulation = colors.regulation;
      body.status_led_rgb_gpio = parseInt(rgbGpio.ref.read(), 10);
    }
    return body;
  }

  function collectPatch(): Partial<RouterConfig> {
    const colors = readPickerColors();
    const gpioA = parseInt(gpioActivity.ref.read(), 10);
    const gpioR = parseInt(gpioRegulation.ref.read(), 10);
    const mode = resolveStatusLedModeForEsp32s3(readMode(), gpioA, gpioR, esp32s3);
    const patch: Partial<RouterConfig> = {
      status_led_mode: mode,
      status_led_gpio_activity: gpioA,
      status_led_gpio_regulation: gpioR,
      status_led_rgb_gpio: parseInt(rgbGpio.ref.read(), 10),
      status_led_active_low: activeLowInput.checked,
      status_led_color_reboot: colors.reboot,
      status_led_color_ap: colors.ap,
    };
    if (mode === "rgb_ws2812" || esp32s3) {
      patch.status_led_color_activity = colors.activity;
      patch.status_led_color_regulation = colors.regulation;
    }
    if (Number.isNaN(patch.status_led_gpio_activity!)) patch.status_led_gpio_activity = -1;
    if (Number.isNaN(patch.status_led_gpio_regulation!)) patch.status_led_gpio_regulation = -1;
    if (Number.isNaN(patch.status_led_rgb_gpio!)) patch.status_led_rgb_gpio = -1;
    return patch;
  }

  const pageRoot = h(
    "div",
    { class: "status-led-page" },
    overviewCard,
    hardwareCard,
    colorsCard,
    previewCard,
  );

  return {
    pageRoot,
    cards: {
      overview: overviewCard,
      hardware: hardwareCard,
      colors: colorsCard,
      preview: previewCard,
    },
    readMode,
    collectPatch,
    setDisabledByMode,
  };
}

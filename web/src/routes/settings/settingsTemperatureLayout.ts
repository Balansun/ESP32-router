import {
  fetchLiveTemperatureTelemetry,
  measurementsWithTemperatureTelemetry,
} from "../../api/temperatureTelemetry";
import type { Measurements, RouterConfig, TemperatureSlotConfig } from "../../api/types";
import { settingsSection } from "./cards/section";
import { textRow } from "./formRows";
import { attachSettingsCardAutosave, type SettingsCardAutosave } from "./cardAutosave";
import { getStrings } from "../../i18n";
import { h } from "../../utils/dom";
import { fmtTempC } from "../../utils/format";
import { pinoutSectionUrl } from "../../fieldHelp/docUrl";
import { localePref } from "../../state/store";
type SettingsCardAutosaveOpts = Omit<
  Parameters<typeof attachSettingsCardAutosave>[0],
  "card" | "collect"
>;

const H = { helpScope: "settings" as const };

const TEMP_GPIO_OPTIONS = [13, 21, 27, 33] as const;

export interface TemperatureSettingsSection {
  root: HTMLElement;
  collectPatch: () => Partial<
    Pick<RouterConfig, "temp_gpio" | "temperature_slots" | "temperature_label">
  >;
  refreshLive: (m: Measurements | undefined, telemetryReady?: boolean) => void;
  attachAutosave: (
    opts: SettingsCardAutosaveOpts,
    onSaved: (patch: Partial<RouterConfig>) => void,
  ) => SettingsCardAutosave;
}

function defaultSlots(cfg: RouterConfig): TemperatureSlotConfig[] {
  const fromApi = cfg.temperature_slots;
  if (fromApi && fromApi.length >= 2) return fromApi.slice(0, 2);
  return [
    { enabled: true, label: cfg.temperature_label || "temperature_c", address: "" },
    { enabled: false, label: "", address: "" },
  ];
}

export function buildTemperatureSettingsSection(cfg: RouterConfig): TemperatureSettingsSection {
  const T = getStrings();
  const slots = defaultSlots(cfg);

  const intro = h("p", { class: "card__sub" }, T.settings.temperature.intro);
  const pinoutLink = h(
    "p",
    { class: "card__sub" },
    h(
      "a",
      { href: pinoutSectionUrl("6-ds18b20-sensor-1-wire--onewire-library--optional", localePref.get()), target: "_blank", rel: "noopener" },
      T.settings.temperature.pinoutLink,
    ),
  );
  const liveLine = h("p", { class: "banner banner--info", role: "status" }, "—");
  const primaryHint = h("p", { class: "card__sub", hidden: true });

  const gpioSelect = h("select", { class: "input", id: "temp_gpio" }) as HTMLSelectElement;
  for (const g of TEMP_GPIO_OPTIONS) {
    const opt = h("option", { value: String(g) }, `GPIO ${g}`) as HTMLOptionElement;
    if (g === (cfg.temp_gpio ?? 13)) opt.selected = true;
    gpioSelect.append(opt);
  }
  const gpioRow = h("label", {}, T.settings.temperature.gpioLabel, gpioSelect);
  const gpioReboot = h("p", { class: "card__sub" }, T.settings.temperature.gpioReboot);

  const slotEnabled: HTMLInputElement[] = [];
  const slotLabel: ReturnType<typeof textRow>[] = [];
  const slotLive: HTMLElement[] = [];
  const slotAddr: HTMLElement[] = [];

  const slotCards: HTMLElement[] = [];
  for (let i = 0; i < 2; i++) {
    const en = h("input", { type: "checkbox", checked: slots[i]?.enabled ?? false }) as HTMLInputElement;
    slotEnabled.push(en);
    const label = textRow(
      `temperature_slot_${i}_label`,
      T.settings.temperature.slotLabel.replace("{n}", String(i + 1)),
      slots[i]?.label ?? "",
      "",
      { ...H, helpKey: "temperature_slots" },
    );
    slotLabel.push(label);
    const live = h("p", { class: "card__sub" }, "—");
    const addr = h("p", { class: "card__sub card__sub--muted" }, "");
    slotLive.push(live);
    slotAddr.push(addr);
    slotCards.push(
      h(
        "div",
        { class: "card card--nested" },
        h("h3", { class: "card__title" }, T.settings.temperature.slotTitle.replace("{n}", String(i + 1))),
        h("label", { class: "row", style: "gap:8px;align-items:center;" }, en, T.settings.temperature.slotEnabled),
        label.el,
        live,
        addr,
      ),
    );
  }

  const card = settingsSection(
    T.settings.temperature.cardTitle,
    intro,
    pinoutLink,
    liveLine,
    primaryHint,
    gpioRow,
    gpioReboot,
    ...slotCards,
  );

  const root = h("div", { class: "settings-section__cards" }, card);

  function refreshLive(m: Measurements | undefined, telemetryReady?: boolean): void {
    const ts = m?.temperature_sensors;
    if (!ts) {
      liveLine.textContent =
        telemetryReady === false
          ? T.settings.temperature.liveTelemetryPending
          : T.settings.temperature.liveUnknown;
      primaryHint.hidden = true;
      for (let i = 0; i < 2; i++) {
        slotLive[i].textContent = "—";
        slotAddr[i].textContent = "";
      }
      return;
    }
    liveLine.textContent = T.settings.temperature.liveCount
      .replace("{discovered}", String(ts.discovered_count ?? 0))
      .replace("{max}", String(ts.max_count ?? 2))
      .replace("{gpio}", String(ts.gpio ?? ""));
    const sensors = ts.sensors ?? [];
    let primarySlot = -1;
    for (const s of sensors) {
      if (s.primary) primarySlot = s.slot;
    }
    if (primarySlot >= 0) {
      primaryHint.hidden = false;
      primaryHint.textContent = T.settings.temperature.primaryHint.replace("{n}", String(primarySlot + 1));
    } else {
      primaryHint.hidden = true;
    }
    for (let i = 0; i < 2; i++) {
      const reading = sensors.find((s) => s.slot === i);
      if (!reading || !reading.enabled) {
        slotLive[i].textContent = T.settings.temperature.slotDisabled;
        slotAddr[i].textContent = "";
        continue;
      }
      if (reading.valid && typeof reading.temperature_c === "number") {
        slotLive[i].textContent = `${fmtTempC(reading.temperature_c)}${reading.primary ? ` (${T.settings.temperature.primaryBadge})` : ""}`;
      } else {
        slotLive[i].textContent = T.settings.temperature.slotNoReading;
      }
      slotAddr[i].textContent = reading.address
        ? T.settings.temperature.romLabel.replace("{addr}", reading.address)
        : "";
    }
  }

  function collectPatch(): Partial<
    Pick<RouterConfig, "temp_gpio" | "temperature_slots" | "temperature_label">
  > {
    const temperature_slots: TemperatureSlotConfig[] = [
      {
        enabled: slotEnabled[0].checked,
        label: slotLabel[0].ref.read(),
        address: slots[0]?.address ?? "",
      },
      {
        enabled: slotEnabled[1].checked,
        label: slotLabel[1].ref.read(),
        address: slots[1]?.address ?? "",
      },
    ];
    return {
      temp_gpio: parseInt(gpioSelect.value, 10) || 13,
      temperature_slots,
      temperature_label: temperature_slots[0]?.label || cfg.temperature_label,
    };
  }

  function attachAutosave(
    opts: SettingsCardAutosaveOpts,
    onSaved: (patch: Partial<RouterConfig>) => void,
  ): SettingsCardAutosave {
    return attachSettingsCardAutosave({
      ...opts,
      card,
      collect: collectPatch,
      onSaved,
    });
  }

  const pollLive = () => {
    void fetchLiveTemperatureTelemetry()
      .then((live) => {
        refreshLive(
          live.temperature_sensors || live.temperature_c != null
            ? (measurementsWithTemperatureTelemetry(live) as Measurements)
            : undefined,
          live.telemetry_ready,
        );
      })
      .catch(() => {});
  };
  pollLive();
  const poll = window.setInterval(pollLive, 5000);

  const origRoot = root;
  const cleanup = () => window.clearInterval(poll);
  (origRoot as HTMLElement & { __tempPollCleanup?: () => void }).__tempPollCleanup = cleanup;

  return { root, collectPatch, refreshLive, attachAutosave };
}

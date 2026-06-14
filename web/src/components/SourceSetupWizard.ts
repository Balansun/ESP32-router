import { h } from "../utils/dom";
import { getStrings } from "../i18n";
import { api } from "../api/client";
import type { RouterConfig, SourceDiagnostics } from "../api/types";
import { openDialog } from "./Dialog";
import { toast } from "./Toast";
import {
  filterSourceRegistry,
  pinoutSectionUrl,
  registryEntry,
  type SourceWireId,
} from "../routes/settings/sourceRegistry";
import { deviceInfo } from "../state/store";
import { buildFieldLabelRow, wrapSwitchWithHelp } from "./FieldHelp";
import { buildPmqttSetupFlow } from "./PmqttSetupFlow";
import { settingsSwitchLabel } from "../utils/settingsSwitch";

export interface SourceWizardOptions {
  initialConfig: RouterConfig;
  signal?: AbortSignal;
  onSaved?: (cfg: RouterConfig) => void;
  /** Pre-fill Ext split-deployment (meter ESP on LAN). */
  preset?: "split_ext";
  mode?: "setup" | "edit_pmqtt";
  lockSource?: "Pmqtt";
  /** Override `deviceInfo` meters filter (tests). */
  allowedMeters?: string[];
}

type Step = 1 | 2 | 3 | 4 | 5;

/** Wizard test/save: do not tie to route signal (aborts on navigation); allow slow EEPROM + Ext polls. */
const WIZARD_API_OPTS = { timeoutMs: 25_000, retry: 1 } as const;

function wizardFetchError(e: unknown, timeoutLabel: string): string {
  if (e instanceof DOMException && e.name === "AbortError") {
    return timeoutLabel;
  }
  if (e instanceof Error) return e.message;
  return String(e);
}

function wizardMetersList(opts: SourceWizardOptions): string[] | undefined {
  return opts.allowedMeters ?? deviceInfo.get()?.capabilities?.firmware_capabilities?.meters;
}

function isSingleMeterFirmware(opts: SourceWizardOptions): boolean {
  const meters = wizardMetersList(opts);
  return Array.isArray(meters) && meters.length === 1;
}

export function openSourceSetupWizard(opts: SourceWizardOptions): void {
  const singleMeter = isSingleMeterFirmware(opts);
  if (singleMeter && opts.mode !== "edit_pmqtt" && opts.preset !== "split_ext") {
    return;
  }
  const T = getStrings();
  const minStep: Step =
    singleMeter && opts.mode !== "edit_pmqtt" && !opts.preset ? 2 : 1;
  let step: Step =
    opts.mode === "edit_pmqtt"
      ? 3
      : opts.preset === "split_ext"
        ? 3
        : minStep;
  let selected: SourceWireId =
    opts.mode === "edit_pmqtt"
      ? "Pmqtt"
      : opts.preset === "split_ext"
        ? "BalansunPeer"
        : "JsyMk194";
  if (opts.lockSource === "Pmqtt") selected = "Pmqtt";

  function allowedEntries() {
    const meters =
      opts.allowedMeters ??
      deviceInfo.get()?.capabilities?.firmware_capabilities?.meters;
    return filterSourceRegistry(meters);
  }

  function ensureSelectedInProfile(): void {
    const entries = allowedEntries();
    const ids = new Set(entries.map((e) => e.id));
    if (ids.has(selected)) return;
    if (opts.lockSource && ids.has(opts.lockSource)) {
      selected = opts.lockSource;
      return;
    }
    if (entries.length) selected = entries[0].id;
  }
  ensureSelectedInProfile();

  let draft: RouterConfig = { ...opts.initialConfig };
  if (opts.preset === "split_ext") {
    draft.source = "BalansunPeer";
    draft.ext_protocol = "json";
    draft.ext_peer_path = "/api/v1/measurements";
    if (!draft.ext_peer_port) draft.ext_peer_port = 80;
  }
  let testResult = "";
  let testOk: boolean | null = null;
  let testRunning = false;
  let deferTest = false;

  const body = h("div", { class: "wizard" });
  const foot = h("div", { class: "row", style: "gap:8px;justify-content:flex-end;margin-top:12px;" });

  const pmqttFlow = buildPmqttSetupFlow(
    T,
    draft.pmqtt_bindings ?? [],
    {
      mqtt_ip: draft.mqtt_ip ?? "",
      mqtt_port: draft.mqtt_port ?? 1883,
      mqtt_user: draft.mqtt_user ?? "",
      mqtt_password: draft.mqtt_password ?? "",
    },
    () => {
      draft.pmqtt_bindings = pmqttFlow.getBindings();
      Object.assign(draft, pmqttFlow.getMqttBroker());
    },
  );

  function fieldRow(
    id: string,
    label: string,
    value: string,
    type: "text" | "number" = "text",
  ): HTMLElement {
    const input = h("input", {
      class: "field__input",
      id,
      type,
      value,
      onInput: () => {
        const v = input.value;
        if (id === "ext_peer_ip") draft.ext_peer_ip = v;
        else if (id === "ext_peer_port") draft.ext_peer_port = parseInt(v, 10) || 80;
        else if (id === "ext_peer_path") draft.ext_peer_path = v;
        else if (id === "enphase_user") draft.enphase_user = v;
        else if (id === "enphase_password") draft.enphase_password = v;
        else if (id === "enphase_serial" || id === "meter_channel") {
          draft.enphase_serial = v;
          draft.meter_channel = v;
        }
        else if (id === "pmqtt_topic") draft.pmqtt_topic = v;
        else if (id === "mqtt_ip") draft.mqtt_ip = v;
        else if (id === "mqtt_port") draft.mqtt_port = parseInt(v, 10) || 1883;
        else if (id === "mqtt_user") draft.mqtt_user = v;
        else if (id === "mqtt_password") draft.mqtt_password = v;
        else if (id === "jsy_mk333_serial_baud") draft.jsy_mk333_serial_baud = parseInt(v, 10) || 9600;
        else if (id === "calib_u") draft.calib_u = parseInt(v, 10) || 1000;
        else if (id === "calib_i") draft.calib_i = parseInt(v, 10) || 1000;
      },
    }) as HTMLInputElement;
    return h(
      "div",
      { class: "field" },
      buildFieldLabelRow({
        label,
        forId: id,
        helpScope: "sourceWizard",
        helpKey: id,
      }),
      input,
    );
  }

  function render() {
    body.replaceChildren();
    if (step === 1) {
      if (singleMeter) {
        const entry = allowedEntries()[0];
        body.append(
          h("p", {}, T.sourceWizard.step1Hint),
          h("p", { class: "ok" }, entry ? entry.id : wizardMetersList(opts)?.[0] ?? ""),
        );
      } else {
      body.append(h("p", {}, T.sourceWizard.step1Hint));
      const grid = h("div", { class: "wizard-sources" });
      for (const entry of allowedEntries()) {
        const id = entry.id;
        const btn = h(
          "button",
          {
            type: "button",
            class: `btn ${selected === id ? "btn--primary" : "btn--ghost"}`,
            onClick: () => {
              if (opts.lockSource === "Pmqtt" && id !== "Pmqtt") return;
              selected = id;
              if (id === "BalansunPeer") {
                draft.ext_protocol = "json";
                if (!draft.ext_peer_path) draft.ext_peer_path = "/api/v1/measurements";
              }
              render();
            },
          },
          id,
        );
        if (opts.lockSource === "Pmqtt" && id !== "Pmqtt") {
          (btn as HTMLButtonElement).disabled = true;
        }
        grid.append(btn);
      }
      body.append(grid);
      }
    } else if (step === 2) {
      const entry = registryEntry(selected)!;
      body.append(
        h("p", {}, T.sourceWizard.step2Hint),
        h(
          "p",
          {},
          h(
            "a",
            { href: pinoutSectionUrl(entry.pinoutAnchor), target: "_blank", rel: "noopener" },
            T.sourceWizard.openPinout,
          ),
        ),
      );
    } else if (step === 3) {
      body.append(h("p", {}, T.sourceWizard.step3Hint));
      const entry = registryEntry(selected)!;
      for (const f of entry.fields) {
        if (f === "pmqtt_schema") continue;
        const labels: Record<string, string> = {
          ext_peer_ip: T.settings.mqttIp.replace("MQTT", "IP"),
          ext_peer_port: T.settings.mqttPort.replace("MQTT", "HTTP"),
          ext_peer_path: "HTTP path",
          enphase_user: "Enphase user",
          enphase_password: "Enphase password",
          enphase_serial: "Enphase serial",
          meter_channel: "Meter channel",
          pmqtt_topic: T.settings.pmqttTopic,
          jsy_mk333_serial_baud: T.settings.jsyMk333Baud,
          calib_u: T.settings.calibU,
          calib_i: T.settings.calibI,
        };
        const val =
          f === "ext_peer_ip"
            ? draft.ext_peer_ip
            : f === "ext_peer_port"
              ? String(draft.ext_peer_port)
              : f === "ext_peer_path"
                ? draft.ext_peer_path
                : f === "enphase_user"
                  ? draft.enphase_user
                  : f === "enphase_password"
                    ? draft.enphase_password
                    : f === "enphase_serial" || f === "meter_channel"
                      ? draft.meter_channel ?? draft.enphase_serial
                      : f === "pmqtt_topic"
                        ? draft.pmqtt_topic
                        : f === "jsy_mk333_serial_baud"
                          ? String(draft.jsy_mk333_serial_baud ?? 9600)
                          : f === "calib_u"
                            ? String(draft.calib_u)
                            : String(draft.calib_i);
        body.append(
          fieldRow(
            f,
            labels[f] || f,
            val,
            f.includes("port") || f.includes("baud") || f.startsWith("calib") ? "number" : "text",
          ),
        );
      }
      if (entry.fields.includes("pmqtt_schema")) {
        draft.pmqtt_bindings = pmqttFlow.getBindings();
        Object.assign(draft, pmqttFlow.getMqttBroker());
        body.append(...pmqttFlow.sectionRows);
      }
    } else if (step === 4) {
      body.append(h("p", {}, T.sourceWizard.step4Hint));
      const deferInput = h("input", {
        type: "checkbox",
        checked: deferTest,
        onChange: () => {
          deferTest = (deferInput as HTMLInputElement).checked;
        },
      }) as HTMLInputElement;
      body.append(
        h(
          "div",
          { class: "field" },
          wrapSwitchWithHelp(
            settingsSwitchLabel(deferInput, T.sourceWizard.deferTest),
            "sourceWizard",
            "defer_test",
          ),
        ),
      );
      const testBtn = h(
        "button",
        {
          type: "button",
          class: "btn btn--ghost",
          disabled: testRunning,
          onClick: async () => {
            if (testRunning) return;
            testRunning = true;
            testResult = T.loading;
            testOk = null;
            render();
            try {
              draft.source = selected;
              if (selected === "Pmqtt") {
                draft.pmqtt_bindings = pmqttFlow.getBindings();
                Object.assign(draft, pmqttFlow.getMqttBroker());
                const req = pmqttFlow.validateRequired();
                if (!req.ok) {
                  testOk = false;
                  testResult = "Pmqtt: compléter les mesures requises (signée, split ou snapshot).";
                  return;
                }
              }
              await api.putConfig(draft, WIZARD_API_OPTS);
              const d = await api.getSourceDiagnostics(256, WIZARD_API_OPTS);
              const diag = d.diagnostics as SourceDiagnostics["diagnostics"];
              testOk = !!diag?.last_poll_ok;
              const err =
                diag?.last_error ||
                (d.ext as { last_error?: string } | undefined)?.last_error;
              testResult = testOk
                ? T.sourceWizard.testOk
                : `${T.sourceWizard.testFail}${err ? `: ${err}` : ""}`;
            } catch (e) {
              testOk = false;
              testResult = `${T.sourceWizard.testFail}: ${wizardFetchError(
                e,
                T.sourceWizard.testTimeout,
              )}`;
            } finally {
              testRunning = false;
              render();
            }
          },
        },
        testRunning ? T.loading : T.sourceWizard.runTest,
      );
      body.append(testBtn, h("p", { class: testOk === true ? "ok" : testOk === false ? "err" : "" }, testResult));
    } else {
      body.append(h("p", {}, T.sourceWizard.step5Hint));
    }
    foot.replaceChildren();
    if (step > minStep) {
      foot.append(
        h(
          "button",
          {
            type: "button",
            class: "btn btn--ghost",
            onClick: () => {
              step = (step - 1) as Step;
              render();
            },
          },
          T.sourceWizard.back,
        ),
      );
    }
    if (step < 5) {
      foot.append(
        h(
          "button",
          {
            type: "button",
            class: "btn btn--primary",
            onClick: () => {
              if (step === 3 && selected === "Pmqtt") {
                draft.pmqtt_bindings = pmqttFlow.getBindings();
                Object.assign(draft, pmqttFlow.getMqttBroker());
                const req = pmqttFlow.validateRequired();
                if (!req.ok) {
                  toast("Pmqtt: compléter les mesures requises avant de continuer.", "error");
                  return;
                }
              }
              step = (step + 1) as Step;
              render();
            },
          },
          T.sourceWizard.next,
        ),
      );
    } else {
      foot.append(
        h(
          "button",
          {
            type: "button",
            class: "btn btn--primary",
            onClick: async () => {
              try {
                draft.source = selected;
                if (selected === "Pmqtt") {
                  draft.pmqtt_bindings = pmqttFlow.getBindings();
                  Object.assign(draft, pmqttFlow.getMqttBroker());
                  if (!draft.pmqtt_schema) draft.pmqtt_schema = "Pw";
                }
                if (selected === "BalansunPeer") {
                  draft.ext_protocol = "json";
                  if (!draft.ext_peer_path) draft.ext_peer_path = "/api/v1/measurements";
                }
                await api.putConfig(draft, WIZARD_API_OPTS);
                opts.onSaved?.(draft);
                toast(T.saved, "success");
                close();
              } catch (e) {
                toast(
                  `${T.saveError}: ${wizardFetchError(e, T.sourceWizard.testTimeout)}`,
                  "error",
                );
              }
            },
          },
          T.save,
        ),
      );
    }
  }

  const { close } = openDialog({
    title: T.sourceWizard.title,
    body: h("div", {}, body, foot),
    actions: [{ label: T.close, kind: "ghost", onClick: () => {} }],
  });
  render();
}

import { h } from "../utils/dom";
import type { AppStrings } from "../i18n";
import type { PmqttBinding } from "../api/types";
import { buildFieldLabelRow } from "./FieldHelp";
import { buildJsonPathPicker } from "./JsonPathPicker";
import { normalizeJsonPath } from "../utils/jsonPathIndex";
import { metricDef, PMQTT_METRIC_CATALOG, validateRequiredGroups, type PmqttScenarioId } from "../utils/pmqttMetricCatalog";
import { previewBinding, type PmqttPreviewResult } from "../utils/pmqttPreview";

export interface PmqttMqttBroker {
  mqtt_ip: string;
  mqtt_port: number;
  mqtt_user: string;
  mqtt_password: string;
}

function cloneBinding(b: PmqttBinding): PmqttBinding {
  return {
    metric: b.metric,
    topic: b.topic,
    format: b.format,
    path: b.path ?? "",
    enabled: b.enabled ?? true,
  };
}

function inferScenario(bindings: PmqttBinding[]): PmqttScenarioId {
  const active = bindings.filter((b) => (b.enabled ?? true));
  if (active.some((b) => b.metric === "house.signed_net_w")) return "signed_power";
  if (active.some((b) => b.metric === "house.snapshot")) return "house_snapshot";
  const split =
    active.some((b) => b.metric === "house.active_import_w") &&
    active.some((b) => b.metric === "house.active_export_w");
  if (split) return "split_import_export";
  return "custom";
}

function scenarioPreset(s: PmqttScenarioId): PmqttBinding[] {
  if (s === "signed_power") {
    return [{ metric: "house.signed_net_w", topic: "", format: "json", path: "Pw", enabled: true }];
  }
  if (s === "house_snapshot") {
    return [{ metric: "house.snapshot", topic: "", format: "snapshot", path: "house", enabled: true }];
  }
  if (s === "split_import_export") {
    return [
      { metric: "house.active_import_w", topic: "", format: "json", path: "value", enabled: true },
      { metric: "house.active_export_w", topic: "", format: "json", path: "value", enabled: true },
    ];
  }
  return [];
}

export function buildPmqttSetupFlow(
  T: AppStrings,
  initialBindings: PmqttBinding[],
  initialBroker: PmqttMqttBroker,
  onChange: () => void,
): {
  sectionRows: HTMLElement[];
  getBindings: () => PmqttBinding[];
  getMqttBroker: () => PmqttMqttBroker;
  validateRequired: () => { ok: boolean; missing: string[] };
} {
  let mqttIp = initialBroker.mqtt_ip ?? "";
  let mqttPort = initialBroker.mqtt_port > 0 ? initialBroker.mqtt_port : 1883;
  let mqttUser = initialBroker.mqtt_user ?? "";
  let mqttPwd = initialBroker.mqtt_password ?? "";

  let scenario: PmqttScenarioId = inferScenario(initialBindings);
  let bindings: PmqttBinding[] = initialBindings.length
    ? initialBindings.map(cloneBinding)
    : scenarioPreset("signed_power");

  const wrap = h("div", { class: "pmqtt-setup-flow" });
  const brokerHost = h("div", { class: "pmqtt-broker-fields" });
  const mqttIpInput = h("input", {
    class: "field__input",
    id: "mqtt_ip",
    value: mqttIp,
    spellcheck: "false",
    autocomplete: "off",
    onInput: () => {
      mqttIp = mqttIpInput.value;
      onChange();
    },
  }) as HTMLInputElement;
  const mqttPortInput = h("input", {
    class: "field__input",
    id: "mqtt_port",
    type: "number",
    value: String(mqttPort),
    onInput: () => {
      mqttPort = parseInt(mqttPortInput.value, 10) || 1883;
      onChange();
    },
  }) as HTMLInputElement;
  const mqttUserInput = h("input", {
    class: "field__input",
    id: "mqtt_user",
    value: mqttUser,
    spellcheck: "false",
    autocomplete: "off",
    onInput: () => {
      mqttUser = mqttUserInput.value;
      onChange();
    },
  }) as HTMLInputElement;
  const mqttPwdInput = h("input", {
    class: "field__input",
    id: "mqtt_password",
    type: "password",
    value: mqttPwd,
    autocomplete: "new-password",
    onInput: () => {
      mqttPwd = mqttPwdInput.value;
      onChange();
    },
  }) as HTMLInputElement;
  brokerHost.append(
    h("h3", { class: "field__hint" }, T.settings.pmqttBrokerSection),
    h(
      "div",
      { class: "field" },
      buildFieldLabelRow({
        label: T.settings.mqttIp,
        forId: "mqtt_ip",
        helpScope: "settings",
        helpKey: "mqtt_ip",
      }),
      mqttIpInput,
    ),
    h(
      "div",
      { class: "field" },
      buildFieldLabelRow({
        label: T.settings.mqttPort,
        forId: "mqtt_port",
        helpScope: "settings",
        helpKey: "mqtt_port",
      }),
      mqttPortInput,
    ),
    h(
      "div",
      { class: "field" },
      buildFieldLabelRow({
        label: T.settings.mqttUser,
        forId: "mqtt_user",
        helpScope: "settings",
        helpKey: "mqtt_user",
      }),
      mqttUserInput,
    ),
    h(
      "div",
      { class: "field" },
      buildFieldLabelRow({
        label: T.settings.mqttPwd,
        forId: "mqtt_password",
        helpScope: "settings",
        helpKey: "mqtt_password",
      }),
      mqttPwdInput,
    ),
  );
  const scenarioGrid = h("div", { class: "wizard-sources" });
  const cards = h("div", { class: "pmqtt-bindings" });
  const requiredHint = h("p", { class: "field__hint" });

  function renderScenarioButtons() {
    scenarioGrid.replaceChildren();
    const options: Array<{ id: PmqttScenarioId; label: string }> = [
      { id: "signed_power", label: "Une valeur signée (W)" },
      { id: "house_snapshot", label: "Un JSON maison (snapshot)" },
      { id: "split_import_export", label: "Deux topics import/export" },
      { id: "custom", label: "Personnaliser" },
    ];
    for (const opt of options) {
      scenarioGrid.append(
        h(
          "button",
          {
            type: "button",
            class: `btn ${scenario === opt.id ? "btn--primary" : "btn--ghost"}`,
            onClick: () => {
              scenario = opt.id;
              bindings = scenarioPreset(opt.id);
              if (opt.id === "custom" && bindings.length === 0) {
                bindings = [{ metric: "house.signed_net_w", topic: "", format: "json", path: "Pw", enabled: true }];
              }
              renderCards();
              onChange();
            },
          },
          opt.label,
        ),
      );
    }
  }

  function renderPreview(resEl: HTMLElement, binding: PmqttBinding, sample: string) {
    if (!sample.trim()) {
      resEl.textContent = "Aperçu: collez un exemple JSON/texte pour vérifier ce champ.";
      resEl.className = "field__hint";
      return;
    }
    const p: PmqttPreviewResult = previewBinding(binding, sample);
    if (!p.ok) {
      resEl.textContent = `Aperçu: ${p.error ?? "erreur de lecture"}`;
      resEl.className = "err";
      return;
    }
    resEl.textContent = `Aperçu: ${p.displayValue ?? p.value ?? ""}`;
    resEl.className = p.hint ? "field__hint" : "ok";
  }

  function renderCards() {
    cards.replaceChildren();
    bindings.forEach((binding, idx) => {
      const metric = metricDef(binding.metric);
      const sampleInput = h("textarea", {
        class: "field__input",
        rows: "3",
        placeholder: '{"house":{"Pw":-1200}}',
      }) as HTMLTextAreaElement;
      const previewEl = h("p", { class: "field__hint" }, "Aperçu: en attente");
      const pathWrap = h("div", { class: "field" });
      const pathInput = h("input", {
        class: "field__input",
        value: binding.path ?? "",
        onInput: () => {
          bindings[idx].path = normalizeJsonPath(pathInput.value);
          renderPreview(previewEl, bindings[idx], sampleInput.value);
          onChange();
        },
      }) as HTMLInputElement;
      const pathPickerHost = h("div", {});
      function renderPathPicker() {
        pathPickerHost.replaceChildren();
        if (bindings[idx].format !== "json" && bindings[idx].format !== "snapshot") return;
        if (!sampleInput.value.trim()) return;
        pathPickerHost.append(
          buildJsonPathPicker(sampleInput.value, {
            initialPath: bindings[idx].path,
            onSelect: (path) => {
              bindings[idx].path = normalizeJsonPath(path);
              pathInput.value = bindings[idx].path ?? "";
              renderPreview(previewEl, bindings[idx], sampleInput.value);
              onChange();
            },
          }),
        );
      }
      const metricSelect = h("select", {
        class: "field__input",
        onChange: () => {
          bindings[idx].metric = metricSelect.value;
          onChange();
          renderCards();
        },
      }) as HTMLSelectElement;
      for (const m of PMQTT_METRIC_CATALOG) {
        metricSelect.append(h("option", { value: m.id }, `${m.label}${m.unit ? ` (${m.unit})` : ""}`));
      }
      metricSelect.value = binding.metric;
      const formatSelect = h(
        "select",
        {
          class: "field__input",
          onChange: () => {
            bindings[idx].format = formatSelect.value as "plain" | "json" | "snapshot";
            renderPathPicker();
            renderPreview(previewEl, bindings[idx], sampleInput.value);
            onChange();
          },
        },
        h("option", { value: "plain" }, "Nombre simple"),
        h("option", { value: "json" }, "JSON"),
        h("option", { value: "snapshot" }, "Snapshot JSON"),
      ) as HTMLSelectElement;
      formatSelect.value = binding.format ?? "json";
      const topicInput = h("input", {
        class: "field__input",
        value: binding.topic ?? "",
        onInput: () => {
          bindings[idx].topic = topicInput.value;
          onChange();
        },
      }) as HTMLInputElement;
      sampleInput.addEventListener("input", () => {
        renderPathPicker();
        renderPreview(previewEl, bindings[idx], sampleInput.value);
      });
      pathWrap.append(
        buildFieldLabelRow({
          label: "Chemin JSON",
          forId: `pmqtt_path_${idx}`,
          helpScope: "settings",
          helpKey: "pmqtt_schema_custom",
        }),
        pathInput,
        pathPickerHost,
      );
      const tier = metric?.tier ?? "optional";
      const badge = h(
        "span",
        { class: `chip chip--${tier === "required" ? "danger" : tier === "recommended" ? "warn" : "info"}` },
        tier === "required" ? "Requis" : tier === "recommended" ? "Recommandé" : "Optionnel",
      );
      const card = h(
        "section",
        { class: "card" },
        h("div", { class: "row", style: "justify-content:space-between;align-items:center;" }, h("h3", {}, "Mesure MQTT"), badge),
        h("div", { class: "field" }, buildFieldLabelRow({ label: "Mesure cible", forId: `pmqtt_metric_${idx}` }), metricSelect),
        h("div", { class: "field" }, buildFieldLabelRow({ label: "Topic MQTT", forId: `pmqtt_topic_${idx}` }), topicInput),
        h("div", { class: "field" }, buildFieldLabelRow({ label: "Type de message", forId: `pmqtt_format_${idx}` }), formatSelect),
        formatSelect.value === "plain" ? null : pathWrap,
        h("div", { class: "field" }, buildFieldLabelRow({ label: "Exemple payload", forId: `pmqtt_sample_${idx}` }), sampleInput),
        previewEl,
        h(
          "button",
          {
            type: "button",
            class: "btn btn--ghost",
            onClick: () => {
              bindings.splice(idx, 1);
              renderCards();
              onChange();
            },
          },
          "Supprimer",
        ),
      );
      renderPathPicker();
      cards.append(card);
    });
    cards.append(
      h(
        "button",
        {
          type: "button",
          class: "btn btn--ghost",
          onClick: () => {
            bindings.push({ metric: "house.active_import_w", topic: "", format: "json", path: "value", enabled: true });
            renderCards();
            onChange();
          },
        },
        "Ajouter une mesure",
      ),
    );
    const validity = validateRequiredGroups(bindings);
    requiredHint.textContent = validity.ok
      ? "Configuration minimale OK (au moins un groupe requis est satisfait)."
      : "Configuration incomplète : ajouter une puissance signée, ou import+export, ou snapshot maison.";
    requiredHint.className = validity.ok ? "ok" : "err";
  }

  renderScenarioButtons();
  renderCards();
  wrap.append(brokerHost, h("p", {}, T.settings.pmqttBindingsHint), scenarioGrid, cards, requiredHint);

  return {
    sectionRows: [wrap],
    getBindings: () => bindings.map(cloneBinding),
    getMqttBroker: (): PmqttMqttBroker => ({
      mqtt_ip: mqttIp.trim(),
      mqtt_port: mqttPort > 0 ? mqttPort : 1883,
      mqtt_user: mqttUser,
      mqtt_password: mqttPwd,
    }),
    validateRequired: () => validateRequiredGroups(bindings),
  };
}

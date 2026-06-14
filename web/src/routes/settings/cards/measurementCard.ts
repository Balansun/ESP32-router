import { h } from "../../../utils/dom";
import type { RouterConfig } from "../../../api/types";
import type { getStrings } from "../../../i18n";
import { docsPageUrl } from "../../../fieldHelp/docUrl";
import { localePref } from "../../../state/store";
import { openSourceSetupWizard } from "../../../components/SourceSetupWizard";
import { pmqttBindingsMissing } from "../../../utils/pmqttBindings";
import { settingsSection } from "./section";
import { hasCompiledMeter, isSingleMeterProfile } from "../../../utils/firmwareCaps";

export function buildMeasurementSettingsCard(
  T: ReturnType<typeof getStrings>,
  cfg: RouterConfig,
  signal: AbortSignal,
  sourceSummaryEl: HTMLElement,
  onConfigSaved: (cfg: RouterConfig) => void,
): HTMLElement {
  if (isSingleMeterProfile()) {
    return h("div", { hidden: true, class: "settings-measurement-source--fixed" });
  }
  const pmqttActive = cfg.source === "Pmqtt";
  const pmqttWarn = pmqttBindingsMissing(cfg)
    ? h(
        "p",
        { class: "banner banner--warn", role: "alert" },
        T.home.pmqttBindingsMissing,
      )
    : null;
  return settingsSection(
    T.settings.sectionMeasurement,
    ...(pmqttWarn ? [pmqttWarn] : []),
    sourceSummaryEl,
    h(
      "button",
      {
        type: "button",
        class: "btn btn--primary",
        onClick: () =>
          openSourceSetupWizard({
            initialConfig: cfg,
            signal,
            mode: pmqttActive ? "edit_pmqtt" : "setup",
            lockSource: pmqttActive ? "Pmqtt" : undefined,
            onSaved: onConfigSaved,
          }),
      },
      pmqttActive ? "Modifier les topics MQTT" : T.settings.openSourceWizard,
    ),
    ...(hasCompiledMeter("BalansunPeer")
      ? [
          h(
            "button",
            {
              type: "button",
              class: "btn btn--ghost",
              style: "margin-top:8px;",
              onClick: () =>
                openSourceSetupWizard({
                  initialConfig: cfg,
                  signal,
                  preset: "split_ext",
                  onSaved: onConfigSaved,
                }),
            },
            T.settings.splitDeployWizard,
          ),
        ]
      : []),
    h(
      "p",
      { class: "field__hint" },
      h(
        "a",
        {
          href: docsPageUrl(
            `/${localePref.get() === "fr" ? "fr" : "en"}/split-deployment/index/`,
          ),
          target: "_blank",
          rel: "noopener",
        },
        T.settings.splitDeployDoc,
      ),
    ),
  );
}

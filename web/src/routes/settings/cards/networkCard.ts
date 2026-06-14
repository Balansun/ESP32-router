import { h } from "../../../utils/dom";
import type { getStrings } from "../../../i18n";
import { withBase } from "../../../paths";
import { wifiNavPath } from "../../../wifi/wifiPaths";
import { settingsSection } from "./section";

export function buildNetworkSettingsCard(
  T: ReturnType<typeof getStrings>,
  networkLiveEl: HTMLElement,
  dhcpRow: HTMLElement,
  ipFixedEl: HTMLElement,
  gatewayEl: HTMLElement,
  subnetEl: HTMLElement,
  dnsEl: HTMLElement,
): HTMLElement {
  const wifiLink = h(
    "p",
    { class: "field__hint" },
    h("a", { href: withBase(wifiNavPath()), "data-route": "true" }, T.settings.openWifi),
    " — ",
    T.settings.openWifiHint,
  );

  return settingsSection(
    T.settings.sectionNetwork,
    h("h3", { class: "field__hint" }, T.settings.networkLive.title),
    networkLiveEl,
    h("div", { class: "field" }, dhcpRow),
    ipFixedEl,
    gatewayEl,
    subnetEl,
    dnsEl,
    wifiLink,
  );
}

export function buildMqttSettingsCard(
  T: ReturnType<typeof getStrings>,
  mqttPeriodEl: HTMLElement,
  mqttIpEl: HTMLElement,
  mqttPortEl: HTMLElement,
  mqttUserEl: HTMLElement,
  mqttPwdEl: HTMLElement,
  mqttPrefixEl: HTMLElement,
  mqttDeviceEl: HTMLElement,
  mqttActionsRow: HTMLElement,
): HTMLElement {
  return settingsSection(
    T.settings.sectionMqtt,
    mqttPeriodEl,
    mqttIpEl,
    mqttPortEl,
    mqttUserEl,
    mqttPwdEl,
    mqttPrefixEl,
    mqttDeviceEl,
    mqttActionsRow,
  );
}

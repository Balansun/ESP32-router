import type { DeviceCapabilities } from "../api/types";
import { getStrings } from "../i18n";
import { withBase } from "../paths";
import { h } from "./dom";

export function isConstrainedDevice(caps?: DeviceCapabilities): boolean {
  return caps?.tier === "constrained";
}

export function constrainedDiagBannerValues(caps: DeviceCapabilities): {
  days: string;
  maxPoints: string;
  powerWindow: string;
} {
  return {
    days: String(caps.history_days_retained ?? 7),
    maxPoints: String(caps.hist_max_points ?? 200),
    powerWindow: caps.history_power_window ?? "24h",
  };
}

export function formatConstrainedDiagBanner(
  template: string,
  caps: DeviceCapabilities,
): string {
  const v = constrainedDiagBannerValues(caps);
  return template
    .replace("{days}", v.days)
    .replace("{maxPoints}", v.maxPoints)
    .replace("{powerWindow}", v.powerWindow);
}

/** Warning block for /diag when capabilities.tier is constrained. */
export function buildConstrainedTierBanner(caps: DeviceCapabilities): HTMLElement {
  const T = getStrings();
  const banner = h(
    "p",
    {
      class: "banner banner--warn",
      role: "status",
    },
    formatConstrainedDiagBanner(T.diag.constrainedBanner, caps),
  );
  const link = h(
    "a",
    {
      href: withBase("/settings/backup"),
      class: "btn btn--ghost",
      style: "margin-top:8px;",
      "data-route": "true",
    },
    T.diag.constrainedBackupLink,
  );
  return h("div", { class: "banner-stack" }, banner, link);
}

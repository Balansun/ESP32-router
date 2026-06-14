import { h } from "../utils/dom";
import { getStrings } from "../i18n";
import { withBase } from "../paths";

export interface MainsFrequencyDetail {
  effectiveHz?: number;
  source?: string;
  installCountry?: string;
}

/** Site-wide banner when install country Hz disagrees with the meter (API code or older string form). */
export function buildMainsFrequencyBanner(): {
  el: HTMLElement;
  setWarning: (code: string | null | undefined, detail?: MainsFrequencyDetail) => void;
} {
  const el = h("p", {
    class: "banner banner--warn",
    role: "status",
    hidden: true,
  });
  const hint = h("p", { class: "field__hint banner__hint", hidden: true });
  const wrap = h("div", { class: "banner-stack" }, el, hint);

  return {
    el: wrap,
    setWarning(code: string | null | undefined, detail?: MainsFrequencyDetail) {
      if (!code) {
        el.hidden = true;
        hint.hidden = true;
        el.textContent = "";
        hint.replaceChildren();
        return;
      }
      const T = getStrings();
      let msg = T.settings.mainsFreqWarning;
      if (code === "meter_country_mismatch") {
        msg = T.settings.mainsFreqWarningMismatch;
      }
      el.hidden = false;
      el.textContent = msg;
      if (detail?.effectiveHz != null && detail.source) {
        hint.hidden = false;
        hint.replaceChildren(
          T.settings.mainsFreqWarningDetail
            .replace("{hz}", String(detail.effectiveHz))
            .replace("{source}", detail.source)
            .replace("{country}", detail.installCountry || "—"),
          " ",
          h(
            "a",
            { href: withBase("/settings/general"), "data-route": "true" },
            T.settings.mainsFreqWarningSettingsLink,
          ),
        );
      } else {
        hint.hidden = true;
        hint.replaceChildren();
      }
    },
  };
}

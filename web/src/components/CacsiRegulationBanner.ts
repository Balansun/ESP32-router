import { h } from "../utils/dom";
import { getStrings } from "../i18n";

const DISMISS_KEY = "balansun_cacsi_banner_dismissed";

function isCacsiBannerDismissed(): boolean {
  return localStorage.getItem(DISMISS_KEY) === "1";
}

export function buildCacsiRegulationBanner(): {
  el: HTMLElement;
  setVisible: (visible: boolean) => void;
} {
  const T = getStrings();
  const dismissBtn = h(
    "button",
    {
      type: "button",
      class: "btn btn--ghost btn--sm",
      onClick: () => {
        localStorage.setItem(DISMISS_KEY, "1");
        bannerRoot.hidden = true;
      },
    },
    T.home.cacsiDismiss,
  );
  const bannerRoot = h(
    "div",
    {
      class: "banner banner--info",
      role: "status",
      hidden: true,
    },
    h("p", {}, T.home.cacsiBanner),
    h("p", { class: "field__hint" }, T.home.cacsiBannerHint),
    dismissBtn,
  );

  return {
    el: bannerRoot,
    setVisible(visible: boolean) {
      if (!visible || isCacsiBannerDismissed()) {
        bannerRoot.hidden = true;
        return;
      }
      bannerRoot.hidden = false;
    },
  };
}

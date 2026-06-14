import { h } from "../utils/dom";
import { getStrings } from "../i18n";
import {
  resolveLegionellaBannerState,
  type LegionellaBannerState,
} from "../utils/legionellaBannerState";

export { resolveLegionellaBannerState, type LegionellaBannerState } from "../utils/legionellaBannerState";

export function buildLegionellaGatingBanner(): {
  el: HTMLElement;
  update(opts: { gatingDisabled: boolean; temperatureC: number }): void;
} {
  const T = getStrings();
  const msgP = h("p", {});
  const bannerRoot = h(
    "div",
    {
      class: "banner banner--warn",
      role: "status",
      hidden: true,
    },
    msgP,
  );

  function applyState(state: LegionellaBannerState): void {
    if (state === "hidden") {
      bannerRoot.hidden = true;
      return;
    }
    bannerRoot.hidden = false;
    msgP.textContent = `${T.actions.legionellaWarning} ${T.actions.legionellaWarningBody}`;
  }

  return {
    el: bannerRoot,
    update(opts) {
      applyState(
        resolveLegionellaBannerState({
          gatingDisabled: opts.gatingDisabled,
          temperatureC: opts.temperatureC,
        }),
      );
    },
  };
}

import { h } from "../utils/dom";
import { getStrings } from "../i18n";

/** Shown only in Vite `representation` mode (mock API preview, no hardware). */
export function buildRepresentationBanner(): HTMLElement {
  const T = getStrings();
  return h(
    "p",
    {
      class: "representation-banner",
      role: "status",
    },
    h("strong", {}, T.representation.bannerTitle),
    " — ",
    T.representation.bannerBody,
  );
}

export function isRepresentationMode(): boolean {
  return import.meta.env.MODE === "representation";
}

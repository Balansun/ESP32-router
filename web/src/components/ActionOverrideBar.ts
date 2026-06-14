import type { ActionOverride, OverrideState } from "../api/types";
import { getStrings } from "../i18n";
import { h } from "../utils/dom";

export function overrideMap(
  summary: ActionOverride[] | undefined,
): Map<number, ActionOverride> {
  const m = new Map<number, ActionOverride>();
  for (const o of summary ?? []) m.set(o.index, o);
  return m;
}

export function buildOverrideControls(opts: {
  index: number;
  active?: ActionOverride;
  onSet: (state: OverrideState) => void | Promise<void>;
  /** Action 0 only: manual 100% triac_fixed override. */
  onTriacFull?: () => void | Promise<void>;
  disabled?: boolean;
}): HTMLElement {
  const T = getStrings();
  const cur = opts.active?.state ?? "auto";
  const row = h("div", {
    class: "override-bar",
    role: "group",
    "aria-label": T.home.overrideAria,
  });

  const states: OverrideState[] = ["auto", "on", "off"];
  for (const st of states) {
    const btn = h(
      "button",
      {
        type: "button",
        class: `btn btn--ghost btn--xs${cur === st ? " override-bar__btn--active" : ""}`,
        "aria-pressed": cur === st ? "true" : "false",
        disabled: opts.disabled,
        onClick: () => void opts.onSet(st),
      },
      st === "auto" ? T.home.overrideAuto : st === "on" ? T.on : T.off,
    );
    row.append(btn);
  }

  if (opts.index === 0 && opts.onTriacFull) {
    const triacFullActive =
      cur === "triac_fixed" && (opts.active?.triac_open_percent ?? 0) >= 100;
    row.append(
      h(
        "button",
        {
          type: "button",
          class: `btn btn--ghost btn--xs${triacFullActive ? " override-bar__btn--active" : ""}`,
          "aria-pressed": triacFullActive ? "true" : "false",
          disabled: opts.disabled,
          onClick: () => void opts.onTriacFull!(),
        },
        T.actions.overrideTriacFull,
      ),
    );
  }

  if (cur !== "auto" && opts.active) {
    row.append(
      h("span", { class: "override-bar__badge" }, T.home.overrideActive),
    );
  }

  return row;
}

import { getStrings } from "../i18n";
import { formatStr } from "../i18n/format";
import { h } from "../utils/dom";
import { periodStart2400, periodWindowLabel } from "../utils/actionPeriods";
import type { ActionPeriod, PeriodMode } from "../api/types";

const MODE_COLORS: Record<PeriodMode, string> = {
  off: "var(--c-info)",
  on: "var(--c-import)",
  power: "var(--c-export)",
};

const TRIAC_COLOR = "var(--c-warn)";

export interface PeriodScheduleListEvents {
  onPickPeriod: (index: number) => void;
}

export interface PeriodScheduleListHandle {
  el: HTMLElement;
  setPeriods(periods: ActionPeriod[], isTriac: boolean): void;
}

export function buildPeriodScheduleList(
  events: PeriodScheduleListEvents,
): PeriodScheduleListHandle {
  const T = getStrings();
  const list = h("ul", {
    class: "period-list",
    role: "list",
    "aria-label": T.actions.periodListAria,
  });

  let current: ActionPeriod[] = [];
  let triac = false;

  function modeLabel(mode: PeriodMode): string {
    if (mode === "off") return T.actions.period.off;
    if (mode === "on") return T.actions.period.on;
    return triac ? T.actions.period.triac : T.actions.period.power;
  }

  function periodSummary(p: ActionPeriod): string {
    if (p.mode === "off" || p.mode === "on") return "";
    return formatStr(T.actions.timelineThreshold, { w: p.power_min_w });
  }

  function setPeriods(periods: ActionPeriod[], isTriac: boolean) {
    current = periods.map((p) => ({ ...p }));
    triac = isTriac;
    render();
  }

  function render() {
    list.replaceChildren();
    for (let i = 0; i < current.length; i++) {
      const p = current[i];
      const start = periodStart2400(current, i);
      const end = p.hour_end;
      const timeText = periodWindowLabel(start, end);
      const mode = modeLabel(p.mode);
      const summary = periodSummary(p);
      const color =
        triac && p.mode === "power" ? TRIAC_COLOR : MODE_COLORS[p.mode];

      const modePill = h(
        "span",
        {
          class: "period-list__mode",
          style: `background:color-mix(in oklab, ${color} 22%, transparent);border-color:${color};`,
        },
        mode,
      );

      const meta = h(
        "div",
        { class: "period-list__meta" },
        modePill,
        summary
          ? h("span", { class: "period-list__summary" }, summary)
          : null,
      );

      const row = h(
        "li",
        { class: "period-list__item" },
        h(
          "div",
          { class: "period-list__body" },
          h("span", { class: "period-list__time" }, timeText),
          meta,
        ),
        h(
          "button",
          {
            type: "button",
            class: "btn btn--sm btn--ghost period-list__edit",
            "aria-label": `${T.actions.periodEdit}, ${timeText}, ${mode}`,
            onClick: () => events.onPickPeriod(i),
          },
          T.actions.periodEdit,
        ),
      );
      list.append(row);
    }
  }

  return { el: list, setPeriods };
}

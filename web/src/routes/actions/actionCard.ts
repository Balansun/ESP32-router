import { h } from "../../utils/dom";
import { buildPeriodScheduleList } from "../../components/PeriodScheduleList";
import { toast } from "../../components/Toast";
import { icon } from "../../utils/icons";
import type { ActionConfig } from "../../api/types";
import { getStrings } from "../../i18n";
import { buildFieldLabelRow } from "../../components/FieldHelp";
import { buildSwitch, numberField, textField } from "./formControls";

const AH = { helpScope: "actions" as const };
import { MAX_PERIODS, actionSupportsDailyCapWh } from "./model";
import {
  isActionRegulationEnabled,
  setActionRegulationEnabled,
} from "./regulationMode";

export interface ActionCardContext {
  editing: ActionConfig[];
  idx: number;
  renderCards: () => void;
  editPeriod: (actIdx: number, periodIdx: number) => void;
  dailyCapWh?: number;
  onDailyCapChange?: (wh: number) => void;
  onDirty?: () => void;
}

export function buildActionCard(ctx: ActionCardContext): HTMLElement {
  const { editing, idx, renderCards, editPeriod, dailyCapWh, onDailyCapChange, onDirty } = ctx;
  const touch = () => onDirty?.();
  const T = getStrings();
  const a = editing[idx];
  const isTriac = idx === 0 || a.kind === "triac";
  const titleInput = h("input", {
    class: "field__input",
    type: "text",
    value: a.title,
    "aria-label": T.actions.actionTitle,
    onInput: (ev) => {
      a.title = (ev.target as HTMLInputElement).value;
      touch();
    },
  });

  const enabledSw = buildSwitch(
    isActionRegulationEnabled(a.regulation_mode),
    T.actions.actionEnabled,
    (v) => {
      a.regulation_mode = setActionRegulationEnabled(a.regulation_mode, v);
      touch();
    },
  );

  const card = h("section", {
    class: `card stack${isTriac ? " card--triac" : ""}`,
  });

  const headRow = h(
    "header",
    { class: "spread" },
    h(
      "div",
      { class: "stack stack--sm" },
      h(
        "label",
        { class: "field__label" },
        isTriac ? T.actions.triacName : T.actions.actionTitle,
      ),
      titleInput,
    ),
    enabledSw,
  );

  const removeBtn =
    idx === 0
      ? null
      : h(
          "button",
          {
            type: "button",
            class: "btn btn--danger btn--sm",
            "aria-label": T.remove,
            onClick: () => {
              editing.splice(idx, 1);
              renderCards();
            },
          },
          icon("close"),
          h("span", {}, T.remove),
        );

  let sensRow: HTMLElement | null = null;
  if (isTriac) {
    const valLabel = h(
      "span",
      {
        style:
          "font-variant-numeric:tabular-nums;font-weight:600;min-width:30px;text-align:right;",
      },
      String(a.triac_sensitivity ?? 50),
    );
    const slider = h("input", {
      type: "range",
      min: "1",
      max: "100",
      value: String(a.triac_sensitivity ?? 50),
      "aria-label": T.actions.sensitivity,
      style: "flex:1;",
      onInput: (ev) => {
        const v = Number((ev.target as HTMLInputElement).value);
        a.triac_sensitivity = v;
        a.port = v;
        valLabel.textContent = String(v);
        touch();
      },
    });
    sensRow = h(
      "div",
      { class: "field" },
      buildFieldLabelRow({ label: T.actions.sensitivity, ...AH, helpKey: "sensitivity" }),
      h("p", { class: "field__hint" }, T.actions.sensitivityHelp),
      h("p", { class: "field__hint" }, T.actions.gainHelp),
      h(
        "div",
        { class: "row", style: "gap:10px;" },
        h(
          "span",
          { class: "field__hint", style: "min-width:60px;" },
          T.actions.sensitivitySlow,
        ),
        slider,
        h(
          "span",
          { class: "field__hint", style: "min-width:60px;text-align:right;" },
          T.actions.sensitivityFast,
        ),
        valLabel,
      ),
    );
  }

  let dailyCapRow: HTMLElement | null = null;
  if (actionSupportsDailyCapWh(idx) && onDailyCapChange) {
    dailyCapRow = numberField(
      T.actions.dailyCapWh,
      String(dailyCapWh ?? 0),
      (v) => {
        onDailyCapChange(Math.max(0, Math.floor(Number(v) || 0)));
        touch();
      },
      T.actions.dailyCapWhHint,
      { ...AH, helpKey: "action_daily_cap_wh" },
    );
  }

  let httpRows: HTMLElement | null = null;
  if (!isTriac) {
    const hostInput = textField(T.actions.host, T.actions.hostHint, a.host ?? "", (v) => {
      a.host = v;
      a.kind = v === "localhost" ? "local_gpio" : "remote_http";
      touch();
    }, "text", { ...AH, helpKey: "host" });
    const portInput = textField(
      T.actions.port,
      "",
      String(a.port ?? 80),
      (v) => {
        a.port = parseInt(v, 10) || 80;
        touch();
      },
      "number",
      { ...AH, helpKey: "port" },
    );
    const onInput = textField(T.actions.path_on, T.actions.pathHint, a.path_on ?? "", (v) => {
      a.path_on = v;
      touch();
    }, "text", { ...AH, helpKey: "path_on" });
    const offInput = textField(T.actions.path_off, "", a.path_off ?? "", (v) => {
      a.path_off = v;
      touch();
    }, "text", { ...AH, helpKey: "path_off" });
    const repeatInput = textField(
      T.actions.repeat,
      T.actions.repeatHint,
      String(a.repeat_sec ?? 0),
      (v) => {
        a.repeat_sec = parseInt(v, 10) || 0;
        touch();
      },
      "number",
      { ...AH, helpKey: "repeat_sec" },
    );
    const tempoInput = textField(
      T.actions.tempo,
      T.actions.tempoHint,
      String(a.tempo_sec ?? 0),
      (v) => {
        a.tempo_sec = parseInt(v, 10) || 0;
        touch();
      },
      "number",
      { ...AH, helpKey: "tempo_sec" },
    );
    httpRows = h(
      "div",
      { class: "two-col" },
      hostInput,
      portInput,
      onInput,
      offInput,
      repeatInput,
      tempoInput,
    );
  }

  const scheduleList = buildPeriodScheduleList({
    onPickPeriod: (i) => editPeriod(idx, i),
  });
  scheduleList.setPeriods(a.periods, isTriac);

  const tlHint = h("p", { class: "field__hint" }, T.actions.timelineHint);
  const tlActions = h(
    "div",
    { class: "row", style: "justify-content:flex-end;gap:8px;" },
    h(
      "button",
      {
        type: "button",
        class: "btn btn--sm",
        onClick: () => {
          if (a.periods.length <= 1) return;
          a.periods.pop();
          const last = a.periods[a.periods.length - 1];
          if (last) last.hour_end = 2400;
          scheduleList.setPeriods(a.periods, isTriac);
          touch();
        },
        "aria-label": T.actions.removePeriod,
      },
      icon("minus"),
    ),
    h(
      "button",
      {
        type: "button",
        class: "btn btn--sm",
        onClick: () => {
          if (a.periods.length >= MAX_PERIODS) {
            toast(`Maximum ${MAX_PERIODS} plages`, "warn");
            return;
          }
          const prev =
            a.periods.length >= 2 ? a.periods[a.periods.length - 2].hour_end : 0;
          a.periods.splice(a.periods.length - 1, 0, {
            mode: isTriac ? "power" : "off",
            hour_end: Math.floor((prev + 2400) / 2),
            power_min_w: 0,
            power_max_w: isTriac ? 100 : 0,
            temp_inf_c: 150,
            temp_sup_c: 150,
          });
          scheduleList.setPeriods(a.periods, isTriac);
          touch();
        },
        "aria-label": T.actions.addPeriod,
      },
      icon("plus"),
    ),
  );

  card.append(
    headRow,
    ...(sensRow ? [sensRow] : []),
    ...(dailyCapRow ? [dailyCapRow] : []),
    ...(httpRows ? [httpRows] : []),
    h(
      "div",
      { class: "section" },
      h("h3", { class: "section__title" }, T.actions.timeline),
      tlHint,
      scheduleList.el,
      tlActions,
    ),
    ...(removeBtn ? [removeBtn] : []),
  );
  return card;
}

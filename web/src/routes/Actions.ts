import type { RouteCtx } from "../router";
import { h } from "../utils/dom";
import { api } from "../api/client";
import { openDialog } from "../components/Dialog";
import { toast } from "../components/Toast";
import { icon } from "../utils/icons";
import {
  clampPeriodEndMinutes,
  minutesToTimeInputValue,
  periodStart2400,
  timeInputValueToMinutes,
} from "../utils/actionPeriods";
import {
  fmtHourMinFrom2400,
  from2400ToMinutes,
  isProbeTemperatureReading,
  minutesTo2400,
} from "../utils/format";
import type { ActionConfig, ActionsConfigEnvelope, PeriodMode } from "../api/types";
import { getStrings } from "../i18n";
import { formatStr } from "../i18n/format";
import { setUnsavedGuard } from "../navigationGuard";
import { buildActionCard } from "./actions/actionCard";
import { buildPageHeader } from "../components/ui/pageHeader";
import { buildFieldLabelRow } from "../components/FieldHelp";
import { buildLegionellaGatingBanner } from "../components/LegionellaGatingBanner";
import { triacPeriodsHaveTemperatureGating } from "../utils/triacSafety";
import { numberField } from "./actions/formControls";
import {
  SCHEMA_VERSION,
  MAX_ACTIONS,
  blankHttpAction,
  ensureNormalised,
  normaliseForApi,
  normalizeDailyCapWh,
} from "./actions/model";
import { hasMultiAction, hasTriacRouting } from "../utils/firmwareCaps";
import { isSafetyLockoutActive } from "../utils/safetyLockout";

const AH = { helpScope: "actions" as const };

type SaveStatus = "idle" | "dirty" | "saving" | "saved" | "error";

export async function mountActions(ctx: RouteCtx): Promise<() => void> {
  const { outlet, signal } = ctx;
  const T = getStrings();

  if (!hasTriacRouting()) {
    return () => {};
  }

  const statusEl = h("p", {
    class: "card__save-status",
    hidden: true,
    "aria-live": "polite",
  }) as HTMLParagraphElement;

  const saveHeaderBtn = h(
    "button",
    {
      type: "button",
      class: "btn btn--primary",
      disabled: true,
      onClick: () => void flushSave(),
    },
    icon("save"),
    h("span", {}, T.save),
  ) as HTMLButtonElement;

  const pageHeader = buildPageHeader({
    title: T.actions.title,
    actions: [statusEl, saveHeaderBtn],
  });
  outlet.append(pageHeader);

  const wrap = h("div", { class: "form" });
  outlet.append(wrap);

  const loading = h("p", { class: "empty" }, T.loading);
  wrap.append(loading);

  let env: ActionsConfigEnvelope;
  let temperatureC = -127;
  let dailyCapWh: [number, number, number] = [0, 0, 0];
  let safetyLocked = false;
  try {
    const [live, actionsEnv, configEnv, health] = await Promise.all([
      api.getActionsLive({ signal }),
      api.getActionsConfig({ signal }),
      api.getConfig({ signal }),
      api.getHealth({ signal }),
    ]);
    temperatureC = live.temperature_c;
    env = actionsEnv;
    dailyCapWh = normalizeDailyCapWh(configEnv.config.action_daily_cap_wh);
    safetyLocked = isSafetyLockoutActive(health);
  } catch (e) {
    if ((e as DOMException)?.name === "AbortError") return () => {};
    loading.textContent = T.status.error + " — " + T.retry;
    return () => {};
  }
  loading.remove();

  const safetyBanner = h("p", {
    class: "banner banner--danger",
    role: "alert",
    hidden: !safetyLocked,
  });
  if (safetyLocked) {
    safetyBanner.textContent = T.home.safetyLockoutActionsDisabled;
  }
  wrap.append(safetyBanner);

  const editFieldset = h("fieldset", { disabled: safetyLocked, class: "form" });
  wrap.append(editFieldset);

  let editing: ActionConfig[] = ensureNormalised(env.actions);
  let dirty = false;
  let saveStatus: SaveStatus = "idle";
  let savedHideTimer: ReturnType<typeof setTimeout> | undefined;
  let inFlight: Promise<void> | null = null;

  const legionellaBanner = buildLegionellaGatingBanner();
  const cardsHost = h("div", { class: "form" });
  editFieldset.append(legionellaBanner.el, cardsHost);

  const addBtn = h(
    "button",
    {
      type: "button",
      class: "btn btn--ghost",
      hidden: !hasMultiAction(),
      onClick: () => {
        if (editing.length >= MAX_ACTIONS) {
          toast(formatStr(T.actions.max_actions, { n: MAX_ACTIONS }), "warn");
          return;
        }
        editing.push(blankHttpAction(editing.length));
        markDirty();
        renderCards();
      },
    },
    icon("plus"),
    h("span", {}, T.actions.addAction),
  );

  const saveBar = h("div", { class: "changes-bar", hidden: true });
  const discardBtn = h(
    "button",
    {
      type: "button",
      class: "btn btn--ghost",
      onClick: () => {
        openDialog({
          title: T.unsavedChanges,
          body: h("p", {}, T.discardChanges),
          actions: [
            { label: T.cancel, kind: "ghost", onClick: () => {} },
            {
              label: T.confirm,
              kind: "danger",
              onClick: async () => {
                const [j, cfgEnv] = await Promise.all([
                  api.getActionsConfig({ signal }),
                  api.getConfig({ signal }),
                ]);
                editing = ensureNormalised(j.actions);
                dailyCapWh = normalizeDailyCapWh(cfgEnv.config.action_daily_cap_wh);
                dirty = false;
                setSaveStatus("idle");
                renderCards();
              },
            },
          ],
        });
      },
    },
    T.cancel,
  );
  saveBar.append(
    h("span", { class: "changes-bar__label" }, T.actions.unsavedBarLabel),
    h("div", { class: "changes-bar__actions" }, discardBtn),
  );

  function setSaveStatus(next: SaveStatus) {
    saveStatus = next;
    savedHideTimer && clearTimeout(savedHideTimer);
    if (next === "idle") {
      statusEl.hidden = true;
      statusEl.textContent = "";
      statusEl.className = "card__save-status";
    } else {
      statusEl.hidden = false;
      statusEl.className = `card__save-status card__save-status--${next}`;
      statusEl.textContent =
        next === "dirty"
          ? T.settings.cardPending
          : next === "saving"
            ? T.saving
            : next === "saved"
              ? T.saved
              : T.saveError;
      if (next === "saved") {
        savedHideTimer = setTimeout(() => {
          if (saveStatus === "saved") setSaveStatus("idle");
        }, 2500);
      }
    }
    syncDirtyUi();
  }

  function syncDirtyUi() {
    saveBar.hidden = !dirty;
    const canSave = (dirty || saveStatus === "error") && !safetyLocked;
    const saving = saveStatus === "saving";
    if (canSave && !saving) {
      saveHeaderBtn.removeAttribute("disabled");
    } else {
      saveHeaderBtn.setAttribute("disabled", "true");
    }
    setUnsavedGuard(() => dirty || saveStatus === "saving");
  }

  const markDirty = () => {
    dirty = true;
    setSaveStatus("dirty");
  };

  editFieldset.append(addBtn, saveBar);

  function updateLegionellaBanner() {
    const triac = editing.find((a) => a.index === 0) ?? editing[0];
    const gatingDisabled = !!(
      triac && !triacPeriodsHaveTemperatureGating(triac.periods)
    );
    legionellaBanner.update({ gatingDisabled, temperatureC });
  }

  function renderCards() {
    cardsHost.replaceChildren();
    updateLegionellaBanner();
    for (let i = 0; i < editing.length; i++) {
      cardsHost.append(
        buildActionCard({
          editing,
          idx: i,
          renderCards: () => {
            markDirty();
            renderCards();
          },
          editPeriod,
          dailyCapWh: dailyCapWh[i],
          onDailyCapChange: (wh) => {
            dailyCapWh[i] = wh;
          },
          onDirty: markDirty,
        }),
      );
    }
  }
  renderCards();

  function editPeriod(actIdx: number, periodIdx: number) {
    const action = editing[actIdx];
    const p = { ...action.periods[periodIdx] };
    const isTriac = actIdx === 0 || action.kind === "triac";

    const modeOptions: Array<{ value: PeriodMode; label: string }> = isTriac
      ? [
          { value: "off", label: T.actions.period.off },
          { value: "on", label: T.actions.period.on },
          { value: "power", label: T.actions.period.triac },
        ]
      : [
          { value: "off", label: T.actions.period.off },
          { value: "on", label: T.actions.period.on },
          { value: "power", label: T.actions.period.power },
        ];

    const modeSel = h(
      "select",
      { class: "field__select" },
      ...modeOptions.map((opt) =>
        h(
          "option",
          { value: opt.value, selected: opt.value === p.mode },
          opt.label,
        ),
      ),
    );
    modeSel.addEventListener("change", () => {
      p.mode = modeSel.value as PeriodMode;
      renderPwBlock();
    });

    const isLastPeriod = periodIdx === action.periods.length - 1;
    const start2400 = periodStart2400(action.periods, periodIdx);
    const startHint = h(
      "p",
      { class: "field__hint" },
      formatStr(T.actions.edit.fromTime, {
        time: fmtHourMinFrom2400(start2400),
      }),
    );

    let timeInput: HTMLInputElement | null = null;
    let timeField: HTMLElement;
    if (!isLastPeriod) {
      const endMin = from2400ToMinutes(p.hour_end);
      const minMin = clampPeriodEndMinutes(action.periods, periodIdx, 0);
      const maxMin = clampPeriodEndMinutes(action.periods, periodIdx, 1440);
      const endId = `period-end-${actIdx}-${periodIdx}`;
      timeInput = h("input", {
        class: "field__input",
        type: "time",
        id: endId,
        value: minutesToTimeInputValue(endMin),
        min: minutesToTimeInputValue(minMin),
        max: minutesToTimeInputValue(maxMin),
        "aria-label": T.actions.edit.hour_end,
      }) as HTMLInputElement;
      timeField = h(
        "div",
        { class: "field" },
        buildFieldLabelRow({
          label: T.actions.edit.hour_end,
          forId: endId,
          ...AH,
          helpKey: "edit_hour_end",
        }),
        timeInput,
      );
    } else {
      timeField = h("p", { class: "field__hint" }, T.actions.edit.lastPeriodNote);
    }

    const pwBlock = h("div", { class: "form" });
    function renderPwBlock() {
      pwBlock.replaceChildren();
      if (p.mode === "power") {
        if (isTriac) {
          pwBlock.append(
            numberField(
              T.actions.edit.threshold + " (W)",
              String(p.power_min_w),
              (v) => {
                p.power_min_w = Math.max(0, Math.floor(Number(v) || 0));
              },
              T.actions.thresholdHelp,
              { ...AH, helpKey: "edit_threshold" },
            ),
            numberField(
              T.actions.edit.maxOpen + " (5–100)",
              String(p.power_max_w),
              (v) => {
                p.power_max_w = Math.max(5, Math.min(100, Math.floor(Number(v) || 100)));
              },
              "",
              { ...AH, helpKey: "edit_max_open" },
            ),
          );
        } else {
          pwBlock.append(
            numberField(T.actions.edit.powerOn + " (W)", String(p.power_min_w), (v) => {
              p.power_min_w = Math.floor(Number(v) || 0);
            }, "", { ...AH, helpKey: "edit_power_on" }),
            numberField(T.actions.edit.powerOff + " (W)", String(p.power_max_w), (v) => {
              p.power_max_w = Math.floor(Number(v) || 0);
            }, "", { ...AH, helpKey: "edit_power_off" }),
          );
        }
      }
      if (isProbeTemperatureReading(temperatureC)) {
        const tInf =
          p.temp_inf_c >= 0 && p.temp_inf_c <= 100 ? String(p.temp_inf_c) : "";
        const tSup =
          p.temp_sup_c >= 0 && p.temp_sup_c <= 100 ? String(p.temp_sup_c) : "";
        pwBlock.append(
          h("h3", { class: "section__title" }, T.actions.edit.tempIfBetween),
          h("p", { class: "field__hint" }, T.actions.edit.tempNote),
          numberField(T.actions.edit.tempInf + " °C", tInf, (v) => {
            const n = parseInt(v, 10);
            p.temp_inf_c = isFinite(n) && n >= 0 && n <= 100 ? n : 128;
          }, "", { ...AH, helpKey: "edit_temp_inf" }),
          numberField(T.actions.edit.tempSup + " °C", tSup, (v) => {
            const n = parseInt(v, 10);
            p.temp_sup_c = isFinite(n) && n >= 0 && n <= 100 ? n : 128;
          }, "", { ...AH, helpKey: "edit_temp_sup" }),
        );
      }
    }
    renderPwBlock();

    const dlg = openDialog({
      title: T.actions.edit.title,
      body: h(
        "div",
        { class: "form" },
        startHint,
        timeField,
        h(
          "div",
          { class: "field" },
          buildFieldLabelRow({ label: T.actions.edit.mode, ...AH, helpKey: "edit_mode" }),
          modeSel,
        ),
        pwBlock,
      ),
      actions: [
        { label: T.cancel, kind: "ghost", onClick: () => {} },
        {
          label: T.confirm,
          kind: "primary",
          onClick: () => {
            if (timeInput) {
              const minutes = timeInputValueToMinutes(timeInput.value);
              const clamped = clampPeriodEndMinutes(
                action.periods,
                periodIdx,
                minutes,
              );
              p.hour_end = minutesTo2400(clamped);
            }
            action.periods[periodIdx] = p;
            markDirty();
            renderCards();
          },
        },
      ],
    });
    void dlg;
  }

  async function persistSave(): Promise<void> {
    const outEnv: ActionsConfigEnvelope = {
      schema_version: SCHEMA_VERSION,
      nb_actions: editing.length,
      actions: editing.map((a, i) => normaliseForApi(a, i)),
    };
    await api.putActionsConfig(outEnv, { signal, retry: 1 });
    await api.patchConfig({ action_daily_cap_wh: [...dailyCapWh] }, { signal, retry: 1 });
  }

  async function flushSave(): Promise<void> {
    if (signal.aborted) return;
    if (inFlight) {
      await inFlight;
      if (!dirty && saveStatus !== "error") return;
    } else if (!dirty && saveStatus !== "error") {
      return;
    }
    setSaveStatus("saving");
    inFlight = (async () => {
      try {
        await persistSave();
        dirty = false;
        setSaveStatus("saved");
        updateLegionellaBanner();
        toast(T.saved, "success");
      } catch (e) {
        console.error(e);
        dirty = true;
        setSaveStatus("error");
        toast(T.saveError, "error");
      } finally {
        inFlight = null;
      }
    })();
    await inFlight;
  }

  const tempPoll = window.setInterval(() => {
    void api.getActionsLive({ signal }).then((live) => {
      temperatureC = live.temperature_c;
      updateLegionellaBanner();
    }).catch(() => {});
  }, 30_000);

  function beforeUnload(ev: BeforeUnloadEvent) {
    if (!dirty && saveStatus !== "saving") return;
    ev.preventDefault();
    ev.returnValue = "";
  }
  window.addEventListener("beforeunload", beforeUnload);

  signal.addEventListener(
    "abort",
    () => {
      if (savedHideTimer) clearTimeout(savedHideTimer);
      window.removeEventListener("beforeunload", beforeUnload);
      setUnsavedGuard(null);
    },
    { once: true },
  );

  const actionsCleanup = () => {
    window.clearInterval(tempPoll);
  };
  return () => {
    actionsCleanup();
  };
}

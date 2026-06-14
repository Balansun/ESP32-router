import { api } from "../api/client";
import type { HealthSelfTest } from "../api/types";
import { getStrings } from "../i18n";
import { poll, type PollHandle } from "../state/store";
import { h } from "../utils/dom";
import { icon } from "../utils/icons";
import { toast } from "./Toast";

export interface SelfTestPanelHandle {
  el: HTMLElement;
  setSelfTest(st: HealthSelfTest | undefined): void;
  refresh(): Promise<void>;
  stop(): void;
}

/** Ignore uptime/sentinel values persisted before wall-clock stamping was fixed. */
const MIN_WALL_EPOCH_SEC = 1_000_000_000;

function formatSelfTestLastRunWhen(epoch: number | undefined): string | null {
  if (!epoch || epoch < MIN_WALL_EPOCH_SEC) return null;
  try {
    return new Date(epoch * 1000).toLocaleString();
  } catch {
    return null;
  }
}

function buildFailAlert(
  title: string,
  body: string,
  severity: "critical" | "warning" = "critical",
): HTMLElement {
  const cls = severity === "warning" ? "alert--warn" : "alert--danger";
  return h(
    "div",
    { class: `alert ${cls}`, role: "alert" },
    h(
      "div",
      { class: "alert__head" },
      h("span", { class: "alert__icon" }, icon("alert")),
      h("strong", { class: "alert__title" }, title),
    ),
    h("p", { class: "alert__body" }, body),
  );
}

export function buildSelfTestPanel(opts: { signal: AbortSignal }): SelfTestPanelHandle {
  const T = getStrings();
  let selfTest: HealthSelfTest | undefined;
  let pollHandle: PollHandle | undefined;

  const card = h("section", { class: "card" }, h("h2", { class: "card__title" }, T.diag.selfTestTitle));
  const body = h("div", { class: "alert-stack" });
  card.append(body);

  async function refresh() {
    const hinfo = await api.getHealth({ signal: opts.signal });
    selfTest = hinfo.self_test;
    if (selfTest?.running) {
      if (!pollHandle) startFastPoll();
    } else {
      stopFastPoll();
    }
    render();
  }

  function stopFastPoll() {
    pollHandle?.stop();
    pollHandle = undefined;
  }

  function startFastPoll() {
    stopFastPoll();
    pollHandle = poll(
      async () => {
        await refresh();
        if (!selfTest?.running) stopFastPoll();
      },
      500,
      4000,
    );
  }

  async function runTest() {
    try {
      await api.postHealthSelfTestRun({ signal: opts.signal });
      toast(T.saved, "success");
      startFastPoll();
      await refresh();
    } catch {
      toast(T.saveError, "error");
    }
  }

  async function skipTest() {
    try {
      await api.postHealthSelfTestSkip({ signal: opts.signal });
      toast(T.saved, "success");
      await refresh();
    } catch {
      toast(T.saveError, "error");
    }
  }

  function render() {
    body.replaceChildren();
    const st = selfTest;
    if (!st) return;

    if (st.running) {
      body.append(
        h(
          "div",
          { class: "alert alert--warn alert--loading", role: "status" },
          h(
            "div",
            { class: "alert__head" },
            h("span", { class: "alert__icon" }, icon("alert")),
            h("strong", { class: "alert__title" }, T.diag.selfTestRunning),
          ),
        ),
      );
      return;
    }

    if (st.pending) {
      const alert = h("div", { class: "alert alert--info", role: "status" });
      alert.append(
        h("p", { class: "alert__body" }, T.diag.selfTestPending),
        h("p", { class: "alert__body" }, T.diag.selfTestResistiveWarn),
        h(
          "div",
          { class: "alert__actions" },
          h(
            "button",
            { type: "button", class: "btn btn--primary btn--sm", onClick: () => void runTest() },
            T.diag.selfTestRun,
          ),
          h(
            "button",
            { type: "button", class: "btn btn--ghost btn--sm", onClick: () => void skipTest() },
            T.diag.selfTestSkip,
          ),
        ),
      );
      body.append(alert);
      return;
    }

    if (st.safety_lockout_active) {
      body.append(
        buildFailAlert(T.diag.selfTestLockoutActive, T.home.safetyLockoutBanner, "critical"),
      );
    }

    const r = st.results;
    const sev = st.severity;
    const lastRunWhen = formatSelfTestLastRunWhen(st.last_run_epoch);
    const fails: HTMLElement[] = [];
    if (r) {
      if (!r.zc_ok) {
        const critical = sev?.zc === "critical";
        fails.push(
          buildFailAlert(
            T.diag.selfTestZc,
            critical
              ? T.diag.selfTestCriticalZc
              : T.diag.selfTestZcHint.replace("{rate}", String(r.zc_edges_per_sec ?? 0)),
            critical ? "critical" : "warning",
          ),
        );
      }
      if (!r.triac_ok) {
        const critical = sev?.triac === "critical";
        fails.push(
          buildFailAlert(
            T.diag.selfTestTriac,
            critical ? T.diag.selfTestCriticalTriac : T.diag.selfTestTriacFail,
            critical ? "critical" : "warning",
          ),
        );
      }
      if (!r.source_ok) {
        const critical = sev?.source === "critical";
        fails.push(
          buildFailAlert(
            T.diag.selfTestSource,
            critical ? T.diag.selfTestCriticalSource : T.diag.selfTestSourceFail,
            critical ? "critical" : "warning",
          ),
        );
      }
    }

    if (fails.length === 0 && r) {
      body.append(
        h(
          "div",
          { class: "alert alert--success", role: "status" },
          h(
            "div",
            { class: "alert__head" },
            h("strong", { class: "alert__title" }, T.diag.selfTestAllPass),
            lastRunWhen
              ? h(
                  "span",
                  { class: "alert__meta" },
                  T.diag.selfTestLastRun.replace("{when}", lastRunWhen),
                )
              : null,
          ),
          r.zc_ok
            ? h(
                "p",
                { class: "alert__body" },
                `${T.diag.selfTestZc}: ${T.diag.selfTestPass} (${r.zc_edges_per_sec ?? 0}/s)`,
              )
            : null,
        ),
      );
    } else {
      body.append(...fails);
      if (lastRunWhen) {
        body.append(
          h("p", { class: "field__hint" }, T.diag.selfTestLastRun.replace("{when}", lastRunWhen)),
        );
      }
    }

    const actions = h("div", { class: "alert__actions", style: "margin-top:8px;" });
    actions.append(
      h(
        "button",
        {
          type: "button",
          class: `btn btn--sm${st.safety_lockout_active ? " btn--primary" : " btn--ghost"}`,
          onClick: () => void runTest(),
        },
        st.safety_lockout_active ? T.diag.selfTestRun : T.diag.selfTestRerun,
      ),
    );
    if (!st.safety_lockout_active) {
      actions.append(
        h(
          "button",
          { type: "button", class: "btn btn--ghost btn--sm", onClick: () => void skipTest() },
          T.diag.selfTestSkip,
        ),
      );
    }
    body.append(actions);
  }

  return {
    el: card,
    setSelfTest(st) {
      selfTest = st;
      if (st?.running && !pollHandle) startFastPoll();
      render();
    },
    refresh,
    stop() {
      stopFastPoll();
    },
  };
}

import { api, ApiError } from "../api/client";
import { buildFieldLabelRow } from "../components/FieldHelp";
import { toast } from "../components/Toast";
import { getStrings } from "../i18n";
import { isUnsetIp } from "../utils/networkIp";
import { buildPageHeader } from "../components/ui/pageHeader";
import { h } from "../utils/dom";
import type { WifiInfo } from "../api/types";

export interface WifiFormFetchOpts {
  signal: AbortSignal;
  omitAuth: boolean;
}

export interface MountWifiFormOpts {
  outlet: HTMLElement;
  signal: AbortSignal;
  fetchOpts: WifiFormFetchOpts;
  /** AP captive portal: open API, minimal UI. */
  apMode: boolean;
  title: string;
  description: string;
}

export async function mountWifiForm(opts: MountWifiFormOpts): Promise<void> {
  const { outlet, signal: _signal, fetchOpts, apMode, title, description } = opts;
  if (apMode) {
    await mountApWifiForm({ outlet, fetchOpts, title, description });
    return;
  }
  await mountStationWifiForm({ outlet, fetchOpts, title, description });
}

async function mountApWifiForm(opts: {
  outlet: HTMLElement;
  fetchOpts: WifiFormFetchOpts;
  title: string;
  description: string;
}): Promise<void> {
  const { outlet, fetchOpts, title, description } = opts;
  const T = getStrings();

  outlet.append(buildPageHeader({ title, description }));

  const form = h("form", { class: "form", onSubmit: (e) => e.preventDefault() });
  outlet.append(form);

  const scanBtn = h(
    "button",
    { type: "button", class: "btn btn--ghost" },
    T.wifi.scanBtn,
  );
  const scanStatus = h("p", { class: "empty", hidden: true }, "");
  const networksWrap = h("div", { class: "form__actions", hidden: true });

  const ssidInput = h("input", {
    type: "text",
    name: "ssid",
    class: "field__input",
    autocomplete: "off",
    required: true,
  }) as HTMLInputElement;
  ssidInput.id = "wifi_ssid";

  const pwdInput = h("input", {
    type: "password",
    name: "password",
    class: "field__input",
    autocomplete: "new-password",
  }) as HTMLInputElement;
  pwdInput.id = "wifi_password";

  const submitBtn = h("button", { type: "submit", class: "btn btn--primary" }, T.wifi.connectBtn);
  const doneMsg = h("p", { class: "card__sub", hidden: true, "aria-live": "polite" }, "");

  form.append(
    h("div", { class: "form__actions" }, scanBtn),
    scanStatus,
    networksWrap,
    h(
      "div",
      { class: "field" },
      h("label", { class: "field__label", for: "wifi_ssid" }, T.wifi.ssidLabel),
      ssidInput,
    ),
    h(
      "div",
      { class: "field" },
      h("label", { class: "field__label", for: "wifi_password" }, T.wifi.passwordLabel),
      pwdInput,
    ),
    h("div", { class: "form__actions" }, submitBtn),
    doneMsg,
  );

  let scanning = false;
  scanBtn.addEventListener("click", async () => {
    if (scanning) return;
    scanning = true;
    scanBtn.setAttribute("disabled", "true");
    scanStatus.hidden = false;
    scanStatus.textContent = T.wifi.scanning;
    networksWrap.hidden = true;
    networksWrap.replaceChildren();
    try {
      const res = await api.pollWifiScan({
        ...fetchOpts,
        maxWaitMs: 20_000,
        pollIntervalMs: 400,
      });
      if (res.networks.length === 0) {
        scanStatus.textContent = T.wifi.scanEmpty;
      } else {
        scanStatus.hidden = true;
        networksWrap.hidden = false;
        for (const n of res.networks) {
          const pick = h(
            "button",
            {
              type: "button",
              class: "btn btn--ghost",
            },
            `${n.ssid || T.wifi.hiddenSsid} (${n.rssi})`,
          );
          pick.addEventListener("click", () => {
            ssidInput.value = n.ssid;
            ssidInput.focus();
          });
          networksWrap.append(pick);
        }
      }
    } catch (e) {
      if ((e as DOMException)?.name === "AbortError") return;
      const msg =
        e instanceof ApiError && e.status === 408 ? T.wifi.scanTimeout : T.wifi.scanError;
      scanStatus.textContent = msg;
      toast(msg, "error");
    } finally {
      scanning = false;
      scanBtn.removeAttribute("disabled");
    }
  });

  form.addEventListener("submit", async (ev) => {
    ev.preventDefault();
    const ssid = ssidInput.value.trim();
    if (!ssid) {
      toast(T.wifi.ssidRequired, "error");
      return;
    }
    submitBtn.setAttribute("disabled", "true");
    doneMsg.hidden = true;
    try {
      await api.putWifi(
        {
          ssid,
          password: pwdInput.value,
          persist: true,
        },
        fetchOpts,
      );
      doneMsg.hidden = false;
      doneMsg.textContent = T.wifi.afterConnect;
      toast(T.wifi.savedOk, "success");
    } catch (e) {
      if ((e as DOMException)?.name === "AbortError") return;
      const networkLost =
        e instanceof TypeError ||
        (e instanceof Error && /failed to fetch|networkerror|load failed/i.test(e.message));
      if (networkLost) {
        doneMsg.hidden = false;
        doneMsg.textContent = T.wifi.saveApLikelyOk;
        toast(T.wifi.saveApLikelyOk, "success");
        return;
      }
      if (e instanceof ApiError) {
        toast(e.status === 401 ? T.wifi.saveAuthRequired : T.saveError, "error");
        return;
      }
      toast(T.saveError, "error");
    } finally {
      submitBtn.removeAttribute("disabled");
    }
  });
}

async function mountStationWifiForm(opts: {
  outlet: HTMLElement;
  fetchOpts: WifiFormFetchOpts;
  title: string;
  description: string;
}): Promise<void> {
  const { outlet, fetchOpts, title, description } = opts;
  const T = getStrings();

  outlet.append(buildPageHeader({ title, description }));

  const statusLine = h("p", { class: "card__sub" }, T.loading);
  outlet.append(statusLine);

  let wifiInfo: WifiInfo | null = null;
  try {
    wifiInfo = await api.getWifi(fetchOpts);
  } catch (e) {
    if ((e as DOMException)?.name === "AbortError") return;
    statusLine.textContent = T.status.error;
    toast(T.wifi.loadError, "error");
    return;
  }

  statusLine.textContent = formatWifiStatus(T, wifiInfo);

  if (wifiInfo.connected && !wifiInfo.ssid?.trim()) {
    try {
      const sys = await api.getSystem(fetchOpts);
      if (sys.ssid?.trim()) {
        wifiInfo = { ...wifiInfo, ssid: sys.ssid.trim() };
      }
    } catch {
      /* keep empty ssid */
    }
  }

  const form = h("form", { class: "form", onSubmit: (e) => e.preventDefault() });
  outlet.append(form);

  const scanBtn = h(
    "button",
    { type: "button", class: "btn btn--ghost" },
    T.wifi.scanBtn,
  );
  const scanStatus = h("p", { class: "empty", hidden: true }, "");
  const networksWrap = h("div", { class: "table-wrap", hidden: true });
  const networksBody = h("tbody", {});
  networksWrap.append(
    h(
      "table",
      { class: "table" },
      h(
        "thead",
        {},
        h("tr", {}, h("th", {}, T.wifi.colSsid), h("th", {}, T.wifi.colRssi), h("th", {}, "")),
      ),
      networksBody,
    ),
  );

  const ssidInput = h("input", {
    type: "text",
    name: "ssid",
    class: "field__input",
    autocomplete: "off",
    required: true,
    value: wifiInfo.ssid || "",
  }) as HTMLInputElement;
  ssidInput.id = "wifi_ssid";

  const pwdInput = h("input", {
    type: "password",
    name: "password",
    class: "field__input",
    autocomplete: "new-password",
  }) as HTMLInputElement;
  pwdInput.id = "wifi_password";

  const persistInput = h("input", {
    type: "checkbox",
    checked: true,
  }) as HTMLInputElement;

  const persistRow = h(
    "label",
    { class: "switch" },
    persistInput,
    h("span", { class: "switch__track", "aria-hidden": "true" }),
    h("span", {}, T.wifi.persistEeprom),
  );

  const submitBtn = h("button", { type: "submit", class: "btn btn--primary" }, T.wifi.connectBtn);
  const doneMsg = h("p", { class: "card__sub", hidden: true, "aria-live": "polite" }, "");

  form.append(
    h("div", { class: "form__actions" }, scanBtn),
    scanStatus,
    networksWrap,
    h(
      "div",
      { class: "field" },
      buildFieldLabelRow({
        label: T.wifi.ssidLabel,
        forId: "wifi_ssid",
        helpScope: "wifi",
        helpKey: "wifi_ssid",
      }),
      ssidInput,
    ),
    h(
      "div",
      { class: "field" },
      buildFieldLabelRow({
        label: T.wifi.passwordLabel,
        forId: "wifi_password",
        helpScope: "wifi",
        helpKey: "wifi_password",
      }),
      pwdInput,
    ),
    persistRow,
    h("div", { class: "form__actions" }, submitBtn),
    doneMsg,
  );

  let scanning = false;
  scanBtn.addEventListener("click", async () => {
    if (scanning) return;
    scanning = true;
    scanBtn.setAttribute("disabled", "true");
    scanStatus.hidden = false;
    scanStatus.textContent = T.wifi.scanning;
    networksWrap.hidden = true;
    networksBody.replaceChildren();
    try {
      const res = await api.pollWifiScan({
        ...fetchOpts,
        maxWaitMs: 20_000,
        pollIntervalMs: 400,
      });
      scanStatus.textContent = T.wifi.scanDone;
      if (res.networks.length === 0) {
        scanStatus.textContent = T.wifi.scanEmpty;
      } else {
        networksWrap.hidden = false;
        for (const n of res.networks) {
          const pick = h(
            "button",
            {
              type: "button",
              class: "btn btn--ghost",
            },
            T.wifi.useNetwork,
          );
          pick.addEventListener("click", () => {
            ssidInput.value = n.ssid;
          });
          networksBody.append(
            h(
              "tr",
              {},
              h("td", {}, n.ssid || T.wifi.hiddenSsid),
              h("td", {}, String(n.rssi)),
              h("td", {}, pick),
            ),
          );
        }
      }
    } catch (e) {
      if ((e as DOMException)?.name === "AbortError") return;
      const msg =
        e instanceof ApiError && e.status === 408
          ? T.wifi.scanTimeout
          : T.wifi.scanError;
      scanStatus.textContent = msg;
      toast(msg, "error");
    } finally {
      scanning = false;
      scanBtn.removeAttribute("disabled");
    }
  });

  form.addEventListener("submit", async (ev) => {
    ev.preventDefault();
    const ssid = ssidInput.value.trim();
    if (!ssid) {
      toast(T.wifi.ssidRequired, "error");
      return;
    }
    submitBtn.setAttribute("disabled", "true");
    doneMsg.hidden = true;
    try {
      await api.putWifi(
        {
          ssid,
          password: pwdInput.value,
          persist: persistInput.checked,
        },
        fetchOpts,
      );
      doneMsg.hidden = false;
      doneMsg.textContent = T.wifi.afterConnect;
      toast(T.wifi.savedOk, "success");
    } catch (e) {
      if ((e as DOMException)?.name === "AbortError") return;
      if (e instanceof ApiError) {
        const msg = e.status === 401 ? T.wifi.saveAuthRequired : T.saveError;
        toast(msg, "error");
        return;
      }
      toast(T.saveError, "error");
    } finally {
      submitBtn.removeAttribute("disabled");
    }
  });
}

function formatWifiStatus(
  T: ReturnType<typeof getStrings>,
  w: { mode: string; connected: boolean; ssid: string; ip: string; rssi: number },
): string {
  const modeLabel = w.mode === "ap" ? T.wifi.modeAp : T.wifi.modeSta;
  const conn = w.connected ? T.wifi.connectedYes : T.wifi.connectedNo;
  const ip = isUnsetIp(w.ip) ? "—" : w.ip;
  return `${modeLabel} · ${conn} · ${T.wifi.currentSsid}: ${w.ssid || "—"} · IP: ${ip} · RSSI: ${w.rssi}`;
}

import type { RouteCtx } from "../router";
import { h } from "../utils/dom";
import { api, ApiError } from "../api/client";
import { isBrowserNetworkFailure } from "../api/networkFailure";
import { toast } from "../components/Toast";
import { openDialog } from "../components/Dialog";
import { getStrings } from "../i18n";
import { fmtBytes } from "../utils/format";
import { formatFirmwareVersionFull } from "../firmware/versionCompare";
import { buildFieldLabelRow } from "../components/FieldHelp";
import { buildFirmwareUpdateCard } from "./firmwareUpdateCard";
import { buildPageHeader } from "../components/ui/pageHeader";
import { kvRow } from "./firmwareUi";

const FH = { helpScope: "firmware" as const };

function apiErrMessage(e: unknown): string {
  if (isBrowserNetworkFailure(e)) return "";
  if (e instanceof ApiError) {
    const b = e.body;
    if (b && typeof b === "object" && "message" in b) {
      const m = (b as { message: unknown }).message;
      if (typeof m === "string" && m.trim()) {
        if (/failed to fetch|networkerror|load failed/i.test(m)) return "";
        return m;
      }
    }
  }
  return "";
}

export async function mountFirmware(ctx: RouteCtx) {
  const { outlet, signal } = ctx;
  const T = getStrings();

  outlet.append(buildPageHeader({ title: T.firmware.title, description: T.firmware.subtitle }));

  const statusLine = h("p", { class: "empty" }, T.loading);
  outlet.append(statusLine);

  let device: Awaited<ReturnType<typeof api.getDevice>>;
  let system: Awaited<ReturnType<typeof api.getSystem>>;
  let otaInfo: Awaited<ReturnType<typeof api.getArduinoOta>>;
  try {
    [device, system, otaInfo] = await Promise.all([
      api.getDevice({ signal }),
      api.getSystem({ signal }),
      api.getArduinoOta({ signal }),
    ]);
  } catch (e) {
    if ((e as DOMException)?.name === "AbortError") return;
    statusLine.textContent = T.status.error;
    toast(T.firmware.loadError, "error");
    return;
  }

  statusLine.remove();

  const fw = device.firmware;
  const freeSpace = fw?.free_sketch_space;
  const infoCard = h(
    "section",
    { class: "card" },
    h("h2", { class: "section__title" }, T.firmware.infoTitle),
    kvRow(T.firmware.currentVersion, formatFirmwareVersionFull(device.firmware_version)),
    fw?.sketch_md5 ? kvRow(T.firmware.sketchMd5, fw.sketch_md5) : null,
    typeof freeSpace === "number"
      ? kvRow(T.firmware.freeSpace, fmtBytes(freeSpace))
      : null,
  );
  outlet.append(infoCard);

  const firmwareUpdateUi = buildFirmwareUpdateCard(T);
  outlet.append(firmwareUpdateUi.section);

  const fileInput = h("input", {
    type: "file",
    accept: ".bin,application/octet-stream",
    class: "field__input",
  }) as HTMLInputElement;
  fileInput.id = "fw_file";

  const md5Input = h("input", {
    type: "text",
    class: "field__input",
    placeholder: "0123456789abcdef0123456789abcdef",
    maxLength: 32,
    spellcheck: false,
    autocomplete: "off",
  }) as HTMLInputElement;
  md5Input.id = "fw_md5";

  const sizeWarn = h("p", {
    class: "card__sub",
    hidden: true,
    role: "alert",
  });

  const rebootNote = h("p", { class: "card__sub", hidden: true, "aria-live": "polite" }, "");

  const submitBtn = h(
    "button",
    { type: "submit", class: "btn btn--primary" },
    T.firmware.uploadBtn,
  ) as HTMLButtonElement;

  const form = h(
    "form",
    { class: "form", onSubmit: (e) => e.preventDefault() },
    h("h2", { class: "section__title" }, T.firmware.uploadSectionTitle),
    h(
      "div",
      { class: "field" },
      buildFieldLabelRow({ label: T.firmware.fileLabel, forId: "fw_file", ...FH, helpKey: "fw_file" }),
      fileInput,
    ),
    h(
      "div",
      { class: "field" },
      buildFieldLabelRow({ label: T.firmware.md5Label, forId: "fw_md5", ...FH, helpKey: "fw_md5" }),
      md5Input,
    ),
    h("p", { class: "card__sub" }, T.firmware.md5Hint),
    sizeWarn,
    h("div", { class: "form__actions" }, submitBtn),
    rebootNote,
  );
  outlet.append(h("section", { class: "card" }, form));

  function refreshSizeWarn() {
    const f = fileInput.files?.[0];
    if (!f || typeof freeSpace !== "number") {
      sizeWarn.hidden = true;
      return;
    }
    sizeWarn.hidden = f.size <= freeSpace;
    sizeWarn.textContent = T.firmware.sizeWarn;
  }

  fileInput.addEventListener("change", refreshSizeWarn);

  const routerName = (device.router_name || "").trim();
  const mDnsDisplay = routerName.length > 0 ? T.firmware.mDnsHint.replace("{name}", routerName) : "";

  const ideCard = h(
    "section",
    { class: "card" },
    h("h2", { class: "section__title" }, T.firmware.ideTitle),
    h("p", { class: "card__sub" }, T.firmware.ideBody),
    kvRow(T.firmware.deviceIp, system.ip || "—"),
    routerName ? h("p", { class: "card__sub" }, mDnsDisplay) : null,
  );
  outlet.append(ideCard);

  let otaPasswordSet = otaInfo.password_set;
  const otaStatusValue = h(
    "p",
    { class: "card__sub", style: "margin:0;" },
    otaPasswordSet ? T.firmware.otaIdePassSet : T.firmware.otaIdePassUnset,
  );
  const otaPwdNew = h("input", {
    type: "password",
    class: "field__input",
    autocomplete: "new-password",
  }) as HTMLInputElement;
  otaPwdNew.id = "fw_ota_new";
  const otaPwdConfirm = h("input", {
    type: "password",
    class: "field__input",
    autocomplete: "new-password",
  }) as HTMLInputElement;
  otaPwdConfirm.id = "fw_ota_confirm";
  const otaSaveNote = h("p", { class: "card__sub", hidden: true, "aria-live": "polite" }, "");

  const otaSaveBtn = h("button", { type: "button", class: "btn btn--primary" }, T.firmware.otaIdeSave) as HTMLButtonElement;
  const otaClearBtn = h("button", { type: "button", class: "btn btn--ghost" }, T.firmware.otaIdeClear) as HTMLButtonElement;

  function syncOtaStatusLabel() {
    otaStatusValue.textContent = otaPasswordSet ? T.firmware.otaIdePassSet : T.firmware.otaIdePassUnset;
  }

  async function applyIdeOtaPassword(password: string) {
    otaSaveBtn.setAttribute("disabled", "true");
    otaClearBtn.setAttribute("disabled", "true");
    otaSaveNote.hidden = true;
    try {
      await api.putArduinoOtaPassword(password, { signal });
      otaPasswordSet = password.length > 0;
      syncOtaStatusLabel();
      otaPwdNew.value = "";
      otaPwdConfirm.value = "";
      otaSaveNote.hidden = false;
      otaSaveNote.textContent = T.firmware.otaIdeRebootNote;
      toast(T.firmware.otaIdeSaveOk, "success");
    } catch (e) {
      if ((e as DOMException)?.name === "AbortError") return;
      const detail = apiErrMessage(e);
      toast(detail ? `${T.firmware.otaIdeSaveError}: ${detail}` : T.firmware.otaIdeSaveError, "error");
    } finally {
      otaSaveBtn.removeAttribute("disabled");
      otaClearBtn.removeAttribute("disabled");
    }
  }

  otaSaveBtn.addEventListener("click", async () => {
    const a = otaPwdNew.value;
    const b = otaPwdConfirm.value;
    if (a !== b) {
      toast(T.firmware.otaIdeMismatch, "error");
      return;
    }
    await applyIdeOtaPassword(a);
  });

  otaClearBtn.addEventListener("click", () => {
    openDialog({
      title: T.firmware.otaIdeClear,
      body: T.firmware.otaIdeClearConfirm,
      actions: [
        {
          label: T.cancel,
          kind: "ghost",
          onClick: () => {},
          closeOnClick: true,
        },
        {
          label: T.confirm,
          kind: "danger",
          onClick: async () => {
            await applyIdeOtaPassword("");
          },
          closeOnClick: true,
        },
      ],
    });
  });

  const otaPassCard = h(
    "section",
    { class: "card" },
    h("h2", { class: "section__title" }, T.firmware.otaIdePassSection),
    h("p", { class: "card__sub" }, T.firmware.otaIdePassHelp),
    h("div", { class: "field" }, h("span", { class: "field__label" }, T.firmware.otaIdePassStatus), otaStatusValue),
    h(
      "div",
      { class: "field" },
      buildFieldLabelRow({ label: T.firmware.otaIdeNew, forId: "fw_ota_new", ...FH, helpKey: "ota_new" }),
      otaPwdNew,
    ),
    h(
      "div",
      { class: "field" },
      buildFieldLabelRow({
        label: T.firmware.otaIdeConfirm,
        forId: "fw_ota_confirm",
        ...FH,
        helpKey: "ota_confirm",
      }),
      otaPwdConfirm,
    ),
    h("div", { class: "form__actions" }, otaSaveBtn, otaClearBtn),
    otaSaveNote,
  );
  outlet.append(otaPassCard);

  form.addEventListener("submit", async (ev) => {
    ev.preventDefault();
    const file = fileInput.files?.[0];
    if (!file) {
      toast(T.firmware.fileRequired, "error");
      return;
    }
    const md5Raw = md5Input.value.trim();
    const md5 = /^[0-9a-fA-F]{32}$/.test(md5Raw) ? md5Raw : undefined;
    submitBtn.setAttribute("disabled", "true");
    submitBtn.textContent = T.firmware.uploading;
    rebootNote.hidden = true;
    try {
      await api.postFirmwareOta(file, {
        signal,
        md5,
      });
      rebootNote.hidden = false;
      rebootNote.textContent = T.firmware.rebootNote;
      toast(T.firmware.successToast, "success");
    } catch (e) {
      if ((e as DOMException)?.name === "AbortError") return;
      const detail = apiErrMessage(e);
      toast(detail ? `${T.firmware.uploadError}: ${detail}` : T.firmware.uploadError, "error");
    } finally {
      submitBtn.removeAttribute("disabled");
      submitBtn.textContent = T.firmware.uploadBtn;
    }
  });

  return () => {
    firmwareUpdateUi.destroy();
  };
}

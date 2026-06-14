import type { RouteCtx } from "../router";
import { h } from "../utils/dom";
import { buildSectionTitleWithHelp } from "../components/FieldHelp";
import { icon } from "../utils/icons";
import { api } from "../api/client";
import { toast } from "../components/Toast";
import { getStrings } from "../i18n";
import { withBase } from "../paths";
import {
  backupDownloadFilename,
  backupPartFilename,
  downloadJsonFile,
  type BalansunBackup,
} from "../utils/backupFormat";
import { confirmRestoreBackupFromFile, confirmRestoreBackupFromFiles } from "../utils/backupApply";
import type { RouterConfig } from "../api/types";
import { mergePinFieldsIntoConfig } from "../api/pinMapMerge";
import { adaptBackupForExport } from "../utils/adaptBackupForTarget";
import { buildPageHeader } from "../components/ui/pageHeader";
import { deviceInfo } from "../state/store";

export async function mountBackup(ctx: RouteCtx) {
  const { outlet, signal } = ctx;
  const T = getStrings();

  let caps = deviceInfo.get()?.capabilities;
  if (!caps) {
    try {
      const d = await api.getDevice({ signal });
      caps = d.capabilities;
      deviceInfo.set({
        router_name: d.router_name,
        firmware_version: d.firmware_version,
        probe_house_name: d.probe_house_name,
        probe_second_name: d.probe_second_name,
        temperature_label: d.temperature_label,
        capabilities: d.capabilities,
      });
    } catch {
      // Continue without caps — monolithic export fallback.
    }
  }

  const useParts = caps?.backup_export_mode === "parts";

  outlet.append(
    buildPageHeader({
      title: T.backup.title,
      actions: [
        h(
          "a",
          {
            href: withBase("/settings/general"),
            class: "btn btn--ghost",
            "data-route": "true",
          },
          T.backup.backToSettings,
        ),
      ],
    }),
  );

  outlet.append(
    h(
      "section",
      { class: "card" },
      h("p", {}, useParts ? T.backup.introParts : T.backup.intro),
      useParts
        ? h("p", { class: "field__hint" }, T.backup.tierHintConstrained)
        : caps?.full_config_export === false
          ? h("p", { class: "field__hint" }, T.backup.tierHintSparse)
          : null,
    ),
  );

  outlet.append(
    h(
      "section",
      { class: "card" },
      buildSectionTitleWithHelp(T.backup.sectionSecurity, "backup", "sectionSecurity"),
      h(
        "p",
        { class: "field__hint", style: "display:flex;gap:10px;align-items:flex-start;" },
        icon("alert"),
        h("span", {}, T.backup.security),
      ),
    ),
  );

  const exportBtn = h(
    "button",
    {
      type: "button",
      class: "btn btn--primary",
      onClick: () => void doExport(),
    },
    icon("download"),
    h("span", {}, useParts ? T.backup.exportPartsBtn : T.backup.exportBtn),
  );

  outlet.append(
    h(
      "section",
      { class: "card" },
      buildSectionTitleWithHelp(T.backup.sectionExport, "backup", "sectionExport"),
      exportBtn,
    ),
  );

  const fileInput = h("input", {
    type: "file",
    accept: "application/json,.json",
    hidden: true,
    multiple: useParts,
  }) as HTMLInputElement;

  fileInput.addEventListener("change", () => {
    const list = fileInput.files;
    fileInput.value = "";
    if (!list?.length) return;
    if (useParts && list.length > 1) {
      void confirmRestoreBackupFromFiles(Array.from(list), signal);
      return;
    }
    const f = list[0];
    if (f) void confirmRestoreBackupFromFile(f, signal);
  });

  const importBtn = h(
    "button",
    {
      type: "button",
      class: "btn btn--ghost",
      onClick: () => fileInput.click(),
    },
    icon("upload"),
    h("span", {}, useParts ? T.backup.importPartsBtn : T.backup.importBtn),
  );

  outlet.append(
    h(
      "section",
      { class: "card" },
      buildSectionTitleWithHelp(T.backup.sectionImport, "backup", "sectionImport"),
      fileInput,
      importBtn,
      useParts ? h("p", { class: "field__hint" }, T.backup.importPartsHint) : null,
    ),
  );

  async function doExport() {
    try {
      if (useParts) {
        await api.getSystemBackupManifest({ signal });
        const parts = ["network", "config", "actions"] as const;
        for (const part of parts) {
          const doc = await api.getSystemBackupPart(part, { signal });
          downloadJsonFile(backupPartFilename(part), doc as BalansunBackup);
        }
        toast(T.backup.exportPartsDone, "success");
        return;
      }
      const [doc, pins] = await Promise.all([
        api.getSystemBackup({ signal }),
        api.getSystemPins({ signal }),
      ]);
      const merged = {
        ...(doc as BalansunBackup),
        config: mergePinFieldsIntoConfig(
          (doc as BalansunBackup).config ?? ({} as RouterConfig),
          pins,
        ),
      };
      const adapted = adaptBackupForExport(merged, caps, pins);
      downloadJsonFile(backupDownloadFilename(), adapted);
    } catch (e) {
      if ((e as DOMException)?.name === "AbortError") return;
      console.error(e);
      toast(T.backup.exportError, "error");
    }
  }
}

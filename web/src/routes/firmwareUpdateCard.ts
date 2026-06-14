import { h } from "../utils/dom";
import { toast } from "../components/Toast";
import { deviceInfo } from "../state/store";
import { firmwareUpdate } from "../state/firmwareUpdate";
import {
  firmwareUpdateStatusMessage,
  githubUpdateCheckErrorMessage,
  GITHUB_RELEASES_PAGE,
  hydrateFirmwareUpdateFromStorage,
  runFirmwareUpdateCheck,
} from "../firmware/githubDailyCheck";
import { loadStoredChannel, storeChannel } from "../firmware/githubRelease";
import { formatFirmwareVersionFull } from "../firmware/versionCompare";
import type { AppStrings } from "../i18n";

export function buildFirmwareUpdateCard(T: AppStrings): {
  section: HTMLElement;
  refresh: () => void;
  destroy: () => void;
} {
  const statusEl = h("p", { class: "card__sub", "aria-live": "polite" }, "");
  const checkBtn = h(
    "button",
    { type: "button", class: "btn btn--primary" },
    T.firmware.forceCheckBtn,
  ) as HTMLButtonElement;

  storeChannel(loadStoredChannel());

  function refresh(): void {
    const fw = firmwareUpdate.get();
    const version = deviceInfo.get()?.firmware_version ?? "0";
    statusEl.textContent = firmwareUpdateStatusMessage(fw, T.firmware.updateStatus, version);
    checkBtn.toggleAttribute("disabled", fw.checking);
  }

  async function runCheck(force: boolean): Promise<void> {
    const outcome = await runFirmwareUpdateCheck({ force });
    refresh();
    if (!force && !outcome.ok && outcome.error instanceof Error && outcome.error.message === "skipped_daily") {
      return;
    }
    if (outcome.ok) {
      const fw = firmwareUpdate.get();
      if (fw.available) {
        toast(
          T.firmware.updateStatus.available
            .replace(/\{current\}/g, formatFirmwareVersionFull(deviceInfo.get()?.firmware_version) ?? "")
            .replace(/\{release\}/g, formatFirmwareVersionFull(fw.releaseTag)),
          "success",
        );
      } else {
        toast(T.firmware.updateToastUpToDate, "info");
      }
      return;
    }
    if (outcome.error instanceof Error && outcome.error.message === "skipped_daily") return;
    const msg = githubUpdateCheckErrorMessage(outcome.error, T.firmware.updateErrors);
    toast(msg, "error");
    statusEl.textContent = msg;
  }

  checkBtn.addEventListener("click", () => void runCheck(true));

  hydrateFirmwareUpdateFromStorage();
  const unsubFirmware = firmwareUpdate.subscribe(() => refresh());
  refresh();

  const section = h(
    "section",
    { class: "card" },
    h("h2", { class: "section__title" }, T.firmware.updatesTitle),
    h("p", { class: "card__sub" }, T.firmware.updatesBody),
    h("p", { class: "card__sub" }, T.firmware.updatesHint),
    h("p", { class: "card__sub" }, T.firmware.githubChannelNightlyNote),
    h(
      "p",
      { class: "card__sub" },
      h(
        "a",
        {
          href: GITHUB_RELEASES_PAGE,
          target: "_blank",
          rel: "noopener noreferrer",
        },
        T.firmware.githubReleasesLink,
      ),
    ),
    statusEl,
    h("div", { class: "form__actions" }, checkBtn),
  );

  return {
    section,
    refresh,
    destroy: () => {
      unsubFirmware();
    },
  };
}

import { h } from "../utils/dom";
import { getStrings } from "../i18n";
import { localePref } from "../state/store";

const DISMISS_KEY = "balansun_install_banner_dismissed";

type BeforeInstallPromptEvent = Event & {
  prompt: () => Promise<void>;
  userChoice: Promise<{ outcome: "accepted" | "dismissed" }>;
};

let deferredInstallPrompt: BeforeInstallPromptEvent | null = null;

export function setDeferredInstallPrompt(event: BeforeInstallPromptEvent | null): void {
  deferredInstallPrompt = event;
}

export function getDeferredInstallPrompt(): BeforeInstallPromptEvent | null {
  return deferredInstallPrompt;
}

export function isStandaloneDisplayMode(): boolean {
  if (typeof window === "undefined") return false;
  const nav = navigator as Navigator & { standalone?: boolean };
  if (nav.standalone) return true;
  return window.matchMedia("(display-mode: standalone), (display-mode: fullscreen)").matches;
}

export function isMobileInstallTarget(): boolean {
  if (typeof window === "undefined") return false;
  const coarse = window.matchMedia("(pointer: coarse)").matches;
  const narrow = window.matchMedia("(max-width: 720px)").matches;
  const ua = navigator.userAgent || "";
  const mobileUa = /Android|iPhone|iPad|iPod|Mobile/i.test(ua);
  return mobileUa || (coarse && narrow);
}

export function isInstallBannerDismissed(storage: Storage = localStorage): boolean {
  try {
    return storage.getItem(DISMISS_KEY) === "1";
  } catch {
    return false;
  }
}

export function dismissInstallBanner(storage: Storage = localStorage): void {
  try {
    storage.setItem(DISMISS_KEY, "1");
  } catch {
    /* ignore */
  }
}

function installHintText(): string {
  const T = getStrings();
  const ua = navigator.userAgent || "";
  if (/iPhone|iPad|iPod/i.test(ua)) return T.installBanner.iosHint;
  if (/Android/i.test(ua)) return T.installBanner.androidHint;
  return T.installBanner.genericHint;
}

export function shouldShowInstallBanner(opts?: {
  standalone?: boolean;
  mobile?: boolean;
  dismissed?: boolean;
}): boolean {
  const standalone = opts?.standalone ?? isStandaloneDisplayMode();
  const mobile = opts?.mobile ?? isMobileInstallTarget();
  const dismissed = opts?.dismissed ?? isInstallBannerDismissed();
  return !standalone && mobile && !dismissed;
}

export function canTriggerInstallPrompt(): boolean {
  return deferredInstallPrompt != null;
}

export async function triggerInstallPrompt(): Promise<boolean> {
  const event = deferredInstallPrompt;
  if (!event) return false;
  await event.prompt();
  const { outcome } = await event.userChoice;
  if (outcome === "accepted") {
    setDeferredInstallPrompt(null);
    return true;
  }
  return false;
}

export function buildInstallBanner(
  onDismiss: () => void,
  onInstall?: () => void,
): { banner: HTMLElement; destroy: () => void } {
  const T = getStrings();
  const actions = h("div", { class: "install-banner__actions" });
  if (canTriggerInstallPrompt()) {
    actions.appendChild(
      h(
        "button",
        {
          type: "button",
          class: "install-banner__install",
          onClick: () => {
            void triggerInstallPrompt().then((accepted) => {
              if (accepted) onInstall?.();
            });
          },
        },
        T.installBanner.install,
      ),
    );
  }
  actions.appendChild(
    h(
      "button",
      {
        type: "button",
        class: "install-banner__dismiss",
        "aria-label": T.installBanner.dismiss,
        onClick: onDismiss,
      },
      T.installBanner.dismiss,
    ),
  );

  const banner = h(
    "div",
    {
      class: "install-banner",
      role: "region",
      "aria-label": T.installBanner.aria,
    },
    h("p", { class: "install-banner__text" }, h("strong", {}, T.installBanner.title), " ", installHintText()),
    actions,
  );

  function syncCopy(): void {
    const strings = getStrings();
    banner.setAttribute("aria-label", strings.installBanner.aria);
    const text = banner.querySelector(".install-banner__text");
    if (text) {
      text.replaceChildren(
        h("strong", {}, strings.installBanner.title),
        " ",
        installHintText(),
      );
    }
    const installBtn = banner.querySelector<HTMLButtonElement>(".install-banner__install");
    if (installBtn) installBtn.textContent = strings.installBanner.install;
    const btn = banner.querySelector<HTMLButtonElement>(".install-banner__dismiss");
    if (btn) {
      btn.textContent = strings.installBanner.dismiss;
      btn.setAttribute("aria-label", strings.installBanner.dismiss);
    }
  }

  const unsubLocale = localePref.subscribe(() => syncCopy());

  const destroy = () => {
    unsubLocale();
  };
  banner.dataset.installBanner = "1";
  return { banner, destroy };
}

export function listenForInstallPrompt(): () => void {
  if (typeof window === "undefined") return () => {};
  const handler = (e: Event) => {
    e.preventDefault();
    setDeferredInstallPrompt(e as BeforeInstallPromptEvent);
  };
  window.addEventListener("beforeinstallprompt", handler);
  return () => window.removeEventListener("beforeinstallprompt", handler);
}

export function mountInstallBanner(container: HTMLElement): () => void {
  const stopListen = listenForInstallPrompt();

  const tryMount = (): (() => void) => {
    if (!shouldShowInstallBanner()) {
      return () => {};
    }
    if (container.querySelector(".install-banner")) {
      return () => {};
    }

    const built = buildInstallBanner(
      () => {
        dismissInstallBanner();
        built.banner.remove();
      },
      () => {
        dismissInstallBanner();
        built.banner.remove();
      },
    );
    container.prepend(built.banner);
    return () => {
      built.destroy();
      built.banner.remove();
    };
  };

  let cleanup = tryMount();

  const onBip = () => {
    if (!shouldShowInstallBanner()) return;
    if (container.querySelector(".install-banner")) return;
    cleanup();
    cleanup = tryMount();
  };
  window.addEventListener("beforeinstallprompt", onBip);

  return () => {
    stopListen();
    window.removeEventListener("beforeinstallprompt", onBip);
    cleanup();
  };
}

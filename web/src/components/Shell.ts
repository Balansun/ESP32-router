import { h } from "../utils/dom";
import { icon, type IconName } from "../utils/icons";
import { requestHttpAuthLogin } from "../auth/httpAuthGate";
import {
  conn,
  deviceInfo,
  localePref,
  publicBootstrap,
  themePref,
  type ConnState,
  type LocalePref,
  type ThemePref,
  offline,
} from "../state/store";
import { currentPath, onNav } from "../router";
import { getStrings } from "../i18n";
import { stripBase, withBase } from "../paths";
import { formatFirmwareVersionFull } from "../firmware/versionCompare";
import { formatAvailableReleaseLabel } from "../firmware/githubDailyCheck";
import { firmwareUpdate } from "../state/firmwareUpdate";
import { isMoreRoute, openNavMoreSheet } from "./NavMore";
import { wifiNavPath } from "../wifi/wifiPaths";
import { applyDocumentTitle } from "../utils/documentTitle";
import { mountInstallBanner } from "./InstallBanner";
import { installPwaManifest } from "../pwa/installManifest";
import { brandLogoDataUrl, brandThemeFromPref } from "../brand/brandAssets";
import { docsLangHome } from "../fieldHelp/docUrl";
import { buildRepresentationBanner, isRepresentationMode } from "./RepresentationBanner";
import { meterPackId, showActionsNav } from "../utils/firmwareCaps";
import { productModePioEnv } from "../state/productMode";

interface NavItem {
  href: string;
  label: string;
  icon: IconName;
}

function primaryNavItems(): NavItem[] {
  const T = getStrings();
  const items: NavItem[] = [
    { href: withBase("/"), label: T.nav.home, icon: "home" },
    { href: withBase("/history"), label: T.nav.history, icon: "history" },
  ];
  if (showActionsNav()) {
    items.push({ href: withBase("/actions"), label: T.nav.actions, icon: "actions" });
  }
  items.push({ href: withBase("/settings/general"), label: T.nav.settings, icon: "settings" });
  return items;
}

export interface BuildShellOptions {
  /** Captive portal /wifi — logo, theme, and form only (no nav, footer, or health chrome). */
  variant?: "full" | "ap-setup";
}

export function buildShell(opts: BuildShellOptions = {}): { main: HTMLElement } {
  const apSetup = opts.variant === "ap-setup";
  const T0 = getStrings();
  const led = h("span", {
    class: "led",
    role: "status",
    "aria-live": "polite",
    "aria-label": T0.status.live,
  });

  const titleEl = h("span", { class: "appbar__name" }, "");
  const firmwareRow = h("div", { class: "appbar__firmware" });
  const subEl = h("span", { class: "appbar__sub" }, "");
  const updateBadge = h(
    "a",
    {
      class: "appbar__update-badge",
      href: withBase("/firmware"),
      hidden: true,
      title: T0.shell.firmwareUpdateTitle,
    },
    T0.shell.firmwareUpdateBadge,
  ) as HTMLAnchorElement;
  const logoImg = h("img", {
    class: "brand-logo appbar__logo",
    alt: T0.appName,
    width: "112",
    height: "28",
    decoding: "async",
  }) as HTMLImageElement;

  function syncAppbarLogo(): void {
    logoImg.src = brandLogoDataUrl(brandThemeFromPref(themePref.get()));
  }
  syncAppbarLogo();

  function syncAppbarTitle(routerName?: string): void {
    const T = getStrings();
    const custom = routerName?.trim();
    const useLogo = !custom || custom === T.appName;
    logoImg.hidden = !useLogo;
    titleEl.hidden = useLogo;
    if (useLogo) {
      logoImg.alt = T.appName;
      titleEl.textContent = "";
    } else {
      logoImg.alt = "";
      titleEl.textContent = custom;
    }
  }

  function formatAppbarSub(firmwareVersion?: string): string {
    const full = formatFirmwareVersionFull(firmwareVersion);
    if (full === "—") return "";
    return /^v/i.test(full) ? full : `v${full}`;
  }

  function syncUpdateBadge(u = firmwareUpdate.get()): void {
    const T = getStrings();
    const show = u.available && Boolean(u.releaseTag?.trim());
    updateBadge.hidden = !show;
    updateBadge.classList.toggle("is-available", show);
    if (!show) {
      updateBadge.setAttribute("aria-hidden", "true");
      updateBadge.tabIndex = -1;
      syncFirmwareRowVisibility();
      return;
    }
    updateBadge.removeAttribute("aria-hidden");
    updateBadge.tabIndex = 0;
    const label = formatAvailableReleaseLabel(u.releaseTag);
    updateBadge.textContent = label || T.shell.firmwareUpdateBadge;
    updateBadge.title = T.shell.firmwareUpdateTitle.replace(
      "{tag}",
      formatFirmwareVersionFull(u.releaseTag),
    );
    syncFirmwareRowVisibility();
  }
  firmwareUpdate.subscribe(syncUpdateBadge);

  function syncFirmwareRowVisibility(): void {
    firmwareRow.hidden = subEl.hidden && updateBadge.hidden;
  }

  let rebuildTabbarNav: () => void = () => {};

  const syncAppbarFromDevice = () => {
    const d = deviceInfo.get();
    if (!d) return;
    syncAppbarTitle(d.router_name);
    const pack = meterPackId();
    const fwSub = formatAppbarSub(d.firmware_version);
    subEl.textContent = pack ? `${fwSub} · ${pack}` : fwSub;
    subEl.hidden = !subEl.textContent;
    syncFirmwareRowVisibility();
    installPwaManifest(d.router_name);
    rebuildTabbarNav();
    syncNavActive(currentPath());
  };
  deviceInfo.subscribe(() => syncAppbarFromDevice());
  productModePioEnv.subscribe(() => syncAppbarFromDevice());
  syncAppbarTitle(deviceInfo.get()?.router_name);

  const syncConnLed = (s: ConnState) => {
    const T = getStrings();
    led.dataset.state = s;
    led.setAttribute(
      "aria-label",
      s === "ok" ? T.status.live : s === "loading" ? T.loading : T.status.error,
    );
  };
  conn.subscribe((s) => syncConnLed(s));
  syncConnLed(conn.get());

  const themeBtn = h(
    "button",
    {
      type: "button",
      class: "icon-btn",
      "aria-label": T0.theme.auto,
      title: T0.theme.auto,
      onClick: cycleTheme,
    },
    icon("auto"),
  );
  themePref.subscribe((p) => {
    updateThemeButton(themeBtn, p);
    syncAppbarLogo();
  });
  updateThemeButton(themeBtn, themePref.get());

  const langSelect = h(
    "select",
    {
      class: "locale-select",
      "aria-label": T0.shell.languageAria,
      title: T0.shell.languageAria,
      onChange: (ev) => {
        const v = (ev.target as HTMLSelectElement).value as LocalePref;
        if (v === "en" || v === "fr") localePref.set(v);
      },
    },
    h("option", { value: "en" }, "EN"),
    h("option", { value: "fr" }, "FR"),
  ) as HTMLSelectElement;
  langSelect.value = localePref.get();

  const signOutBtn = h(
    "button",
    {
      type: "button",
      class: "icon-btn",
      hidden: true,
      "aria-label": T0.httpAuth.signOut,
      title: T0.httpAuth.signOut,
      onClick: () => {
        void requestHttpAuthLogin();
      },
    },
    icon("logout"),
  ) as HTMLButtonElement;

  function syncSignOutVisibility(): void {
    const boot = publicBootstrap.get();
    signOutBtn.hidden = !boot.ready || !boot.httpAuthEnabled;
  }
  publicBootstrap.subscribe(() => syncSignOutVisibility());
  syncSignOutVisibility();

  firmwareRow.append(subEl, updateBadge);
  syncFirmwareRowVisibility();

  const appbar = h(
    "header",
    { class: "appbar" },
    led,
    h(
      "div",
      { class: "appbar__title" },
      h("div", { class: "appbar__brand" }, logoImg, titleEl, firmwareRow),
    ),
    h("div", { class: "appbar__spacer" }),
    h("div", { class: "appbar__actions" }, ...(apSetup ? [] : [signOutBtn]), langSelect, themeBtn),
  );

  const offlineBanner = h(
    "div",
    { class: "offline-banner", hidden: true, role: "alert" },
    T0.offline,
  );
  offline.subscribe((isOff) => {
    offlineBanner.hidden = !isOff;
  });
  if (offline.get()) offlineBanner.hidden = false;

  const tabbar = apSetup
    ? null
    : h("nav", {
        class: "tabbar",
        "aria-label": T0.shell.tabNavAria,
      });

  const navAnchors: HTMLAnchorElement[] = [];
  let moreBtn: HTMLButtonElement | null = null;
  if (tabbar) {
    moreBtn = h(
      "button",
      {
        type: "button",
        class: "tabbar__item tabbar__item--more",
        "aria-label": T0.shell.more,
        "aria-haspopup": "dialog",
        onClick: () => openNavMoreSheet(),
      },
      icon("more"),
      h("span", { class: "tabbar__label" }, T0.shell.more),
    ) as HTMLButtonElement;

    tabbar.append(moreBtn);
    rebuildTabbarNav = () => {
      if (!tabbar || !moreBtn) return;
      for (const a of navAnchors) a.remove();
      navAnchors.length = 0;
      for (const n of primaryNavItems()) {
        const a = buildNavLink(n);
        navAnchors.push(a);
        tabbar.insertBefore(a, moreBtn);
      }
    };
    rebuildTabbarNav();
  }

  const main = h("main", {
    id: "view",
    class: "main",
    "aria-busy": "true",
    tabindex: "-1",
  });

  const footerBrandEl = h("span", { class: "footer__brand" }, T0.shell.footerBrand);
  const footerDocsEl = h(
    "a",
    {
      href: docsWebsiteHref(localePref.get()),
      target: "_blank",
      rel: "noopener noreferrer",
    },
    T0.shell.footerDocsLabel,
  );
  const footerGithubEl = h(
    "a",
    {
      href: "https://github.com/Balansun/ESP32-router",
      target: "_blank",
      rel: "noopener noreferrer",
    },
    T0.shell.footerLinkLabel,
  );
  const footerLinks = h("nav", { class: "footer__links", "aria-label": T0.shell.footerNavAria }, footerDocsEl, footerGithubEl);
  const footer = h("footer", { class: "footer" }, footerBrandEl, footerLinks);

  const subnav = apSetup
    ? null
    : h("nav", {
        class: "subnav",
        "aria-label": T0.shell.moreNavAria,
      });
  if (subnav) {
    const moreNavItems: Array<{ path: string; key: keyof typeof T0.nav }> = [
      { path: wifiNavPath(), key: "wifi" },
      { path: "/api", key: "api" },
      { path: "/firmware", key: "firmware" },
      { path: "/backup", key: "backup" },
      { path: "/diag", key: "diag" },
    ];
    for (const { path, key } of moreNavItems) {
      const label = T0.nav[key];
      subnav.append(
        h(
          "a",
          {
            href: withBase(path),
            class: "subnav__link",
            "data-route": "true",
          },
          label,
        ),
      );
    }
  }

  const representationBanner =
    !apSetup && isRepresentationMode() ? buildRepresentationBanner() : null;
  const chrome = h(
    "div",
    { class: "layout__chrome" },
    appbar,
    ...(representationBanner ? [representationBanner] : []),
    offlineBanner,
    ...(apSetup ? [] : [subnav!]),
  );
  if (!apSetup) mountInstallBanner(chrome);
  const layout = h(
    "div",
    { class: "layout" },
    chrome,
    main,
    ...(apSetup ? [] : [footer]),
  );

  if (tabbar) {
    const mqWide = window.matchMedia("(min-width: 720px)");
    function applyTabbarPlacement() {
      const wide = mqWide.matches;
      tabbar!.classList.toggle("tabbar--dock", !wide);
      if (wide) chrome.append(tabbar!);
      else document.body.append(tabbar!);
    }
    applyTabbarPlacement();
    mqWide.addEventListener("change", applyTabbarPlacement);
  }

  document.body.append(layout);

  function syncShellChrome() {
    const T = getStrings();
    offlineBanner.textContent = T.offline;
    langSelect.setAttribute("aria-label", T.shell.languageAria);
    langSelect.setAttribute("title", T.shell.languageAria);
    langSelect.value = localePref.get();
    if (tabbar) tabbar.setAttribute("aria-label", T.shell.tabNavAria);
    if (moreBtn) {
      moreBtn.setAttribute("aria-label", T.shell.more);
      const moreLab = moreBtn.querySelector<HTMLSpanElement>(".tabbar__label");
      if (moreLab) moreLab.textContent = T.shell.more;
    }
    if (!apSetup) {
      footerBrandEl.textContent = T.shell.footerBrand;
      footerDocsEl.textContent = T.shell.footerDocsLabel;
      footerDocsEl.href = docsWebsiteHref(localePref.get());
      footerGithubEl.textContent = T.shell.footerLinkLabel;
      footerLinks.setAttribute("aria-label", T.shell.footerNavAria);
    }
    syncConnLed(conn.get());
    syncUpdateBadge(firmwareUpdate.get());
    updateThemeButton(themeBtn, themePref.get());
    signOutBtn.setAttribute("aria-label", T.httpAuth.signOut);
    signOutBtn.setAttribute("title", T.httpAuth.signOut);
    syncSignOutVisibility();
    const d = deviceInfo.get();
    syncAppbarTitle(d?.router_name);
    subEl.textContent = d ? formatAppbarSub(d.firmware_version) : "";
    subEl.hidden = !subEl.textContent;
    syncFirmwareRowVisibility();
    if (subnav) {
      const wifiLink = subnav.querySelector<HTMLAnchorElement>(
        'a.subnav__link[href$="/wifi"], a.subnav__link[href$="/wifi/station"]',
      );
      if (wifiLink) wifiLink.setAttribute("href", withBase(wifiNavPath()));
    }
    const items = primaryNavItems();
    for (let i = 0; i < navAnchors.length && i < items.length; i++) {
      const a = navAnchors[i]!;
      const it = items[i]!;
      a.setAttribute("href", it.href);
      a.setAttribute("aria-label", it.label);
      const lab = a.querySelector<HTMLSpanElement>(".tabbar__label");
      if (lab) lab.textContent = it.label;
    }
    syncNavActive(currentPath());
  }

  localePref.subscribe(() => {
    syncShellChrome();
  });
  publicBootstrap.subscribe(() => {
    syncShellChrome();
  });

  function syncNavActive(path: string) {
    if (!apSetup) {
      for (const a of navAnchors) {
        const active = matchActive(a.getAttribute("href") || "/", path);
        if (active) a.setAttribute("aria-current", "page");
        else a.removeAttribute("aria-current");
      }
      if (moreBtn && subnav) {
        if (isMoreRoute(path)) {
          moreBtn.setAttribute("aria-current", "page");
          moreBtn.classList.add("tabbar__item--active");
          subnav.classList.add("subnav--visible");
        } else {
          moreBtn.removeAttribute("aria-current");
          moreBtn.classList.remove("tabbar__item--active");
          subnav.classList.remove("subnav--visible");
        }
        for (const a of subnav.querySelectorAll<HTMLAnchorElement>(".subnav__link")) {
          const href = a.getAttribute("href") || "/";
          if (matchActive(href, path)) a.setAttribute("aria-current", "page");
          else a.removeAttribute("aria-current");
        }
      }
    }
    applyDocumentTitle(path);
  }

  rebuildTabbarNav();
  syncNavActive(currentPath());

  onNav((path) => {
    syncNavActive(path);
    main.setAttribute("aria-busy", "false");
    queueMicrotask(() => main.focus({ preventScroll: true }));
  });

  return { main };
}

function docsWebsiteHref(locale: LocalePref): string {
  return docsLangHome(locale === "fr" ? "fr" : "en");
}

function matchActive(linkHref: string, currentPath: string): boolean {
  const linkLogical = stripBase(
    linkHref.startsWith("http")
      ? new URL(linkHref).pathname
      : linkHref,
  );
  if (linkLogical === "/")
    return currentPath === "/" || currentPath === "";
  return currentPath === linkLogical || currentPath.startsWith(linkLogical + "/");
}

function buildNavLink(n: NavItem): HTMLAnchorElement {
  return h(
    "a",
    {
      href: n.href,
      "data-route": "true",
      class: "tabbar__item",
      "aria-label": n.label,
    },
    icon(n.icon),
    h("span", { class: "tabbar__label" }, n.label),
  );
}

const THEME_CYCLE: ThemePref[] = ["auto", "light", "dark"];
function cycleTheme() {
  const cur = themePref.get();
  const next = THEME_CYCLE[(THEME_CYCLE.indexOf(cur) + 1) % THEME_CYCLE.length];
  themePref.set(next);
}

function updateThemeButton(btn: HTMLButtonElement, p: ThemePref) {
  btn.replaceChildren(
    icon(p === "auto" ? "auto" : p === "dark" ? "moon" : "sun"),
  );
  const T = getStrings();
  const lbl =
    p === "auto" ? T.theme.auto : p === "dark" ? T.theme.dark : T.theme.light;
  btn.setAttribute("aria-label", lbl);
  btn.setAttribute("title", lbl);
}

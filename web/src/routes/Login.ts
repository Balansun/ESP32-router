import { brandLogoDataUrl, brandThemeFromPref } from "../brand/brandAssets";
import { verifyLoginPassword } from "../api/apiSession";
import { getStrings } from "../i18n";
import {
  deviceInfo,
  localePref,
  themePref,
  type LocalePref,
  type ThemePref,
} from "../state/store";
import { h } from "../utils/dom";
import { icon } from "../utils/icons";
import { buildFieldLabelRow } from "../components/FieldHelp";

const THEME_CYCLE: ThemePref[] = ["auto", "light", "dark"];

function cycleTheme() {
  const cur = themePref.get();
  const next = THEME_CYCLE[(THEME_CYCLE.indexOf(cur) + 1) % THEME_CYCLE.length];
  themePref.set(next);
}

function updateThemeButton(btn: HTMLButtonElement, p: ThemePref) {
  btn.replaceChildren(icon(p === "auto" ? "auto" : p === "dark" ? "moon" : "sun"));
  const T = getStrings();
  const lbl =
    p === "auto" ? T.theme.auto : p === "dark" ? T.theme.dark : T.theme.light;
  btn.setAttribute("aria-label", lbl);
  btn.setAttribute("title", lbl);
}

export interface MountLoginPageOpts {
  onSuccess: () => void;
}

/** Full-screen login gate; calls onSuccess after password is verified and stored. */
export function mountLoginPage(opts: MountLoginPageOpts): () => void {
  document.querySelector(".layout")?.remove();
  document.querySelector("nav.tabbar")?.remove();
  const appRoot = document.getElementById("app")!;
  appRoot.replaceChildren();

  const T0 = getStrings();
  const errorEl = h("p", { class: "login-page__error", hidden: true, role: "alert" });
  const submitBtn = h(
    "button",
    { type: "submit", class: "btn btn--primary btn--block" },
    T0.httpAuth.submit,
  ) as HTMLButtonElement;

  const passInput = h("input", {
    type: "password",
    class: "field__input",
    autocomplete: "current-password",
    required: true,
  }) as HTMLInputElement;

  const logoImg = h("img", {
    class: "brand-logo login-page__logo",
    alt: "",
    width: "200",
    height: "48",
  }) as HTMLImageElement;

  function syncLogo() {
    const theme = brandThemeFromPref(themePref.get());
    logoImg.src = brandLogoDataUrl(theme);
  }
  syncLogo();

  const unsubDevice = deviceInfo.subscribe((d) => {
    const T = getStrings();
    const name = d?.router_name?.trim() || T.appName;
    logoImg.alt = name;
  });
  const d0 = deviceInfo.get();
  logoImg.alt = d0?.router_name?.trim() || T0.appName;

  const themeBtn = h(
    "button",
    {
      type: "button",
      class: "icon-btn",
      "aria-label": T0.theme.auto,
      onClick: cycleTheme,
    },
    icon("auto"),
  ) as HTMLButtonElement;
  const unsubTheme = themePref.subscribe((p) => {
    updateThemeButton(themeBtn, p);
    syncLogo();
  });
  updateThemeButton(themeBtn, themePref.get());

  const langSelect = h(
    "select",
    {
      class: "locale-select",
      "aria-label": T0.shell.languageAria,
      onChange: (ev) => {
        const v = (ev.target as HTMLSelectElement).value as LocalePref;
        if (v === "en" || v === "fr") localePref.set(v);
      },
    },
    h("option", { value: "en" }, "EN"),
    h("option", { value: "fr" }, "FR"),
  ) as HTMLSelectElement;
  langSelect.value = localePref.get();

  async function submitLogin() {
    const T = getStrings();
    const pw = passInput.value;
    if (!pw) return;
    errorEl.hidden = true;
    submitBtn.disabled = true;
    passInput.disabled = true;
    try {
      const ok = await verifyLoginPassword(pw);
      if (ok) {
        opts.onSuccess();
        return;
      }
      errorEl.textContent = T.httpAuth.loginError;
      errorEl.hidden = false;
      passInput.value = "";
      passInput.focus();
    } finally {
      submitBtn.disabled = false;
      passInput.disabled = false;
    }
  }

  const form = h(
    "form",
    {
      class: "login-page__form form",
      onSubmit: (ev) => {
        ev.preventDefault();
        void submitLogin();
      },
    },
    h(
      "div",
      { class: "field" },
      buildFieldLabelRow({
        label: T0.httpAuth.password,
        helpScope: "httpAuth",
        helpKey: "login_password",
      }),
      passInput,
    ),
    errorEl,
    submitBtn,
  );

  const unsubLocale = localePref.subscribe(() => {
    const T = getStrings();
    submitBtn.textContent = T.httpAuth.submit;
    errorEl.textContent = T.httpAuth.loginError;
    langSelect.setAttribute("aria-label", T.shell.languageAria);
    updateThemeButton(themeBtn, themePref.get());
    const info = deviceInfo.get();
    logoImg.alt = info?.router_name?.trim() || T.appName;
    const passLab = form.querySelector<HTMLSpanElement>(".field__label");
    if (passLab) passLab.textContent = T.httpAuth.password;
    if (pageTitle) pageTitle.textContent = T.httpAuth.pageTitle;
    if (pageSubtitle) pageSubtitle.textContent = T.httpAuth.pageSubtitle;
  });

  const toolbar = h("div", { class: "login-page__toolbar" }, langSelect, themeBtn);

  const pageTitle = h("h1", { class: "login-page__title" }, T0.httpAuth.pageTitle);
  const pageSubtitle = h("p", { class: "login-page__subtitle" }, T0.httpAuth.pageSubtitle);

  const card = h(
    "div",
    { class: "card login-page__card" },
    logoImg,
    pageTitle,
    pageSubtitle,
    form,
  );

  const page = h("div", { class: "login-page" }, toolbar, card);
  appRoot.append(page);
  appRoot.removeAttribute("aria-busy");

  queueMicrotask(() => passInput.focus());

  return () => {
    unsubDevice();
    unsubTheme();
    unsubLocale();
  };
}

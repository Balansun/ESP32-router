import { h } from "../../utils/dom";
import { getStrings } from "../../i18n";
import { withBase } from "../../paths";
import { buildPageHeader } from "../../components/ui/pageHeader";
import { icon, type IconName } from "../../utils/icons";
import { openDialog } from "../../components/Dialog";
import { api } from "../../api/client";
import { toast } from "../../components/Toast";
import {
  SETTINGS_SECTIONS,
  settingsPath,
  type SettingsSection,
} from "./settingsPaths";

const SECTION_ICONS: Record<SettingsSection, IconName> = {
  product: "chip",
  general: "settings",
  metering: "chip",
  network: "wifi",
  hardware: "chip",
  led: "led",
  advanced: "diag",
};

export interface SettingsChrome {
  root: HTMLElement;
  subnav: HTMLElement;
  sectionOutlet: HTMLElement;
  form: HTMLFormElement;
  setSection: (section: SettingsSection) => void;
  /** Prepends actions before the reboot button in the page header. */
  addHeaderActions: (...els: HTMLElement[]) => void;
}

export function buildSettingsChrome(signal: AbortSignal): SettingsChrome {
  const T = getStrings();

  function confirmReboot() {
    openDialog({
      title: T.settings.rebootConfirmTitle,
      body: h("p", {}, T.settings.rebootConfirmBody),
      actions: [
        { label: T.cancel, kind: "ghost", onClick: () => {} },
        {
          label: T.settings.rebootBtn,
          kind: "danger",
          onClick: async () => {
            try {
              await api.reboot({ signal });
              toast(T.settings.rebooting, "info", 6000);
            } catch {
              toast(T.saveError, "error");
            }
          },
        },
      ],
    });
  }

  const rebootBtn = h(
    "button",
    {
      type: "button",
      class: "btn btn--danger",
      onClick: confirmReboot,
    },
    icon("reboot"),
    h("span", {}, T.settings.rebootBtn),
  );
  const headerActionsHost = h("div", { class: "page-header__actions" }, rebootBtn);

  const header = buildPageHeader({
    title: T.settings.title,
    description: T.settings.navLead,
  });
  header.append(headerActionsHost);

  function addHeaderActions(...els: HTMLElement[]) {
    for (const el of els) headerActionsHost.insertBefore(el, rebootBtn);
  }

  const sectionHints: Record<SettingsSection, string> = {
    product: T.settings.tabIntro.product,
    general: T.settings.tabIntro.general,
    metering: T.settings.tabIntro.metering,
    network: T.settings.tabIntro.network,
    hardware: T.settings.tabIntro.hardware,
    led: T.settings.tabIntro.led,
    advanced: T.settings.tabIntro.advanced,
  };

  const subnavLinks: Record<SettingsSection, HTMLAnchorElement> = {} as never;
  const subnav = h(
    "nav",
    { class: "settings-nav", "aria-label": T.settings.tabsAria },
    ...SETTINGS_SECTIONS.map((id) => {
      const link = h(
        "a",
        {
          href: withBase(settingsPath(id)),
          class: "settings-nav__item",
          "data-route": "true",
        },
        icon(SECTION_ICONS[id]),
        h(
          "span",
          { class: "settings-nav__body" },
          h("span", { class: "settings-nav__label" }, T.settings.tabs[id]),
          h("span", { class: "settings-nav__hint" }, sectionHints[id]),
        ),
        h("span", { class: "settings-nav__chevron", "aria-hidden": "true" }, "›"),
      ) as HTMLAnchorElement;
      subnavLinks[id] = link;
      return link;
    }),
  );

  const sectionOutlet = h("div", { class: "settings-section" });
  const layout = h("div", { class: "settings-layout" }, subnav, sectionOutlet);
  const form = h("form", { class: "form", onSubmit: (e) => e.preventDefault() }) as HTMLFormElement;
  form.append(layout);

  const root = h("div", { class: "settings-page" }, header, form);

  function setSection(section: SettingsSection) {
    for (const id of SETTINGS_SECTIONS) {
      const active = id === section;
      subnavLinks[id].setAttribute("aria-current", active ? "page" : "false");
      subnavLinks[id].classList.toggle("settings-nav__item--active", active);
      if (!active) subnavLinks[id].classList.remove("settings-nav__item--active");
    }
  }

  return { root, subnav, sectionOutlet, form, setSection, addHeaderActions };
}

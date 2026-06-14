import { go } from "../router";
import { getStrings } from "../i18n";
import { h } from "../utils/dom";
import { icon, type IconName } from "../utils/icons";
import { openDialog } from "./Dialog";
import { wifiNavPath } from "../wifi/wifiPaths";

interface MoreItem {
  path: string;
  label: string;
  hint: string;
  icon: IconName;
}

export const MORE_ROUTE_PATHS = [
  "/api",
  "/diag",
  "/firmware",
  "/backup",
  "/wifi",
  "/wifi/station",
] as const;

export function isMoreRoute(path: string): boolean {
  const p = path.replace(/\/+$/, "") || "/";
  return MORE_ROUTE_PATHS.some((r) => p === r || p.startsWith(r + "/"));
}

function moreGroups(T: ReturnType<typeof getStrings>): Array<{ title: string; items: MoreItem[] }> {
  return [
    {
      title: T.nav.moreGroupConnectivity,
      items: [
        { path: wifiNavPath(), label: T.nav.wifi, hint: T.nav.wifiHint, icon: "wifi" },
        { path: "/api", label: T.nav.api, hint: T.nav.apiHint, icon: "chip" },
      ],
    },
    {
      title: T.nav.moreGroupDevice,
      items: [
        { path: "/firmware", label: T.nav.firmware, hint: T.nav.firmwareHint, icon: "reboot" },
        { path: "/backup", label: T.nav.backup, hint: T.nav.backupHint, icon: "save" },
      ],
    },
    {
      title: T.nav.moreGroupDiagnostics,
      items: [{ path: "/diag", label: T.nav.diag, hint: T.nav.diagHint, icon: "diag" }],
    },
  ];
}

export function openNavMoreSheet() {
  const T = getStrings();
  const groups = moreGroups(T);

  const list = h(
    "nav",
    { class: "nav-more", "aria-label": T.shell.moreNavAria },
    ...groups.map((group) =>
      h(
        "div",
        { class: "nav-more__group" },
        h("h3", { class: "nav-more__group-title" }, group.title),
        ...group.items.map((it) =>
          h(
            "button",
            {
              type: "button",
              class: "nav-more__item",
              onClick: () => {
                dlg.close();
                void go(it.path);
              },
            },
            icon(it.icon),
            h(
              "span",
              { class: "nav-more__item-body" },
              h("span", {}, it.label),
              h("span", { class: "nav-more__item-desc" }, it.hint),
            ),
          ),
        ),
      ),
    ),
  );

  const dlg = openDialog({
    title: T.shell.more,
    body: list,
    closeOnBackdrop: true,
  });
}

// Inline SVG icons (no external font / asset). 24x24 stroke icons.
import { svg } from "./dom";

export type IconName =
  | "home"
  | "history"
  | "actions"
  | "settings"
  | "diag"
  | "sun"
  | "moon"
  | "auto"
  | "plus"
  | "minus"
  | "edit"
  | "close"
  | "copy"
  | "reboot"
  | "save"
  | "alert"
  | "chip"
  | "wifi"
  | "led"
  | "more"
  | "download"
  | "upload"
  | "logout";

const PATHS: Record<IconName, string[]> = {
  home: ["M3 11l9-8 9 8", "M5 9.5V21h14V9.5"],
  history: [
    "M3 12a9 9 0 1 0 3-6.7",
    "M3 4v5h5",
    "M12 7v5l3 2",
  ],
  actions: [
    "M4 5h12",
    "M4 12h12",
    "M4 19h12",
    "M19 5l1 0 -1 14 -1 -14z",
  ],
  settings: [
    "M12 15a3 3 0 1 0 0-6 3 3 0 0 0 0 6Z",
    "M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 1 1-4 0v-.09a1.65 1.65 0 0 0-1-1.51 1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 1 1 0-4h.09a1.65 1.65 0 0 0 1.51-1 1.65 1.65 0 0 0-.33-1.82l-.06-.06A2 2 0 1 1 7.04 4.31l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 1 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 1 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1Z",
  ],
  diag: [
    "M9 17l3-3 3 3",
    "M12 14V4",
    "M5 20h14",
  ],
  sun: [
    "M12 3v1.5",
    "M12 19.5V21",
    "M3 12h1.5",
    "M19.5 12H21",
    "M5.6 5.6l1.1 1.1",
    "M17.3 17.3l1.1 1.1",
    "M5.6 18.4l1.1-1.1",
    "M17.3 6.7l1.1-1.1",
    "M12 7.5a4.5 4.5 0 1 0 0 9 4.5 4.5 0 0 0 0-9Z",
  ],
  moon: ["M21 12.8A9 9 0 1 1 11.2 3a7 7 0 0 0 9.8 9.8Z"],
  auto: [
    "M12 3a9 9 0 1 0 0 18 9 9 0 0 0 0-18Z",
    "M12 3v18",
    "M12 3a9 9 0 0 1 0 18Z",
  ],
  plus: ["M12 5v14", "M5 12h14"],
  minus: ["M5 12h14"],
  edit: [
    "M4 20h4l11-11-4-4L4 16v4Z",
    "M14 6l4 4",
  ],
  close: ["M6 6l12 12", "M18 6L6 18"],
  copy: [
    "M9 9h10v12H9z",
    "M5 15V3h10v3",
  ],
  reboot: [
    "M3 12a9 9 0 1 0 3-6.7",
    "M3 4v5h5",
  ],
  save: [
    "M5 5h11l3 3v11H5z",
    "M9 5v5h6V5",
    "M9 19v-6h6v6",
  ],
  alert: [
    "M12 3l10 18H2L12 3z",
    "M12 10v5",
    "M12 17.5h0.01",
  ],
  chip: [
    "M8 6h8v12H8z",
    "M10 4v2",
    "M14 4v2",
    "M10 18v2",
    "M14 18v2",
    "M4 10h2",
    "M4 14h2",
    "M18 10h2",
    "M18 14h2",
  ],
  wifi: [
    "M5 12.5a14 14 0 0 1 14 0",
    "M8.5 15.5a9 9 0 0 1 7 0",
    "M12 19h0.01",
  ],
  led: [
    "M9 18h6",
    "M10 22h4",
    "M8 6a4 4 0 1 1 8 0c0 3-2 4-2 7H10c0-3-2-4-2-7Z",
  ],
  more: ["M6 12h0.01", "M12 12h0.01", "M18 12h0.01"],
  download: ["M12 3v12", "M7 10l5 5 5-5", "M5 21h14"],
  upload: ["M12 21V9", "M7 14l5-5 5 5", "M5 3h14"],
  logout: ["M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4", "M16 17l5-5-5-5", "M21 12H9"],
};

export function icon(name: IconName, opts: { size?: number; ariaLabel?: string } = {}): SVGElement {
  const size = opts.size ?? 22;
  const root = svg("svg", {
    width: size,
    height: size,
    viewBox: "0 0 24 24",
    fill: "none",
    stroke: "currentColor",
    "stroke-width": "1.8",
    "stroke-linecap": "round",
    "stroke-linejoin": "round",
    "aria-hidden": opts.ariaLabel ? "false" : "true",
  });
  if (opts.ariaLabel) {
    root.setAttribute("role", "img");
    root.setAttribute("aria-label", opts.ariaLabel);
  }
  for (const d of PATHS[name]) {
    root.append(svg("path", { d }));
  }
  return root;
}

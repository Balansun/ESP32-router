import { h } from "../utils/dom";

export interface OtaProgressHandle {
  root: HTMLElement;
  setLabel: (text: string) => void;
  setProgress: (done: number, total: number) => void;
  setIndeterminate: (on: boolean) => void;
  setTrackVisible: (visible: boolean) => void;
  setSpinnerVisible: (visible: boolean) => void;
  reset: () => void;
  show: () => void;
  hide: () => void;
}

export function createOtaProgress(): OtaProgressHandle {
  const label = h("p", { class: "card__sub ota-progress__label", style: "margin:0;" }, "");
  const bar = h("div", {
    class: "progress__bar",
    role: "progressbar",
    "aria-valuemin": "0",
    "aria-valuemax": "100",
    "aria-valuenow": "0",
  }) as HTMLElement;
  const track = h(
    "div",
    { class: "progress ota-progress__track", "aria-hidden": "false" },
    bar,
  ) as HTMLElement;
  const spinner = h("div", {
    class: "inline-spinner",
    "aria-hidden": "true",
  }) as HTMLElement;

  const root = h(
    "div",
    {
      class: "ota-progress",
      hidden: true,
      "aria-busy": "false",
      "aria-live": "polite",
    },
    h("div", { class: "ota-progress__head" }, spinner, label),
    track,
  ) as HTMLElement;

  function setLabel(text: string) {
    label.textContent = text;
  }

  function setProgress(done: number, total: number) {
    track.classList.remove("progress--indeterminate");
    if (total > 0) {
      const pct = Math.min(100, Math.max(0, Math.round((done / total) * 100)));
      bar.style.width = `${pct}%`;
      bar.setAttribute("aria-valuenow", String(pct));
      bar.setAttribute("aria-valuemax", "100");
    } else {
      bar.style.width = "0%";
      bar.setAttribute("aria-valuenow", "0");
    }
  }

  function setIndeterminate(on: boolean) {
    track.classList.toggle("progress--indeterminate", on);
    if (on) {
      bar.style.width = "";
      bar.removeAttribute("aria-valuenow");
    } else {
      bar.setAttribute("aria-valuenow", "0");
    }
  }

  function setTrackVisible(visible: boolean) {
    track.hidden = !visible;
    track.setAttribute("aria-hidden", visible ? "false" : "true");
  }

  function setSpinnerVisible(visible: boolean) {
    spinner.hidden = !visible;
    spinner.setAttribute("aria-hidden", visible ? "false" : "true");
  }

  function reset() {
    setLabel("");
    setIndeterminate(true);
    setTrackVisible(false);
    setSpinnerVisible(false);
    bar.style.width = "0%";
    bar.setAttribute("aria-valuenow", "0");
  }

  function show() {
    root.hidden = false;
    root.setAttribute("aria-busy", "true");
  }

  function hide() {
    root.hidden = true;
    root.setAttribute("aria-busy", "false");
  }

  reset();
  return {
    root,
    setLabel,
    setProgress,
    setIndeterminate,
    setTrackVisible,
    setSpinnerVisible,
    reset,
    show,
    hide,
  };
}

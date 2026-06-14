/** WAI-ARIA tabs: roving tabindex + arrow keys. */
export function wireTabsKeyboard(
  tablist: HTMLElement,
  tabs: HTMLButtonElement[],
  panels: HTMLElement[],
  onSelect: (index: number) => void,
): () => void {
  const setSelected = (index: number) => {
    tabs.forEach((tab, i) => {
      const selected = i === index;
      tab.setAttribute("aria-selected", selected ? "true" : "false");
      tab.tabIndex = selected ? 0 : -1;
      panels[i]?.toggleAttribute("hidden", !selected);
    });
    onSelect(index);
  };

  const onKeyDown = (ev: KeyboardEvent) => {
    const current = tabs.findIndex((t) => t.getAttribute("aria-selected") === "true");
    if (current < 0) return;
    let next = current;
    if (ev.key === "ArrowRight" || ev.key === "ArrowDown") {
      next = (current + 1) % tabs.length;
      ev.preventDefault();
    } else if (ev.key === "ArrowLeft" || ev.key === "ArrowUp") {
      next = (current - 1 + tabs.length) % tabs.length;
      ev.preventDefault();
    } else if (ev.key === "Home") {
      next = 0;
      ev.preventDefault();
    } else if (ev.key === "End") {
      next = tabs.length - 1;
      ev.preventDefault();
    } else {
      return;
    }
    setSelected(next);
    tabs[next]?.focus();
  };

  tablist.addEventListener("keydown", onKeyDown);
  tabs.forEach((tab, i) => {
    tab.addEventListener("click", () => setSelected(i));
  });

  return () => tablist.removeEventListener("keydown", onKeyDown);
}

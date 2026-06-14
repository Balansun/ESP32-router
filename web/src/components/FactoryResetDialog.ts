import { h } from "../utils/dom";
import { api } from "../api/client";
import { getStrings } from "../i18n";
import { toast } from "./Toast";
import { openDialog } from "./Dialog";

export function openFactoryResetDialog(opts: { signal: AbortSignal }): void {
  const T = getStrings();
  const { signal } = opts;

  async function runAction(fn: () => Promise<unknown>) {
    try {
      await fn();
      toast(T.settings.actionOk, "success");
    } catch {
      toast(T.settings.actionFailed, "error");
    }
  }

  const input = h("input", {
    type: "text",
    class: "field__input",
    autocomplete: "off",
    spellcheck: "false",
  }) as HTMLInputElement;
  openDialog({
    title: T.settings.factoryReset,
    body: h(
      "div",
      { class: "form" },
      h("p", {}, T.settings.factoryResetConfirm),
      h("p", { class: "field__hint" }, T.settings.factoryResetConfirm2),
      input,
    ),
    actions: [
      { label: T.cancel, kind: "ghost", onClick: () => {} },
      {
        label: T.settings.factoryReset,
        kind: "danger",
        onClick: async () => {
          if (input.value.trim() !== T.settings.factoryResetToken) {
            toast(T.settings.actionFailed, "error");
            return;
          }
          await runAction(() => api.factoryReset({ signal }));
        },
      },
    ],
  });
}

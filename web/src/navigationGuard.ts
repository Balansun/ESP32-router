import { openDialog } from "./components/Dialog";
import { h } from "./utils/dom";
import { getStrings } from "./i18n";

let unsavedGuard: (() => boolean) | null = null;

export function setUnsavedGuard(fn: (() => boolean) | null) {
  unsavedGuard = fn;
}

export function hasUnsavedChanges(): boolean {
  return unsavedGuard?.() ?? false;
}

/** Resolves true if navigation may proceed (discard or no dirty state). */
export function confirmDiscardChanges(): Promise<boolean> {
  if (!hasUnsavedChanges()) return Promise.resolve(true);
  const T = getStrings();
  return new Promise((resolve) => {
    openDialog({
      title: T.unsavedChanges,
      body: h("p", {}, T.discardChanges),
      closeOnBackdrop: false,
      actions: [
        { label: T.cancel, kind: "ghost", onClick: () => resolve(false) },
        {
          label: T.confirm,
          kind: "danger",
          onClick: () => resolve(true),
        },
      ],
    });
  });
}

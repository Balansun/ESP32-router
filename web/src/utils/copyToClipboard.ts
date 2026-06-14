/**
 * Copy text to the system clipboard. Uses the Clipboard API on secure contexts
 * (HTTPS, localhost); falls back to execCommand on plain HTTP (e.g. device LAN).
 *
 * execCommand must run synchronously inside the click handler (user activation).
 * Dialog actions must not await non-promise handlers — see Dialog.runDialogAction.
 */
export function copyTextToClipboardSync(
  text: string,
  source?: HTMLInputElement | HTMLTextAreaElement,
): boolean {
  if (!text || typeof document === "undefined") return false;
  if (source && copyFromElement(source, text)) return true;
  return copyViaExecCommand(text);
}

export async function copyTextToClipboard(
  text: string,
  source?: HTMLInputElement | HTMLTextAreaElement,
): Promise<boolean> {
  if (
    typeof navigator !== "undefined" &&
    typeof document !== "undefined" &&
    globalThis.isSecureContext &&
    navigator.clipboard?.writeText
  ) {
    try {
      await navigator.clipboard.writeText(text);
      return true;
    } catch {
      return copyTextToClipboardSync(text, source);
    }
  }
  return copyTextToClipboardSync(text, source);
}

function copyFromElement(el: HTMLInputElement | HTMLTextAreaElement, text: string): boolean {
  const value = el.value || text;
  if (!value) return false;
  el.focus({ preventScroll: true });
  el.select();
  try {
    el.setSelectionRange(0, value.length);
  } catch {
    /* readonly inputs: select() is enough on most browsers */
  }
  return execCopyWithClipboardData(text);
}

function copyViaExecCommand(text: string): boolean {
  const textarea = document.createElement("textarea");
  textarea.value = text;
  textarea.setAttribute("aria-hidden", "true");
  textarea.style.cssText =
    "position:fixed;top:0;left:0;width:2em;height:2em;padding:0;border:none;outline:none;box-shadow:none;background:transparent;opacity:0;pointer-events:none;";
  document.body.appendChild(textarea);
  textarea.focus({ preventScroll: true });
  textarea.select();
  const ok = execCopyWithClipboardData(text);
  textarea.remove();
  return ok;
}

/** execCommand('copy') with an explicit clipboard payload (works on LAN HTTP). */
function execCopyWithClipboardData(text: string): boolean {
  const onCopy = (event: ClipboardEvent) => {
    event.clipboardData?.setData("text/plain", text);
    event.preventDefault();
  };
  document.addEventListener("copy", onCopy);
  let ok = false;
  try {
    ok = document.execCommand("copy");
  } catch {
    ok = false;
  } finally {
    document.removeEventListener("copy", onCopy);
  }
  return ok;
}

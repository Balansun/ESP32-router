import { api } from "../../api/client";
import { applyPublicBootstrap } from "../../api/publicBootstrap";
import { clearSession } from "../../api/apiSession";
import { requestHttpAuthLogin } from "../../auth/httpAuthGate";
import { buildFieldLabelRow } from "../../components/FieldHelp";
import { toast } from "../../components/Toast";
import { getStrings } from "../../i18n";
import { h } from "../../utils/dom";

export async function buildHttpAuthSection(
  T: ReturnType<typeof getStrings>,
  signal: AbortSignal,
  opts?: {
    helpScope?: "settings" | "api";
    /** Called after HTTP API password is set or cleared (e.g. refresh PAT list). */
    onHttpAuthChanged?: () => void | Promise<void>;
  },
): Promise<HTMLElement> {
  const helpScope = opts?.helpScope ?? "api";
  const A = T.apiPage;
  const statusEl = h("p", { class: "card__sub" }, T.loading);
  const passInput = h("input", {
    type: "password",
    class: "field__input",
    autocomplete: "new-password",
  }) as HTMLInputElement;

  let authEnabled = false;

  const signOutBtn = h(
    "button",
    {
      type: "button",
      class: "btn btn--ghost",
      hidden: true,
      onClick: () => {
        void requestHttpAuthLogin();
      },
    },
    T.httpAuth.signOut,
  ) as HTMLButtonElement;

  async function refreshStatus() {
    try {
      const pub = await api.getPublic({ signal });
      applyPublicBootstrap(pub);
      authEnabled = pub.http_auth.enabled;
      statusEl.textContent = authEnabled ? A.httpAuthEnabled : A.httpAuthDisabled;
      signOutBtn.hidden = !authEnabled;
    } catch {
      statusEl.textContent = T.status.error;
    }
  }

  await refreshStatus();

  return h(
    "section",
    { class: "card" },
    h("h2", { class: "section__title" }, A.sectionHttpAuth),
    h("p", { class: "field__hint" }, A.httpAuthHint),
    h("p", { class: "field__label" }, A.httpAuthStatus),
    statusEl,
    h(
      "div",
      { class: "field" },
      buildFieldLabelRow({
        label: A.httpAuthPassword,
        helpScope,
        helpKey: "http_auth_password",
      }),
      passInput,
    ),
    h(
      "div",
      { class: "row", style: "gap:8px;flex-wrap:wrap;" },
      h(
        "button",
        {
          type: "button",
          class: "btn btn--primary",
          onClick: async () => {
            const pw = passInput.value;
            if (!pw) {
              toast(A.httpAuthPassword, "error");
              return;
            }
            try {
              const res = await api.putHttpAuthPasswordOpen(pw.trim(), { signal });
              if (pw.trim() && !res.enabled) {
                toast(T.saveError, "error");
                return;
              }
              passInput.value = "";
              await refreshStatus();
              await opts?.onHttpAuthChanged?.();
              toast(T.saved, "success");
              if (res.enabled) {
                clearSession();
                void requestHttpAuthLogin();
              }
            } catch {
              toast(T.saveError, "error");
            }
          },
        },
        A.httpAuthSet,
      ),
      h(
        "button",
        {
          type: "button",
          class: "btn btn--ghost",
          onClick: async () => {
            try {
              await api.putHttpAuthPasswordOpen("", { signal });
              clearSession();
              passInput.value = "";
              await refreshStatus();
              await opts?.onHttpAuthChanged?.();
              toast(T.saved, "success");
            } catch {
              toast(T.saveError, "error");
            }
          },
        },
        A.httpAuthClear,
      ),
      signOutBtn,
    ),
  );
}

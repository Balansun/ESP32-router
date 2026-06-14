import { h } from "../utils/dom";
import { getStrings } from "../i18n";
import { fieldHelpDocUrl } from "../fieldHelp/docUrl";
import type { FieldHelpTable } from "../i18n/locales/fieldHelp.en";

export type FieldHelpScope = keyof FieldHelpTable;

function resolveHelp(scope: FieldHelpScope, key: string): string | undefined {
  const table = getStrings().fieldHelp[scope] as Record<string, string | undefined>;
  return table[key];
}

let fieldHelpDismissBound = false;

function bindFieldHelpDismissOnClickOutside(): void {
  if (fieldHelpDismissBound) return;
  fieldHelpDismissBound = true;
  document.addEventListener(
    "pointerdown",
    (event) => {
      const target = event.target;
      if (!(target instanceof Node)) return;
      document
        .querySelectorAll<HTMLDetailsElement>("details.field-help[open]")
        .forEach((details) => {
          if (!details.contains(target)) details.open = false;
        });
    },
    true,
  );
}

export interface FieldHelpDocRef {
  scope: FieldHelpScope;
  key: string;
}

/** Expandable inline help (? trigger) with optional link to repo markdown. */
export function buildInlineFieldHelp(
  detail: string | readonly string[],
  doc?: FieldHelpDocRef,
): HTMLElement {
  bindFieldHelpDismissOnClickOutside();
  const T = getStrings();
  const paragraphs = Array.isArray(detail) ? detail : [detail];
  const bodyChildren: HTMLElement[] = paragraphs.map((p) => h("p", {}, p));
  if (doc) {
    bodyChildren.push(
      h(
        "p",
        { class: "field-help__doc" },
        h(
          "a",
          {
            href: fieldHelpDocUrl(doc.scope, doc.key),
            target: "_blank",
            rel: "noopener noreferrer",
          },
          T.common.fieldHelpDocLink,
        ),
      ),
    );
  }
  return h(
    "details",
    { class: "field-help" },
    h(
      "summary",
      { class: "field-help__trigger", "aria-label": T.common.helpAria },
      "?",
    ),
    h("div", { class: "field-help__body" }, ...bodyChildren),
  );
}

export interface FieldLabelRowOpts {
  label: string;
  forId?: string;
  helpScope?: FieldHelpScope;
  helpKey?: string;
  /** Pre-resolved text (overrides scope/key). */
  helpDetail?: string | readonly string[];
}

/** Label row with optional ? help beside the label. */
export function buildFieldLabelRow(opts: FieldLabelRowOpts): HTMLElement {
  const detail =
    opts.helpDetail ??
    (opts.helpScope && opts.helpKey ? resolveHelp(opts.helpScope, opts.helpKey) : undefined);
  const labelEl = opts.forId
    ? h("label", { class: "field__label", for: opts.forId }, opts.label)
    : h("span", { class: "field__label" }, opts.label);
  const row = h("div", { class: "field__label-row" }, labelEl);
  if (detail) {
    const doc =
      opts.helpScope && opts.helpKey && !opts.helpDetail
        ? { scope: opts.helpScope, key: opts.helpKey }
        : undefined;
    row.append(buildInlineFieldHelp(detail, doc));
  }
  return row;
}

/** Switch row: switch label + optional ? on the same row. */
export function wrapSwitchWithHelp(
  switchEl: HTMLElement,
  helpScope: FieldHelpScope,
  helpKey: string,
): HTMLElement {
  const detail = resolveHelp(helpScope, helpKey);
  const row = h("div", { class: "field__label-row field__label-row--switch" }, switchEl);
  if (detail) row.append(buildInlineFieldHelp(detail, { scope: helpScope, key: helpKey }));
  return row;
}

/** Section title row with ? (backup cards, etc.). */
export function buildSectionTitleWithHelp(
  title: string,
  helpScope: FieldHelpScope,
  helpKey: string,
): HTMLElement {
  const detail = resolveHelp(helpScope, helpKey);
  const row = h("div", { class: "section__title-row" }, h("h2", { class: "section__title" }, title));
  if (detail) row.append(buildInlineFieldHelp(detail, { scope: helpScope, key: helpKey }));
  return row;
}

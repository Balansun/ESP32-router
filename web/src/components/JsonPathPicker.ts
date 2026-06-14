import { h } from "../utils/dom";
import { indexJsonPaths, normalizeJsonPath, type JsonPathEntry } from "../utils/jsonPathIndex";

export interface JsonPathPickerOptions {
  initialPath?: string;
  onSelect: (path: string) => void;
}

export function buildJsonPathPicker(
  jsonText: string,
  opts: JsonPathPickerOptions,
): HTMLElement {
  let entries: JsonPathEntry[] = [];
  let parseError = "";
  try {
    entries = indexJsonPaths(JSON.parse(jsonText));
  } catch (e) {
    parseError = e instanceof Error ? e.message : String(e);
  }
  const root = h("div", { class: "json-path-picker" });
  if (parseError) {
    root.append(h("p", { class: "err" }, `JSON invalide: ${parseError}`));
    return root;
  }
  const search = h("input", {
    class: "input",
    type: "search",
    placeholder: "Rechercher un champ JSON...",
  }) as HTMLInputElement;
  const selectedEl = h("p", { class: "field__hint" });
  const list = h("div", { class: "json-path-picker__list" });
  const rows = entries.filter((e) => e.path && (e.kind === "number" || e.kind === "string" || e.kind === "boolean"));
  let selectedPath = normalizeJsonPath(opts.initialPath ?? "");

  function renderRows() {
    list.replaceChildren();
    const q = search.value.trim().toLowerCase();
    const filtered = rows.filter((r) => !q || r.pathDisplay.toLowerCase().includes(q));
    for (const row of filtered) {
      const btn = h(
        "button",
        {
          type: "button",
          class: `btn btn--ghost`,
          style: "justify-content:flex-start;width:100%;margin-bottom:4px;",
          onClick: () => {
            selectedPath = row.path;
            selectedEl.textContent = `Champ sélectionné : ${row.pathDisplay}`;
            opts.onSelect(selectedPath);
          },
        },
        `${row.pathDisplay} = ${String(row.value)}`,
      );
      if (selectedPath === row.path) btn.classList.add("btn--primary");
      list.append(btn);
    }
  }
  search.addEventListener("input", renderRows);
  selectedEl.textContent = selectedPath
    ? `Champ sélectionné : $.${selectedPath}`
    : "Sélectionnez un champ numérique";
  renderRows();
  root.append(search, selectedEl, list);
  return root;
}

export interface JsonPathEntry {
  path: string;
  pathDisplay: string;
  value: unknown;
  kind: "number" | "string" | "boolean" | "object" | "array" | "null";
}

export function normalizeJsonPath(path: string): string {
  let p = path.trim();
  if (p === "$") return "";
  if (p.startsWith("$.")) p = p.slice(2);
  if (p.startsWith("$")) p = p.slice(1);
  while (p.startsWith(".")) p = p.slice(1);
  return p;
}

export function pathDisplay(path: string): string {
  const p = normalizeJsonPath(path);
  return p ? `$.${p}` : "$";
}

export function indexJsonPaths(value: unknown): JsonPathEntry[] {
  const out: JsonPathEntry[] = [];
  const walk = (node: unknown, path: string) => {
    const t = typeof node;
    if (node === null) {
      out.push({ path, pathDisplay: pathDisplay(path), value: null, kind: "null" });
      return;
    }
    if (Array.isArray(node)) {
      out.push({ path, pathDisplay: pathDisplay(path), value: node, kind: "array" });
      node.forEach((it, i) => walk(it, path ? `${path}.${i}` : String(i)));
      return;
    }
    if (t === "object") {
      out.push({ path, pathDisplay: pathDisplay(path), value: node, kind: "object" });
      for (const [k, v] of Object.entries(node as Record<string, unknown>)) {
        walk(v, path ? `${path}.${k}` : k);
      }
      return;
    }
    if (t === "number") out.push({ path, pathDisplay: pathDisplay(path), value: node, kind: "number" });
    else if (t === "boolean") out.push({ path, pathDisplay: pathDisplay(path), value: node, kind: "boolean" });
    else out.push({ path, pathDisplay: pathDisplay(path), value: node, kind: "string" });
  };
  walk(value, "");
  return out;
}

export function getByJsonPath(value: unknown, rawPath: string): unknown {
  const path = normalizeJsonPath(rawPath);
  if (!path) return value;
  const parts = path.split(".").filter(Boolean);
  let cur: unknown = value;
  for (const part of parts) {
    if (cur === null || cur === undefined) return undefined;
    if (Array.isArray(cur)) {
      const idx = Number(part);
      if (!Number.isInteger(idx)) return undefined;
      cur = cur[idx];
      continue;
    }
    if (typeof cur !== "object") return undefined;
    cur = (cur as Record<string, unknown>)[part];
  }
  return cur;
}

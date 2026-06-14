/** Vite `base` — always `'/'` (dev and firmware build). */
export function normalizeBase(): string {
  let b = import.meta.env.BASE || "/";
  if (!b.startsWith("/")) b = `/${b}`;
  if (b !== "/" && !b.endsWith("/")) b += "/";
  return b;
}

/** Map logical route (`/reglages`) to browser path (same as logical when base is `/`). */
export function withBase(logicalPath: string): string {
  const p = logicalPath.startsWith("/") ? logicalPath : `/${logicalPath}`;
  const b = normalizeBase();
  if (b === "/") return p;
  return `${b.replace(/\/$/, "")}${p}`;
}

/** Map `location.pathname` to logical route key used by `register()`. */
export function stripBase(pathname: string): string {
  const raw = pathname || "/";
  const b = normalizeBase();
  if (b === "/") return raw;
  const prefix = b.replace(/\/$/, "");
  if (raw === prefix || raw === `${prefix}/`) return "/";
  if (raw.startsWith(`${prefix}/`)) {
    const rest = raw.slice(prefix.length);
    /* v8 ignore next -- `rest` always starts with `/` when `raw` matches `${prefix}/` */
    return rest.startsWith("/") ? rest : `/${rest}`;
  }
  return raw;
}

/** Logical route for matching (strips app base and trailing slashes). */
export function normalizeLogicalPath(pathname: string): string {
  let p = stripBase(pathname || "/");
  if (p.length > 1 && p.endsWith("/")) p = p.slice(0, -1);
  return p;
}

/** Target URL for `history.pushState` / `replaceState` (full path under the app base). */
export function toBrowserPath(path: string): string {
  const p = path.startsWith("/") ? path : `/${path}`;
  const b = normalizeBase();
  if (b === "/") return p;
  const prefix = b.replace(/\/$/, "");
  if (p === prefix || p.startsWith(`${prefix}/`)) return p;
  return withBase(p);
}

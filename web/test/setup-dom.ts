import { beforeEach, vi } from "vitest";

const { localStorageBacking, sessionStorageBacking } = vi.hoisted(() => {
  const localStorageBacking = new Map<string, string>();
  const sessionStorageBacking = new Map<string, string>();

  const makeStorage = (backing: Map<string, string>) => ({
    getItem: (k: string) => backing.get(k) ?? null,
    setItem: (k: string, v: string) => {
      backing.set(k, v);
    },
    removeItem: (k: string) => {
      backing.delete(k);
    },
    clear: () => backing.clear(),
    key: (i: number) => Array.from(backing.keys())[i] ?? null,
    get length() {
      return backing.size;
    },
  });

  vi.stubGlobal("localStorage", makeStorage(localStorageBacking));
  vi.stubGlobal("sessionStorage", makeStorage(sessionStorageBacking));
  return { localStorageBacking, sessionStorageBacking };
});

import { localePref } from "../src/state/store";

beforeEach(() => {
  localStorageBacking.clear();
  sessionStorageBacking.clear();
  localePref.set("en");
});

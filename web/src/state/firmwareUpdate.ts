/** GitHub release check: update-available flag for app bar badge (browser-only). */

export type FirmwareUpdateState = {
  /** True only when GitHub publishes a strictly newer release than the device. */
  available: boolean;
  /** `tag_name` of the newer release when `available`; empty otherwise. */
  releaseTag: string;
  checking: boolean;
  /** -1 newer on GitHub, 0 same, 1 device ahead; null before first check. */
  compare: -1 | 0 | 1 | null;
  lastCheckedAt: string;
  error: string;
};

const defaultState: FirmwareUpdateState = {
  available: false,
  releaseTag: "",
  checking: false,
  compare: null,
  lastCheckedAt: "",
  error: "",
};

type Listener = (s: FirmwareUpdateState) => void;

let state = { ...defaultState };
const listeners = new Set<Listener>();

export const firmwareUpdate = {
  get(): FirmwareUpdateState {
    return state;
  },
  set(next: Partial<FirmwareUpdateState>): void {
    state = { ...state, ...next };
    for (const fn of listeners) fn(state);
  },
  subscribe(fn: Listener): () => void {
    listeners.add(fn);
    fn(state);
    return () => listeners.delete(fn);
  },
  reset(): void {
    state = { ...defaultState };
    for (const fn of listeners) fn(state);
  },
};

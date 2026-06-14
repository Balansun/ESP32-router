import { readFileSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";
import {
  configForBackupImport,
  FIRMWARE_PUT_REQUIRED_KEYS,
  PERSISTED_CONFIG_KEYS,
  pickPersistedConfig,
  READ_ONLY_CONFIG_KEYS,
  ensureRouterConfigPutPayload,
} from "../src/api/configPut";
import { PERSISTED_PIN_MAP_KEYS } from "../src/api/pinMapMerge";
import type { ActionsConfigEnvelope, RouterConfig } from "../src/api/types";
import {
  BACKUP_SCHEMA_VERSION,
  buildBackup,
  normalizeActionsForBackup,
  parseBackupJson,
  sanitizeConfigForImport,
} from "../src/utils/backupFormat";

const fixtureDir = join(dirname(fileURLToPath(import.meta.url)), "fixtures");
const backupFixturePath = join(fixtureDir, "balansun-backup.json");

function sampleActions(): ActionsConfigEnvelope {
  return {
    schema_version: 4,
    nb_actions: 1,
    actions: [
      {
        index: 0,
        title: "Tank",
        kind: "triac",
        repeat_sec: 0,
        tempo_sec: 0,
        regulation_mode: 1,
        periods: [],
      },
    ],
  };
}

function sampleConfig(): RouterConfig {
  return ensureRouterConfigPutPayload({
    source: "JsyMk194",
    router_name: "Balansun Test",
    mqtt_prefix: "balansun",
  } as RouterConfig);
}

describe("backup format", () => {
  it("normalizeActionsForBackup fills nb_actions from normalized list", () => {
    const env = normalizeActionsForBackup({
      schema_version: 4,
      nb_actions: 99,
      actions: [],
    });
    expect(env.nb_actions).toBe(env.actions.length);
  });

  it("normalizeActionsForBackup treats null actions as empty", () => {
    const env = normalizeActionsForBackup({
      schema_version: 4,
      nb_actions: 0,
      actions: null as unknown as ActionsConfigEnvelope["actions"],
    });
    expect(env.actions.length).toBeGreaterThan(0);
  });

  it("normalizeActionsForBackup preserves existing actions", () => {
    const env = normalizeActionsForBackup({
      schema_version: 4,
      nb_actions: 0,
      actions: [],
    });
    expect(env.nb_actions).toBe(env.actions.length);
    expect(env.actions.length).toBeGreaterThan(0);
  });

  it("exports default triac when device returns empty actions array", () => {
    const doc = buildBackup(
      sampleConfig(),
      { schema_version: 4, nb_actions: 0, actions: [] },
      { tz: "CET-1CEST", ntp1: "pool.ntp.org", ntp2: "time.google.com" },
      { ssid: "home-wifi", password: "" },
    );
    expect(doc.actions.actions.length).toBeGreaterThan(0);
    expect(doc.actions.nb_actions).toBe(doc.actions.actions.length);
    const r = parseBackupJson(JSON.stringify(doc));
    expect(r.ok).toBe(true);
  });

  it("imports older backup files that have empty actions array", () => {
    const doc = buildBackup(
      sampleConfig(),
      sampleActions(),
      { tz: "CET-1CEST", ntp1: "pool.ntp.org", ntp2: "time.google.com" },
      { ssid: "home-wifi", password: "wpa-secret" },
    );
    (doc.actions as { actions: unknown[] }).actions = [];
    const r = parseBackupJson(JSON.stringify(doc));
    expect(r.ok).toBe(true);
    if (r.ok) expect(r.backup.actions.actions.length).toBeGreaterThan(0);
  });

  it("parseBackupJson defaults nb_actions from actions array length", () => {
    const doc = buildBackup(
      sampleConfig(),
      { schema_version: 4, actions: sampleActions().actions } as ActionsConfigEnvelope,
      { tz: "UTC", ntp1: "pool.ntp.org", ntp2: "time.google.com" },
      { ssid: "home-wifi", password: "" },
    );
    const parsed = JSON.parse(JSON.stringify(doc)) as Record<string, unknown>;
    const actions = parsed["actions"] as Record<string, unknown>;
    delete actions["nb_actions"];
    const r = parseBackupJson(JSON.stringify(parsed));
    expect(r.ok).toBe(true);
    if (r.ok) {
      expect(r.backup.actions.nb_actions).toBe(r.backup.actions.actions.length);
    }
  });

  it("round-trips via buildBackup and parseBackupJson", () => {
    const doc = buildBackup(
      sampleConfig(),
      sampleActions(),
      { tz: "CET-1CEST", ntp1: "pool.ntp.org", ntp2: "time.google.com" },
      { ssid: "home-wifi", password: "wpa-secret" },
    );
    expect(doc.backupSchemaVersion).toBe(BACKUP_SCHEMA_VERSION);
    const raw = JSON.stringify(doc);
    const r = parseBackupJson(raw);
    expect(r.ok).toBe(true);
    if (r.ok) {
      expect(r.backup.config.source).toBe("JsyMk194");
      expect(r.backup.wifi.ssid).toBe("home-wifi");
    }
  });

  it("loads balansun-backup.json fixture", () => {
    if (!process.env.VITEST_UPDATE_FIXTURES) {
      const raw = readFileSync(backupFixturePath, "utf8");
      const r = parseBackupJson(raw);
      expect(r.ok).toBe(true);
      if (r.ok) {
        expect(r.backup.config.router_name).toBe("Balansun Test");
      }
      return;
    }
    const doc = buildBackup(
      sampleConfig(),
      sampleActions(),
      { tz: "CET-1CEST", ntp1: "pool.ntp.org", ntp2: "time.google.com" },
      { ssid: "home-wifi", password: "wpa-secret" },
    );
    writeFileSync(backupFixturePath, JSON.stringify(doc, null, 2) + "\n");
    expect(parseBackupJson(JSON.stringify(doc)).ok).toBe(true);
  });

  it("rejects unsupported backupSchemaVersion", () => {
    const raw = JSON.stringify({
      backupSchemaVersion: 2,
      exportedAt: "2026-01-01T00:00:00.000Z",
      config: {},
      actions: { schema_version: 4, nb_actions: 0, actions: [] },
      time: { tz: "UTC", ntp1: "a", ntp2: "b" },
      wifi: { ssid: "net", password: "" },
    });
    const r = parseBackupJson(raw);
    expect(r.ok).toBe(false);
    if (!r.ok) expect(r.errorKey).toBe("badSchemaVersion");
  });

  it("pickPersistedConfig strips read-only GET fields", () => {
    const cfg = {
      ...sampleConfig(),
      mains_frequency_effective_hz: 50,
      mains_frequency_source: "meter",
      mains_frequency_warning: "warn",
    } as RouterConfig;
    const picked = pickPersistedConfig(cfg);
    for (const k of READ_ONLY_CONFIG_KEYS) {
      expect(k in picked).toBe(false);
    }
    expect(picked.source).toBe("JsyMk194");
  });

  it("sanitizeConfigForImport removes read-only keys", () => {
    const cfg = {
      ...sampleConfig(),
      mains_frequency_effective_hz: 50,
    } as RouterConfig;
    const out = sanitizeConfigForImport(cfg);
    expect(out.mains_frequency_effective_hz).toBeUndefined();
  });
});

describe("config put keys", () => {
  it("includes all firmware PUT required keys in persisted set", () => {
    for (const k of FIRMWARE_PUT_REQUIRED_KEYS) {
      expect((PERSISTED_CONFIG_KEYS as readonly string[]).includes(k)).toBe(true);
    }
  });

  it("includes action_daily_cap_wh in persisted optional keys", () => {
    expect(PERSISTED_CONFIG_KEYS).toContain("action_daily_cap_wh");
  });

  it("pickPersistedConfig merges pin map from /system/pins", () => {
    const cfg = sampleConfig();
    const picked = pickPersistedConfig(cfg, {
      pins: {
        pin_triac_dim: 18,
        pin_uart_rx: 26,
        board_pin_profile: "wroom32",
      },
    });
    expect(picked.pin_triac_dim).toBe(18);
    expect(picked.pin_uart_rx).toBe(26);
    expect(picked.board_pin_profile).toBe("wroom32");
    expect(PERSISTED_PIN_MAP_KEYS).toContain("pin_triac_dim");
  });
});

describe("backup export/import parity", () => {
  it("parseBackupJson preserves every persisted config key from buildBackup", () => {
    const doc = buildBackup(
      sampleConfig(),
      sampleActions(),
      { tz: "CET-1CEST", ntp1: "pool.ntp.org", ntp2: "time.google.com" },
      { ssid: "home-wifi", password: "wpa-secret" },
    );
    const r = parseBackupJson(JSON.stringify(doc));
    expect(r.ok).toBe(true);
    if (!r.ok) return;
    for (const k of PERSISTED_CONFIG_KEYS) {
      expect(r.backup.config[k]).toEqual(doc.config[k]);
    }
    expect(r.backup.actions).toEqual(doc.actions);
    expect(r.backup.time).toEqual(doc.time);
    expect(r.backup.wifi).toEqual(doc.wifi);
  });

  it("round-trips pmqtt_bindings when present on export", () => {
    const bindings = [
      {
        metric: "house_p",
        topic: "solar_router/solar_router_state",
        format: "json" as const,
        path: "PuissanceS_M",
        enabled: true,
      },
    ];
    const cfg = ensureRouterConfigPutPayload({
      ...sampleConfig(),
      source: "Pmqtt",
      mqtt_ip: "192.168.1.101",
      pmqtt_bindings: bindings,
    } as RouterConfig);
    const doc = buildBackup(
      cfg,
      sampleActions(),
      { tz: "UTC", ntp1: "a", ntp2: "b" },
      { ssid: "net", password: "" },
    );
    expect(doc.config.pmqtt_bindings).toEqual(bindings);
    const r = parseBackupJson(JSON.stringify(doc));
    expect(r.ok).toBe(true);
    if (r.ok) {
      expect(r.backup.config.pmqtt_bindings).toEqual(bindings);
      expect(r.backup.config.source).toBe("Pmqtt");
    }
  });

  it("exports empty pmqtt_bindings array when device returns []", () => {
    const cfg = ensureRouterConfigPutPayload({
      ...sampleConfig(),
      source: "Pmqtt",
      pmqtt_bindings: [],
    } as RouterConfig);
    const doc = buildBackup(
      cfg,
      sampleActions(),
      { tz: "UTC", ntp1: "a", ntp2: "b" },
      { ssid: "net", password: "" },
    );
    expect(doc.config.pmqtt_bindings).toEqual([]);
  });

  it("exports pmqtt_bindings [] for Pmqtt when GET omits the field", () => {
    const cfg = ensureRouterConfigPutPayload({
      ...sampleConfig(),
      source: "Pmqtt",
      mqtt_ip: "192.168.1.101",
    } as RouterConfig);
    expect(cfg.pmqtt_bindings).toBeUndefined();
    const doc = buildBackup(
      cfg,
      sampleActions(),
      { tz: "UTC", ntp1: "a", ntp2: "b" },
      { ssid: "net", password: "" },
    );
    expect(doc.config.pmqtt_bindings).toEqual([]);
  });

  it("configForBackupImport always PUTs pmqtt_bindings for Pmqtt", () => {
    const cfg = ensureRouterConfigPutPayload({
      ...sampleConfig(),
      source: "Pmqtt",
    } as RouterConfig);
    const out = configForBackupImport(cfg);
    expect(out.pmqtt_bindings).toEqual([]);
  });

  it("parseBackupJson rejects Pmqtt with non-array pmqtt_bindings", () => {
    const doc = buildBackup(
      ensureRouterConfigPutPayload({
        ...sampleConfig(),
        source: "Pmqtt",
        pmqtt_bindings: [],
      } as RouterConfig),
      sampleActions(),
      { tz: "UTC", ntp1: "a", ntp2: "b" },
      { ssid: "net", password: "" },
    );
    (doc.config as { pmqtt_bindings?: unknown }).pmqtt_bindings = "bad";
    const r = parseBackupJson(JSON.stringify(doc));
    expect(r.ok).toBe(false);
    if (!r.ok) expect(r.errorKey).toBe("badPmqttBindings");
  });

  it("parseBackupJson rejects Pmqtt backup without pmqtt_bindings", () => {
    const doc = buildBackup(
      ensureRouterConfigPutPayload({
        ...sampleConfig(),
        source: "Pmqtt",
        pmqtt_bindings: [{ metric: "house_p", topic: "t", format: "json" }],
      } as RouterConfig),
      sampleActions(),
      { tz: "UTC", ntp1: "a", ntp2: "b" },
      { ssid: "net", password: "" },
    );
    delete (doc.config as { pmqtt_bindings?: unknown }).pmqtt_bindings;
    const r = parseBackupJson(JSON.stringify(doc));
    expect(r.ok).toBe(false);
    if (!r.ok) expect(r.errorKey).toBe("missingPmqttBindings");
  });
});

describe("backup api block", () => {
  it("parseBackupJson accepts optional api with access tokens", () => {
    const doc = buildBackup(
      ensureRouterConfigPutPayload(sampleConfig() as RouterConfig),
      sampleActions(),
      { tz: "UTC", ntp1: "a", ntp2: "b" },
      { ssid: "net", password: "pw" },
    );
    const withApi = {
      ...doc,
      api: {
        http_api_password: "secret",
        access_tokens: [{ id: 2, label: "HA", token: "b".repeat(64) }],
      },
    };
    const r = parseBackupJson(JSON.stringify(withApi));
    expect(r.ok).toBe(true);
    if (r.ok) {
      expect(r.backup.api?.http_api_password).toBe("secret");
      expect(r.backup.api?.access_tokens?.[0]?.token).toHaveLength(64);
    }
  });

  it("parseBackupJson rejects bad api section shape", () => {
    const doc = buildBackup(
      ensureRouterConfigPutPayload(sampleConfig() as RouterConfig),
      sampleActions(),
      { tz: "UTC", ntp1: "a", ntp2: "b" },
      { ssid: "net", password: "" },
    );
    const r = parseBackupJson(JSON.stringify({ ...doc, api: "nope" }));
    expect(r.ok).toBe(false);
    if (!r.ok) expect(r.errorKey).toBe("badApi");
  });

  it("parseBackupJson rejects invalid http_api_password type", () => {
    const doc = buildBackup(
      ensureRouterConfigPutPayload(sampleConfig() as RouterConfig),
      sampleActions(),
      { tz: "UTC", ntp1: "a", ntp2: "b" },
      { ssid: "net", password: "" },
    );
    const r = parseBackupJson(JSON.stringify({ ...doc, api: { http_api_password: 42 } }));
    expect(r.ok).toBe(false);
    if (!r.ok) expect(r.errorKey).toBe("badApi");
  });

  it("parseBackupJson rejects more than four access_tokens", () => {
    const doc = buildBackup(
      ensureRouterConfigPutPayload(sampleConfig() as RouterConfig),
      sampleActions(),
      { tz: "UTC", ntp1: "a", ntp2: "b" },
      { ssid: "net", password: "" },
    );
    const tokens = Array.from({ length: 5 }, (_, i) => ({
      id: i + 1,
      label: `t${i}`,
      token: "a".repeat(64),
    }));
    const r = parseBackupJson(JSON.stringify({ ...doc, api: { access_tokens: tokens } }));
    expect(r.ok).toBe(false);
    if (!r.ok) expect(r.errorKey).toBe("badApiTokens");
  });

  it("parseBackupJson rejects non-object token entries", () => {
    const doc = buildBackup(
      ensureRouterConfigPutPayload(sampleConfig() as RouterConfig),
      sampleActions(),
      { tz: "UTC", ntp1: "a", ntp2: "b" },
      { ssid: "net", password: "" },
    );
    const r = parseBackupJson(
      JSON.stringify({ ...doc, api: { access_tokens: ["bad"] } }),
    );
    expect(r.ok).toBe(false);
    if (!r.ok) expect(r.errorKey).toBe("badApiTokens");
  });

  it("parseBackupJson rejects non-array access_tokens", () => {
    const doc = buildBackup(
      ensureRouterConfigPutPayload(sampleConfig() as RouterConfig),
      sampleActions(),
      { tz: "UTC", ntp1: "a", ntp2: "b" },
      { ssid: "net", password: "" },
    );
    const r = parseBackupJson(
      JSON.stringify({ ...doc, api: { access_tokens: {} } }),
    );
    expect(r.ok).toBe(false);
    if (!r.ok) expect(r.errorKey).toBe("badApiTokens");
  });

  it("parseBackupJson rejects token entry missing id", () => {
    const doc = buildBackup(
      ensureRouterConfigPutPayload(sampleConfig() as RouterConfig),
      sampleActions(),
      { tz: "UTC", ntp1: "a", ntp2: "b" },
      { ssid: "net", password: "" },
    );
    const r = parseBackupJson(
      JSON.stringify({
        ...doc,
        api: {
          access_tokens: [
            { label: "x", token: "b".repeat(64) },
          ],
        },
      }),
    );
    expect(r.ok).toBe(false);
    if (!r.ok) expect(r.errorKey).toBe("badApiTokens");
  });

  it("parseBackupJson omits empty api block", () => {
    const doc = buildBackup(
      ensureRouterConfigPutPayload(sampleConfig() as RouterConfig),
      sampleActions(),
      { tz: "UTC", ntp1: "a", ntp2: "b" },
      { ssid: "net", password: "" },
    );
    const r = parseBackupJson(
      JSON.stringify({ ...doc, api: { http_api_password: "", access_tokens: [] } }),
    );
    expect(r.ok).toBe(true);
    if (r.ok) expect(r.backup.api).toBeUndefined();
  });

  it("parseBackupJson rejects invalid token hex", () => {
    const doc = buildBackup(
      ensureRouterConfigPutPayload(sampleConfig() as RouterConfig),
      sampleActions(),
      { tz: "UTC", ntp1: "a", ntp2: "b" },
      { ssid: "net", password: "" },
    );
    const bad = {
      ...doc,
      api: { access_tokens: [{ id: 1, label: "x", token: "not-hex" }] },
    };
    const r = parseBackupJson(JSON.stringify(bad));
    expect(r.ok).toBe(false);
    if (!r.ok) expect(r.errorKey).toBe("badApiTokens");
  });
});

describe("backup action_daily_cap_wh", () => {
  it("round-trips daily cap array in config", () => {
    const cfg = ensureRouterConfigPutPayload({
      ...sampleConfig(),
      action_daily_cap_wh: [5000, 0, 0],
    } as RouterConfig);
    const doc = buildBackup(
      cfg,
      sampleActions(),
      { tz: "CET-1CEST", ntp1: "pool.ntp.org", ntp2: "time.google.com" },
      { ssid: "home-wifi", password: "" },
    );
    expect(doc.config.action_daily_cap_wh).toEqual([5000, 0, 0]);
    const r = parseBackupJson(JSON.stringify(doc));
    expect(r.ok).toBe(true);
    if (r.ok) {
      expect(r.backup.config.action_daily_cap_wh).toEqual([5000, 0, 0]);
    }
  });
});

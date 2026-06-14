import { describe, expect, it } from "vitest";
import {
  backupDownloadFilename,
  buildBackup,
  parseBackupJson,
} from "../src/utils/backupFormat";
import { ensureRouterConfigPutPayload } from "../src/api/configPut";
import type { ActionsConfigEnvelope, RouterConfig } from "../src/api/types";

function sampleConfig(): RouterConfig {
  return ensureRouterConfigPutPayload({
    source: "JsyMk194",
    router_name: "Balansun Test",
    mqtt_prefix: "balansun",
  } as RouterConfig);
}

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

function minimalBackupDoc(overrides: Record<string, unknown> = {}) {
  const doc = buildBackup(
    sampleConfig(),
    sampleActions(),
    { tz: "UTC", ntp1: "a", ntp2: "b" },
    { ssid: "s", password: "" },
  );
  return { ...doc, ...overrides };
}

describe("parseBackupJson errors", () => {
  it("rejects invalid json and shapes", () => {
    const badJson = parseBackupJson("{");
    expect(badJson.ok).toBe(false);
    if (!badJson.ok) expect(badJson.errorKey).toBe("invalidJson");
    const notObj = parseBackupJson("[]");
    expect(notObj.ok).toBe(false);
    if (!notObj.ok) expect(notObj.errorKey).toBe("notObject");
  });

  it("rejects missing exportedAt", () => {
    const base = minimalBackupDoc();
    delete (base as { exportedAt?: string }).exportedAt;
    const r = parseBackupJson(JSON.stringify(base));
    expect(r.ok).toBe(false);
    if (!r.ok) expect(r.errorKey).toBe("missingExportedAt");
  });

  it("backupDownloadFilename format", () => {
    expect(backupDownloadFilename()).toMatch(/^balansun-\d{4}-\d{2}-\d{2}\.json$/);
  });

  it("rejects schema, keys, and nested sections", () => {
    const base = minimalBackupDoc();
    const cases: Array<{ doc: Record<string, unknown>; key: string }> = [
      { doc: { ...base, backupSchemaVersion: 2 }, key: "badSchemaVersion" },
      { doc: { ...base, extra: 1 }, key: "unknownTopLevelKey" },
      { doc: { ...base, config: null }, key: "missingConfig" },
      { doc: { ...base, actions: null }, key: "missingActions" },
      { doc: { ...base, time: null }, key: "missingTime" },
      { doc: { ...base, wifi: null }, key: "missingWifi" },
    ];
    for (const { doc, key } of cases) {
      const r = parseBackupJson(JSON.stringify(doc));
      expect(r.ok, key).toBe(false);
      if (!r.ok) expect(r.errorKey).toBe(key);
    }
  });

  it("rejects incomplete config and bad actions envelope", () => {
    const base = minimalBackupDoc();
    const cfg = { ...(base.config as Record<string, unknown>) };
    delete cfg.source;
    const incomplete = parseBackupJson(JSON.stringify({ ...base, config: cfg }));
    expect(incomplete.ok).toBe(false);
    if (!incomplete.ok) expect(incomplete.errorKey).toBe("incompleteConfig");

    const badAct = parseBackupJson(
      JSON.stringify({
        ...base,
        actions: { schema_version: 4, nb_actions: 0, actions: "x" },
      }),
    );
    expect(badAct.ok).toBe(false);

    const tooMany = parseBackupJson(
      JSON.stringify({
        ...base,
        actions: {
          schema_version: 4,
          nb_actions: 21,
          actions: Array.from({ length: 21 }, (_, i) => ({
            index: i,
            kind: "triac",
            title: "T",
            regulation_mode: 1,
            repeat_sec: 0,
            tempo_sec: 0,
            periods: [],
          })),
        },
      }),
    );
    expect(tooMany.ok).toBe(false);
    if (!tooMany.ok) expect(tooMany.errorKey).toBe("actionsTooMany");

    const badEnv = parseBackupJson(
      JSON.stringify({ ...base, actions: { nb_actions: 1, actions: base.actions.actions } }),
    );
    expect(badEnv.ok).toBe(false);
    if (!badEnv.ok) expect(badEnv.errorKey).toBe("actionsBadEnvelope");
  });

  it("rejects bad time and wifi", () => {
    const base = minimalBackupDoc();
    const badTime = parseBackupJson(JSON.stringify({ ...base, time: { tz: "UTC", ntp1: 1, ntp2: "b" } }));
    expect(badTime.ok).toBe(false);
    if (!badTime.ok) expect(badTime.errorKey).toBe("badTime");
    const badWifi = parseBackupJson(JSON.stringify({ ...base, wifi: { ssid: "s", password: 1 } }));
    expect(badWifi.ok).toBe(false);
    if (!badWifi.ok) expect(badWifi.errorKey).toBe("badWifi");
  });
});

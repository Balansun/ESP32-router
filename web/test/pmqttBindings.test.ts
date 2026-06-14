import { describe, expect, it } from "vitest";
import type { RouterConfig } from "../src/api/types";
import {
  omitPmqttBindingsFromConfigPatch,
  pmqttActiveBindingCount,
  pmqttBindingsMissing,
} from "../src/utils/pmqttBindings";

const base: RouterConfig = {
  dhcp_on: true,
  source: "Pmqtt",
  mqtt_ip: "192.168.1.1",
};

describe("pmqttBindings", () => {
  it("pmqttBindingsMissing when source Pmqtt and no bindings", () => {
    expect(pmqttBindingsMissing({ ...base, pmqtt_bindings: [] })).toBe(true);
    expect(pmqttBindingsMissing({ ...base, pmqtt_bindings: undefined })).toBe(true);
  });

  it("not missing for other sources", () => {
    expect(pmqttBindingsMissing({ ...base, source: "JsyMk194", pmqtt_bindings: [] })).toBe(
      false,
    );
  });

  it("not missing when at least one enabled binding", () => {
    expect(
      pmqttBindingsMissing({
        ...base,
        pmqtt_bindings: [
          {
            metric: "house.active_import_w",
            topic: "t/state",
            format: "json",
            path: "Pw",
            enabled: true,
          },
        ],
      }),
    ).toBe(false);
    expect(pmqttActiveBindingCount([{ metric: "a", topic: "t", format: "json", enabled: false }])).toBe(0);
  });

  it("omitPmqttBindingsFromConfigPatch when bindings on device but not in GET", () => {
    expect(
      omitPmqttBindingsFromConfigPatch(
        { source: "Pmqtt", pmqtt_bindings_on_device: true },
        [],
      ),
    ).toBe(true);
    expect(
      omitPmqttBindingsFromConfigPatch(
        { source: "Pmqtt", pmqtt_bindings_on_device: true, pmqtt_bindings: [] },
        [],
      ),
    ).toBe(true);
    expect(
      omitPmqttBindingsFromConfigPatch(
        { source: "Pmqtt", pmqtt_bindings_on_device: true },
        [
          {
            metric: "house.active_import_w",
            topic: "t",
            format: "json",
            path: "Pw",
            enabled: true,
          },
        ],
      ),
    ).toBe(false);
  });
});

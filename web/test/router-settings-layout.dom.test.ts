import { afterEach, beforeAll, beforeEach, describe, expect, it, vi } from "vitest";
import { stripBase } from "../src/paths";
import {
  configure,
  go,
  register,
  start,
} from "../src/router";
import { setSettingsLayoutHandlers } from "../src/routes/settings/settingsRouterBridge";

describe("router settings layout", () => {
  const outlet = document.createElement("main");
  let mountCount = 0;
  let updateCount = 0;

  beforeAll(() => {
    document.body.append(outlet);
    configure({ outlet, useHash: false });
    register("/", () => {});
    register(
      "/settings",
      () => {
        mountCount += 1;
        setSettingsLayoutHandlers(
          () => {
            updateCount += 1;
          },
          () => {
            setSettingsLayoutHandlers(null, null);
          },
        );
      },
      { layout: "settings" },
    );
  });

  beforeEach(() => {
    mountCount = 0;
    updateCount = 0;
    setSettingsLayoutHandlers(null, null);
    history.replaceState({}, "", "/");
    outlet.replaceChildren();
  });

  afterEach(() => {
    setSettingsLayoutHandlers(null, null);
    outlet.replaceChildren();
  });

  it("redirects bare /settings to /settings/general", async () => {
    await go("/settings");
    expect(stripBase(location.pathname)).toBe("/settings/general");
  });

  it("reuses layout without remounting between settings sections", async () => {
    await go("/settings/general");
    expect(mountCount).toBe(1);
    expect(stripBase(location.pathname)).toBe("/settings/general");

    await go("/settings/network");
    expect(mountCount).toBe(1);
    expect(updateCount).toBe(1);
    expect(stripBase(location.pathname)).toBe("/settings/network");

    await go("/settings/led");
    expect(mountCount).toBe(1);
    expect(updateCount).toBe(2);
    expect(stripBase(location.pathname)).toBe("/settings/led");
  });

  it("remounts when leaving and re-entering settings", async () => {
    await go("/settings/general");
    expect(mountCount).toBe(1);

    await go("/");
    await go("/settings/advanced");
    expect(mountCount).toBe(2);
  });
});

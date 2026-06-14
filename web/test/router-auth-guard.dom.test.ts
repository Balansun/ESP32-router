import { afterEach, beforeAll, beforeEach, describe, expect, it } from "vitest";
import { stripBase } from "../src/paths";
import {
  configure,
  go,
  register,
  setRouteAuthGuard,
  start,
} from "../src/router";

describe("router auth guard", () => {
  const outlet = document.createElement("main");
  let homeMounted = false;
  let protectedMounted = false;

  beforeAll(() => {
    document.body.append(outlet);
    configure({ outlet, useHash: false });
    register("/", () => {
      homeMounted = true;
    });
    register("/protected", () => {
      protectedMounted = true;
    });
  });

  beforeEach(() => {
    homeMounted = false;
    protectedMounted = false;
    history.replaceState({}, "", "/");
    setRouteAuthGuard(null);
  });

  afterEach(() => {
    setRouteAuthGuard(null);
    outlet.replaceChildren();
  });

  it("go() does not mount route when guard returns false", async () => {
    setRouteAuthGuard((path) => path !== "/protected");
    await go("/protected");
    expect(protectedMounted).toBe(false);
    expect(stripBase(location.pathname)).toBe("/");
  });

  it("start() does not mount protected route when guard blocks current path", async () => {
    history.replaceState({}, "", "/protected");
    setRouteAuthGuard((path) => path !== "/protected");
    await start();
    expect(protectedMounted).toBe(false);
    expect(outlet.childNodes.length).toBe(0);
  });

  it("go() mounts route when guard allows", async () => {
    setRouteAuthGuard(() => true);
    await go("/protected");
    expect(protectedMounted).toBe(true);
    expect(stripBase(location.pathname)).toBe("/protected");
  });
});

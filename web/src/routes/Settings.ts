import type { RouteCtx } from "../router";
import { mountSettingsLayout } from "./settings/settingsLayout";

export async function mountSettings(ctx: RouteCtx): Promise<() => void> {
  return mountSettingsLayout(ctx);
}

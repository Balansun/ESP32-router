import { localePref } from "../state/store";
import { en, type AppStrings } from "./locales/en";
import { fr } from "./locales/fr";

const tables: Record<"en" | "fr", AppStrings> = { en, fr: fr as unknown as AppStrings };

export type { AppStrings };

/** Active UI strings for the current locale. */
export function getStrings(): AppStrings {
  return tables[localePref.get()];
}

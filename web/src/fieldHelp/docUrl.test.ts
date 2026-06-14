import { describe, expect, it } from "vitest";
import {
  DOCS_SITE_ORIGIN,
  docsLangHome,
  docsPageUrl,
  fieldHelpDocUrl,
} from "./docUrl";

describe("fieldHelpDocUrl", () => {
  it("builds English website URL with anchor", () => {
    expect(fieldHelpDocUrl("actions", "host", "en")).toBe(
      `${DOCS_SITE_ORIGIN}/en/field-help/actions/#host`,
    );
  });

  it("builds French doc path with kebab-case scope slug", () => {
    expect(fieldHelpDocUrl("settings", "mqtt_ip", "fr")).toBe(
      `${DOCS_SITE_ORIGIN}/fr/field-help/settings/#mqtt_ip`,
    );
    expect(fieldHelpDocUrl("sourceWizard", "source", "en")).toContain(
      "/en/field-help/source-wizard/#source",
    );
  });
});

describe("docsPageUrl", () => {
  it("builds lang home", () => {
    expect(docsLangHome("fr")).toBe(`${DOCS_SITE_ORIGIN}/fr/`);
  });
});

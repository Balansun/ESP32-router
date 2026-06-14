export type DocsLang = "en" | "fr";

export type ExtraBlock = string;

/** Website-only markdown blocks appended under each field heading. */
export type ExtraMap = Partial<Record<string, Record<string, readonly ExtraBlock[]>>>;

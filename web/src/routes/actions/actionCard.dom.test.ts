import { describe, expect, it, vi } from "vitest";

vi.mock("../../components/Toast", () => ({ toast: vi.fn() }));
import { toast } from "../../components/Toast";
import { MAX_PERIODS } from "./model";
import type { ActionConfig } from "../../api/types";
import { ensureNormalised } from "./model";
import { MODE_DECOUPE_ONOFF, MODE_INACTIF } from "./regulationMode";
import { buildActionCard } from "./actionCard";

describe("actionCard DOM", () => {
  it("shows daily cap field for triac when onDailyCapChange provided", () => {
    const editing = ensureNormalised([]);
    const card = buildActionCard({
      editing,
      idx: 0,
      renderCards: () => {},
      editPeriod: () => {},
      dailyCapWh: 1200,
      onDailyCapChange: () => {},
    });
    expect(card.textContent).toContain("Daily energy cap");
    expect(card.querySelector(".field-help")).not.toBeNull();
  });

  it("omits daily cap field for remote actions", () => {
    const editing: ActionConfig[] = [
      ...ensureNormalised([]),
      {
        index: 1,
        regulation_mode: MODE_INACTIF,
        kind: "remote_http",
        title: "Relay",
        host: "192.168.1.2",
        port: 80,
        path_on: "/on",
        path_off: "/off",
        repeat_sec: 60,
        tempo_sec: 0,
        periods: [
          {
            mode: "off",
            hour_end: 2400,
            power_min_w: 0,
            power_max_w: 0,
            temp_inf_c: 150,
            temp_sup_c: 150,
          },
        ],
      },
    ];
    const card = buildActionCard({
      editing,
      idx: 1,
      renderCards: () => {},
      editPeriod: () => {},
      onDailyCapChange: () => {},
    });
    expect(card.textContent).not.toContain("Daily energy cap");
  });

  it("regulation switch toggles regulation_mode on action", () => {
    const editing = ensureNormalised([]);
    editing[0].regulation_mode = MODE_INACTIF;
    const card = buildActionCard({
      editing,
      idx: 0,
      renderCards: vi.fn(),
      editPeriod: () => {},
    });
    const switches = card.querySelectorAll('input[type="checkbox"]');
    const regSwitch = switches[0] as HTMLInputElement;
    expect(regSwitch.checked).toBe(false);
    regSwitch.checked = true;
    regSwitch.dispatchEvent(new Event("change", { bubbles: true }));
    expect(editing[0].regulation_mode).toBe(MODE_DECOUPE_ONOFF);
  });

  it("onDailyCapChange receives parsed Wh from input", () => {
    const editing = ensureNormalised([]);
    const onCap = vi.fn();
    const card = buildActionCard({
      editing,
      idx: 0,
      renderCards: () => {},
      editPeriod: () => {},
      dailyCapWh: 0,
      onDailyCapChange: onCap,
    });
    const inputs = card.querySelectorAll('input[type="number"]');
    const capInput = Array.from(inputs).find((el) =>
      (el as HTMLInputElement).value === "0",
    ) as HTMLInputElement | undefined;
    expect(capInput).toBeDefined();
    capInput!.value = "7500";
    capInput!.dispatchEvent(new Event("input", { bubbles: true }));
    expect(onCap).toHaveBeenCalledWith(7500);
  });

  it("triac sensitivity slider updates port", () => {
    const editing = ensureNormalised([]);
    const card = buildActionCard({
      editing,
      idx: 0,
      renderCards: () => {},
      editPeriod: () => {},
    });
    const slider = card.querySelector('input[type="range"]') as HTMLInputElement;
    slider.value = "80";
    slider.dispatchEvent(new Event("input", { bubbles: true }));
    expect(editing[0].triac_sensitivity).toBe(80);
    expect(editing[0].port).toBe(80);
  });

  it("remote action shows HTTP fields and localhost kind", () => {
    const editing: ActionConfig[] = [
      ...ensureNormalised([]),
      {
        index: 1,
        regulation_mode: MODE_INACTIF,
        kind: "remote_http",
        title: "Relay",
        host: "192.168.1.2",
        port: 80,
        path_on: "/on",
        path_off: "/off",
        repeat_sec: 60,
        tempo_sec: 0,
        periods: [
          {
            mode: "off",
            hour_end: 2400,
            power_min_w: 0,
            power_max_w: 0,
            temp_inf_c: 150,
            temp_sup_c: 150,
          },
        ],
      },
    ];
    const card = buildActionCard({
      editing,
      idx: 1,
      renderCards: () => {},
      editPeriod: () => {},
    });
    const host = Array.from(card.querySelectorAll("input")).find(
      (el) => (el as HTMLInputElement).value === "192.168.1.2",
    ) as HTMLInputElement;
    host.value = "localhost";
    host.dispatchEvent(new Event("input", { bubbles: true }));
    expect(editing[1].kind).toBe("local_gpio");
  });

  it("remote repeat and tempo inputs update action", () => {
    const editing: ActionConfig[] = [
      ...ensureNormalised([]),
      {
        index: 1,
        regulation_mode: MODE_INACTIF,
        kind: "remote_http",
        title: "Relay",
        host: "h",
        port: 80,
        path_on: "/on",
        path_off: "/off",
        repeat_sec: 60,
        tempo_sec: 0,
        periods: [
          {
            mode: "off",
            hour_end: 2400,
            power_min_w: 0,
            power_max_w: 0,
            temp_inf_c: 150,
            temp_sup_c: 150,
          },
        ],
      },
    ];
    const card = buildActionCard({
      editing,
      idx: 1,
      renderCards: () => {},
      editPeriod: () => {},
    });
    const inputs = Array.from(card.querySelectorAll('input[type="number"]'));
    const repeat = inputs.find((el) => (el as HTMLInputElement).value === "60") as HTMLInputElement;
    repeat.value = "120";
    repeat.dispatchEvent(new Event("input", { bubbles: true }));
    expect(editing[1].repeat_sec).toBe(120);
    const tempo = inputs.find((el) => (el as HTMLInputElement).value === "0") as HTMLInputElement;
    tempo.value = "5";
    tempo.dispatchEvent(new Event("input", { bubbles: true }));
    expect(editing[1].tempo_sec).toBe(5);
  });

  it("remote port and path fields update action", () => {
    const editing: ActionConfig[] = [
      ...ensureNormalised([]),
      {
        index: 1,
        regulation_mode: MODE_INACTIF,
        kind: "remote_http",
        title: "Relay",
        host: "h",
        port: 80,
        path_on: "/on",
        path_off: "/off",
        repeat_sec: 0,
        tempo_sec: 0,
        periods: [
          {
            mode: "off",
            hour_end: 2400,
            power_min_w: 0,
            power_max_w: 0,
            temp_inf_c: 150,
            temp_sup_c: 150,
          },
        ],
      },
    ];
    const card = buildActionCard({
      editing,
      idx: 1,
      renderCards: () => {},
      editPeriod: () => {},
    });
    const port = Array.from(card.querySelectorAll('input[type="number"]')).find(
      (el) => (el as HTMLInputElement).value === "80",
    ) as HTMLInputElement;
    port.value = "99";
    port.dispatchEvent(new Event("input", { bubbles: true }));
    expect(editing[1].port).toBe(99);
    const paths = Array.from(card.querySelectorAll('input[type="text"]')).filter(
      (el) => (el as HTMLInputElement).value.startsWith("/"),
    );
    (paths[0] as HTMLInputElement).value = "/a";
    paths[0].dispatchEvent(new Event("input", { bubbles: true }));
    expect(editing[1].path_on).toBe("/a");
    (paths[1] as HTMLInputElement).value = "/b";
    paths[1].dispatchEvent(new Event("input", { bubbles: true }));
    expect(editing[1].path_off).toBe("/b");
  });

  it("remove button splices non-triac action", () => {
    const editing: ActionConfig[] = [
      ...ensureNormalised([]),
      {
        index: 1,
        regulation_mode: MODE_INACTIF,
        kind: "remote_http",
        title: "Relay",
        host: "h",
        port: 80,
        path_on: "/on",
        path_off: "/off",
        repeat_sec: 0,
        tempo_sec: 0,
        periods: [
          {
            mode: "off",
            hour_end: 2400,
            power_min_w: 0,
            power_max_w: 0,
            temp_inf_c: 150,
            temp_sup_c: 150,
          },
        ],
      },
    ];
    const renderCards = vi.fn();
    const card = buildActionCard({
      editing,
      idx: 1,
      renderCards,
      editPeriod: () => {},
    });
    const removeBtn = card.querySelector(".btn--danger") as HTMLButtonElement;
    removeBtn.click();
    expect(editing.length).toBe(1);
    expect(renderCards).toHaveBeenCalled();
  });

  it("add and remove period buttons mutate periods", () => {
    const editing = ensureNormalised([]);
    editing[0].periods = [
      {
        mode: "power",
        hour_end: 1200,
        power_min_w: 0,
        power_max_w: 100,
        temp_inf_c: 150,
        temp_sup_c: 150,
      },
      {
        mode: "power",
        hour_end: 2400,
        power_min_w: 0,
        power_max_w: 100,
        temp_inf_c: 150,
        temp_sup_c: 150,
      },
    ];
    const card = buildActionCard({
      editing,
      idx: 0,
      renderCards: () => {},
      editPeriod: () => {},
    });
    const buttons = card.querySelectorAll("button");
    const addBtn = Array.from(buttons).find((b) =>
      b.getAttribute("aria-label")?.includes("Add time window"),
    );
    const removeBtn = Array.from(buttons).find((b) =>
      b.getAttribute("aria-label")?.includes("Remove last window"),
    );
    addBtn?.click();
    expect(editing[0].periods.length).toBe(3);
    removeBtn?.click();
    expect(editing[0].periods.length).toBe(2);
  });

  it("does not remove last period when only one remains", () => {
    const editing = ensureNormalised([]);
    const card = buildActionCard({
      editing,
      idx: 0,
      renderCards: () => {},
      editPeriod: () => {},
    });
    const removeBtn = Array.from(card.querySelectorAll("button")).find((b) =>
      b.getAttribute("aria-label")?.includes("Remove last window"),
    );
    removeBtn?.click();
    expect(editing[0].periods.length).toBe(1);
  });

  it("title input updates action title", () => {
    const editing = ensureNormalised([]);
    const card = buildActionCard({
      editing,
      idx: 0,
      renderCards: () => {},
      editPeriod: () => {},
    });
    const title = card.querySelector('input[type="text"]') as HTMLInputElement;
    title.value = "New title";
    title.dispatchEvent(new Event("input", { bubbles: true }));
    expect(editing[0].title).toBe("New title");
  });

  it("add period on remote uses off mode and mid-hour_end", () => {
    const editing: ActionConfig[] = [
      ...ensureNormalised([]),
      {
        index: 1,
        regulation_mode: MODE_INACTIF,
        kind: "remote_http",
        title: "Relay",
        host: "h",
        port: 80,
        path_on: "/on",
        path_off: "/off",
        repeat_sec: 0,
        tempo_sec: 0,
        periods: [
          {
            mode: "off",
            hour_end: 1200,
            power_min_w: 0,
            power_max_w: 0,
            temp_inf_c: 150,
            temp_sup_c: 150,
          },
          {
            mode: "off",
            hour_end: 2400,
            power_min_w: 0,
            power_max_w: 0,
            temp_inf_c: 150,
            temp_sup_c: 150,
          },
        ],
      },
    ];
    const card = buildActionCard({
      editing,
      idx: 1,
      renderCards: () => {},
      editPeriod: () => {},
    });
    const addBtn = Array.from(card.querySelectorAll("button")).find((b) =>
      b.getAttribute("aria-label")?.includes("Add time window"),
    );
    addBtn?.click();
    expect(editing[1].periods.length).toBe(3);
    expect(editing[1].periods[1].mode).toBe("off");
    expect(editing[1].periods[1].power_max_w).toBe(0);
  });

  it("editPeriod fires when period edit clicked", () => {
    const editing = ensureNormalised([]);
    const editPeriod = vi.fn();
    const card = buildActionCard({
      editing,
      idx: 0,
      renderCards: () => {},
      editPeriod,
    });
    const editBtn = card.querySelector(".period-list__edit") as HTMLButtonElement;
    editBtn.click();
    expect(editPeriod).toHaveBeenCalledWith(0, 0);
  });

  it("triac add period with single window uses prev 0", () => {
    const editing = ensureNormalised([]);
    expect(editing[0].periods.length).toBe(1);
    const card = buildActionCard({
      editing,
      idx: 0,
      renderCards: () => {},
      editPeriod: () => {},
    });
    const addBtn = Array.from(card.querySelectorAll("button")).find((b) =>
      b.getAttribute("aria-label")?.includes("Add time window"),
    );
    addBtn?.click();
    expect(editing[0].periods.length).toBe(2);
    expect(editing[0].periods[0].mode).toBe("power");
    expect(editing[0].periods[0].power_max_w).toBe(100);
  });

  it("remote host non-localhost keeps remote_http kind", () => {
    const editing: ActionConfig[] = [
      ...ensureNormalised([]),
      {
        index: 1,
        regulation_mode: MODE_INACTIF,
        kind: "remote_http",
        title: "Relay",
        host: "192.168.1.5",
        port: 80,
        path_on: "/on",
        path_off: "/off",
        repeat_sec: 0,
        tempo_sec: 0,
        periods: [
          {
            mode: "off",
            hour_end: 2400,
            power_min_w: 0,
            power_max_w: 0,
            temp_inf_c: 150,
            temp_sup_c: 150,
          },
        ],
      },
    ];
    const card = buildActionCard({
      editing,
      idx: 1,
      renderCards: () => {},
      editPeriod: () => {},
    });
    const host = Array.from(card.querySelectorAll('input[type="text"]')).find(
      (el) => (el as HTMLInputElement).value === "192.168.1.5",
    ) as HTMLInputElement;
    host.value = "10.0.0.8";
    host.dispatchEvent(new Event("input", { bubbles: true }));
    expect(editing[1].kind).toBe("remote_http");
  });

  it("remote parseInt fallbacks use defaults for empty numeric fields", () => {
    const editing: ActionConfig[] = [
      ...ensureNormalised([]),
      {
        index: 1,
        regulation_mode: MODE_INACTIF,
        kind: "remote_http",
        title: "Relay",
        host: "h",
        port: 80,
        path_on: "/on",
        path_off: "/off",
        repeat_sec: 5,
        tempo_sec: 3,
        periods: [
          {
            mode: "off",
            hour_end: 2400,
            power_min_w: 0,
            power_max_w: 0,
            temp_inf_c: 150,
            temp_sup_c: 150,
          },
        ],
      },
    ];
    const card = buildActionCard({
      editing,
      idx: 1,
      renderCards: () => {},
      editPeriod: () => {},
    });
    const numbers = Array.from(card.querySelectorAll('input[type="number"]'));
    const port = numbers.find((el) => (el as HTMLInputElement).value === "80") as HTMLInputElement;
    port.value = "";
    port.dispatchEvent(new Event("input", { bubbles: true }));
    expect(editing[1].port).toBe(80);
    const repeat = numbers.find((el) => (el as HTMLInputElement).value === "5") as HTMLInputElement;
    repeat.value = "abc";
    repeat.dispatchEvent(new Event("input", { bubbles: true }));
    expect(editing[1].repeat_sec).toBe(0);
    const tempo = numbers.find((el) => (el as HTMLInputElement).value === "3") as HTMLInputElement;
    tempo.value = "";
    tempo.dispatchEvent(new Event("input", { bubbles: true }));
    expect(editing[1].tempo_sec).toBe(0);
  });

  it("shows triac title label for index 0", () => {
    const editing = ensureNormalised([]);
    const card = buildActionCard({
      editing,
      idx: 0,
      renderCards: () => {},
      editPeriod: () => {},
    });
    expect(card.querySelector(".field__label")?.textContent).toMatch(/triac|Triac/i);
    expect(card.querySelector(".btn--danger")).toBeNull();
  });

  it("turning regulation off sets inactif mode", () => {
    const editing = ensureNormalised([]);
    editing[0].regulation_mode = MODE_DECOUPE_ONOFF;
    const card = buildActionCard({
      editing,
      idx: 0,
      renderCards: () => {},
      editPeriod: () => {},
    });
    const regSwitch = card.querySelector('input[type="checkbox"]') as HTMLInputElement;
    regSwitch.checked = false;
    regSwitch.dispatchEvent(new Event("change", { bubbles: true }));
    expect(editing[0].regulation_mode).toBe(MODE_INACTIF);
  });

  it("remove period resets last hour_end to 2400", () => {
    const editing = ensureNormalised([]);
    editing[0].periods = [
      {
        mode: "power",
        hour_end: 1200,
        power_min_w: 0,
        power_max_w: 100,
        temp_inf_c: 150,
        temp_sup_c: 150,
      },
      {
        mode: "power",
        hour_end: 1800,
        power_min_w: 0,
        power_max_w: 100,
        temp_inf_c: 150,
        temp_sup_c: 150,
      },
    ];
    const card = buildActionCard({
      editing,
      idx: 0,
      renderCards: () => {},
      editPeriod: () => {},
    });
    const removeBtn = Array.from(card.querySelectorAll("button")).find((b) =>
      b.getAttribute("aria-label")?.includes("Remove last window"),
    );
    removeBtn?.click();
    expect(editing[0].periods.length).toBe(1);
    expect(editing[0].periods[0].hour_end).toBe(2400);
  });

  it("daily cap input floors invalid numbers to zero", () => {
    const editing = ensureNormalised([]);
    const onCap = vi.fn();
    const card = buildActionCard({
      editing,
      idx: 0,
      renderCards: () => {},
      editPeriod: () => {},
      dailyCapWh: 0,
      onDailyCapChange: onCap,
    });
    const capInput = Array.from(card.querySelectorAll('input[type="number"]')).find(
      (el) => (el as HTMLInputElement).value === "0",
    ) as HTMLInputElement;
    capInput.value = "abc";
    capInput.dispatchEvent(new Event("input", { bubbles: true }));
    expect(onCap).toHaveBeenCalledWith(0);
  });

  it("updates path_on and path_off on remote action", () => {
    const editing: ActionConfig[] = [
      ...ensureNormalised([]),
      {
        index: 1,
        regulation_mode: MODE_INACTIF,
        kind: "remote_http",
        title: "Relay",
        host: "h",
        port: 80,
        path_on: "/on",
        path_off: "/off",
        repeat_sec: 0,
        tempo_sec: 0,
        periods: [
          {
            mode: "off",
            hour_end: 2400,
            power_min_w: 0,
            power_max_w: 0,
            temp_inf_c: 150,
            temp_sup_c: 150,
          },
        ],
      },
    ];
    const card = buildActionCard({
      editing,
      idx: 1,
      renderCards: () => {},
      editPeriod: () => {},
    });
    const paths = Array.from(card.querySelectorAll('input[type="text"]')).filter((el) =>
      (el as HTMLInputElement).value.startsWith("/"),
    ) as HTMLInputElement[];
    paths[0].value = "/new-on";
    paths[0].dispatchEvent(new Event("input", { bubbles: true }));
    paths[1].value = "/new-off";
    paths[1].dispatchEvent(new Event("input", { bubbles: true }));
    expect(editing[1].path_on).toBe("/new-on");
    expect(editing[1].path_off).toBe("/new-off");
  });

  it("remote action uses default port when port undefined", () => {
    const editing: ActionConfig[] = [
      ...ensureNormalised([]),
      {
        index: 1,
        regulation_mode: MODE_INACTIF,
        kind: "remote_http",
        title: "Relay",
        host: "10.0.0.1",
        path_on: "/a",
        path_off: "/b",
        repeat_sec: 0,
        tempo_sec: 0,
        periods: [
          {
            mode: "off",
            hour_end: 2400,
            power_min_w: 0,
            power_max_w: 0,
            temp_inf_c: 150,
            temp_sup_c: 150,
          },
        ],
      },
    ];
    const card = buildActionCard({
      editing,
      idx: 1,
      renderCards: () => {},
      editPeriod: () => {},
    });
    const port = Array.from(card.querySelectorAll('input[type="number"]')).find(
      (el) => (el as HTMLInputElement).value === "80",
    ) as HTMLInputElement;
    expect(port).toBeDefined();
  });

  it("triac kind on secondary index still shows sensitivity row", () => {
    const editing = ensureNormalised([]);
    editing.push({
      index: 1,
      regulation_mode: MODE_INACTIF,
      kind: "triac",
      title: "Extra triac",
      triac_sensitivity: 40,
      port: 40,
      repeat_sec: 0,
      tempo_sec: 0,
      periods: [
        {
          mode: "power",
          hour_end: 2400,
          power_min_w: 0,
          power_max_w: 100,
          temp_inf_c: 150,
          temp_sup_c: 150,
        },
      ],
    });
    const card = buildActionCard({
      editing,
      idx: 1,
      renderCards: () => {},
      editPeriod: () => {},
    });
    expect(card.querySelector('input[type="range"]')).not.toBeNull();
  });

  it("triac slider defaults to 50 when sensitivity and port unset", () => {
    const editing = ensureNormalised([]);
    Reflect.deleteProperty(editing[0], "triac_sensitivity");
    Reflect.deleteProperty(editing[0], "port");
    const card = buildActionCard({
      editing,
      idx: 0,
      renderCards: () => {},
      editPeriod: () => {},
    });
    const slider = card.querySelector('input[type="range"]') as HTMLInputElement;
    expect(slider.value).toBe("50");
    slider.value = "75";
    slider.dispatchEvent(new Event("input", { bubbles: true }));
    expect(editing[0].triac_sensitivity).toBe(75);
    expect(editing[0].port).toBe(75);
  });

  it("http action shows empty optional fields and applies defaults on input", () => {
    const editing: ActionConfig[] = [
      ...ensureNormalised([]),
      {
        index: 1,
        regulation_mode: MODE_INACTIF,
        kind: "remote_http",
        title: "Relay",
        periods: [
          {
            mode: "off",
            hour_end: 2400,
            power_min_w: 0,
            power_max_w: 0,
            temp_inf_c: 150,
            temp_sup_c: 150,
          },
        ],
      },
    ];
    const card = buildActionCard({
      editing,
      idx: 1,
      renderCards: () => {},
      editPeriod: () => {},
    });
    const texts = Array.from(card.querySelectorAll('input[type="text"]')) as HTMLInputElement[];
    const host = texts.find((el) => el.value === "")!;
    expect(host).toBeDefined();
    host.value = "localhost";
    host.dispatchEvent(new Event("input", { bubbles: true }));
    expect(editing[1].kind).toBe("local_gpio");
    const numbers = Array.from(card.querySelectorAll('input[type="number"]')) as HTMLInputElement[];
    const port = numbers.find((el) => el.value === "80")!;
    port.value = "";
    port.dispatchEvent(new Event("input", { bubbles: true }));
    expect(editing[1].port).toBe(80);
    const repeat = numbers.find((el) => el.value === "0")!;
    repeat.value = "x";
    repeat.dispatchEvent(new Event("input", { bubbles: true }));
    expect(editing[1].repeat_sec).toBe(0);
  });

  it("daily cap display uses zero when dailyCapWh omitted", () => {
    const editing = ensureNormalised([]);
    const card = buildActionCard({
      editing,
      idx: 0,
      renderCards: () => {},
      editPeriod: () => {},
      onDailyCapChange: vi.fn(),
    });
    expect(card.textContent).toContain("Daily energy cap");
    const cap = Array.from(card.querySelectorAll('input[type="number"]')).find(
      (el) => (el as HTMLInputElement).value === "0",
    ) as HTMLInputElement;
    expect(cap).toBeDefined();
  });

  it("warns and does not grow when add clicked at cap", () => {
    const editing = ensureNormalised([]);
    while (editing[0].periods.length < MAX_PERIODS) {
      editing[0].periods.push({
        mode: "power",
        hour_end: 2400,
        power_min_w: 0,
        power_max_w: 100,
        temp_inf_c: 150,
        temp_sup_c: 150,
      });
    }
    const card = buildActionCard({
      editing,
      idx: 0,
      renderCards: () => {},
      editPeriod: () => {},
    });
    const addBtn = Array.from(card.querySelectorAll("button")).find((b) =>
      b.getAttribute("aria-label")?.includes("Add time window"),
    );
    vi.mocked(toast).mockClear();
    addBtn?.click();
    expect(editing[0].periods.length).toBe(MAX_PERIODS);
    expect(vi.mocked(toast)).toHaveBeenCalledWith(expect.stringMatching(/Maximum|M[ae]ximum/i), "warn");
  });

  describe("onDirty", () => {
    it("fires on title, switch, slider, daily cap, HTTP fields, and period buttons", () => {
      const editing = ensureNormalised([]);
      editing.push({
        index: 1,
        regulation_mode: MODE_INACTIF,
        kind: "remote_http",
        title: "Relay",
        host: "192.168.1.2",
        port: 80,
        path_on: "/on",
        path_off: "/off",
        repeat_sec: 60,
        tempo_sec: 0,
        periods: [
          {
            mode: "off",
            hour_end: 1200,
            power_min_w: 0,
            power_max_w: 0,
            temp_inf_c: 150,
            temp_sup_c: 150,
          },
          {
            mode: "off",
            hour_end: 2400,
            power_min_w: 0,
            power_max_w: 0,
            temp_inf_c: 150,
            temp_sup_c: 150,
          },
        ],
      });
      const onDirty = vi.fn();
      const onCap = vi.fn();
      const triacCard = buildActionCard({
        editing,
        idx: 0,
        renderCards: () => {},
        editPeriod: () => {},
        dailyCapWh: 0,
        onDailyCapChange: onCap,
        onDirty,
      });
      const remoteCard = buildActionCard({
        editing,
        idx: 1,
        renderCards: () => {},
        editPeriod: () => {},
        onDirty,
      });

      const triacTitle = triacCard.querySelector('input[type="text"]') as HTMLInputElement;
      triacTitle.value = "Triac title";
      triacTitle.dispatchEvent(new Event("input", { bubbles: true }));
      expect(onDirty).toHaveBeenCalledTimes(1);

      const regSwitch = triacCard.querySelector('input[type="checkbox"]') as HTMLInputElement;
      regSwitch.checked = true;
      regSwitch.dispatchEvent(new Event("change", { bubbles: true }));
      expect(onDirty).toHaveBeenCalledTimes(2);

      const slider = triacCard.querySelector('input[type="range"]') as HTMLInputElement;
      slider.value = "70";
      slider.dispatchEvent(new Event("input", { bubbles: true }));
      expect(onDirty).toHaveBeenCalledTimes(3);

      const capInput = Array.from(triacCard.querySelectorAll('input[type="number"]')).find(
        (el) => (el as HTMLInputElement).value === "0",
      ) as HTMLInputElement;
      capInput.value = "500";
      capInput.dispatchEvent(new Event("input", { bubbles: true }));
      expect(onDirty).toHaveBeenCalledTimes(4);
      expect(onCap).toHaveBeenCalledWith(500);

      const host = Array.from(remoteCard.querySelectorAll('input[type="text"]')).find(
        (el) => (el as HTMLInputElement).value === "192.168.1.2",
      ) as HTMLInputElement;
      host.value = "10.0.0.1";
      host.dispatchEvent(new Event("input", { bubbles: true }));
      expect(onDirty).toHaveBeenCalledTimes(5);

      const addBtn = Array.from(remoteCard.querySelectorAll("button")).find((b) =>
        b.getAttribute("aria-label")?.includes("Add time window"),
      );
      addBtn?.click();
      expect(onDirty).toHaveBeenCalledTimes(6);

      const removeBtn = Array.from(remoteCard.querySelectorAll("button")).find((b) =>
        b.getAttribute("aria-label")?.includes("Remove last window"),
      );
      removeBtn?.click();
      expect(onDirty).toHaveBeenCalledTimes(7);
    });
  });
});

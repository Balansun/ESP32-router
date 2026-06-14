import { currentPath, go, type RouteCtx } from "../../router";
import { h } from "../../utils/dom";
import { api, ApiError } from "../../api/client";
import { mergePinFieldsIntoConfig } from "../../api/pinMapMerge";
import type { MqttTestResponse } from "../../api/types";
import { toast } from "../../components/Toast";
import { icon } from "../../utils/icons";
import { isProbeTemperatureReading } from "../../utils/format";
import {
  applyTemperatureFieldVisibility,
  collectAdvancedTemperaturePatch,
  collectIdentityTemperaturePatch,
} from "../../utils/temperatureSettingsVisibility";
import type { HardwarePresenceItem, RouterConfig } from "../../api/types";
import { getStrings } from "../../i18n";
import { setUnsavedGuard } from "../../navigationGuard";
import { withBase } from "../../paths";
import { hydrateDeviceInfoFromApi } from "../../state/deviceInfoHydrate";
import { deviceInfo, localePref } from "../../state/store";
import {
  hasCompiledMeter,
  hasSurplusRegulation,
  hasTriac,
  isSingleMeterProfile,
  showAdvancedMeterSourcesSection,
} from "../../utils/firmwareCaps";
import { isSafetyLockoutActive } from "../../utils/safetyLockout";
import {
  applyInstallCountryToConfig,
  buildInstallCountrySection,
  readInstallCountryState,
} from "../../components/InstallCountryFields";
import { buildTimezoneCountryField } from "../../components/TimezoneCountryField";
import { resolveIntlTimeZone } from "../../utils/zonedDateTime";
import { buildVacationEndField } from "../../components/VacationEndField";
import { buildPmqttSetupFlow } from "../../components/PmqttSetupFlow";
import { omitPmqttBindingsFromConfigPatch } from "../../utils/pmqttBindings";
import { buildMeasurementSourceSummary } from "../../components/MeasurementSourceSummary";
import { buildNetworkStatusSummary } from "../../components/NetworkStatusSummary";
import { displayStoredIp } from "../../utils/networkIp";
import { effectiveMqttDeviceName } from "../../utils/mqttDeviceName";
import {
  attachCardAutosave,
  attachSettingsCardAutosave,
  type SettingsCardAutosave,
} from "./cardAutosave";
import { wrapSwitchWithHelp } from "../../components/FieldHelp";
import { settingsSwitchLabel } from "../../utils/settingsSwitch";
import { buildTempoRteStatus } from "../../components/TempoRteStatus";
import { numberRow, passwordRow, selectRow, textRow, validateIp } from "./formRows";
import { loadProfileOptions, normalizeLoadProfile } from "../../utils/loadProfileSettings";
import {
  normalizePwmGpio,
  normalizePwmMode,
  pwmGpioOptions,
  pwmModeOptions,
} from "./pwmFields";
import { buildAdvancedSection } from "./cards/advancedCard";
import { buildMeasurementSettingsCard } from "./cards/measurementCard";
import { buildMqttSettingsCard, buildNetworkSettingsCard } from "./cards/networkCard";
import { settingsSection } from "./cards/section";
import { buildSettingsChrome } from "./settingsChrome";
import { buildStatusLedPage } from "./statusLedSettings";
import { buildHardwarePinsCard } from "./hardwarePinsSettings";
import { fetchLiveTemperatureTelemetry, measurementsWithTemperatureTelemetry } from "../../api/temperatureTelemetry";
import { buildTemperatureSettingsSection } from "./settingsTemperatureLayout";
import { resolveBoardPinProfile, resolveHardwarePinDefaults } from "./hardwarePinDefaults";
import { resolveEffectiveMeterSource } from "./hardwarePinVisibility";
import { isEsp32s3Chip } from "./statusLedEsp32s3";
import {
  parseSettingsSection,
  SETTINGS_SECTIONS,
  settingsPath,
  type SettingsSection,
} from "./settingsPaths";
import { setSettingsLayoutHandlers } from "./settingsRouterBridge";
import { buildProductTypeSettingsCard } from "./productTypeSettings";
import { productModePioEnv } from "../../state/productMode";
import type { FirmwareCatalog } from "../../utils/productProfile";
import { bundledFirmwareCatalog } from "../../data/bundledFirmwareCatalog";
const H = { helpScope: "settings" as const };

export async function mountSettingsLayout(ctx: RouteCtx): Promise<() => void> {
  const { outlet, signal } = ctx;
  const T = getStrings();
  const initialSection = parseSettingsSection(ctx.path);

  const chrome = buildSettingsChrome(signal);
  const loading = h("p", { class: "empty" }, T.loading);
  outlet.append(chrome.root, loading);

  let cfg: RouterConfig;
  let deviceUid = "";
  let chipModel = "";
  let esp32s3 = false;
  let tempSensorPresent = false;
  let hardwareItems: HardwarePresenceItem[] | undefined;
  let safetyLockoutActive = false;
  let firmwareCatalog: FirmwareCatalog = bundledFirmwareCatalog;
  try {
    const [env, device, health, state, pins, catalogRes] = await Promise.all([
      api.getConfig({ signal }),
      api.getDevice({ signal }),
      api.getHealth({ signal }),
      api.getState({ signal }).catch((e) => {
        if (e instanceof ApiError && e.status === 503) return null;
        throw e;
      }),
      api.getSystemPins({ signal }).catch(() => null),
      fetch(withBase("/firmware-catalog.json"), { signal })
        .then((r) => (r.ok ? (r.json() as Promise<FirmwareCatalog>) : null))
        .catch(() => null),
    ]);
    if (catalogRes) firmwareCatalog = catalogRes;
    cfg = pins ? mergePinFieldsIntoConfig(env.config, pins) : env.config;
    hydrateDeviceInfoFromApi(device);
    deviceUid = device.device_uid?.trim() ?? "";
    chipModel = device.chip?.model ?? "";
    esp32s3 = isEsp32s3Chip(chipModel);
    tempSensorPresent = state ? isProbeTemperatureReading(state.temperature_c) : false;
    hardwareItems = health.hardware?.items;
    safetyLockoutActive = isSafetyLockoutActive(health, state?.status ?? null);
  } catch (e) {
    if ((e as DOMException)?.name === "AbortError") return () => {};
    loading.textContent = T.status.error + " — " + T.retry;
    return () => {};
  }
  loading.remove();

  const dhcpInput = h("input", {
    type: "checkbox",
    checked: cfg.dhcp_on,
    onChange: () => updateNetworkDisabled(),
  });
  const dhcpRow = wrapSwitchWithHelp(
    settingsSwitchLabel(dhcpInput as HTMLInputElement, T.settings.dhcp),
    "settings",
    "dhcp",
  );

  const ipFixed = textRow(
    "ip_fixed",
    T.settings.ipFixed,
    displayStoredIp(cfg.ip_fixed),
    T.settings.requireReboot,
    { ...H, helpKey: "ip_fixed" },
  );
  const gateway = textRow("gateway", T.settings.gateway, displayStoredIp(cfg.gateway), T.settings.boxHint, {
    ...H,
    helpKey: "gateway",
  });
  const subnet = textRow("subnet_mask", T.settings.subnet, displayStoredIp(cfg.subnet_mask), "", {
    ...H,
    helpKey: "subnet_mask",
  });
  const dns = textRow("dns", T.settings.dns, displayStoredIp(cfg.dns), T.settings.boxHint, { ...H, helpKey: "dns" });

  const router = textRow("router_name", T.settings.routerName, cfg.router_name, "", { ...H, helpKey: "router_name" });
  const probeHouse = textRow("probe_house_name", T.settings.probeHouse, cfg.probe_house_name, "", {
    ...H,
    helpKey: "probe_house_name",
  });
  const probeSecond = textRow("probe_second_name", T.settings.probeSecond, cfg.probe_second_name, "", {
    ...H,
    helpKey: "probe_second_name",
  });
  const probeTemp = textRow("temperature_label", T.settings.probeTemperature, cfg.temperature_label, "", {
    ...H,
    helpKey: "temperature_label",
  });

  const mqttPeriod = numberRow(
    "mqtt_repeat_sec",
    T.settings.mqttPeriod,
    cfg.mqtt_repeat_sec,
    T.settings.mqttPeriodHint,
    () => updateMqttDisabled(),
    { ...H, helpKey: "mqtt_repeat_sec" },
  );
  const mqttIp = textRow("mqtt_ip", T.settings.mqttIp, cfg.mqtt_ip, "", { ...H, helpKey: "mqtt_ip" });
  const mqttPort = numberRow("mqtt_port", T.settings.mqttPort, cfg.mqtt_port, "", undefined, {
    ...H,
    helpKey: "mqtt_port",
  });
  const mqttUser = textRow("mqtt_user", T.settings.mqttUser, cfg.mqtt_user, "", { ...H, helpKey: "mqtt_user" });
  const mqttPwd = passwordRow("mqtt_password", T.settings.mqttPwd, cfg.mqtt_password, "", {
    ...H,
    helpKey: "mqtt_password",
  });
  const mqttPrefix = textRow("mqtt_prefix", T.settings.mqttPrefix, cfg.mqtt_prefix, T.settings.mqttPrefixHint, {
    ...H,
    helpKey: "mqtt_prefix",
  });
  const resolveMqttDeviceName = (raw: string) => effectiveMqttDeviceName(raw, deviceUid);
  const mqttDevice = textRow(
    "mqtt_device_name",
    T.settings.mqttDevice,
    resolveMqttDeviceName(cfg.mqtt_device_name),
    "",
    {
      ...H,
      helpKey: "mqtt_device_name",
    },
  );
  const vacationInput = h("input", {
    type: "checkbox",
    checked: !!cfg.vacation_enabled,
  }) as HTMLInputElement;
  const vacationRow = wrapSwitchWithHelp(
    settingsSwitchLabel(vacationInput, T.settings.vacationEnabled),
    "settings",
    "vacation_enabled",
  );
  let deviceTz = "";
  const getVacationTimeZone = () =>
    resolveIntlTimeZone(deviceTz, cfg.install_country || "FR", cfg.install_country_variant);
  const vacationEnd = buildVacationEndField({
    T,
    initialEpoch: cfg.vacation_end_epoch ?? 0,
    getTimeZone: getVacationTimeZone,
    helpScope: "settings",
    helpKey: "vacation_end_epoch",
  });
  const triacStaleInput = h("input", {
    type: "checkbox",
    checked: !!cfg.triac_off_when_source_stale,
  }) as HTMLInputElement;
  const triacStaleRow = wrapSwitchWithHelp(
    settingsSwitchLabel(triacStaleInput, T.settings.triacOffWhenSourceStale),
    "settings",
    "triac_off_when_source_stale",
  );
  const triacStaleHint = h("p", { class: "field__hint" }, T.settings.triacOffWhenSourceStaleHint);
  const heaterBackoffInput = h("input", {
    type: "checkbox",
    checked: !!cfg.triac_backoff_when_heater_idle,
  }) as HTMLInputElement;
  const heaterBackoffRow = wrapSwitchWithHelp(
    settingsSwitchLabel(heaterBackoffInput, T.settings.triacBackoffWhenHeaterIdle),
    "settings",
    "triac_backoff_when_heater_idle",
  );
  const heaterBackoffHint = h("p", { class: "field__hint" }, T.settings.triacBackoffWhenHeaterIdleHint);
  const maxRouted = numberRow("max_routed_w", T.settings.maxRoutedW, cfg.max_routed_w ?? 0, "", undefined, {
    ...H,
    helpKey: "max_routed_w",
  });
  const mqttJsonInput = h("input", {
    type: "checkbox",
    checked: !!cfg.mqtt_json_commands,
  }) as HTMLInputElement;
  const mqttJsonRow = wrapSwitchWithHelp(
    settingsSwitchLabel(mqttJsonInput, T.settings.mqttJsonCommands),
    "settings",
    "mqtt_json_commands",
  );
  const mqttJsonHint = h("p", { class: "field__hint" }, T.settings.mqttJsonCommandsHint);

  const loadProfile = selectRow(
    "load_profile",
    T.settings.loadProfile,
    normalizeLoadProfile(cfg.load_profile),
    loadProfileOptions(T),
    "",
    { ...H, helpKey: "load_profile" },
  );
  const calibU = numberRow("calib_u", T.settings.calibU, cfg.calib_u, T.settings.calibTypical, undefined, {
    ...H,
    helpKey: "calib_u",
  });
  const calibI = numberRow("calib_i", T.settings.calibI, cfg.calib_i, T.settings.calibTypical, undefined, {
    ...H,
    helpKey: "calib_i",
  });
  const pmqttTopic = textRow("pmqtt_topic", T.settings.pmqttTopic, cfg.pmqtt_topic ?? "", "", {
    ...H,
    helpKey: "pmqtt_topic",
  });
  const pmqttFlow = buildPmqttSetupFlow(
    T,
    cfg.pmqtt_bindings ?? [],
    {
      mqtt_ip: cfg.mqtt_ip ?? "",
      mqtt_port: cfg.mqtt_port ?? 1883,
      mqtt_user: cfg.mqtt_user ?? "",
      mqtt_password: cfg.mqtt_password ?? "",
    },
    () => {},
  );
  const triacTempCap = numberRow(
    "triac_override_max_temp_c",
    T.settings.triacOverrideMaxTempC,
    cfg.triac_override_max_temp_c ?? 70,
    "0 = off, 40–120",
    undefined,
    { ...H, helpKey: "triac_override_max_temp_c" },
  );
  const jsyMk333Baud = numberRow("jsy_mk333_serial_baud", T.settings.jsyMk333Baud, cfg.jsy_mk333_serial_baud ?? 9600, "", undefined, {
    ...H,
    helpKey: "jsy_mk333_serial_baud",
  });
  const pwmGpio = selectRow(
    "pwm_gpio",
    T.settings.pwmGpio,
    String(normalizePwmGpio(cfg.pwm_gpio)),
    pwmGpioOptions(T.settings),
    "",
    { ...H, helpKey: "pwm_gpio" },
  );
  const pwmMode = selectRow(
    "pwm_mode",
    T.settings.pwmMode,
    normalizePwmMode(cfg.pwm_mode),
    pwmModeOptions(T.settings),
    "",
    { ...H, helpKey: "pwm_mode" },
  );
  const pwmDuty = numberRow(
    "pwm_duty_percent",
    T.settings.pwmDuty,
    (cfg as RouterConfig & { pwm_duty_percent?: number }).pwm_duty_percent ?? 0,
    "0–100",
    undefined,
    { ...H, helpKey: "pwm_duty_percent" },
  );
  const pwmInvertedInput = h("input", {
    type: "checkbox",
    checked: !!(cfg as RouterConfig & { pwm_inverted?: boolean }).pwm_inverted,
  }) as HTMLInputElement;
  const pwmInvertedRow = wrapSwitchWithHelp(
    settingsSwitchLabel(pwmInvertedInput, T.settings.pwmInverted),
    "settings",
    "pwm_inverted",
  );
  const statusLedPage = buildStatusLedPage(cfg, T, signal, {
    esp32s3,
    chipModel,
  });
  const ledSettingsLink = h(
    "div",
    { class: "field status-led-advanced-link" },
    h(
      "a",
      {
        href: withBase(settingsPath("led")),
        class: "link",
        "data-route": "true",
      },
      `${T.settings.advancedLedLink} →`,
    ),
    h("p", { class: "field__hint" }, T.settings.advancedLedLinkHint),
  );
  const tempoRteInput = h("input", {
    type: "checkbox",
    checked: !!(cfg as RouterConfig & { tempo_rte_enabled?: boolean }).tempo_rte_enabled,
  }) as HTMLInputElement;
  const tempoRteRow = wrapSwitchWithHelp(
    settingsSwitchLabel(tempoRteInput, T.settings.tempoRteEnabled),
    "settings",
    "tempo_rte_enabled",
  );
  const tempoRteHint = h("p", { class: "field__hint" }, T.settings.tempoRteHint);
  const tempoRteStatus = buildTempoRteStatus(signal);
  tempoRteInput.addEventListener("change", () => {
    tempoRteStatus.setEnabled(tempoRteInput.checked);
  });
  tempoRteStatus.setEnabled(tempoRteInput.checked);

  const installUi = buildInstallCountrySection(
    T,
    localePref.get(),
    readInstallCountryState(cfg),
    () => {},
  );
  installUi.setFrequencyWarning(cfg.mains_frequency_warning);

  function updateNetworkDisabled() {
    const off = !!dhcpInput.checked;
    for (const f of [ipFixed.ref, gateway.ref, subnet.ref, dns.ref]) {
      f.el.disabled = off;
    }
  }
  function updateMqttDisabled() {
    const off = (parseInt(mqttPeriod.ref.el.value, 10) || 0) === 0;
    for (const f of [mqttIp.ref, mqttPort.ref, mqttUser.ref, mqttPwd.ref, mqttPrefix.ref, mqttDevice.ref]) {
      f.el.disabled = off;
    }
  }
  updateNetworkDisabled();
  updateMqttDisabled();

  const mqttTestBtn = h(
    "button",
    { type: "button", class: "btn btn--ghost" },
    T.settings.mqttTest,
  ) as HTMLButtonElement;

  async function runMqttTest() {
    if (!validateIp(mqttIp.ref)) {
      toast(T.settings.mqttTestInvalidIp, "error");
      return;
    }
    mqttTestBtn.disabled = true;
    try {
      const res = await api.mqttTest(
        {
          mqtt_ip: mqttIp.ref.read().trim(),
          mqtt_port: parseInt(mqttPort.ref.read(), 10) || 1883,
          mqtt_user: mqttUser.ref.read(),
          mqtt_password: mqttPwd.ref.read(),
          mqtt_device_name: resolveMqttDeviceName(mqttDevice.ref.read()),
        },
        { signal },
      );
      if (res.ok) {
        toast(T.settings.mqttTestOk.replace("{message}", res.message), "success");
      } else {
        toast(
          T.settings.mqttTestFailed
            .replace("{message}", res.message)
            .replace("{code}", String(res.error_code)),
          "error",
        );
      }
    } catch (e) {
      const body = e instanceof ApiError ? e.body : null;
      if (body && typeof body === "object" && "message" in body) {
        const fail = body as MqttTestResponse;
        toast(
          T.settings.mqttTestFailed
            .replace("{message}", String(fail.message))
            .replace("{code}", String(fail.error_code ?? "")),
          "error",
        );
      } else {
        toast(T.settings.actionFailed, "error");
      }
    } finally {
      mqttTestBtn.disabled = false;
    }
  }
  mqttTestBtn.addEventListener("click", () => void runMqttTest());

  const mqttActionsRow = h("div", { class: "row", style: "gap:8px;flex-wrap:wrap;margin-top:8px;" });

  async function runMqttAction(
    fn: () => Promise<unknown>,
    okMsg: string,
    btn: HTMLButtonElement,
  ) {
    btn.disabled = true;
    try {
      await fn();
      toast(okMsg, "success");
    } catch {
      toast(T.settings.actionFailed, "error");
    } finally {
      btn.disabled = false;
    }
  }

  for (const [label, fn, okMsg] of [
    [T.settings.mqttPublish, () => api.mqttPublishNow({ signal }), T.settings.mqttPublishOk],
    [T.settings.mqttDiscover, () => api.mqttRepublishDiscovery({ signal }), T.settings.mqttDiscoverOk],
    [T.settings.mqttReconnect, () => api.mqttReconnect({ signal }), T.settings.mqttReconnectOk],
  ] as const) {
    const btn = h(
      "button",
      { type: "button", class: "btn btn--ghost" },
      label,
    ) as HTMLButtonElement;
    btn.addEventListener("click", () => void runMqttAction(fn, okMsg, btn));
    mqttActionsRow.append(btn);
  }
  mqttActionsRow.prepend(mqttTestBtn);

  const timeNowEl = h("p", { class: "card__sub" }, "");
  const tzField = buildTimezoneCountryField(
    T,
    localePref.get(),
    "",
    cfg.install_country,
    () => {},
  );
  const ntp1Field = textRow("time_ntp1", T.settings.ntp1, "", "", { ...H, helpKey: "time_ntp1" });
  const ntp2Field = textRow("time_ntp2", T.settings.ntp2, "", "", { ...H, helpKey: "time_ntp2" });
  const sourceSummary = isSingleMeterProfile()
    ? null
    : buildMeasurementSourceSummary({
        signal,
        initialConfig: cfg,
      });

  const networkLive = buildNetworkStatusSummary({
    signal,
    initialConfig: cfg,
    networkFields: {
      ipFixed: ipFixed.ref,
      gateway: gateway.ref,
      subnet: subnet.ref,
      dns: dns.ref,
    },
  });

  let dirty = false;
  const cardSavers: SettingsCardAutosave[] = [];
  const cardLabels = {
    pending: T.settings.cardPending,
    saving: T.saving,
    saved: T.saved,
    error: T.saveError,
  };
  let syncGlobalDirty = () => {
    dirty = cardSavers.some((s) => s.isDirty() || s.isPending());
  };
  /** Always invoke the latest syncGlobalDirty (Save button wiring is assigned later). */
  const notifyCardStateChange = () => syncGlobalDirty();
  const mergeCfg = (patch: Partial<RouterConfig>) => {
    cfg = { ...cfg, ...patch };
  };

  const boardPinProfile = resolveBoardPinProfile(cfg, chipModel);
  const hardwarePins = buildHardwarePinsCard(
    cfg,
    resolveHardwarePinDefaults(cfg, boardPinProfile),
    boardPinProfile,
    hardwareItems,
    signal,
    cardLabels,
    notifyCardStateChange,
    mergeCfg,
  );

  const temperatureSection = buildTemperatureSettingsSection(cfg);
  const temperatureSaver = temperatureSection.attachAutosave(
    {
      signal,
      labels: cardLabels,
      onStateChange: notifyCardStateChange,
    },
    mergeCfg,
  );
  cardSavers.push(temperatureSaver);

  const measurementSection = buildMeasurementSettingsCard(
    T,
    cfg,
    signal,
    sourceSummary?.el ?? document.createElement("div"),
    (c) => {
      cfg = c;
      sourceSummary?.setConfig(c);
      hardwarePins.setEffectiveSource(resolveEffectiveMeterSource(c));
      syncGlobalDirty();
    },
  );

  const advancedSection = buildAdvancedSection(T, signal, timeNowEl, tzField, ntp1Field, ntp2Field);

  const productTypeCard = buildProductTypeSettingsCard(T, {
    catalog: firmwareCatalog,
    productProfile: deviceInfo.get()?.capabilities?.product_profile,
    firmwareCapabilities: deviceInfo.get()?.capabilities?.firmware_capabilities,
    firmwareVersion: deviceInfo.get()?.firmware_version ?? "",
    onModeChange: () => syncSettingsCapabilityGates(),
  }).card;

  const installCard = installUi.section;
  const identityCard = settingsSection(
    T.settings.sectionIdentity,
    router.el,
    probeHouse.el,
    probeSecond.el,
    probeTemp.el,
  );
  const backupCard = h(
    "section",
    { class: "card" },
    h("h2", { class: "section__title" }, T.settings.sectionBackup),
    h("p", { class: "field__hint" }, T.settings.backupHint),
    h(
      "a",
      {
        href: withBase("/backup"),
        class: "btn btn--ghost",
        "data-route": "true",
      },
      T.settings.openBackup,
    ),
  );

  const routingCard = settingsSection(
    T.settings.sectionRouting,
    loadProfile.el,
    h("div", { class: "field" }, vacationRow),
    vacationEnd.el,
    h("div", { class: "field" }, triacStaleRow, triacStaleHint),
    h("div", { class: "field" }, heaterBackoffRow, heaterBackoffHint),
    maxRouted.el,
    h("div", { class: "field" }, mqttJsonRow, mqttJsonHint),
  );
  const calibrationCard = settingsSection(T.settings.sectionCalibration, calibU.el, calibI.el);
  const networkCard = buildNetworkSettingsCard(
    T,
    networkLive.el,
    dhcpRow,
    ipFixed.el,
    gateway.el,
    subnet.el,
    dns.el,
  );
  const mqttCard = buildMqttSettingsCard(
    T,
    mqttPeriod.el,
    mqttIp.el,
    mqttPort.el,
    mqttUser.el,
    mqttPwd.el,
    mqttPrefix.el,
    mqttDevice.el,
    mqttActionsRow,
  );
  const tempoRteWrap = h(
    "div",
    { class: "field" },
    tempoRteRow,
    tempoRteHint,
    tempoRteStatus.el,
  );
  const advancedMeterSourcesCard = settingsSection(
    T.settings.sectionAdvancedSources,
    pmqttTopic.el,
    ...pmqttFlow.sectionRows,
    jsyMk333Baud.el,
    tempoRteWrap,
  );
  const pwmLockoutHint = h("p", {
    class: "banner banner--danger",
    role: "alert",
    hidden: !safetyLockoutActive,
  });
  if (safetyLockoutActive) {
    pwmLockoutHint.textContent = T.home.safetyLockoutActionsDisabled;
  }
  if (safetyLockoutActive) {
    for (const row of [pwmGpio, pwmMode, pwmDuty]) {
      row.el.querySelectorAll("input, select").forEach((el) => {
        (el as HTMLInputElement | HTMLSelectElement).disabled = true;
      });
    }
    pwmInvertedInput.disabled = true;
  }
  const advancedOutputCard = settingsSection(
    T.settings.sectionAdvancedOutput,
    triacTempCap.el,
    pwmLockoutHint,
    h("h3", { class: "field__hint" }, T.settings.pwmSection),
    pwmGpio.el,
    pwmMode.el,
    pwmDuty.el,
    h("div", { class: "field" }, pwmInvertedRow),
    ledSettingsLink,
  );

  const autosaveOpts = {
    signal,
    labels: cardLabels,
    onStateChange: notifyCardStateChange,
  };

  cardSavers.push(hardwarePins.saver);
  cardSavers.push(
    attachSettingsCardAutosave({
      ...autosaveOpts,
      card: installCard,
      collect: () => {
        const p = applyInstallCountryToConfig(cfg, installUi.getState());
        return {
          install_country: p.install_country,
          install_country_variant: p.install_country_variant ?? "",
          mains_nominal_v: p.mains_nominal_v,
          mains_frequency_mode: p.mains_frequency_mode,
          mains_frequency_hz_manual: p.mains_frequency_hz_manual,
        };
      },
      onSaved: (patch) => {
        mergeCfg(patch);
        installUi.setFrequencyWarning(cfg.mains_frequency_warning);
        networkLive.setConfig(cfg);
        if (!deviceTz) {
          vacationEnd.writeEpoch(cfg.vacation_end_epoch ?? 0);
        }
        vacationEnd.refreshTzHint();
      },
    }),
    attachSettingsCardAutosave({
      ...autosaveOpts,
      card: identityCard,
      collect: () => ({
        router_name: router.ref.read(),
        probe_house_name: probeHouse.ref.read(),
        probe_second_name: probeSecond.ref.read(),
        ...collectIdentityTemperaturePatch(tempSensorPresent, probeTemp.ref.read()),
      }),
      onSaved: mergeCfg,
    }),
    attachSettingsCardAutosave({
      ...autosaveOpts,
      card: routingCard,
      validate: () => vacationEnd.validate(),
      collect: () => ({
        load_profile: loadProfile.ref.read(),
        vacation_enabled: vacationInput.checked,
        vacation_end_epoch: vacationEnd.readEpoch(),
        max_routed_w: parseInt(maxRouted.ref.read(), 10) || 0,
        triac_off_when_source_stale: triacStaleInput.checked,
        triac_backoff_when_heater_idle: heaterBackoffInput.checked,
        mqtt_json_commands: mqttJsonInput.checked,
      }),
      onSaved: mergeCfg,
    }),
    attachSettingsCardAutosave({
      ...autosaveOpts,
      card: calibrationCard,
      collect: () => ({
        calib_u: parseInt(calibU.ref.read(), 10) || 1000,
        calib_i: parseInt(calibI.ref.read(), 10) || 1000,
      }),
      onSaved: mergeCfg,
    }),
    attachSettingsCardAutosave({
      ...autosaveOpts,
      card: networkCard,
      validate: () => {
        if (dhcpInput.checked) return true;
        const ok = validateIp(ipFixed.ref) && validateIp(gateway.ref);
        if (!ok) toast(T.settings.badIp, "error");
        return ok;
      },
      collect: () => ({
        dhcp_on: dhcpInput.checked,
        ip_fixed: ipFixed.ref.read(),
        gateway: gateway.ref.read(),
        subnet_mask: subnet.ref.read(),
        dns: dns.ref.read(),
      }),
      onSaved: (patch) => {
        mergeCfg(patch);
        networkLive.setConfig(cfg);
      },
    }),
    attachSettingsCardAutosave({
      ...autosaveOpts,
      card: mqttCard,
      validate: () => {
        if ((parseInt(mqttPeriod.ref.read(), 10) || 0) === 0) return true;
        const ok = validateIp(mqttIp.ref);
        if (!ok) toast(T.settings.badIp, "error");
        return ok;
      },
      collect: () => ({
        mqtt_repeat_sec: parseInt(mqttPeriod.ref.read(), 10) || 0,
        mqtt_ip: mqttIp.ref.read(),
        mqtt_port: parseInt(mqttPort.ref.read(), 10) || 1883,
        mqtt_user: mqttUser.ref.read(),
        mqtt_password: mqttPwd.ref.read(),
        mqtt_prefix: mqttPrefix.ref.read(),
        mqtt_device_name: resolveMqttDeviceName(mqttDevice.ref.read()),
      }),
      onSaved: mergeCfg,
    }),
    attachSettingsCardAutosave({
      ...autosaveOpts,
      card: advancedOutputCard,
      collect: () => {
        const g = safetyLockoutActive
          ? (cfg.pwm_gpio ?? -1)
          : parseInt(pwmGpio.ref.read(), 10);
        const bindings = pmqttFlow.getBindings();
        const first = bindings.find((b) => (b.enabled ?? true));
        const compatSchema =
          first?.metric === "house.snapshot"
            ? "house"
            : first?.metric === "house.signed_net_w"
              ? "Pw,Pf"
              : cfg.pmqtt_schema ?? "Pw";
        const broker = pmqttFlow.getMqttBroker();
        const patch: Partial<RouterConfig> = {
          mqtt_ip: broker.mqtt_ip,
          mqtt_port: broker.mqtt_port,
          mqtt_user: broker.mqtt_user,
          mqtt_password: broker.mqtt_password,
          pmqtt_topic: pmqttTopic.ref.read() || first?.topic || "",
          pmqtt_schema: compatSchema,
          ...collectAdvancedTemperaturePatch(
            tempSensorPresent,
            parseInt(triacTempCap.ref.read(), 10) || 0,
          ),
          jsy_mk333_serial_baud: parseInt(jsyMk333Baud.ref.read(), 10) || 9600,
          pwm_gpio: Number.isNaN(g) ? -1 : g,
          pwm_mode: safetyLockoutActive
            ? normalizePwmMode(cfg.pwm_mode)
            : normalizePwmMode(pwmMode.ref.read()),
          pwm_duty_percent: safetyLockoutActive
            ? ((cfg as RouterConfig & { pwm_duty_percent?: number }).pwm_duty_percent ?? 0)
            : parseInt(pwmDuty.ref.read(), 10) || 0,
          pwm_inverted: safetyLockoutActive
            ? !!(cfg as RouterConfig & { pwm_inverted?: boolean }).pwm_inverted
            : pwmInvertedInput.checked,
          tempo_rte_enabled: tempoRteInput.checked,
        };
        if (!omitPmqttBindingsFromConfigPatch(cfg, bindings)) {
          patch.pmqtt_bindings = bindings;
        }
        return patch;
      },
      onSaved: (patch) => {
        mergeCfg(patch);
        tempoRteStatus.setEnabled(tempoRteInput.checked);
        tempoRteStatus.refresh();
      },
    }),
    attachSettingsCardAutosave({
      ...autosaveOpts,
      card: statusLedPage.pageRoot,
      collect: () => statusLedPage.collectPatch(),
      onSaved: (patch) => {
        mergeCfg(patch);
        statusLedPage.setDisabledByMode();
      },
    }),
    attachCardAutosave({
      ...autosaveOpts,
      card: advancedSection,
      collect: () => ({
        tz: tzField.readTz(),
        ntp1: ntp1Field.ref.read(),
        ntp2: ntp2Field.ref.read(),
      }),
      persist: async (body) => {
        await api.putTime(body, { signal, retry: 1 });
      },
      onSaved: (body) => {
        deviceTz = (body.tz || "").trim();
        vacationEnd.refreshTzHint();
      },
    }),
  );

  type HeaderSaveStatus = "idle" | "saving" | "saved" | "error";
  let headerSaveStatus: HeaderSaveStatus = "idle";
  let savedHideTimer: ReturnType<typeof setTimeout> | undefined;

  const headerStatusEl = h("p", {
    class: "card__save-status",
    hidden: true,
    "aria-live": "polite",
  }) as HTMLParagraphElement;

  const saveHeaderBtn = h(
    "button",
    {
      type: "button",
      class: "btn btn--primary",
      disabled: true,
      onClick: () => void saveAll(),
    },
    icon("save"),
    h("span", {}, T.save),
  ) as HTMLButtonElement;

  chrome.addHeaderActions(headerStatusEl, saveHeaderBtn);

  const saveBar = h("div", { class: "changes-bar", hidden: true });
  saveBar.append(
    h("span", { class: "changes-bar__label" }, T.actions.unsavedBarLabel),
  );
  chrome.form.append(saveBar);

  function setHeaderSaveStatus(next: HeaderSaveStatus) {
    headerSaveStatus = next;
    savedHideTimer && clearTimeout(savedHideTimer);
    if (next === "idle") {
      headerStatusEl.hidden = true;
      headerStatusEl.textContent = "";
      headerStatusEl.className = "card__save-status";
    } else {
      headerStatusEl.hidden = false;
      headerStatusEl.className = `card__save-status card__save-status--${next}`;
      headerStatusEl.textContent =
        next === "saving"
          ? T.saving
          : next === "saved"
            ? T.saved
            : next === "error"
              ? T.saveError
              : T.settings.cardPending;
      if (next === "saved") {
        savedHideTimer = setTimeout(() => {
          if (headerSaveStatus === "saved") setHeaderSaveStatus("idle");
        }, 2500);
      }
    }
    syncGlobalDirty();
  }

  async function saveAll() {
    const toSave = cardSavers.filter((s) => s.isDirty());
    if (toSave.length === 0) return;
    setHeaderSaveStatus("saving");
    for (const s of toSave) {
      await s.flush();
    }
    const stillDirty = cardSavers.some((s) => s.isDirty());
    setHeaderSaveStatus(stillDirty ? "error" : "saved");
    if (!stillDirty) toast(T.saved, "success");
  }

  syncGlobalDirty = () => {
    dirty = cardSavers.some((s) => s.isDirty() || s.isPending());
    saveBar.hidden = !dirty;
    const saving = cardSavers.some((s) => s.isPending());
    const canSave = cardSavers.some((s) => s.isDirty()) && !saving;
    if (canSave) saveHeaderBtn.removeAttribute("disabled");
    else saveHeaderBtn.setAttribute("disabled", "true");
    setUnsavedGuard(() => dirty || saving);
  };

  const ledSectionRoot = h(
    "div",
    { class: "settings-section__cards settings-section__cards--led" },
    statusLedPage.pageRoot,
  );

  function settingsSectionNavVisible(section: SettingsSection): boolean {
    switch (section) {
      case "product":
      case "general":
      case "network":
        return true;
      case "metering":
        return true;
      case "hardware":
        return true;
      case "led":
        return hasTriac();
      case "advanced":
        return true;
      default:
        return true;
    }
  }

  function syncSettingsCapabilityGates(): void {
    for (const id of SETTINGS_SECTIONS) {
      const link = chrome.subnav.querySelector<HTMLElement>(
        `a.settings-nav__item[href$="${settingsPath(id)}"]`,
      );
      if (link) link.hidden = !settingsSectionNavVisible(id);
    }
    measurementSection.hidden = isSingleMeterProfile();
    advancedMeterSourcesCard.hidden = !showAdvancedMeterSourcesSection();
    routingCard.hidden = !hasSurplusRegulation();
    calibrationCard.hidden = !hasCompiledMeter("Analog");
    probeSecond.el.hidden = !hasTriac();
    triacTempCap.el.hidden = !hasTriac();
    pmqttTopic.el.hidden = !hasCompiledMeter("Pmqtt");
    jsyMk333Baud.el.hidden = !hasCompiledMeter("JsyMk333");
    tempoRteWrap.hidden = isSingleMeterProfile() || hasCompiledMeter("Linky");
    for (const row of pmqttFlow.sectionRows) {
      row.hidden = !hasCompiledMeter("Pmqtt");
    }
    const pwmModeSelect =
      pwmMode.ref.el instanceof HTMLSelectElement ? pwmMode.ref.el : null;
    if (pwmModeSelect) {
      for (const opt of pwmModeSelect.querySelectorAll("option")) {
        if (opt.getAttribute("value") === "follow_triac") {
          opt.hidden = !hasTriac();
        }
      }
      if (!hasTriac() && pwmModeSelect.value === "follow_triac") {
        pwmModeSelect.value = "off";
      }
    }
    const meteringHint = chrome.subnav.querySelector('[href*="metering"] .settings-nav__hint');
    if (meteringHint) {
      meteringHint.textContent = isSingleMeterProfile()
        ? T.settings.tabIntro.meteringSingle
        : T.settings.tabIntro.metering;
    }
    const advancedHint = chrome.subnav.querySelector('[href*="advanced"] .settings-nav__hint');
    if (advancedHint) {
      advancedHint.textContent = isSingleMeterProfile()
        ? T.settings.tabIntro.advancedSingle
        : T.settings.tabIntro.advanced;
    }
    const active = parseSettingsSection(currentPath());
    if (!settingsSectionNavVisible(active)) {
      void go(settingsPath("general"), { replace: true });
    }
  }
  deviceInfo.subscribe(() => syncSettingsCapabilityGates());
  productModePioEnv.subscribe(() => syncSettingsCapabilityGates());
  syncSettingsCapabilityGates();

  const sectionRoots: Record<SettingsSection, HTMLElement> = {
    product: h("div", { class: "settings-section__cards" }, productTypeCard),
    general: h("div", { class: "settings-section__cards" }, installCard, identityCard, routingCard, backupCard),
    metering: h("div", { class: "settings-section__cards" }, measurementSection, calibrationCard),
    network: h("div", { class: "settings-section__cards" }, networkCard, mqttCard),
    hardware: h(
      "div",
      { class: "settings-section__cards" },
      hardwarePins.card,
      temperatureSection.root,
    ),
    led: ledSectionRoot,
    advanced: h(
      "div",
      { class: "settings-section__cards" },
      advancedMeterSourcesCard,
      advancedOutputCard,
      advancedSection,
    ),
  };

  function showSection(section: SettingsSection) {
    chrome.setSection(section);
    chrome.sectionOutlet.replaceChildren(sectionRoots[section]);
  }

  showSection(initialSection);

  function applyTemperatureSettingsVisibility(present: boolean): void {
    tempSensorPresent = present;
    applyTemperatureFieldVisibility(
      { probeLabel: probeTemp.el, triacCap: triacTempCap.el },
      present,
    );
  }
  applyTemperatureSettingsVisibility(tempSensorPresent);

  const tempPoll = window.setInterval(() => {
    void Promise.all([
      api.getState({ signal }).catch(() => null),
      fetchLiveTemperatureTelemetry({ signal }),
    ]).then(([st, live]) => {
      const tempC =
        (st && isProbeTemperatureReading(st.temperature_c) ? st.temperature_c : null) ??
        (live.temperature_c != null && isProbeTemperatureReading(live.temperature_c)
          ? live.temperature_c
          : null);
      const measurementsPatch =
        live.temperature_sensors || live.temperature_c != null
          ? (measurementsWithTemperatureTelemetry(live) as import("../../api/types").Measurements)
          : undefined;
      if (tempC != null) {
        applyTemperatureSettingsVisibility(true);
        temperatureSection.refreshLive(measurementsPatch, live.telemetry_ready);
      } else {
        applyTemperatureSettingsVisibility(
          st ? isProbeTemperatureReading(st.temperature_c) : tempSensorPresent,
        );
        temperatureSection.refreshLive(measurementsPatch, live.telemetry_ready);
      }
    });
  }, 5000);

  try {
    const timeInfo = await api.getTime({ signal });
    deviceTz = (timeInfo.tz || "").trim();
    tzField.writeTz(deviceTz);
    ntp1Field.ref.write(timeInfo.ntp1 || "");
    ntp2Field.ref.write(timeInfo.ntp2 || "");
    vacationEnd.writeEpoch(cfg.vacation_end_epoch ?? 0);
    vacationEnd.refreshTzHint();
    timeNowEl.textContent = timeInfo.date_valid
      ? `${T.settings.timeNow}: ${timeInfo.now}`
      : T.settings.timeInvalid;
  } catch {
    timeNowEl.textContent = T.settings.timeInvalid;
  }

  function beforeUnload(ev: BeforeUnloadEvent) {
    if (!dirty) return;
    ev.preventDefault();
    ev.returnValue = "";
  }
  window.addEventListener("beforeunload", beforeUnload);
  syncGlobalDirty();
  signal.addEventListener(
    "abort",
    () => {
      window.removeEventListener("beforeunload", beforeUnload);
      setUnsavedGuard(null);
    },
    { once: true },
  );

  const cleanup = () => {
    savedHideTimer && clearTimeout(savedHideTimer);
    window.clearInterval(tempPoll);
    sourceSummary?.stop();
    networkLive.stop();
    tempoRteStatus.stop();
    window.removeEventListener("beforeunload", beforeUnload);
    setUnsavedGuard(null);
    setSettingsLayoutHandlers(null, null);
  };

  setSettingsLayoutHandlers(
    (path) => showSection(parseSettingsSection(path)),
    cleanup,
  );

  return cleanup;
}

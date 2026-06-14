import { readFileSync } from "node:fs";
import { join } from "node:path";
import type { DocsLang, ExtraMap } from "./types";
import {
  docsPath,
  exampleCode,
  exampleHeading,
  exampleJson,
  exampleSubheading,
  exampleTable,
  linkToFieldHelp,
  linkToGuide,
  readMeterFixture,
} from "./helpers";

const firmwareRoot = join(import.meta.dirname, "../../..");

const FIXTURE = {
  pmqttPwPf: readMeterFixture("pmqtt/pw_pf.json"),
  pmqttHouse: readMeterFixture("pmqtt/house_snapshot.json"),
  extMeasurements: readMeterFixture("external/measurements.json"),
  enphase: readMeterFixture("enphase/net_consumption.json"),
  shelly: readMeterFixture("shelly_em/monophase.json"),
  homewizard: readMeterFixture("homewizard/api_v1_data.json"),
} as const;

function backupSnippet(): string {
  const raw = readFileSync(
    join(firmwareRoot, "web/test/fixtures/balansun-backup.json"),
    "utf8",
  );
  const full = JSON.parse(raw) as {
    backupSchemaVersion: number;
    exportedAt: string;
    config: Record<string, unknown>;
  };
  const excerpt = {
    backupSchemaVersion: full.backupSchemaVersion,
    exportedAt: full.exportedAt,
    config: {
      router_name: full.config.router_name,
      mqtt_prefix: full.config.mqtt_prefix,
      source: full.config.source,
      install_country: full.config.install_country,
    },
  };
  return JSON.stringify(excerpt, null, 2);
}

function staticIpTable(lang: DocsLang) {
  return exampleTable(lang, lang === "fr" ? "LAN domestique" : "home LAN", [
    [lang === "fr" ? "IP" : "IP", "`192.168.1.50`"],
    [lang === "fr" ? "Masque" : "Subnet", "`255.255.255.0`"],
    [lang === "fr" ? "Passerelle" : "Gateway", "`192.168.1.1`"],
    [
      lang === "fr" ? "DNS" : "DNS",
      lang === "fr" ? "`192.168.1.1` ou `1.1.1.1`" : "`192.168.1.1` or `1.1.1.1`",
    ],
  ]);
}

function pmqttPresetBlocks(lang: DocsLang): readonly string[] {
  const simple = lang === "fr" ? "Pw simple" : "Simple Pw";
  const house = lang === "fr" ? "Maison" : "House snapshot";
  return [
    ...exampleJson(lang, simple, FIXTURE.pmqttPwPf),
    linkToGuide(lang, "user-guide/#guide-a7-mqtt-source"),
    ...exampleJson(lang, house, FIXTURE.pmqttHouse),
  ];
}

function pmqttTopicBlocks(lang: DocsLang): readonly string[] {
  const topic = lang === "fr" ? "home/energy/grid" : "home/energy/grid";
  return [
    exampleHeading(lang),
    lang === "fr"
      ? "Abonnement broker : topic `home/energy/grid` (même broker que la télémétrie)."
      : "Broker subscribe topic: `home/energy/grid` (same broker as telemetry).",
    ...exampleJson(lang, null, FIXTURE.pmqttPwPf),
  ];
}

function pmqttSchemaCustomBlocks(lang: DocsLang): readonly string[] {
  return [
    exampleHeading(lang),
    lang === "fr"
      ? "Schéma `Pw,Pf` (preset simple) ou clé unique :"
      : "Schema `Pw,Pf` (simple preset) or single key aliases:",
    ...exampleJson(lang, "power_w", '{"power_w": -800}'),
    ...exampleJson(lang, "active_power_w", '{"active_power_w": 900}'),
    lang === "fr"
      ? "Fixtures firmware : `firmware/test/fixtures/meters/pmqtt/`."
      : "Firmware fixtures: `firmware/test/fixtures/meters/pmqtt/`.",
  ];
}

function mqttTopicTree(lang: DocsLang, prefix = "balansun", device = "6809475d1df8"): readonly string[] {
  return [
    exampleHeading(lang),
    "```",
    `${prefix}/${device}_state`,
    `${prefix}/${device}/triac/set`,
    `${prefix}/${device}/source/set`,
    `${prefix}/${device}/action_1/set`,
    "```",
  ];
}

function calibBlocks(lang: DocsLang): readonly string[] {
  return [
    exampleHeading(lang),
    lang === "fr"
      ? "Défaut usine `1000`. Si la tension affichée est ~5 % basse vs un référentiel : `1050`."
      : "Factory default `1000`. If displayed voltage reads ~5% low vs a reference meter: `1050`.",
  ];
}

function extPeerBlocks(lang: DocsLang): readonly string[] {
  const url =
    lang === "fr"
      ? "GET `http://192.168.1.50:80/api/v1/measurements`"
      : "GET `http://192.168.1.50:80/api/v1/measurements`";
  return [exampleHeading(lang), url, ...exampleJson(lang, null, FIXTURE.extMeasurements)];
}

function actionHostBlocks(lang: DocsLang): readonly string[] {
  if (lang === "fr") {
    return [
      exampleSubheading(lang, "HTTP distant"),
      "`192.168.1.40` — GET `http://192.168.1.40:80/relay/on`",
      exampleSubheading(lang, "GPIO local"),
      "`localhost` avec `gpio=5&out=1`",
    ];
  }
  return [
    exampleSubheading(lang, "Remote HTTP"),
    "`192.168.1.40` — GET `http://host:port/path_on` (mock: `/relay/on`)",
    exampleSubheading(lang, "Local GPIO"),
    "`localhost` with `gpio=5&out=1`",
  ];
}

function actionPathBlocks(lang: DocsLang, on: boolean): readonly string[] {
  if (lang === "fr") {
    return on
      ? [
          exampleHeading(lang),
          "Distant (Shelly) : `/rpc/Switch.Set?id=0&on=true`",
          "GPIO local : `gpio=14&out=1`",
        ]
      : [
          exampleHeading(lang),
          "Distant : `/relay/off`",
          "GPIO local : `gpio=14&out=0`",
        ];
  }
  return on
    ? [
        exampleHeading(lang),
        "Remote (Shelly): `/rpc/Switch.Set?id=0&on=true`",
        "Local GPIO: `gpio=14&out=1`",
      ]
    : [
        exampleHeading(lang),
        "Remote: `/relay/off`",
        "Local GPIO: `gpio=14&out=0`",
      ];
}

function installCountryBlocks(lang: DocsLang): readonly string[] {
  return [
    exampleHeading(lang),
    lang === "fr"
      ? "Ex. `FR` → 230 V, 50 Hz ; `JP` → variante 50/60 Hz selon région."
      : "e.g. `FR` → 230 V, 50 Hz; `JP` → pick 50/60 Hz regional variant.",
    linkToFieldHelp(lang, "settings", "install_country"),
  ];
}

function settingsExtras(lang: DocsLang): Record<string, readonly string[]> {
  const h = exampleHeading(lang);
  return {
    install_country: installCountryBlocks(lang),
    install_country_variant: [
      h,
      lang === "fr"
        ? "Brésil : `BR_127` ou `BR_220` ; Japon : `JP_50` / `JP_60` selon la région d'installation."
        : "Brazil: `BR_127` or `BR_220`; Japan: `JP_50` / `JP_60` for your region.",
    ],
    mains_nominal_v: [h, "`230` (EU single-phase L-N), `120` (US leg), `127` (Brazil)."],
    mains_frequency_mode: [
      h,
      lang === "fr"
        ? "`auto` avec Linky/JSY ; `manual` + `50` ou `60` si la source ne remonte pas la fréquence."
        : "`auto` with Linky/JSY; `manual` + `50` or `60` when the meter omits frequency.",
    ],
    mains_frequency_hz_manual: [h, "`50` (Europe) ou `60` (Amérique du Nord, Japon 60 Hz)."],
    edit_mains_defaults: [
      h,
      lang === "fr"
        ? "Ex. pays `FR` mais réseau mesuré à 235 V : déverrouiller puis `mains_nominal_v: 235`."
        : "e.g. country `FR` but meter reads 235 V: unlock then set `mains_nominal_v: 235`.",
    ],
    router_name: [h, "`Balansun-Garage` → mDNS `http://Balansun-Garage.local` si supporté."],
    probe_house_name: [h, lang === "fr" ? "`Maison`, `Réseau`, `Grid`" : "`Grid`, `House`, `Maison`"],
    probe_second_name: [
      h,
      lang === "fr" ? "`Cumulus`, `Charge routée` (CH2 JSY)" : "`Routed load`, `Tank` (JSY CH2)",
    ],
    temperature_label: [h, lang === "fr" ? "`Ballon`, `Extérieur`" : "`Tank`, `Ambient`"],
    dhcp: [
      h,
      lang === "fr"
        ? "`true` (défaut) : IP du routeur ; `false` + IP fixe pour adressage statique."
        : "`true` (default): router assigns IP; `false` + static fields for fixed addressing.",
    ],
    ip_fixed: staticIpTable(lang),
    gateway: [...staticIpTable(lang), h, lang === "fr" ? "Souvent la box : `192.168.1.1`." : "Often the router: `192.168.1.1`."],
    subnet_mask: [h, "`255.255.255.0` (/24) sur la majorité des LAN domestiques."],
    dns: [h, "`192.168.1.1` (box) ou `1.1.1.1` / `8.8.8.8`."],
    mqtt_repeat_sec: [h, "`60` (1 min) ; `0` désactive la publication tout en gardant le broker configuré."],
    mqtt_ip: [h, "`192.168.1.10` (Home Assistant Mosquitto add-on sur le LAN)."],
    mqtt_port: [h, "`1883` (Mosquitto) ; `8883` si TLS (selon build firmware)."],
    mqtt_user: [h, "`mqtt` ou utilisateur HA ; vide uniquement sur LAN de confiance."],
    mqtt_password: [h, lang === "fr" ? "Mot de passe du compte broker (stocké EEPROM)." : "Broker account password (stored in EEPROM)."],
    mqtt_prefix: mqttTopicTree(lang),
    mqtt_device_name: [
      h,
      "`6809475d1df8` (12 hex, UID usine) ; topics :",
      ...mqttTopicTree(lang, "balansun", "6809475d1df8"),
    ],
    vacation_enabled: [
      h,
      lang === "fr"
        ? "Activer avant les vacances ; couper le triac et les actions selon la politique « vacation »."
        : "Enable before holidays; triac and actions follow vacation policy.",
    ],
    vacation_end_epoch: [
      h,
      "`1735689600` (2025-01-01 00:00:00 UTC) — reprise auto au fuseau de l'appareil.",
    ],
    triac_off_when_source_stale: [
      h,
      lang === "fr"
        ? "Recommandé `true` pour **BalansunPeer** / compteurs HTTP / **Pmqtt**. Exemple BalansunPeer (~800 ms entre polls) : deux échecs HTTP consécutifs → `health_score` < 50 → triac OFF ; une poll JSON OK → reprise AUTO."
        : "Recommended `true` for **BalansunPeer** / HTTP meters / **Pmqtt**. BalansunPeer example (~800 ms poll period): two consecutive HTTP failures → `health_score` < 50 → triac OFF; one OK JSON poll → AUTO resumes.",
      exampleSubheading(lang, "Calcul"),
      lang === "fr"
        ? "Score 0–100 = fraîcheur (âge vs `3 × poll_period_ms`) + succès du dernier poll + série d’erreurs. Seuil stale : **< 50**. Priorité L1 : `safety_lockout` → vacances → `source_stale` → commissioning."
        : "Score 0–100 = freshness (age vs `3 × poll_period_ms`) + last poll OK + error streak. Stale threshold: **< 50**. L1 priority: `safety_lockout` → vacation → `source_stale` → commissioning.",
      exampleSubheading(lang, "API"),
      lang === "fr"
        ? "`GET /api/v1/state` → `status.output_suspend` ; diagnostics → `health_score`, `health_score_factors`. MQTT : `source_stale` ON/OFF (télémetrie seule si l’option est désactivée)."
        : "`GET /api/v1/state` → `status.output_suspend`; diagnostics → `health_score`, `health_score_factors`. MQTT: `source_stale` ON/OFF (telemetry only when this option is off).",
    ],
    triac_backoff_when_heater_idle: [
      h,
      lang === "fr"
        ? "JSY-MK-194 + CH2 sur la charge routée : triac coupé si P consigne haute mais CH2 ≈ 0 W ~45 s."
        : "JSY-MK-194 + CH2 on routed branch: triac off if commanded high but CH2 ≈ 0 W for ~45 s.",
    ],
    load_profile: [
      h,
      lang === "fr"
        ? "`water_heater_triac` (défaut) ou `generic_triac`."
        : "`water_heater_triac` (default) or `generic_triac`.",
      exampleSubheading(lang, "API"),
      "`PUT /api/v1/config` body : `{ \"load_profile\": \"generic_triac\" }`.",
    ],
    max_routed_w: [h, "`3000` plafond site ; `0` = désactivé (mock emulator : `0`)."],
    mqtt_json_commands: [
      h,
      lang === "fr"
        ? "Topic : `balansun/6809475d1df8/action_1/config/set` avec JSON schéma v1."
        : "Topic: `balansun/6809475d1df8/action_1/config/set` with JSON schema v1.",
    ],
    calib_u: calibBlocks(lang),
    calib_i: calibBlocks(lang),
    pmqtt_topic: pmqttTopicBlocks(lang),
    pmqtt_preset: pmqttPresetBlocks(lang),
    pmqtt_schema_custom: pmqttSchemaCustomBlocks(lang),
    triac_override_max_temp_c: [h, "`70` °C (mock) ; `0` = désactivé."],
    jsy_mk333_serial_baud: [
      h,
      lang === "fr"
        ? "`9600` (défaut JSY-MK-333) ; aligner sur le DIP du compteur."
        : "`9600` (JSY-MK-333 default); match the meter DIP/baud setting.",
    ],
    pin_triac_dim: [
      h,
      lang === "fr"
        ? "`22` défaut WROOM ; numéro GPIO concret ; redémarrage requis."
        : "`22` WROOM default; concrete GPIO number; reboot required.",
    ],
    pin_zero_cross: [
      h,
      lang === "fr" ? "`23` défaut WROOM ; numéro GPIO concret." : "`23` WROOM default; concrete GPIO.",
    ],
    pin_uart_rx: [h, "`26` (WROOM) / `4` (S3) ; redémarrage requis."],
    pin_uart_tx: [h, "`27` (WROOM) / `5` (S3) ; doit ≠ RX."],
    pin_temp: [
      h,
      lang === "fr"
        ? "`13` (WROOM) / `21` (S3) ; `-1` = sonde désactivée."
        : "`13` (WROOM) / `21` (S3); `-1` disables the sensor.",
    ],
    pin_analog0: [h, "`35` (WROOM) / `1` (S3) ; entrée ADC Analog."],
    pin_analog1: [h, "`32` (WROOM) / `2` (S3)."],
    pin_analog2: [h, "`33` (WROOM) / `3` (S3)."],
    pwm_gpio: [h, "`25` avec driver SSR DC ; `-1` = désactivé."],
    pwm_mode: [h, "`follow_triac` (miroir triac) ou `independent` + duty fixe."],
    pwm_duty_percent: [h, "`80` % en mode `independent` ; `0` = sortie off."],
    pwm_inverted: [h, lang === "fr" ? "`true` pour SSR active-low." : "`true` for active-low DC SSR modules."],
    status_led_mode: [
      h,
      lang === "fr"
        ? "`dual_gpio` (18/19 Balansun) ou `rgb_ws2812` (onboard S3, GPIO `-1`)."
        : "`dual_gpio` (18/19 Balansun) or `rgb_ws2812` (S3 onboard, GPIO `-1`).",
    ],
    status_led_gpio_activity: [h, "`18` activité / Wi‑Fi ; `-1` = désactivé."],
    status_led_gpio_regulation: [h, "`19` charge triac ; `-1` = désactivé."],
    status_led_rgb_gpio: [h, "`-1` = LED RGB DevKit S3 ; `38` si bande externe."],
    status_led_active_low: [h, lang === "fr" ? "`true` si la LED s’allume au niveau bas." : "`true` when the LED turns on at logic low."],
    status_led_color_activity: [h, "`[255,180,0]` ambre en mode RGB."],
    status_led_color_regulation: [h, "`[0,255,0]` vert en mode RGB."],
    status_led_color_reboot: [
      h,
      lang === "fr"
        ? "`[255,0,0]` clignote avant redémarrage planifié."
        : "`[255,0,0]` flashes before a scheduled reboot.",
    ],
    status_led_color_ap: [
      h,
      lang === "fr"
        ? "`[102,0,255]` clignote en point d’accès / portail captif."
        : "`[102,0,255]` flashes during soft‑AP setup.",
    ],
    tempo_rte_enabled: [
      h,
      lang === "fr"
        ? "`true` sans Linky : couleurs Tempo via api-couleur-tempo.fr."
        : "`true` without Linky: Tempo day colors from api-couleur-tempo.fr.",
    ],
    tz_country: [h, "`FR` → `Europe/Paris` ; `US` → fuseau selon région choisie."],
    time_ntp1: [h, "`pool.ntp.org` ou IP de la box si elle sert NTP."],
    time_ntp2: [h, "`time.google.com` en secours."],
    http_cors_enabled: [
      h,
      lang === "fr"
        ? "Lab : `true` pour « Try it out » sur `/en/api/` vers `http://192.168.1.117`."
        : "Lab: `true` for docs Try it out against `http://192.168.1.117`.",
    ],
    http_auth_enabled: [
      h,
      lang === "fr"
        ? "Activer puis définir un mot de passe ; session navigateur pour l'UI."
        : "Enable then set password; browser session for the UI.",
    ],
    http_auth_password: [
      h,
      lang === "fr"
        ? "Mot de passe fort ; les jetons permanents sont révoqués au changement."
        : "Strong password; permanent API tokens revoked on change.",
    ],
    factory_reset: [
      h,
      lang === "fr"
        ? "1) Exporter une sauvegarde  2) Confirmer reset  3) Reconfigurer Wi‑Fi/AP."
        : "1) Export a backup  2) Confirm reset  3) Reconfigure via Wi‑Fi/AP.",
    ],
  };
}

function apiExtras(lang: DocsLang): Record<string, readonly string[]> {
  const h = exampleHeading(lang);
  const curl =
    lang === "fr"
      ? 'curl -sS -H "Authorization: Bearer VOTRE_JETON" http://192.168.1.117/api/v1/status'
      : 'curl -sS -H "Authorization: Bearer YOUR_TOKEN" http://192.168.1.117/api/v1/status';
  return {
    http_auth_password: settingsExtras(lang).http_auth_password,
    http_cors_enabled: settingsExtras(lang).http_cors_enabled,
    api_access_token: [...exampleCode(lang, null, curl), h, lang === "fr" ? "Créé via Réglages → Accès HTTP ; affiché une seule fois." : "Created in Settings → HTTP access; shown once at creation."],
  };
}

function actionsExtras(lang: DocsLang): Record<string, readonly string[]> {
  const h = exampleHeading(lang);
  return {
    sensitivity: [
      h,
      lang === "fr"
        ? "`17` (mock triac) ; `1` = lent/stable, `100` = réactif."
        : "`17` (mock triac); `1` = slow/stable, `100` = aggressive.",
    ],
    host: actionHostBlocks(lang),
    port: [
      h,
      lang === "fr"
        ? "`80` pour HTTP distant ; ignoré si `localhost` (GPIO)."
        : "`80` for remote HTTP; ignored for `localhost` (GPIO).",
    ],
    path_on: actionPathBlocks(lang, true),
    path_off: actionPathBlocks(lang, false),
    repeat_sec: [
      h,
      lang === "fr"
        ? "`15` (mock chauffe-eau) ; `0` = une seule requête."
        : "`15` (mock water heater); `0` = single request only.",
    ],
    tempo_sec: [
      h,
      lang === "fr"
        ? "`120` s entre changements HTTP (mock action 1)."
        : "`120` s minimum between HTTP toggles (mock action 1).",
    ],
    edit_mode: [
      h,
      lang === "fr"
        ? "Créneaux mock : `off` 06:00, `power` 06:00–18:30, `on` 18:30–22:15, `off` minuit."
        : "Mock bands: `off` until 06:00, `power` 06:00–18:30, `on` 18:30–22:15, `off` at midnight.",
    ],
    edit_threshold: [
      h,
      lang === "fr"
        ? "`-500` W (export) pour ouvrir en mode `power`."
        : "`-500` W (export/surplus) to open in `power` mode.",
    ],
    edit_max_open: [
      h,
      lang === "fr" ? "`100` % plafond triac sur le créneau." : "`100` % max triac opening in this window.",
    ],
    edit_power_on: [
      h,
      lang === "fr"
        ? "`520` W (mock) : marche si Pw maison < seuil."
        : "`520` W (mock): on when house Pw is below threshold.",
    ],
    edit_power_off: [
      h,
      lang === "fr"
        ? "`800` W : arrêt si Pw maison > seuil."
        : "`800` W: off when house Pw is above threshold.",
    ],
    edit_temp_inf: [
      h,
      lang === "fr"
        ? "`-40` ou vide (128) = pas de borne basse."
        : "`-40` or empty (128) = no lower bound.",
    ],
    edit_temp_sup: [
      h,
      lang === "fr"
        ? "`85` °C plafond créneau avec sonde DS18B20."
        : "`85` °C upper bound for this window with DS18B20.",
    ],
    edit_hour_end: [
      h,
      lang === "fr"
        ? "`2200` → fin à 22:00 (mock action 1) ; prochaine fenêtre à cette heure."
        : "`2200` → ends at 22:00 (mock action 1); next window starts then.",
    ],
    action_daily_cap_wh: [
      h,
      lang === "fr"
        ? "`8000` Wh/jour sur CH2 ; `0` = pas de plafond."
        : "`8000` Wh/day on CH2; `0` = no cap.",
    ],
  };
}

function firmwareExtras(lang: DocsLang): Record<string, readonly string[]> {
  const h = exampleHeading(lang);
  return {
    fw_file: [
      h,
      lang === "fr"
        ? "`balansun_0.4.0_wroom32.bin` (ESP32-WROOM-32, partition projet)."
        : "`balansun_0.4.0_wroom32.bin` (ESP32-WROOM-32, project partition table).",
    ],
    fw_md5: [
      h,
      lang === "fr"
        ? "`a1b2c3d4e5f6789012345678abcdef01` (32 hex, vérifié avant OTA)."
        : "`a1b2c3d4e5f6789012345678abcdef01` (32 hex, verified before OTA reboot).",
    ],
    ota_new: [h, lang === "fr" ? "Mot de passe OTA PlatformIO optionnel." : "Optional PlatformIO OTA upload password."],
    ota_confirm: [h, lang === "fr" ? "Répéter le même mot de passe OTA." : "Repeat the same OTA password."],
  };
}

function wifiExtras(lang: DocsLang): Record<string, readonly string[]> {
  const h = exampleHeading(lang);
  return {
    wifi_ssid: [
      h,
      lang === "fr"
        ? "`MaBox-WiFi-2.4GHz` (SSID exact, sensible à la casse)."
        : "`MyRouter-WiFi-2.4GHz` (exact SSID, case-sensitive).",
    ],
    wifi_password: [
      h,
      lang === "fr"
        ? "Phrase WPA2/WPA3 ; ne pas commiter dans git."
        : "WPA2/WPA3 passphrase; never commit to git.",
    ],
  };
}

function sourceWizardExtras(lang: DocsLang): Record<string, readonly string[]> {
  const h = exampleHeading(lang);
  return {
    ext_peer_ip: extPeerBlocks(lang),
    ext_peer_port: [
      h,
      lang === "fr"
        ? "`80` (défaut) ; `8080` si le pair écoute ailleurs."
        : "`80` (default); `8080` when the peer listens elsewhere.",
    ],
    ext_peer_path: [
      h,
      lang === "fr"
        ? "`/api/v1/measurements` (défaut, 48 car. max)"
        : "`/api/v1/measurements` (default, 48 chars max)",
      ...exampleJson(lang, null, FIXTURE.extMeasurements),
    ],
    enphase_user: [
      h,
      lang === "fr"
        ? "`envoy` ou compte local selon firmware Envoy."
        : "`envoy` or local account per Envoy firmware.",
    ],
    enphase_password: [h, lang === "fr" ? "Mot de passe API locale Envoy." : "Enphase Envoy local API password."],
    enphase_serial: [
      h,
      ...exampleJson(lang, "net_consumption", FIXTURE.enphase),
    ],
    pmqtt_topic: pmqttTopicBlocks(lang),
    jsy_mk333_serial_baud: settingsExtras(lang).jsy_mk333_serial_baud,
    calib_u: calibBlocks(lang),
    calib_i: calibBlocks(lang),
    pmqtt_preset: pmqttPresetBlocks(lang),
    defer_test: [
      h,
      lang === "fr"
        ? "Cocher si le compteur est hors ligne à l'enregistrement ; lancer le diagnostic plus tard."
        : "Check when the meter is offline at save; run diagnostics once the peer is up.",
    ],
  };
}

function httpAuthExtras(lang: DocsLang): Record<string, readonly string[]> {
  return {
    login_password: [
      exampleHeading(lang),
      lang === "fr"
        ? "Même mot de passe que **Réglages → Accès HTTP** ; session via `/login`."
        : "Same password as **Settings → HTTP access**; session via `/login`.",
      linkToFieldHelp(lang, "settings", "http_auth_password"),
    ],
  };
}

function installExtras(lang: DocsLang): Record<string, readonly string[]> {
  return { install_country: installCountryBlocks(lang) };
}

function backupExtras(lang: DocsLang): Record<string, readonly string[]> {
  const h = exampleHeading(lang);
  const snippet = backupSnippet();
  return {
    sectionSecurity: [
      h,
      lang === "fr"
        ? "Fichier = credentials Wi‑Fi/MQTT ; stocker hors cloud non chiffré."
        : "File contains Wi‑Fi/MQTT credentials; do not store unencrypted in public cloud.",
    ],
    sectionExport: [...exampleJson(lang, "backup (extrait)", snippet), h, linkToGuide(lang, "user-guide/")],
    sectionImport: [
      h,
      lang === "fr"
        ? "Importer le JSON complet ; rejet si `backupSchemaVersion` ≠ 1."
        : "Import full backup JSON; rejected if `backupSchemaVersion` ≠ 1.",
      ...exampleJson(lang, null, snippet),
    ],
  };
}

export function buildFieldHelpExtras(lang: DocsLang): ExtraMap {
  return {
    settings: settingsExtras(lang),
    api: apiExtras(lang),
    actions: actionsExtras(lang),
    firmware: firmwareExtras(lang),
    wifi: wifiExtras(lang),
    sourceWizard: sourceWizardExtras(lang),
    httpAuth: httpAuthExtras(lang),
    install: installExtras(lang),
    backup: backupExtras(lang),
  };
}

/** Flat keys `scope/key` for parity tests. */
export function collectExtraKeys(extras: ExtraMap): string[] {
  return Object.entries(extras).flatMap(([scope, entries]) =>
    Object.keys(entries ?? {}).map((key) => `${scope}/${key}`),
  );
}

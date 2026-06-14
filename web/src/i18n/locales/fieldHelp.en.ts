/** Long-form inline help (? expandable) — English. */
export const fieldHelpEn = {
  settings: {
    install_country:
      "Country preset sets default mains voltage and frequency for regulation and UI labels. Use Custom only if your grid differs from the listed profiles.",
    install_country_variant:
      "Some countries use split grids (e.g. Japan 50/60 Hz regions, Brazil 127/220 V). Pick the variant that matches your installation.",
    mains_nominal_v:
      "Nominal line-to-neutral voltage used for power calculations and display. Typical EU: 230 V; North America split-phase often 120/240 V depending on wiring.",
    mains_frequency_mode:
      "Auto reads frequency from the active meter when available. Manual fixes 50 or 60 Hz when the source does not report frequency.",
    mains_frequency_hz_manual: "Fallback frequency (50 or 60 Hz) when mode is Manual.",
    edit_mains_defaults:
      "Unlocks editing of default voltage and frequency for the selected country preset. Use only when you know your local grid differs from the table.",
    router_name:
      "Friendly name for this router on the LAN and in MQTT discovery (device name). Also used for mDNS as http://{name}.local when supported.",
    probe_house_name:
      "Label for the house incomer channel in the UI and MQTT (e.g. “Grid”, “Maison”). Does not change measurement wiring.",
    probe_second_name:
      "Label for the second measurement channel when the source provides it (e.g. routed load, CH2 on JSY modules).",
    temperature_label:
      "Name shown for the DS18B20 probe on GPIO13 in charts and MQTT. Install optional 1-Wire sensor for tank or ambient temperature.",
    dhcp:
      "When enabled, the ESP32 requests an IP address from your router (DHCP). Disable only if you need a fixed LAN address.",
    ip_fixed:
      "Static IPv4 address for this device when DHCP is off. Must be free on your subnet and outside the DHCP pool. Requires reboot.",
    gateway:
      "Default gateway (usually your internet box). Required for static IP so the device can reach MQTT, NTP, and HTTP peers.",
    subnet_mask:
      "Subnet mask for static IP (often 255.255.255.0 on home LANs). Must match your network layout.",
    dns:
      "DNS server for static IP (often the gateway or 1.1.1.1). Used for NTP hostnames and HTTP meter peers.",
    mqtt_repeat_sec:
      "How often the router publishes MQTT state (seconds). Set 0 to disable all MQTT publishing while keeping broker settings stored.",
    mqtt_ip:
      "IPv4 address of your MQTT broker (Home Assistant add-on, Mosquitto, etc.). Must be reachable from the ESP32 on the LAN.",
    mqtt_port:
      "MQTT broker TCP port. Default Mosquitto port is 1883; TLS brokers often use 8883 (not all builds support TLS).",
    mqtt_user:
      "MQTT username if the broker requires authentication. Leave empty for open brokers on trusted LANs only.",
    mqtt_password:
      "MQTT password paired with the username. Stored in device EEPROM; protect LAN access accordingly.",
    mqtt_prefix:
      "Prefix for state and command topics (e.g. balansun). Home Assistant discovery still uses homeassistant/ as per HA convention.",
    mqtt_device_name:
      "Short device id in MQTT topics and discovery (e.g. 6809475d1df8). Defaults to this board’s device_uid (factory MAC, 12 hex digits). Avoid spaces.",
    vacation_enabled:
      "Suspends surplus routing (triac and actions follow “vacation” policy). Useful when away or during maintenance.",
    vacation_end_epoch:
      "Date and time when routing resumes automatically (device timezone). Leave empty to end vacation manually only.",
    triac_off_when_source_stale:
      "Routing fail-safe: when enabled and the measurement source is stale (health score < 50), firmware suspends outputs (triac 0%, local GPIO/HTTP actions off) with `output_suspend.reason: source_stale`. Routing resumes automatically when fresh data returns — no persistent lockout (unlike `safety_lockout`). Strongly recommended for BalansunPeer (split install), HTTP meters, and Pmqtt. MQTT `source_stale` reports status; this switch decides whether the triac is cut. Not the same as `triac_backoff_when_heater_idle` (CH2 load feedback ~45 s).",
    triac_backoff_when_heater_idle:
      "With JSY-MK-194 (or similar) and CH2 wired on the routed load: turns triac off if commanded power is high but CH2 shows no load for ~45 s (series thermostat open). Wire CH2 on the routed load branch (after the triac), not another circuit.",
    load_profile:
      "Kind of load receiving surplus power (water heater triac or generic triac dimming profile).",
    max_routed_w:
      "Site-wide cap on routed power (watts). 0 = disabled. Limits total diversion when multiple loads or safety margins require it.",
    mqtt_json_commands:
      "Enables MQTT subscription to action_N/config/set with JSON schema v1 for remote action configuration from Home Assistant or scripts.",
    calib_u:
      "Scales measured voltage for Analog inputs. Factory default 1000; increase if readings are low vs a reference meter.",
    calib_i:
      "Scales measured current for Analog inputs. Factory default 1000; adjust after clamp installation and reference comparison.",
    pmqtt_topic:
      "MQTT topic the router subscribes to when source is Pmqtt. Payload must be JSON; schema is set by the preset below. Uses the same broker as telemetry.",
    pmqtt_preset:
      "Simple Pw / Pf: flat JSON {\"Pw\":-1500,\"Pf\":0.98}. House: nested object with active_import_w / active_export_w. Custom: enter key names manually.",
    pmqtt_schema_custom:
      "Comma-separated JSON key paths matching your publisher (advanced). See user guide § A.7 and firmware test fixtures for examples.",
    triac_override_max_temp_c:
      "Blocks 100% triac override from UI/MQTT when DS18B20 reads above this temperature (°C). 0 = disabled. Range 40–120 when enabled.",
    jsy_mk333_serial_baud:
      "UART baud rate for JSY-MK-333 on UART2. Default 9600; must match meter configuration (1200–115200).",
    pin_triac_dim:
      "Triac gate / DIM output GPIO (typically GPIO 22 on WROOM). Applied after reboot.",
    pin_zero_cross:
      "Zero-cross detection input for phase-cut dimming (typically GPIO 23). Applied after reboot.",
    pin_uart_rx:
      "UART RX for JSY-MK-194 / Linky / JSY-333 metering. Applied after reboot.",
    pin_uart_tx:
      "UART TX for metering. Must differ from RX. Applied after reboot.",
    pin_temp:
      "1-Wire bus for optional DS18B20. Use -1 to disable the sensor; otherwise set the GPIO number. Applied after reboot.",
    pin_analog0:
      "Analog probe analog input 0 (ADC). Applied after reboot.",
    pin_analog1:
      "Analog probe analog input 1. Applied after reboot.",
    pin_analog2:
      "Analog probe analog input 2. Applied after reboot.",
    pwm_gpio:
      "GPIO for optional DC SSR PWM output (-1 = off). Allowed pins: 4, 5, 14, 16, 17, 21, 25. Requires appropriate hardware driver.",
    pwm_mode:
      "off: disabled. follow_triac: mirrors triac duty. independent: uses pwm_duty_percent regardless of triac.",
    pwm_duty_percent:
      "Duty cycle 0–100% when pwm_mode is independent. 0 = off.",
    pwm_inverted:
      "Invert PWM polarity for active-low SSR modules.",
    status_led_mode:
      "off: no status LEDs. dual_gpio: two GPIO outputs (Balansun yellow/green). rgb_ws2812: one WS2812 LED (ESP32-S3 onboard or custom GPIO).",
    status_led_gpio_activity:
      "GPIO for Wi‑Fi / activity blink. Default 18 on Balansun PCB. -1 disables this output.",
    status_led_gpio_regulation:
      "GPIO for triac load blink. Default 19 on Balansun PCB. -1 disables this output.",
    status_led_rgb_gpio:
      "WS2812 data pin. -1 = onboard RGB on ESP32-S3 N16R8 (GPIO 48 / RGB_BUILTIN; solder the RGB pad if needed). Use 48 explicitly if -1 fails.",
    status_led_active_low:
      "Check if your external GPIO LEDs turn on with a low level (dual GPIO mode only).",
    status_led_color_activity:
      "RGB color [r,g,b] for activity / Wi‑Fi heartbeat when mode is rgb_ws2812.",
    status_led_color_regulation:
      "RGB color [r,g,b] for triac regulation blink when mode is rgb_ws2812.",
    status_led_color_reboot:
      "RGB overlay while a reboot is scheduled (default red). Dual GPIO mode flashes both LEDs together.",
    status_led_color_ap:
      "RGB overlay during soft‑AP setup / captive portal (default purple [102,0,255]). Dual GPIO flashes both outputs.",
    tempo_rte_enabled:
      "When source is not Linky, fetches EDF Tempo day colors from api-couleur-tempo.fr instead of Linky TIC tariff.",
    tz_country:
      "Select region to set IANA timezone for schedules, history day boundaries, and vacation end times.",
    time_ntp1:
      "Primary NTP server hostname or IP for clock sync. Required for accurate schedules and MQTT timestamps.",
    time_ntp2:
      "Secondary NTP server if the primary is unreachable.",
    http_cors_enabled:
      "Allows cross-origin GET /api/v1 and OPTIONS preflight (docs Try it out with your router IP). Lab only; GET reads — not cross-origin login. Do not enable on Internet-exposed networks.",
    http_auth_enabled:
      "Requires a login session on all /api/v1 requests when enabled. The session token is stored only in this browser tab.",
    http_auth_password:
      "API password stored hashed on device. Wi‑Fi AP mode still allows /wifi and backup restore without auth for recovery.",
    factory_reset:
      "Erases EEPROM configuration and history, then reboots. Export a backup first; this cannot be undone.",
  },

  api: {
    http_auth_password:
      "API password stored hashed on device. Wi‑Fi AP mode still allows /wifi and backup restore without auth for recovery. Changing it revokes all permanent access tokens.",
    http_cors_enabled:
      "Allows cross-origin GET /api/v1 and OPTIONS preflight (docs Try it out with your router IP). Lab only; GET reads — not cross-origin login. Do not enable on Internet-exposed networks.",
    api_access_token:
      "Long-lived bearer token for automation (curl, Home Assistant). Shown once at creation; the secret is stored on the device and included in Settings → Backup export.",
  },

  actions: {
    sensitivity:
      "How aggressively the triac adjusts to surplus changes. Low = stable but slow; high = fast but may oscillate around the setpoint. Stored as triac_sensitivity 1–100.",
    host:
      "Target for HTTP actions: peer IP/hostname, or localhost to toggle a GPIO on this ESP32 (see path format in hint).",
    port: "TCP port for HTTP actions (default 80). Ignored for localhost GPIO commands.",
    path_on:
      "URL path or query string sent to turn the load on. Remote example: /rpc/Switch.Set?id=0&on=true. Local GPIO: gpio=5&out=1",
    path_off: "URL or GPIO command to turn the load off. Can differ from path_on for asymmetric devices.",
    repeat_sec:
      "Re-sends the on/off command every N seconds while state should hold (0 = send once only). Useful for flaky HTTP devices.",
    tempo_sec:
      "Minimum seconds between HTTP state changes to avoid hammering the peer when regulation hunts.",
    edit_mode:
      "Window mode: forced off/on, power routing (thresholds on Pw), or triac routing. Last window runs until midnight.",
    edit_threshold:
      "Net house power threshold (W) for opening/closing in power mode. Sign convention: negative often means export/surplus.",
    edit_max_open: "Maximum triac opening (%) allowed in this time window.",
    edit_power_on: "In power mode: turn on when house power is below this value (W).",
    edit_power_off: "In power mode: turn off when house power is above this value (W).",
    edit_temp_inf:
      "Optional lower temperature bound (°C) with DS18B20. Leave empty (128) to disable gating for this window.",
    edit_temp_sup:
      "Optional upper temperature bound (°C). Legionella safety: avoid forcing 100% when probe is above cap in settings.",
    edit_hour_end: "Hour (0–24) when this window ends; next window starts at this hour.",
    action_daily_cap_wh:
      "Maximum energy (Wh) routed through the triac per calendar day, counted from CH2 daily export when available. 0 = no cap. Resets at midnight device local time.",
  },

  firmware: {
    fw_file: "Signed firmware binary (.bin) built for ESP32-WROOM-32 Balansun. Wrong chip or partition layout can brick the device.",
    fw_md5:
      "Optional MD5 checksum verified before OTA reboot. Strongly recommended when uploading files copied across networks.",
    ota_new: "New password for Arduino/PlatformIO OTA upload (optional security).",
    ota_confirm: "Repeat new OTA password to avoid typos.",
  },

  wifi: {
    wifi_ssid: "Wi‑Fi network name (SSID) to join in station mode. In AP setup mode, scan lists nearby networks.",
    wifi_password:
      "WPA passphrase for the selected network. Leave empty only for open networks (not recommended).",
  },

  sourceWizard: {
    ext_peer_ip:
      "IPv4 of the remote ESP or meter exposing GET /api/v1/measurements. Both devices must be on the same LAN.",
    ext_peer_port: "TCP port (default 80). Use if the peer listens on a non-standard port or behind a reverse proxy.",
    ext_peer_path:
      "HTTP path starting with / (default /api/v1/measurements). Max 48 characters.",
    enphase_user: "Enphase Envoy local API username if required by your gateway firmware.",
    enphase_password: "Enphase Envoy password for HTTPS/HTTP local access.",
    enphase_serial:
      "Optional serial or channel selector (e.g. Shelly Pro 3EM total channel = 3). See source-specific guide.",
    pmqtt_topic: "Broker topic for JSON power data when using Pmqtt source (same broker as MQTT settings).",
    jsy_mk333_serial_baud: "Baud rate for JSY-MK-333; must match meter DIP/ configuration (default 9600).",
    calib_u: "Analog voltage calibration coefficient (default 1000). Adjust after wiring analog clamps.",
    calib_i: "Analog current calibration coefficient (default 1000).",
    pmqtt_preset: "Choose JSON layout published by your external meter or Home Assistant integration.",
    defer_test:
      "Skip live diagnostics test on save. Use only if the peer is offline during setup; verify measurements afterward.",
  },

  httpAuth: {
    login_password:
      "HTTP API password set in Settings → HTTP access. Sign in on the login page to obtain a session token.",
  },

  install: {
    install_country:
      "Country preset sets default mains voltage and frequency. Same field as Settings; chosen early during first-time install.",
  },

  backup: {
    sectionSecurity:
      "Backup files contain Wi‑Fi credentials, MQTT passwords, and action URLs. Store encrypted or offline; treat like a password vault export.",
    sectionExport:
      "Downloads full router configuration as JSON. Export before firmware upgrades that bump EEPROM layout.",
    sectionImport:
      "Restores configuration from a backup file. Device may reboot; reconnect on the LAN afterward.",
  },
} as const;

export type FieldHelpTable = typeof fieldHelpEn;

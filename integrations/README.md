# Integrations

Third-party install packs referenced from this repo (not embedded in ESP32 flash).

| Integration | Install | Documentation |
|-------------|---------|---------------|
| Home Assistant — HACS custom component (REST polling) | [HACS](https://github.com/Balansun/HACS) | [HACS README](https://github.com/Balansun/HACS/blob/main/README.md) |
| Home Assistant — MQTT discovery (default) | Router MQTT on your broker | [HACS README — When to use HACS vs MQTT](https://github.com/Balansun/HACS/blob/main/README.md#when-to-use-hacs-vs-mqtt) |

The HACS integration source lives in **[HACS](https://github.com/Balansun/HACS)** (`custom_components/balansun/` at repository root). MQTT discovery is enabled by default on the router when a broker is configured.

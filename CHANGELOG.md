# Changelog

## [1.1.0](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/compare/hack_pack_nixie_clock-v1.0.0...hack_pack_nixie_clock-v1.1.0) (2026-09-02)


### Features

* **display:** log sanitized text string alongside raw message and clean up SelectType enum ([01b4e56](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/01b4e56ca65ef99938254df14cbca47db2b98e5d))
* **ota:** reduce Wi-Fi output power to 13 dBm during OTA firmware update ([cc532ec](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/cc532ec9c0bb29beb263b07d2bc51af953eabaaf))
* **timer:** overhaul timer background lifecycle, multi-mode button controls, and API actions ([e064e54](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/e064e54f11002167f201a1cd4c268710d5b9db2a))


### Bug Fixes

* **power:** add 1200ms power-stabilization grace period before initial LED write ([12366d8](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/12366d8013eea1d1bd37386ff982a89105822b5e))


### Documentation

* add power management and OTA update brownout protection section to README ([6b2744e](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/6b2744e0f7c47a81454cdc5be35b4ae38c5ed7d8))


### Code Refactoring

* modularize codebase and implement strategy pattern for color animations ([69460ca](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/69460ca7772b5cd817c06df0662f0e6e2000f871))

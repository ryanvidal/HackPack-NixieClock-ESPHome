# Changelog

## [1.1.0](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/compare/hack_pack_nixie_clock-v1.0.0...hack_pack_nixie_clock-v1.1.0) (2026-09-02)


### Features

* add core sensor, switch, and light platform entities ([f22f571](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/f22f5717da94caf9697516df56f3e83ae91d4002))
* **config:** internalize default hardware pin mappings ([a4f6265](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/a4f6265a08722a96ceee2bc7086ea1ef6d75c1c7))
* **display:** log sanitized text string alongside raw message and clean up SelectType enum ([c0e9f03](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/c0e9f0341826326cbf13d04f089a79529d94caf8))
* initial Nixie Clock ESPHome integration structure ([527edf3](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/527edf3315b1d9647c105c252bf5fe2eba57464d))
* initial repository setup ([0d88ec5](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/0d88ec57317a0d60b3375ab4b31d553abf1c0088))
* **ota:** reduce Wi-Fi output power to 13 dBm during OTA firmware update ([de20313](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/de2031301e2b28d1307ff4a7845ff528ec3dda1c))
* **timer:** overhaul timer background lifecycle, multi-mode button controls, and API actions ([bf5f564](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/bf5f564d9c59df12c72449b533a4e001677935a9))


### Bug Fixes

* **display:** correct colon formatting during scrolling and improve OTA ([1296491](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/129649195a271b81d3cdfe36651ad8e33f04458a))
* **examples:** correct relative components path in advanced configuration ([502bfac](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/502bfac3b375e6d5586c8e6bd6926f115b7f20e9))
* **lighting:** correct brightness gamma curves and panel linking ([d7c1670](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/d7c1670dede4a5ae9e5d4cc8a814e8bc3595629a))
* **ota:** optimize OTA pre-flight LED darkening ([8319f50](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/8319f506b7429925b28844ebafdd7aa0888ee181))
* **power:** add 1200ms power-stabilization grace period before initial LED write ([57c78c3](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/57c78c3a17ae24356d072746a894bdb84110c81b))


### Documentation

* add power management and OTA update brownout protection section to README ([2a3db75](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/2a3db75199895c3e1a7d4d05557b0d38a67a5a5b))


### Code Refactoring

* **display:** remove redundant display_mode select entity ([138375e](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/138375e3cac1587e3bf7e56616679ba29619d272))
* modularize codebase and implement strategy pattern for color animations ([d3113c3](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/d3113c396f3342938dace49372781a57a89e5c67))
* **timer:** remove redundant timer_remaining_seconds numeric sensor ([944068b](https://github.com/ryanvidal/HackPack-NixieClock-ESPHome/commit/944068be4f46193ec5bfd53473d22411eda96335))

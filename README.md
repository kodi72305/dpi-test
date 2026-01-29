# zmk-for-crosses46-dual

## This template is only compatible with the wireless Crosses46 keyboard whose PCB I have modified, not with the original pins.

### Default Firmware Keymap
![Keymap](keymap-drawer/crosses.svg)

## Trackball DPI controls

- Зажмите слой support (моментарный `&lt 1 ENTER`) и используйте клавиши в рядку трекбольных кнопок:
  1. Самая левая — выводит текущий CPI в лог (`Current Trackball …`).
  2. Следующая через один — большой шаг «−» (перескакивает через один пресет).
  3. После `LCLK/RCLK` идёт сброс на значение по умолчанию.
  4. Далее малый шаг «−», колёсико вниз, колёсико вверх, малый шаг «+».
  5. Перед `studio_unlock` стоит большой шаг «+» (тоже перескакивает через один пресет).
- Центральный трекбол (правая половина) теперь имеет четыре режима: 300 → 600 → 900 → 1200 CPI (по умолчанию 600).
- Левый трекбол использует 200 → 400 → 600 → 800 CPI (по умолчанию 400).
- Все значения и назначение клавиш можно менять прямо в `config/crosses.keymap` (секция `behaviors` и слой `support_layuer`).

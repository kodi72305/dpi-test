# zmk-for-crosses46-dual

## This template is only compatible with the wireless Crosses46 keyboard whose PCB I have modified, not with the original pins.

### Default Firmware Keymap
![Keymap](keymap-drawer/crosses.svg)

## Trackball DPI controls

- Hold the support layer and use the three keys around the scroll controls to manage DPI:
  - the key immediately after the left/right clicks resets DPI to the default for that half,
  - the next key decreases DPI,
  - the key before `studio_unlock` increases DPI.
- The right firmware targets the central trackball with the following steps: 200, 400, 600, 800, 1200 DPI (default 400).
- The left firmware targets the peripheral trackball with steps: 200, 300, 400, 500 DPI (default 200).
- You can customize the available steps or wrapping behaviour inside `config/crosses (1).keymap`.

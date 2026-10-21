# p002-matrix-demo

**Status:** prototype  ·  **Last HIL check:** never

One sentence on what it is and where it lives.

## Wiring
| Signal | GPIO | Goes to | Notes |
| ------ | ---- | ------- | ----- |
| LED data | `BOARD_LED_PIN` | level shifter → strip DIN | 330 Ω inline |

## Power
LEDs: 8 × WS2812B · cap 500 mA · supply: USB 5 V

## Demo
`just build p002-matrix-demo && just flash p002-matrix-demo && just watch p002-matrix-demo`; expect `boot`, `leds`, then `fps` every 5 s; LEDs go red → green → blue.

## Open issues
- [ ] …

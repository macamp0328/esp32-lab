# {{PROJECT}}

**Status:** prototype  ·  **Last HIL check:** never

One sentence on what it is and where it lives.

## Wiring
| Signal | GPIO | Goes to | Notes |
| ------ | ---- | ------- | ----- |
| LED data | `BOARD_LED_PIN` | level shifter → strip DIN | 330 Ω inline |

## Power
LEDs: 8 × WS2812B · cap 500 mA · supply: USB 5 V

## Demo
`just build {{PROJECT}} && just flash {{PROJECT}} && just watch {{PROJECT}}`; expect `boot`, `leds`, then `fps` every 5 s.

## Open issues
- [ ] …

# p000-blink HIL checklist

Asked in this order, batched in one connect (see docs/hil-protocol.md).

1. **s3mini.** **Do:** nothing; the board is on USB. **Look for:** the small onboard LED goes red → green → blue, about 1 s each, repeating. **Fail looks like:** dark, stuck on one color, or a different order. **Reply:** `ok` or `fail: <colors in the order you saw them>`.
2. **m5stickc-plus11.** **Do:** plug the M5 into USB-C. **Look for:** the screen shows `p000-blink` and a short code, and the small red LED near the USB port blinks once per second. **Fail looks like:** blank screen or LED off/solid. **Reply:** `ok` or `fail: <what you saw>`.

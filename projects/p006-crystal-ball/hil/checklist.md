# p006-crystal-ball HIL checklist

Asked in this order, batched in one connect (see docs/hil-protocol.md).

1. **Do:** plug the board in. **Look for:** within 10 s the ring glows amber, all 20 LEDs, evenly. **Fail looks like:** dark, blue or green (color order), first LED only, flicker. **Reply:** `ok`, `fail: <what you saw>`, or a photo.
2. **Do:** nothing; watch for the minute after plugging in. **Look for:** it slowly shifts hue and brightness, then fades out (unless it is 18:00–22:00). **Fail looks like:** steps, blinking, stays on. **Reply:** `ok`, or `fail: …`.
3. **Do:** set the ring under the orb. **Look for:** the orb glows; the dots hardly show. **Reply:** how bright (too dim / good / too bright) and the color you like.

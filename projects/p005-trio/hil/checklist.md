# p005-trio HIL checklist

All three boards on the dock, flashed with `just flash p005 --all`; `just watch p005 --all` green
first. Asked in this order, batched in one sitting (see docs/hil-protocol.md).

1. **Look for:** within ~10 s of boot, the two S3 pixels and the matrix's three corners flash
   together, once a second. **Fail looks like:** one board out of step after 20 s, matrix flicker.
2. **Do:** hold M5 button A (front, big) ~1 s. **Look for:** within ~20 s the flash walks round
   the three (S3, S3, M5), and on the matrix each pulse reaches the next corner as it fires.
   Hold A again to return to unison. **Fail looks like:** two flashing together, no rotation.
3. **Do:** clap once near the M5. **Look for:** all three flash at that instant.
4. **Do:** hold a flat hand between the two S3s for ~3 s. **Look for:** the matrix's bottom edge
   (a-b) brightens toward white-violet, then fades within ~2 s of taking the hand away.
5. **Do:** clap five times, then leave the dock alone. **Look for:** within ~3 min the three
   colors spread far apart (a triad); over the next ~10 min they drift back together.
6. **Do:** press M5 button B. **Look for:** the TFT switches between the loom (vertical threads,
   growing left to right) and the link numbers. With a `LOOM_THREAD_S 2` build, ~30 threads a
   minute.
7. **Do (CSI, phase 6):** `just watch p005 --all --seconds 60`; from 20 s to 40 s wave a hand
   between the S3s. **Reply:** done.

**Reply:** `ok`, `fail: <what you saw>`, or a photo, per step.

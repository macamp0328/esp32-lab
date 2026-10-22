# p003-bench HIL checklist

Asked in this order, batched in one connect (see docs/hil-protocol.md). Machine checks first:
`just watch p003` and `just bench p003` must pass.

1. **m5stickc-plus11 showcase.** **Do:** watch the screen, then press A (the big button) twice, about 5 s apart. **Look for:** the matrix plays plasma, fire and Life; the screen's fps reads about 210 at `n 64`, about 85 at `n 256`, about 25 at `n 1024`. **Fail looks like:** a frozen screen, a dark matrix, fps not changing. **Reply:** `ok` or `fail: <what you saw>`.
2. **s3mini.** **Do:** nothing. **Look for:** the onboard LED changes color steadily (pixel 0 of the showcase), and flickers fast during `just bench`. **Fail looks like:** dark or stuck. **Reply:** `ok` or `fail: <what you saw>`.

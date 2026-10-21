# Inventory

`stock.yaml` is the record of what exists. Miles edits it on every purchase, breakage or
assignment; the agent reads it before proposing hardware and may propose a diff.

`just inventory` prints a table and validates; `just check` validates too.

## Entry shape
```yaml
<category>:            # boards | leds | mics | power | parts | tools
  - id: s3mini-01..03  # kebab-case, unique; '..N' marks a numbered set
    model: Waveshare ESP32-S3 Mini     # required
    qty: 3                             # required, integer >= 0 (0 = planned or used up)
    source: Amazon B0CR2RH7PS          # required: where it came from, or "on hand"
    revision: ESP32-S3FH4R2, 4MB flash # optional: the revision that matters for pins/power
    specs: 5V 10A                      # optional
    date: 2026-10-20                   # optional, YYYY-MM-DD acquired
    projects: [p000-blink]             # optional: projects using it
    notes: headers shipped loose       # optional
```

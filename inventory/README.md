# Inventory

`stock.yaml` is the record of what exists. The agent edits it when Miles asks: typically
"I ordered X, look at the order and update stock", a breakage, or assigning parts to a project.
The agent also reads it before proposing hardware. Miles rarely edits it by hand.

`just inventory` prints a table and validates; `just check` validates too.

## Entry shape
```yaml
<category>:            # boards | leds | mics | power | parts | tools
  - id: s3mini-01..03  # kebab-case, unique; '..N' marks a numbered set
    model: Waveshare ESP32-S3 Mini     # required
    board: s3mini                      # boards only: the boards/<name> it uses (`just new` lists these)
    qty: 3                             # required, integer >= 0 (0 = planned or used up)
    source: Amazon B0CR2RH7PS          # required: where it came from, or "on hand"
    revision: ESP32-S3FH4R2, 4MB flash # optional: the revision that matters for pins/power
    specs: 5V 10A                      # optional
    date: 2026-10-20                   # optional, YYYY-MM-DD acquired
    projects: [p000-blink]             # optional: projects using it
    notes: headers shipped loose       # optional
```

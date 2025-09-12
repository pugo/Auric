# MOS 6522 VIA – Shift Register timing notes

The 6522 shift register (SR) can be clocked from T2. This is one of the most confusing parts of the VIA, since the official datasheet leaves out key details. The following reflects **real silicon behaviour**, confirmed on NMOS, CMOS, and Visual 6502.

---

## General rules
- SR mode is selected by **ACR bits 4–2**.
- In all **T2-driven SR modes** (shift in/out, free-run or one-shot), **only T2C-L (low latch)** is used. The high latch is ignored. Effective period = 1–255 cycles.
- On entry into any T2-driven SR mode (ACR change, VIA reset), clear:
    - `sr_counter = 0` (bit counter)
    - `sr_started = false` (forces initial wait)
    - `byte_gap_pending = false`

---

## Shift **out** under T2 – one-shot (`ACR = 0x14`)
- **Initial delay:** 1 × T2 period before first bit.
- **8 shifts, then stop.**
- No extra delay after the 8th bit (no “gap”).
- **IFR.SR** is set on the 8th shift.
- The SR halts until explicitly re-armed (typically by a CPU write to SR).
- Writing SR while idle re-arms: next transfer will again wait 1 × T2 before bit 0.

### Timing (one-shot out)

```
T2 expiry:    |----|----|----|----|----|----|----|----|    
CB1 pulse:    ^    ^    ^    ^    ^    ^    ^    ^        
SR shift:     b0   b1   b2   b3   b4   b5   b6   b7       
IFR.SR:                                     set           
```

---

## Shift **out** under T2 – free-running (`ACR = 0x10`)
- **Initial delay:** 1 × T2 period before first bit of the first byte.
- **8 shifts per byte**.
- **+1 extra T2 period** occurs between bytes (so total = 9 T2 expiries per byte).
- **IFR.SR** is set at the end of each byte (after 8th shift).
- Clocking continues indefinitely; if CPU doesn’t rewrite SR, the same contents keep circulating (with defined fill bit on input side).
- Writing SR mid-stream just reloads data, no re-arm delay.

### Timing (free-running out, two bytes)

```
T2 expiry:    |----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|    
CB1 pulse:    ^    ^    ^    ^    ^    ^    ^    ^         ^    ^    ^    ^    ^    ^    ^    
SR shift:     b0   b1   b2   b3   b4   b5   b6   b7        b0   b1   b2   b3   b4   b5   b6   
IFR.SR:                                     set                                set            
```

Note the **gap**: one extra T2 expiry between the last bit of a byte and the first bit of the next.

---

## Shift **in** under T2 (`ACR = 0x18`)
- **Initial delay:** 1 × T2 period before first bit.
- Exactly **8 shifts**, no extra gap.
- **IFR.SR** is set on the 8th bit.
- SR stops until re-armed (by writing SR to clear IFR, or re-entering mode).

### Timing (shift-in)

```
T2 expiry:    |----|----|----|----|----|----|----|----|    
CB1 pulse:    ^    ^    ^    ^    ^    ^    ^    ^        
SR sample:    b0   b1   b2   b3   b4   b5   b6   b7       
IFR.SR:                                     set           
```

---

## CB1/CB2 behaviour
- **CB1**: pulses once per T2 expiry (not a continuous square wave).
- **CB2**: serves as SR data out (in output modes) or data in (in input modes).
- Idle levels and directions follow PCR setup.

---

## Implementation tips
- Always clear `sr_counter = 0` when entering an SR mode.
- Track an `sr_started` flag to insert the initial 1 × T2 delay.
- In free-running output, track a `byte_gap_pending` flag to add the extra T2 between bytes.
- Do **not** reset `sr_started` on SR writes; only on VIA reset or ACR mode change.
- Only use `T2C-L` for the SR timer period in these modes.

---

This matches what you’ll see if you scope CB1/CB2 on a real 6522 or run the Visual 6502 transistor-level model.

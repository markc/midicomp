# Adversarial test fixtures

Malformed inputs that previously crashed midicomp (NULL deref, OOB read, SIGFPE)
or caused undefined behaviour in the compile path. The `security` CTest asserts
midicomp processes each without crashing (clean exit, no signal). See the v0.2.0
security audit and the project CHANGELOG.

- `meta-tempo-len0.mid`   FF 51 00 — zero-length tempo meta (was NULL deref at metaevent)
- `meta-smpte-len0.mid`   FF 54 00 — zero-length SMPTE meta (was OOB read m[0..4])
- `meta-keysig-len0.mid`  FF 59 00 — zero-length key-signature meta (was OOB read)
- `header-division0.mid`  MThd division=0 (was SIGFPE in prtime under -t)
- `huge-varlen.mid`       6-byte variable-length quantity (was shift-past-width UB)
- `compile-value-oob.txt`     v=200 (was UB: error() didn't abort, wrote bad byte)
- `compile-timesig-denom0.txt` TimeSig denominator 0 (was divide-by-zero path)
- `compile-hex-oob.txt`       hex byte 0x1234 (was truncated silently)

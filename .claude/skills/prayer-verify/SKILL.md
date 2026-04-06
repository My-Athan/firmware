---
name: prayer-verify
description: Verify prayer time calculation accuracy for a specific city. Use to validate PrayerCalculator output against known references.
allowed-tools: Bash Read WebSearch WebFetch
argument-hint: "[city_name]"
---

# Verify Prayer Time Accuracy

Compare on-device PrayerCalculator output against the Aladhan API for a given city.

## Steps

1. Look up the city coordinates and timezone
2. Fetch reference prayer times from Aladhan API:
   ```
   https://api.aladhan.com/v1/timings/{date}?latitude={lat}&longitude={lon}&method={method}
   ```
3. Compare against our PrayerCalculator output for the same inputs
4. Report differences per prayer time (should be within ±3 minutes)

## Common Test Cities
| City | Lat | Lon | Method | Notes |
|------|-----|-----|--------|-------|
| Mecca | 21.4225 | 39.8262 | MAKKAH | Reference city |
| New York | 40.7128 | -74.0060 | ISNA | Standard US |
| London | 51.5074 | -0.1278 | MWL | European |
| Cairo | 30.0444 | 31.2357 | EGYPT | Egyptian method |
| Karachi | 24.8607 | 67.0011 | KARACHI | South Asian |
| Reykjavik | 64.1466 | -21.9426 | MWL | High latitude |
| Oslo | 59.9139 | 10.7522 | MWL | High latitude |

## Acceptable Tolerance
- Normal latitudes (<48°): ±3 minutes
- High latitudes (>48°): ±5 minutes (methods vary more)
- Makkah method Isha: fixed 90 min after Maghrib (exact match expected)

## Method Codes (Aladhan API)
- ISNA = 2, MWL = 3, EGYPT = 5, MAKKAH = 4, KARACHI = 1, TEHRAN = 7, JAFARI = 0

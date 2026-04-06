#include "PrayerCalculator.h"

// ─────────────────────────────────────────────────────────────
// Constants
// ─────────────────────────────────────────────────────────────

static const double DEG2RAD = M_PI / 180.0;
static const double RAD2DEG = 180.0 / M_PI;

// ─────────────────────────────────────────────────────────────
// Method configuration
// ─────────────────────────────────────────────────────────────

void PrayerCalculator::setMethod(CalcMethod method) {
    _method = method;
}

void PrayerCalculator::setMethodFromString(const char* s) {
    if      (strcasecmp(s, "ISNA") == 0)    _method = CalcMethod::ISNA;
    else if (strcasecmp(s, "MWL") == 0)     _method = CalcMethod::MWL;
    else if (strcasecmp(s, "EGYPT") == 0)   _method = CalcMethod::EGYPT;
    else if (strcasecmp(s, "MAKKAH") == 0)  _method = CalcMethod::MAKKAH;
    else if (strcasecmp(s, "KARACHI") == 0) _method = CalcMethod::KARACHI;
    else if (strcasecmp(s, "TEHRAN") == 0)  _method = CalcMethod::TEHRAN;
    else if (strcasecmp(s, "JAFARI") == 0)  _method = CalcMethod::JAFARI;
    else                                     _method = CalcMethod::ISNA;
}

void PrayerCalculator::setAsrJuristic(AsrJuristic j) {
    _asrJuristic = j;
}

void PrayerCalculator::setAsrFromString(const char* s) {
    _asrJuristic = (strcasecmp(s, "hanafi") == 0) ? AsrJuristic::HANAFI : AsrJuristic::STANDARD;
}

void PrayerCalculator::setHighLatMethod(HighLatMethod m) {
    _highLat = m;
}

void PrayerCalculator::setHighLatFromString(const char* s) {
    if      (strcasecmp(s, "angle_based") == 0)  _highLat = HighLatMethod::ANGLE_BASED;
    else if (strcasecmp(s, "midnight") == 0)     _highLat = HighLatMethod::MIDNIGHT;
    else if (strcasecmp(s, "one_seventh") == 0)  _highLat = HighLatMethod::ONE_SEVENTH;
    else                                          _highLat = HighLatMethod::NONE;
}

PrayerCalculator::MethodParams PrayerCalculator::_getParams() const {
    switch (_method) {
        case CalcMethod::ISNA:    return {15.0,  15.0,   0};
        case CalcMethod::MWL:     return {18.0,  17.0,   0};
        case CalcMethod::EGYPT:   return {19.5,  17.5,   0};
        case CalcMethod::MAKKAH:  return {18.5,   0.0,  90};  // 90 min after Maghrib
        case CalcMethod::KARACHI: return {18.0,  18.0,   0};
        case CalcMethod::TEHRAN:  return {17.7,  14.0,   0};
        case CalcMethod::JAFARI:  return {16.0,  14.0,   0};
        default:                  return {15.0,  15.0,   0};
    }
}

// ─────────────────────────────────────────────────────────────
// Astronomical calculations
// ─────────────────────────────────────────────────────────────

double PrayerCalculator::_julianDate(int year, int month, int day) {
    if (month <= 2) {
        year -= 1;
        month += 12;
    }
    double A = floor(year / 100.0);
    double B = 2 - A + floor(A / 4.0);
    return floor(365.25 * (year + 4716)) + floor(30.6001 * (month + 1)) + day + B - 1524.5;
}

double PrayerCalculator::_sunPosition(double jd, double& decl, double& eqt) {
    double D = jd - 2451545.0;  // Days since J2000.0

    double g = _fixHour(357.529 + 0.98560028 * D);  // Mean anomaly
    double q = _fixHour(280.459 + 0.98564736 * D);  // Mean longitude
    double L = _fixHour(q + 1.915 * sin(g * DEG2RAD) + 0.020 * sin(2 * g * DEG2RAD));

    double e = 23.439 - 0.00000036 * D;  // Obliquity of ecliptic
    double RA = RAD2DEG * atan2(cos(e * DEG2RAD) * sin(L * DEG2RAD), cos(L * DEG2RAD)) / 15.0;

    decl = RAD2DEG * asin(sin(e * DEG2RAD) * sin(L * DEG2RAD));
    eqt = q / 15.0 - _fixHour(RA);

    return D;
}

// ─────────────────────────────────────────────────────────────
// Time calculations
// ─────────────────────────────────────────────────────────────

double PrayerCalculator::_computeMidDay(double eqt) const {
    return _fixHour(12.0 - eqt);
}

double PrayerCalculator::_computeAsr(double decl, double lat) const {
    double factor = (_asrJuristic == AsrJuristic::HANAFI) ? 2.0 : 1.0;
    double angle = RAD2DEG * atan(1.0 / (factor + tan(fabs(lat - decl) * DEG2RAD)));
    return angle;
}

double PrayerCalculator::_computeAngleTime(double angle, double decl, double lat, bool ccw) const {
    double cosVal = (sin(angle * DEG2RAD) - sin(lat * DEG2RAD) * sin(decl * DEG2RAD))
                  / (cos(lat * DEG2RAD) * cos(decl * DEG2RAD));

    // Clamp to valid range
    if (cosVal > 1.0) cosVal = 1.0;
    if (cosVal < -1.0) cosVal = -1.0;

    double t = RAD2DEG * acos(cosVal) / 15.0;
    return ccw ? -t : t;
}

// ─────────────────────────────────────────────────────────────
// Main calculation
// ─────────────────────────────────────────────────────────────

PrayerTimes PrayerCalculator::calculate(int year, int month, int day, double lat, double lon) {
    MethodParams params = _getParams();
    double jd = _julianDate(year, month, day);

    double decl, eqt;
    _sunPosition(jd, decl, eqt);

    double midday = _computeMidDay(eqt);
    double lonCorrection = lon / 15.0;  // Not used in local solar time, but needed for timezone offset

    // Dhuhr
    double dhuhr = midday;

    // Sunrise & Sunset (sun at -0.8333° for atmospheric refraction)
    double sunAngleTime = _computeAngleTime(-0.8333, decl, lat, false);
    double sunrise = dhuhr - sunAngleTime;
    double sunset = dhuhr + sunAngleTime;

    // Fajr
    double fajrAngle = _computeAngleTime(-params.fajrAngle, decl, lat, false);
    double fajr = dhuhr - fajrAngle;

    // ASR
    double asrAngle = _computeAsr(decl, lat);
    double asrTime = _computeAngleTime(-asrAngle, decl, lat, false);
    double asr = dhuhr + asrTime;

    // Maghrib (sunset)
    double maghrib = sunset;

    // Isha
    double isha;
    if (params.ishaMins > 0) {
        isha = maghrib + params.ishaMins / 60.0;
    } else {
        double ishaAngle = _computeAngleTime(-params.ishaAngle, decl, lat, false);
        isha = dhuhr + ishaAngle;
    }

    // Build result in hours
    PrayerTimes times;
    times.fajr    = _toMinutes(fajr);
    times.sunrise = _toMinutes(sunrise);
    times.dhuhr   = _toMinutes(dhuhr);
    times.asr     = _toMinutes(asr);
    times.maghrib = _toMinutes(maghrib);
    times.isha    = _toMinutes(isha);

    // Apply high-latitude adjustments
    _adjustHighLatitude(times, lat);

    return times;
}

// ─────────────────────────────────────────────────────────────
// High-latitude adjustments
// ─────────────────────────────────────────────────────────────

void PrayerCalculator::_adjustHighLatitude(PrayerTimes& times, double lat) const {
    if (_highLat == HighLatMethod::NONE) return;
    if (fabs(lat) < 48.0) return;  // Only apply above 48° N/S

    int nightDuration = times.sunrise + (24 * 60 - times.maghrib);  // Night in minutes
    MethodParams params = _getParams();

    int fajrAdj = 0, ishaAdj = 0;

    switch (_highLat) {
        case HighLatMethod::MIDNIGHT: {
            int halfNight = nightDuration / 2;
            fajrAdj = times.sunrise - halfNight;
            ishaAdj = times.maghrib + halfNight;
            break;
        }

        case HighLatMethod::ONE_SEVENTH: {
            int seventh = nightDuration / 7;
            fajrAdj = times.sunrise - seventh;
            ishaAdj = times.maghrib + seventh;
            break;
        }

        case HighLatMethod::ANGLE_BASED: {
            // Proportion of night based on angle
            double fajrRatio = params.fajrAngle / 60.0;
            double ishaRatio = (params.ishaMins > 0) ? (params.ishaMins / 60.0 / 24.0) : (params.ishaAngle / 60.0);

            fajrAdj = times.sunrise - (int)(nightDuration * fajrRatio);
            ishaAdj = times.maghrib + (int)(nightDuration * ishaRatio);
            break;
        }

        default:
            return;
    }

    // Only adjust if calculated time is invalid (past sunrise for Fajr, or NaN-like)
    if (times.fajr < 0 || times.fajr > times.sunrise) {
        times.fajr = fajrAdj;
    }
    if (times.isha < times.maghrib || times.isha > 24 * 60) {
        times.isha = ishaAdj;
    }
}

// ─────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────

int PrayerCalculator::_toMinutes(double hours) {
    hours = _fixHour(hours);
    return (int)round(hours * 60.0);
}

double PrayerCalculator::_fixHour(double h) {
    h = fmod(h, 24.0);
    if (h < 0) h += 24.0;
    return h;
}

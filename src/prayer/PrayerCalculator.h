#pragma once

#include <Arduino.h>
#include <math.h>

// ─────────────────────────────────────────────────────────────
// Prayer Time Calculator — based on PrayTimes.org algorithms
// Fully on-device, no cloud dependency required.
// ─────────────────────────────────────────────────────────────

// Calculation methods
enum class CalcMethod {
    ISNA,       // Islamic Society of North America (Fajr 15°, Isha 15°)
    MWL,        // Muslim World League (Fajr 18°, Isha 17°)
    EGYPT,      // Egyptian General Authority (Fajr 19.5°, Isha 17.5°)
    MAKKAH,     // Umm al-Qura, Makkah (Fajr 18.5°, Isha 90min/120min Ramadan)
    KARACHI,    // University of Islamic Sciences, Karachi (Fajr 18°, Isha 18°)
    TEHRAN,     // Institute of Geophysics, Tehran (Fajr 17.7°, Isha 14°)
    JAFARI      // Shia Ithna-Ashari (Fajr 16°, Isha 14°)
};

// ASR juristic method
enum class AsrJuristic {
    STANDARD,   // Shafi'i, Maliki, Hanbali (shadow = 1x object)
    HANAFI      // Hanafi (shadow = 2x object)
};

// High-latitude adjustment
enum class HighLatMethod {
    NONE,
    ANGLE_BASED,     // Proportion of night based on angle
    MIDNIGHT,        // Split night in half
    ONE_SEVENTH      // 1/7 of night for Fajr/Isha
};

// Result structure: times in minutes since midnight
struct PrayerTimes {
    int fajr;
    int sunrise;
    int dhuhr;
    int asr;
    int maghrib;
    int isha;
};

class PrayerCalculator {
public:
    void setMethod(CalcMethod method);
    void setMethodFromString(const char* method);
    void setAsrJuristic(AsrJuristic juristic);
    void setAsrFromString(const char* juristic);
    void setHighLatMethod(HighLatMethod method);
    void setHighLatFromString(const char* method);

    // Calculate prayer times for a given date and location
    PrayerTimes calculate(int year, int month, int day, double latitude, double longitude);

    CalcMethod getMethod() const { return _method; }

private:
    CalcMethod _method = CalcMethod::ISNA;
    AsrJuristic _asrJuristic = AsrJuristic::STANDARD;
    HighLatMethod _highLat = HighLatMethod::ANGLE_BASED;

    // Method parameters
    struct MethodParams {
        double fajrAngle;
        double ishaAngle;
        int ishaMins;       // 0 = use angle, >0 = fixed minutes after maghrib
    };

    MethodParams _getParams() const;

    // Astronomical calculations
    static double _julianDate(int year, int month, int day);
    static double _sunDeclination(double jd);
    static double _equationOfTime(double jd);
    static double _sunPosition(double jd, double& decl, double& eqt);

    // Time calculations
    double _computeMidDay(double eqt) const;
    double _computeAsr(double decl, double lat) const;
    double _computeAngleTime(double angle, double decl, double lat, bool ccw) const;

    // High-latitude adjustments
    void _adjustHighLatitude(PrayerTimes& times, double lat) const;

    // Helpers
    static int _toMinutes(double hours);
    static double _fixHour(double h);
};

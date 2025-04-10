/*
 * 42TinyJS
 *
 * A fork of TinyJS with the goal to makes a more JavaScript/ECMA compliant engine
 *
 * Authored By Armin Diedering <armin@diedering.de>
 *
 * Copyright (C) 2010-2025 ardisoft
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <chrono>
#include "TinyJS.h"

#if __cpp_lib_chrono < 201907L

#include <ctime>


#ifndef _MSC_VER

#define _tzset tzset
#define _get_timezone(pTimezone) do{*pTimezone=timezone;}while(0)
#define sprintf_s sprintf

#endif

#ifndef _WIN32
#define localtime_s(tm, time) localtime_r(time, tm)
#endif

#endif /* __cpp_lib_chrono < 201907L */

using namespace std::chrono;

namespace TinyJS {


template <int32_t RANGE>
constexpr static void normalize(int32_t &higherUnit, int32_t &lowerUnit) {
	if (lowerUnit >= RANGE) {
		higherUnit += lowerUnit / RANGE;
		lowerUnit %= RANGE;
	} else if (lowerUnit < 0) {
		higherUnit += (RANGE - 1 - lowerUnit) / RANGE;
		lowerUnit = RANGE - 1 - (RANGE - 1 - lowerUnit) % RANGE;
	}
}


#if __cpp_lib_chrono >= 201907L

//////////////////////////////////////////////////////////////////////////
/// CScriptTime
//////////////////////////////////////////////////////////////////////////

using namespace std::chrono;
class CScriptTime {
public:
	CScriptTime(bool isLocalTime = true) : islocal(isLocalTime), isvalid(true) {
		setTime(0);
	}
	CScriptTime(int64_t Time, bool isLocalTime = true) : islocal(isLocalTime), isvalid(true) {
		setTime(Time);
	}

	bool isValid() const { return isvalid; }
	void setInvalide() { setTime(0); isvalid = false; }

	std::string toDateString() const;
	std::string toTimeString() const;
	std::string castToString() const;
	operator std::string() const { return castToString(); }
	std::string toISOString() const;

	int64_t setTime(int32_t Year, int32_t Month, int32_t Day = 1, int32_t Hour = 0, int32_t Min = 0, int32_t Sec = 0, int32_t MSec = 0) {
		normalize<12>(Year, Month);
		// Datum bauen (Achtung: Month ist 0-basiert im Originalcode – hier korrigiert)
		year_month_day ymd = year{ Year } / month{ unsigned(Month + 1) } / 1;
		local_days ld = local_days{ ymd } + days{ Day - 1 };

		// Zeit in lokalen Millisekunden
		auto lt = local_time<milliseconds>{ ld } + hours{ Hour } + minutes{ Min } + seconds{ Sec } + milliseconds{ MSec };

		// Zeitzone anwenden (current_zone = system local time zone)
		auto zt = zoned_time{ current_zone(), lt };

		// UTC-Zeit extrahieren
		tp = zt.get_sys_time();
		invalidateCache();
		return duration_cast<milliseconds>(tp.time_since_epoch()).count();
	}
	int64_t setUTCTime(int32_t Year, int32_t Month, int32_t Day = 1, int32_t Hour = 0, int32_t Min = 0, int32_t Sec = 0, int32_t MSec = 0) {
		normalize<12>(Year, Month);
		// Datum bauen (Achtung: Month ist 0-basiert im Originalcode – hier korrigiert)
		year_month_day ymd = year{ Year } / month{ unsigned(Month + 1) } / 1;
		sys_days sd = sys_days{ ymd } + days{ Day - 1 };

		// UTC-Zeit direkt aus Datum und Uhrzeit (kein zoned_time nötig)
		auto utc_time = sys_time<milliseconds>{ sd } + hours{ Hour } + minutes{ Min } + seconds{ Sec } + milliseconds{ MSec };

		tp = utc_time;
		invalidateCache();
		return duration_cast<milliseconds>(tp.time_since_epoch()).count();
	}
	int64_t setTime(int64_t Time) {
		tp = system_clock::time_point{ milliseconds(Time) };
		invalidateCache();
		return duration_cast<milliseconds>(tp.time_since_epoch()).count();
	}
	int64_t getTime() const { return duration_cast<milliseconds>(tp.time_since_epoch()).count(); }
	operator int64_t() const { return getTime(); }


	int64_t setYear(int32_t Year) { return  setFullYear(Year + 1900); }
	int32_t getYear() const { return getFullYear() - 1900; }

	int64_t setFullYear(int32_t Year) { return setFullYear(Year, getMonth()); }
	int64_t setFullYear(int32_t Year, int32_t Month) { return setFullYear(Year, Month, getDay()); }
	int64_t setFullYear(int32_t Year, int32_t Month, int32_t Day) { return setTime(Year, Month, Day, getHours(), getMinutes(), getSeconds(), getMilliseconds()); }
	int32_t getFullYear() const { return int(yyyymmddLocal().year()); }

	int64_t setUTCFullYear(int32_t Year) { return setUTCFullYear(Year, getUTCMonth()); }
	int64_t setUTCFullYear(int32_t Year, int32_t Month) { return setUTCFullYear(Year, getUTCMonth(), getUTCDay()); }
	int64_t setUTCFullYear(int32_t Year, int32_t Month, int32_t Day) { return setUTCTime(Year, Month, Day, getUTCHours(), getUTCMinutes(), getUTCSeconds(), getUTCMilliseconds()); }
	int32_t getUTCFullYear() const { return int(yyyymmdd().year()); }

	int64_t setMonth(int32_t Month) { return setTime(getYear(), Month, getDay(), getHours(), getMinutes(), getSeconds(), getMilliseconds()); }
	int32_t getMonth() const { return unsigned(yyyymmddLocal().month()) - 1; }
	int64_t setUTCMonth(int32_t Month) { return setUTCTime(getUTCFullYear(), Month, getUTCDay(), getUTCHours(), getUTCMinutes(), getUTCSeconds(), getUTCMilliseconds()); }
	int32_t getUTCMonth() const { return unsigned(yyyymmdd().month()) - 1; }

	int64_t setDate(int32_t Date) { tp += days{ Date - getDate() }; return getTime(); }
	int32_t getDate() const { return unsigned(yyyymmddLocal().day()); }
	int64_t setUTCDate(int32_t Date) { tp += days{ Date - getUTCDate() }; return getTime(); }
	int32_t getUTCDate() const { return unsigned(yyyymmdd().day()); }

	int32_t getDay() const { return int32_t((floor<days>(localTime().time_since_epoch()).count() % 7) + 11) % 7; }
	int32_t getUTCDay() const { return int32_t((floor<days>(tp.time_since_epoch()).count() % 7) + 11) % 7; }

	int64_t setHours(int32_t Hour) { tp += hours{ Hour - getHours() }; invalidateCache(); return getTime(); }
	int32_t getHours() const { return hhmmssLocal().hours().count(); }
	int64_t setUTCHours(int32_t Hour) { tp += hours{ Hour - getUTCHours() }; invalidateCache(); return getTime(); }
	int32_t getUTCHours() const { return hhmmss().hours().count(); }

	int64_t setMinutes(int32_t Min) { tp += minutes{ Min - getMinutes() }; invalidateCache(); return getTime(); }
	int32_t getMinutes() const { return hhmmssLocal().minutes().count(); }
	int64_t setUTCMinutes(int32_t Min) { tp += minutes{ Min - getUTCMinutes() }; invalidateCache(); return getTime(); }
	int32_t getUTCMinutes() const { return hhmmss().minutes().count(); }

	int64_t setSeconds(int32_t Sec) { tp += seconds{ Sec - getSeconds() }; invalidateCache(); return getTime(); }
	int32_t getSeconds() const { return int32_t(hhmmssLocal().seconds().count()); }
	int64_t setUTCSeconds(int32_t Sec) { tp += seconds{ Sec - getUTCSeconds() }; invalidateCache(); return getTime(); }
	int32_t getUTCSeconds() const { return int32_t(hhmmss().seconds().count()); }

	int64_t setMilliseconds(int32_t Msec) { tp += milliseconds{ Msec - getMilliseconds() }; invalidateCache(); return getTime(); }
	int32_t getMilliseconds() const { return int32_t(hhmmssLocal().subseconds().count()); }
	int64_t setUTCMilliseconds(int32_t Msec) { tp += milliseconds{ Msec - getUTCMilliseconds() }; invalidateCache(); return getTime(); }
	int32_t getUTCMilliseconds() const { return int32_t(hhmmss().subseconds().count()); }

	
	int32_t getTimezoneOffset() const { return duration_cast<minutes>(tp.time_since_epoch() - localTime().time_since_epoch()).count(); }

// 	int32_t getTimezoneOffset() const {
// 		zoned_time zt(current_zone(), tp);
// 		auto d = duration_cast<seconds>(zt.get_sys_time().time_since_epoch() - zt.get_local_time().time_since_epoch()).count();
// 		return duration_cast<minutes>(zt.get_sys_time().time_since_epoch() - zt.get_local_time().time_since_epoch()).count();
// 	}
	int32_t getGMTOffset() const { int32_t offset = std::abs(getTimezoneOffset()); return ((offset / 60) * 100 + offset % 60) * (getTimezoneOffset() < 0 ? 1 : -1); }

	static int64_t UTC(int32_t Year, int32_t Month, int32_t Day = 1, int32_t Hour = 0, int32_t Min = 0, int32_t Sec = 0, int32_t MSec = 0) {
		return CScriptTime().setUTCTime(Year, Month, Day, Hour, Min, Sec, MSec);
	}
	static int64_t now() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }
	static bool ParseISODate(const char *s, int64_t *result);
	static bool ParseDate(const char *s, int64_t *result);
	static bool Parse(const std::string &DateString, int64_t *result) { return ParseDate(DateString.c_str(), result); }

private:
	system_clock::time_point tp;
	bool islocal;
	bool isvalid;

	mutable std::optional<local_time<system_clock::duration>> localTime_cached;
	local_time<system_clock::duration> localTime() const {
		if (!localTime_cached) {
			if (islocal) {
				// Hole lokale Zeit direkt
				localTime_cached = current_zone()->to_local(tp);
			} else {
				// Konvertiere UTC-Zeitpunkt zu local_time
				localTime_cached = local_time{ tp.time_since_epoch() };
			}
		}
		return *localTime_cached;
	}
	mutable std::optional<year_month_day> yyyymmdd_cache;
	year_month_day yyyymmdd() const {
		if(!yyyymmdd_cache) yyyymmdd_cache.emplace(floor<days>(tp));
		return *yyyymmdd_cache;
	}
	mutable std::optional<year_month_day> yyyymmddLocal_cache;
	year_month_day yyyymmddLocal() const {
		if(!yyyymmddLocal_cache) yyyymmddLocal_cache.emplace(floor<days>(localTime()));
		return *yyyymmddLocal_cache;
	}
	mutable std::optional<hh_mm_ss<milliseconds>> hhmmss_cache;
	hh_mm_ss<milliseconds> hhmmss() const {
		if (!hhmmss_cache) hhmmss_cache.emplace(duration_cast<milliseconds>(tp - floor<days>(tp)));
		return *hhmmss_cache;
	}
	mutable std::optional<hh_mm_ss<milliseconds>> hhmmssLocal_cache;
	hh_mm_ss<milliseconds> hhmmssLocal() const {
		if (!hhmmssLocal_cache)hhmmssLocal_cache.emplace(duration_cast<milliseconds>(localTime() - floor<days>(localTime())));
		return *hhmmssLocal_cache;
	}
	void invalidateCache() {
		localTime_cached.reset();
		yyyymmdd_cache.reset();
		yyyymmddLocal_cache.reset();
		hhmmss_cache.reset();
		hhmmssLocal_cache.reset();
		isvalid = true;
	}
	static constexpr int32_t leapsThruEndOf(int32_t y) { return (y + 4) / 4 - y / 100 + y / 400; }
	static constexpr int32_t isLeapYear(int32_t y) { return ((!(y % 4) && (y % 100)) || !(y % 400)) ? 1 : 0; }

	static inline const int32_t monthLengths[2][12] = {
		{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
		{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	};
	static inline const int32_t firstDayOfMonth[2][13] = {
		{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
		{0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}
	};

};

#else

namespace {


static class CTimeInitializer {
public:
	CTimeInitializer() {
		static bool tzset_called = false;
		if (!tzset_called) {
			_tzset();
			_get_timezone(&timezone);
			tzset_called = true;
		}
	}
	long timezone = 0;
} initializer;

#define TIMEZONE initializer.timezone

} /* namespace */


//////////////////////////////////////////////////////////////////////////
/// CScriptTime
//////////////////////////////////////////////////////////////////////////

class CScriptTime {
public:
	CScriptTime(bool isLocalTime=true);
	CScriptTime(int64_t Time, bool isLocalTime=true);

	static int64_t UTC(int32_t Year, int32_t Month, int32_t Day=1, int32_t Hour=0, int32_t Min=0, int32_t Sec=0, int32_t MSec=0);
	static int64_t now();
	static bool ParseISODate(const char *s, int64_t *result);
	static bool ParseDate(const char *s, int64_t *result);
	static bool Parse(const std::string &DateString, int64_t *result) { return ParseDate(DateString.c_str(), result); }

	bool isValid() const { return tm_isvalid; }
	void setInvalide() { setTime(0); tm_isvalid= false; }

	int64_t setTime(int32_t Year, int32_t Month, int32_t Day=1, int32_t Hour=0, int32_t Min=0, int32_t Sec=0, int32_t MSec=0);
	int64_t setTime(int64_t Time);
	int64_t getTime() const					{ return tm_time; }
	operator int64_t() const				{ return getTime(); }

	int64_t setDate(int32_t Date)			{ tm_mday = Date; return fix(); }
	int32_t getDate() const					{ return tm_mday; }

	int64_t setUTCDate(int32_t Date)		{ return setTime(CScriptTime(*this, false).setDate(Date)); }
	int32_t getUTCDate() const				{ return CScriptTime(*this, false).tm_mday; }


	int32_t getDay() const					{ return tm_wday; }
	int32_t getUTCDay() const				{ return CScriptTime(*this, false).tm_wday; }

	int64_t setYear(int32_t Year);
	int32_t getYear() const					{ return tm_year - 1900; }

	int64_t setFullYear(int32_t Year);
	int64_t setFullYear(int32_t Year, int32_t Month);
	int64_t setFullYear(int32_t Year, int32_t Month, int32_t Day);
	int32_t getFullYear() const											{ return tm_year; }

	int64_t setUTCFullYear(int32_t Year)								{ return setTime(CScriptTime(*this, false).setFullYear(Year)); }
	int64_t setUTCFullYear(int32_t Year, int32_t Month)					{ return setTime(CScriptTime(*this, false).setFullYear(Year, Month)); }
	int64_t setUTCFullYear(int32_t Year, int32_t Month, int32_t Day)	{ return setTime(CScriptTime(*this, false).setFullYear(Year, Month, Day)); }
	int32_t getUTCFullYear() const										{ return CScriptTime(*this, false).tm_year; }


	int64_t setHours(int32_t Hour)				{ tm_hour = Hour; return fix(); }
	int32_t getHours() const					{ return tm_hour; }
	int64_t setUTCHours(int32_t Hour)			{ return setTime(CScriptTime(*this, false).setHours(Hour)); }
	int32_t getUTCHours() const					{ return CScriptTime(*this, false).tm_hour; }

	int64_t setMilliseconds(int32_t Msec)		{ tm_msec = Msec; return fix(); }
	int32_t getMilliseconds() const				{ return tm_msec; }
	int64_t setUTCMilliseconds(int32_t Msec)	{ return setTime(CScriptTime(*this, false).setMilliseconds(Msec)); }
	int32_t getUTCMilliseconds() const			{ return CScriptTime(*this, false).tm_msec; }

	int64_t setMinutes(int32_t Min)				{ tm_min = Min; return fix(); }
	int32_t getMinutes() const					{ return tm_min; }
	int64_t setUTCMinutes(int32_t Min)			{ return setTime(CScriptTime(*this, false).setMinutes(Min)); }
	int32_t getUTCMinutes() const				{ return CScriptTime(*this, false).tm_min; }

	int64_t setMonth(int32_t Month)				{ tm_mon = Month; return fix(); }
	int32_t getMonth() const					{ return tm_mon; }
	int64_t setUTCMonth(int32_t Month)			{ return setTime(CScriptTime(*this, false).setMonth(Month)); }
	int32_t getUTCMonth() const					{ return CScriptTime(*this, false).tm_mon; }

	int64_t setSeconds(int32_t Sec)				{ tm_sec = Sec; return fix(); }
	int32_t getSeconds() const					{ return tm_sec; }
	int64_t setUTCSeconds(int32_t Sec)			{ return setTime(CScriptTime(*this, false).setSeconds(Sec)); }
	int32_t getUTCSeconds() const				{ return CScriptTime(*this, false).tm_sec; }

	int32_t getTimezoneOffset() const			{ return tm_offset / 60; }
	int32_t getGMTOffset() const				{ int32_t offset = std::abs(getTimezoneOffset()); return ((offset / 60) * 100 + offset % 60) * (tm_offset < 0 ? 1 : -1); }

	std::string toDateString() const;
	std::string toTimeString() const;

	std::string castToString() const;
	operator std::string() const				{ return castToString(); }
	std::string toUTCString() const				{ return CScriptTime(*this, false).castToString(); } // corresponds toGMTString()
	std::string toISOString() const;			// corresponds toJSON()



	static constexpr int32_t leapsThruEndOf(int32_t y) { return (y+4)/4 - y/100 + y/400; }
	static constexpr int32_t isLeapYear(int32_t y) { return ((!(y % 4) && (y % 100)) || !(y % 400)) ? 1 : 0; }
	static const int32_t monthLengths[2][12];
	static const int32_t firstDayOfMonth[2][13];
private:
	bool islocal;    /**/
	bool tm_isvalid;
	int32_t tm_msec;    /* milliseconds after the second - [0,999] */
	int32_t tm_sec;     /* seconds after the minute      - [0,59]  */
	int32_t tm_min;     /* minutes after the hour        - [0,59]  */
	int32_t tm_hour;    /* hours since midnight          - [0,23]  */
	int32_t tm_mday;    /* day of the month              - [1,31]  */
	int32_t tm_mon;     /* months since January          - [0,11]  */
	int32_t tm_year;    /* years since 0000 */
	int32_t tm_wday;    /* days since Sunday             - [0,6]   */
	int32_t tm_offset;  /* offset to UTC time in seconds           */
	int64_t tm_time;	/* ms after 01.01.1970 always UTC          */

	//************************************
	// Method:    fix
	// FullName:  TinyJS::CScriptTime::fix
	// Access:    private 
	// Returns:   int64_t
	// normalize tm_mon, tm_mday, tm_hour, tm_min, tm_sec, tm_msec and update tm_year and tm_wday
	// calculate tm_time from tm_year ... tm_msec
	// localtime is always tm_time - tm_offset
	//************************************
	int64_t fix();
};

const int32_t CScriptTime::monthLengths[2][12] = {
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
};
const int32_t CScriptTime::firstDayOfMonth[2][13] = {
	{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
	{0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}
};

CScriptTime::CScriptTime(bool isLocalTime/*=true*/) : islocal(isLocalTime), tm_isvalid(true) {
	setTime(0);
}
CScriptTime::CScriptTime(int64_t Time, bool isLocalTime/*=true*/) : islocal(isLocalTime), tm_isvalid(true) {
	setTime(Time);
}
int64_t CScriptTime::setTime(int64_t Time) {
	tm_time = Time;
	for (int daylightSavingCheck = 1; daylightSavingCheck >= 0; --daylightSavingCheck) {
		if (islocal) tm_time -= TIMEZONE * 1000; // if tm_islocal than tm_time is now localtime
		tm_time += 62167219200000; // tm_time now ms from 1.1.0000
		tm_mon = tm_wday = tm_hour = tm_min = tm_sec = 0;
		tm_year = 4 * int32_t(tm_time / (1461LL * 24LL * 60LL * 60LL * 1000LL));
		tm_time -= (int64_t(tm_year) * 365LL + leapsThruEndOf(tm_year - 1)) * 24LL * 60LL * 60LL * 1000LL;
		tm_mday = int32_t(tm_time / (24LL * 60 * 60 * 1000));
		tm_time -= int64_t(tm_mday++) * 24 * 60 * 60 * 1000; // convert mday to 1-based

		tm_msec = int32_t(tm_time);
		fix();
		if (tm_time == Time) break;
		// daylight saving ? 
		tm_time = Time + (Time - tm_time);
	}
	return tm_time;
}

int64_t CScriptTime::setTime( int32_t Year, int32_t Month, int32_t Day, int32_t Hour, int32_t Min, int32_t Sec, int32_t MSec ) {
	if(0 <= Year && Year<=99) Year += 1900;
	tm_msec	= MSec;
	tm_sec	= Sec;
	tm_min	= Min;
	tm_hour	= Hour;
	tm_mday	= Day;
	tm_mon	= Month;
	tm_year	= Year;
	return fix();
}
int64_t CScriptTime::UTC(int32_t Year, int32_t Month, int32_t Day/*=1*/, int32_t Hour/*=0*/, int32_t Min/*=0*/, int32_t Sec/*=0*/, int32_t MSec/*=0*/) {
	return CScriptTime(false).setTime(Year, Month, Day, Hour, Min, Sec, MSec);
}

int64_t CScriptTime::now() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}




int64_t CScriptTime::setYear( int32_t Year ) {
	if(0 <= Year && Year<=99) Year += 1900;
	tm_year = Year;
	return fix();
}
int64_t CScriptTime::setFullYear( int32_t Year ) {
	tm_year	= Year;
	return fix();
}
int64_t CScriptTime::setFullYear( int32_t Year, int32_t Month ) {
	tm_mon	= Month;
	tm_year	= Year;
	return fix();
}

int64_t CScriptTime::setFullYear( int32_t Year, int32_t Month, int32_t Day ) {
	tm_mday	= Day;
	tm_mon	= Month;
	tm_year	= Year;
	return fix();
}


int64_t CScriptTime::fix() {
	int prevent_endless_loop = 0;
recalc:
	int64_t time_of_year;
	tm_offset = 0;
	int32_t day_of_year_start;

	// Normalisierung mit festen Bereichsgrenzen
	TinyJS::normalize<1000>(tm_sec, tm_msec); // normalize milliseconds
	TinyJS::normalize<60>(tm_min, tm_sec);    // normalize seconds
	TinyJS::normalize<60>(tm_hour, tm_min);   // normalize minutes
	tm_mday -= 1;                             // convert mday to 0 - based
	TinyJS::normalize<24>(tm_mday, tm_hour);  // normalize hours
	if (tm_mday < 0) {                        // normalize days and months 
		while (tm_mday < 0) {
			tm_mon -= 1;
			TinyJS::normalize<12>(tm_year, tm_mon);   // Monate → Jahre
			tm_mday += monthLengths[isLeapYear(tm_year)][tm_mon];
		}
	} else {
		TinyJS::normalize<12>(tm_year, tm_mon);   // normalite months
		while (tm_mday >= monthLengths[isLeapYear(tm_year)][tm_mon]) {
			tm_mday -= monthLengths[isLeapYear(tm_year)][tm_mon];
			tm_mon += 1;
			TinyJS::normalize<12>(tm_year, tm_mon);   // Monate → Jahre
		}
	}
	// days = day of month
	int32_t days = tm_mday++; // set days and convert mday back to 1 - based

	// tm_time = time of day in ms
	tm_time = tm_msec + tm_sec * 1000LL + tm_min * 60LL * 1000LL + tm_hour * 60LL * 60LL * 1000LL;

	// days = day of year
	days += firstDayOfMonth[isLeapYear(tm_year)][tm_mon % 12];

	time_of_year = tm_time + int64_t(days) * 24 * 60 * 60 * 1000;

	// days = days since 1.1.0000
	days += (day_of_year_start = tm_year * 365 + leapsThruEndOf(tm_year - 1));

	// tm_time = ms since 1.1.0000
	tm_time += int64_t(days) * 24 * 60 * 60 * 1000 - 62167219200000;

	tm_wday = (days+6) % 7;

	if(islocal) {
		time_t time4dst;
		struct tm tm;  // Struktur für die lokale Zeit

		if ((tm_time + TIMEZONE * 1000LL) < 0 || (tm_time + TIMEZONE * 1000LL) > 2145916800000) {
			static const time_t yearStartingWith[2][7] = {
				{252460800 /*1978*/, 94694400 /*1973*/, 126230400 /*1974*/, 157766400 /*1975*/, 347155200 /*1981*/, 31536000 /*1971*/, 220924800 /*1977*/},
				{441763200 /*1984*/, 820454400 /*1996*/, 315532800 /*1980*/, 694224000 /*1992*/, 189302400 /*1976*/, 567993600 /*1988*/, 63072000 /*1972*/}
			};
			//int32_t days = tm_year*365 + leapsThruEndOf(tm_year-1);
			time4dst = yearStartingWith[isLeapYear(tm_year)][(day_of_year_start + 6) % 7] + time_of_year / 1000;

		} else
			time4dst = time_t(tm_time/1000);


		tm_offset = TIMEZONE;
		time4dst += TIMEZONE; // Zeit mit der Standard-Zeitzonenverschiebung anpassen

		localtime_s(&tm, &time4dst); // Zeit in die lokale Zeit (inkl. Sommerzeit) umwandeln

		if (tm.tm_isdst) {  // Prüfen, ob Sommerzeit aktiv ist (tm_isdst == 1)
			tm.tm_isdst = 0; // Setzen von tm_isdst auf 0, um die Standardzeit zu simulieren

			// Berechnung der tatsächlichen Sommerzeitverschiebung
			// mktime() rechnet tm zurück in time_t unter Berücksichtigung der Standardzeit.
			// Die Differenz zwischen `time4dst` und `mktime(&tm)` ergibt die DST-Korrektur.
			int32_t tm_tz_diff = int32_t(time4dst - mktime(&tm));
			tm_offset += tm_tz_diff;

			// Die Zeit wird erneut angepasst, jetzt mit der DST-Korrektur
			time4dst += tm_tz_diff;
			localtime_s(&tm, &time4dst); // Zeit erneut in die lokale Zeit umwandeln

			// Falls durch die Korrektur tm_isdst plötzlich 0 wurde (also keine Sommerzeit mehr aktiv ist)
			// UND es der erste Versuch ist, wird eine Rekalkulation durchgeführt.
			if (!tm.tm_isdst && prevent_endless_loop++ == 0) {
				tm_sec -= tm_tz_diff; // Ursprüngliche Zeitzonenverschiebung wiederherstellen
				goto recalc; // Springt zu einem Label "recalc" (welches im Code nicht enthalten ist)
			}
		}
	}
	tm_isvalid = true;
	return tm_time += tm_offset * 1000;
}

#endif /* __cpp_lib_chrono >= 201907L */


static const char *day_names[] = { "Sun","Mon","Tue","Wed","Thu","Fri","Sat" };
static const char *month_names[] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };
std::string CScriptTime::toDateString() const {
	char buffer[100];
	sprintf_s(buffer, "%s %s %02d %04d", day_names[getDay()], month_names[getMonth()], getDate(), getFullYear());
	return buffer;
}
std::string CScriptTime::toTimeString() const {
	char buffer[100];
	sprintf_s(buffer, "%02d:%02d:%02d GMT%+05d", getHours(), getMinutes(), getSeconds(), getGMTOffset());
	return buffer;
}
std::string CScriptTime::castToString() const {
	char buffer[100];
	if (islocal) {
		sprintf_s(buffer, "%s %s %02d %04d %02d:%02d:%02d GMT%+05d", day_names[getDay()], month_names[getMonth()], getDate(), getFullYear(), getHours(), getMinutes(), getSeconds(), getGMTOffset());
	} else
		sprintf_s(buffer, "%s, %02d %s %04d %02d:%02d:%02d GMT", day_names[getDay()], getDate(), month_names[getMonth()], getFullYear(), getHours(), getMinutes(), getSeconds());
	return buffer;
}
std::string CScriptTime::toISOString() const {
	char buffer[100];
//	CScriptTime utc(*this, false);
	sprintf_s(buffer, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", getUTCFullYear(), getUTCMonth() + 1, getUTCDate(), getUTCHours(), getUTCMinutes(), getUTCSeconds(), getUTCMilliseconds());
	return buffer;
}

bool CScriptTime::ParseISODate(const char *s, int64_t *result) {
	char *end;
	int32_t yearMul = 1, tzMul = 1;
	int32_t year = 1970, month = 1, day = 1, hour = 0, min = 0, sec = 0, msec = 0;
	int32_t tzHour = 0, tzMin = 0;
	bool isLocalTime = false;

	if (*s == '+' || *s == '-') {
		if (*s++ == '-') yearMul = -1;
		if ('0' > *s || *s > '9') return false;
		year = strtol(s, &end, 10); if (end - s != 6) return false; else s = end;
	} else if (*s != 'T') {
		if ('0' > *s || *s > '9') return false;
		year = strtol(s, &end, 10); if (end - s != 4) return false; else s = end;
	}
	if (*s == '-') {
		++s;
		if ('0' > *s || *s > '9') return false;
		month = strtol(s, &end, 10); if (end - s != 2) return false; else s = end;
		if (*s == '-') {
			++s;
			if ('0' > *s || *s > '9') return false;
			day = strtol(s, &end, 10); if (end - s != 2) return false; else s = end;
		}
	}
	if (*s && *s++ == 'T') {
		if ('0' > *s || *s > '9') return false;
		hour = strtol(s, &end, 10); if (end - s != 2) return false; else s = end;
		if (*s == 0 || *s++ != ':') return false;
		if ('0' > *s || *s > '9') return false;
		min = strtol(s, &end, 10); if (end - s != 2) return false; else s = end;
		if (*s == ':') {
			++s;
			if ('0' > *s || *s > '9') return false;
			sec = strtol(s, &end, 10); if (end - s != 2) return false; else s = end;
			if (*s == '.') {
				if ('0' > s[1] || s[1] > '9') return false;
				msec = int32_t(strtod(s, (char **)&s) * 1000);
			}
		}
		if (*s == 'Z')
			++s;
		else if (*s == '-' || *s == '+') {
			if (*s++ == '-') tzMul = -1;
			if ('0' > *s || *s > '9') return false;
			tzHour = (*s++ - '0') * 10;
			if ('0' > *s || *s > '9') return false;
			tzHour += *s++ - '0';
			if (*s == ':') ++s;
			if ('0' > *s || *s > '9') return false;
			tzMin = (*s++ - '0') * 10;
			if ('0' > *s || *s > '9') return false;
			tzMin += *s++ - '0';
		} else
			isLocalTime = true;
	}
	if (*s || year > 275943 // ceil(1e8/365) + 1970
		|| (month == 0 || month > 12)
		|| (day == 0 || day > monthLengths[isLeapYear(year)][month])
		|| hour > 24 || ((hour == 24) && (min > 0 || sec > 0))
		|| min > 59 || sec > 59 || tzHour > 23 || tzMin > 59) return false;
	month -= 1; /* convert month to 0-based */
	*result = CScriptTime(isLocalTime).setTime(yearMul * year, month, day, hour, min, sec, msec);
	if (!isLocalTime)
		*result -= tzMul * ((tzHour * 60 + tzMin) * 60 * 1000);
	return true;
}
bool CScriptTime::ParseDate(const char *s, int64_t *result) {
	static struct { const char *key; int32_t value; } keywords[] = {
		{ "AM", -1}, { "PM", -2},
		{ "MONDAY", 0}, { "TUESDAY", 0}, { "WEDNESAY", 0}, { "THURSDAY", 0}, { "FRIDAY", 0}, { "SATURDAY", 0}, { "SUNDAY", 0},
		{ "JANUARY", 1}, { "FEBRUARY", 2}, { "MARCH", 3}, { "APRIL", 4}, { "MAY", 5}, { "JUNE", 6},
		{ "JULY", 7}, { "AUGUST", 8}, { "SEPTEMBER", 9}, { "OCTOBER", 10}, { "NOVEMBER", 11}, { "DECEMBER", 12},
		{ "GMT", 10000 + 0}, { "UT", 10000 + 0}, { "UTC", 10000 + 0},
		{ "CEST", 10000 - 2 * 60}, { "CET", 10000 - 1 * 60},
		{ "EST", 10000 + 5 * 60}, { "EDT", 10000 + 4 * 60},
		{ "CST", 10000 + 6 * 60}, { "CDT", 10000 + 5 * 60},
		{ "MST", 10000 + 7 * 60}, { "MDT", 10000 + 6 * 60},
		{ "PST", 10000 + 8 * 60}, { "PDT", 10000 + 7 * 60},
		{ 0, 0}
	};

	if (ParseISODate(s, result))
		return true;

	int32_t year = -1, mon = -1, mday = -1, hour = -1, min = -1, sec = -1, tzOffset = -1;
	int32_t prevc = 0;
	bool seenPlusMinus = false;
	bool seenMonthName = false;
	while (*s) {
		int c = *s++;
		if (c <= ' ' || c == ',' || c == '-') {
			if (c == '-' && '0' <= *s && *s <= '9')
				prevc = c;
			continue;
		}
		if (c == '(') { /* comments) */
			int depth = 1;
			while (*s) {
				c = *s++;
				if (c == '(') {
					depth++;
				} else if (c == ')') {
					if (--depth <= 0)
						break;
				}
			}
			continue;
		}
		if ('0' <= c && c <= '9') {
			int n = c - '0';
			while (*s && '0' <= (c = *s++) && c <= '9') {
				n = n * 10 + c - '0';
			}

			/*
			* Allow TZA before the year, so 'Wed Nov 05 21:49:11 GMT-0800 1997'
			* works.
			*
			* Uses of seenPlusMinus allow ':' in TZA, so Java no-timezone style
			* of GMT+4:30 works.
			*/

			if ((prevc == '+' || prevc == '-')/*  && year>=0 */) {
				/* Make ':' case below change tzOffset. */
				seenPlusMinus = true;

				/* offset */
				if (n < 24)
					n = n * 60; /* EG. "GMT-3" */
				else
					n = n % 100 + n / 100 * 60; /* eg "GMT-0430" */

				if (prevc == '+')       /* plus means east of GMT */
					n = -n;

				if (tzOffset != 0 && tzOffset != -1)
					return false;

				tzOffset = n;
			} else if (prevc == '/' && mon >= 0 && mday >= 0 && year < 0) {
				if (c <= ' ' || c == ',' || c == '/' || *s == 0)
					year = n;
				else
					return false;
			} else if (c == ':') {
				if (hour < 0)
					hour = /*byte*/ n;
				else if (min < 0)
					min = /*byte*/ n;
				else
					return false;
			} else if (c == '/') {
				/*
				* Until it is determined that mon is the actual month, keep
				* it as 1-based rather than 0-based.
				*/
				if (mon < 0)
					mon = /*byte*/ n;
				else if (mday < 0)
					mday = /*byte*/ n;
				else
					return false;
			} else if (*s && c != ',' && c > ' ' && c != '-' && c != '(') {
				return false;
			} else if (seenPlusMinus && n < 60) {  /* handle GMT-3:30 */
				if (tzOffset < 0)
					tzOffset -= n;
				else
					tzOffset += n;
			} else if (hour >= 0 && min < 0) {
				min = /*byte*/ n;
			} else if (prevc == ':' && min >= 0 && sec < 0) {
				sec = /*byte*/ n;
			} else if (mon < 0) {
				mon = /*byte*/n;
			} else if (mon >= 0 && mday < 0) {
				mday = /*byte*/ n;
			} else if (mon >= 0 && mday >= 0 && year < 0) {
				year = n;
			} else {
				return false;
			}
			prevc = 0;
		} else if (c == '/' || c == ':' || c == '+' || c == '-') {
			prevc = c;
		} else {
			const char *st = s - 1;
			int k;
			while (*s) {
				if (!(('A' <= *s && *s <= 'Z') || ('a' <= *s && *s <= 'z')))
					break;
				++s;
			}

			if (s <= st + 1)
				return false;
			for (k = 0; keywords[k].key; ++k) {
				const char *cmp_l = st, *cmp_r = keywords[k].key;
				while (cmp_l < s && toupper(*cmp_l++) == *cmp_r++);
				if (cmp_l == s) {
					int32_t action = keywords[k].value;
					if (action != 0) {
						if (action < 0) {
							/*
							* AM/PM. Count 12:30 AM as 00:30, 12:30 PM as
							* 12:30, instead of blindly adding 12 if PM.
							*/
							if (hour > 12 || hour < 0)
								return false;

							if (action == -1 && hour == 12) /* am */
								hour = 0;
							else if (action == -2 && hour != 12) /* pm */
								hour += 12;
						} else if (action <= 13) { /* month! */
							/*
							* Adjust mon to be 1-based until the final values
							* for mon, mday and year are adjusted below.
							*/
							if (seenMonthName)
								return false;

							seenMonthName = true;

							if (mon < 0) {
								mon = action;
							} else if (mday < 0) {
								mday = mon;
								mon = action;
							} else if (year < 0) {
								year = mon;
								mon = action;
							} else {
								return false;
							}
						} else {
							tzOffset = action - 10000;
						}
					}
					break;
				}
			}

			if (keywords[k].key == 0)
				return false;

			prevc = 0;
		}
	}

	if (year < 0 || mon < 0 || mday < 0)
		return false;

	/*
	 * Case 1. The input string contains an English month name.
	 *         The form of the string can be month f l, or f month l, or
	 *         f l month which each evaluate to the same date.
	 *         If f and l are both greater than or equal to 70, or
	 *         both less than 70, the date is invalid.
	 *         The year is taken to be the greater of the values f, l.
	 *         If the year is greater than or equal to 70 and less than 100,
	 *         it is considered to be the number of years after 1900.
	 * Case 2. The input string is of the form "f/m/l" where f, m and l are
	 *         integers, e.g. 7/16/45.
	 *         Adjust the mon, mday and year values to achieve 100% MSIE
	 *         compatibility.
	 *         a. If 0 <= f < 70, f/m/l is interpreted as month/day/year.
	 *            i.  If year < 100, it is the number of years after 1900
	 *            ii. If year >= 100, it is the number of years after 0.
	 *         b. If 70 <= f < 100
	 *            i.  If m < 70, f/m/l is interpreted as
	 *                year/month/day where year is the number of years after
	 *                1900.
	 *            ii. If m >= 70, the date is invalid.
	 *         c. If f >= 100
	 *            i.  If m < 70, f/m/l is interpreted as
	 *                year/month/day where year is the number of years after 0.
	 *            ii. If m >= 70, the date is invalid.
	 */
	if (seenMonthName) {
		if ((mday >= 70 && year >= 70) || (mday < 70 && year < 70))
			return false;

		if (mday > year) std::swap(mday, year);
		if (year >= 70 && year < 100) year += 1900;

	} else if (mon < 70) { /* (a) month/day/year */
		if (year < 100) {
			year += 1900;
		}
	} else if (mon < 100) { /* (b) year/month/day */
		if (mday < 70) {
			std::swap(year, mon);
			year += 1900;
			std::swap(mon, mday);
		} else {
			return false;
		}
	} else { /* (c) year/month/day */
		if (mday < 70) {
			std::swap(year, mon);
			std::swap(mon, mday);
		} else {
			return false;
		}
	}

	mon -= 1; /* convert month to 0-based */
	if (sec < 0)
		sec = 0;
	if (min < 0)
		min = 0;
	if (hour < 0)
		hour = 0;

	*result = CScriptTime(tzOffset == -1).setTime(year, mon, mday, hour, min, sec);
	if (tzOffset != -1)
		*result += tzOffset * 60 * 1000;
	//    double msec = date_msecFromDate(year, mon, mday, hour, min, sec, 0);

	//    if (tzOffset == -1) /* no time zone specified, have to use local */
	//       msec = UTC(msec, dtInfo);
	//    else
	//        msec += tzOffset * msPerMinute;

	//    *result = msec;
	return true;
}


//////////////////////////////////////////////////////////////////////////
/// CScriptVarDate
//////////////////////////////////////////////////////////////////////////

define_ScriptVarPtr_Type(Date);
class CScriptVarDate : public CScriptVarObject, public CScriptTime {
protected:
	CScriptVarDate(CTinyJS *Context);
	// custom RTTI
	static constexpr uint32_t classHash = fnv1aHash("CScriptVarDate");
	virtual bool isDerivedFrom(uint32_t parentHash) const { return classHash == parentHash || CScriptVarObject::isDerivedFrom(parentHash); }
//	template <typename T> friend T* CScriptVarDynamicCast(CScriptVar* basePtr);
	template <typename T> friend std::shared_ptr<T> CScriptVarDynamicCast(const CScriptVarPtr& basePtr);
public:
	virtual ~CScriptVarDate();
	virtual bool isDate(); // { return true; }
	virtual CScriptVarPrimitivePtr toPrimitive(CScriptResult &execute);

	virtual CScriptVarPtr toString_CallBack(CScriptResult &execute, int radix=0);
	virtual CScriptVarPtr valueOf_CallBack();

	friend inline define_newScriptVar_NamedFnc(Date, CTinyJS *Context);
private:
};
inline define_newScriptVar_NamedFnc(Date, CTinyJS* Context) { return CScriptVarPtr(new CScriptVarDate(Context)); }

//////////////////////////////////////////////////////////////////////////
// CScriptVarDate
//////////////////////////////////////////////////////////////////////////


CScriptVarDate::CScriptVarDate(CTinyJS *Context) : CScriptVarObject(Context, Context->getRoot()->findChildByPath("Date.prototype")) {

}

CScriptVarDate::~CScriptVarDate() {}
bool CScriptVarDate::isDate() { return true; }

CScriptVarPrimitivePtr CScriptVarDate::toPrimitive(CScriptResult &execute) {
	// for the Date Object toPrimitive without hint is hintString instead hintNumber
	return toPrimitive_hintString(execute);
}
CScriptVarPtr CScriptVarDate::toString_CallBack( CScriptResult &execute, int radix/*=0*/ ) {
	if(isValid()) {
		return newScriptVar(castToString());
	} else
		return newScriptVar("Invalid Date");
}
CScriptVarPtr CScriptVarDate::valueOf_CallBack() {
	if(isValid())
		return newScriptVar((double)getTime());
	else
		return constScriptVar(NaN);
}

static void scDate(const CFunctionsScopePtr &c, void *data) {
	if (!CScriptVarFunctionPtr(c->findChild("new.target"))) {
		c->setReturnVar(c->newScriptVar(CScriptTime(CScriptTime::now()).castToString()));
		return;
	}
	CScriptVarDatePtr returnVar = TinyJS::newScriptVarDate(c->getContext());
	c->setReturnVar(returnVar);
	int ArgumentsLength = c->getArgumentsLength();
	if(ArgumentsLength == 1) {
		CScriptVarPtr arg0 = c->getArgument(0);
		CNumber v = arg0->toNumber().floor();
		if(v.isFinite()) {
			returnVar->setTime((int64_t)v.toDouble()); // TODO check range
		} else {
			int64_t result;
			if(CScriptVarDate::Parse(arg0->toString(), &result))
				returnVar->setTime(result);
			else
				returnVar->setInvalide();
		}
	} else if(ArgumentsLength > 1) {
		int32_t args[7] = { 1970, 0, 1, 0, 0, 0, 0};
		for(int i=0; i<std::min(ArgumentsLength,7); ++i) {
			CNumber v = c->getArgument(i)->toNumber().floor();
			if(v.isInt32() || v.isNegativeZero())
				args[i] = v.toInt32();
			else {
				returnVar->setInvalide();
				return;
			}
		}
		returnVar->setTime(args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
	} else
		returnVar->setTime(CScriptTime::now());
}
static void scDate_UTC(const CFunctionsScopePtr &c, void *data) {
	int ArgumentsLength = c->getArgumentsLength();
	int32_t args[7] = { 1900, 0, 1, 0, 0, 0, 0};
	for(int i=0; i<std::min(ArgumentsLength,7); ++i) {
		CNumber v = c->getArgument(i)->toNumber().floor();
		if(v.isInt32() || v.isNegativeZero())
			args[i] = v.toInt32();
		else {
			c->setReturnVar(c->constScriptVar(NaN));
			return;
		}
	}
	c->setReturnVar(c->newScriptVar((double)CScriptTime::UTC(args[0], args[1], args[2], args[3], args[4], args[5], args[6])));

}
static void scDate_now(const CFunctionsScopePtr &c, void *data) {
	c->setReturnVar(c->newScriptVar((double)CScriptTime::now()));
}
static void scDate_parse(const CFunctionsScopePtr &c, void *data) {
	int64_t result;
	if(CScriptTime::Parse(c->getArgument(0)->toString(), &result))
		c->setReturnVar(c->newScriptVar((double)result));
	else
		c->setReturnVar(c->constScriptVar(NaN));
}
static inline void scDateThrowTypeError(const CFunctionsScopePtr &c, const std::string &Fnc) {
	c->throwError(TypeError, Fnc + " method called on incompatible Object");
}
#define DATE_PROTOTYPE_GET(FNC) \
	static void scDate_prototype_##FNC(const CFunctionsScopePtr &c, void *data) {	\
	CScriptVarDatePtr Date(c->getArgument("this"));											\
	if(!Date) scDateThrowTypeError(c, #FNC);													\
	if(Date->isValid())																				\
		c->setReturnVar(c->newScriptVar(Date->FNC()));										\
	else																									\
		c->setReturnVar(c->constScriptVar(NaN));												\
}

#define DATE_PROTOTYPE_SET(FNC) \
	static void scDate_prototype_##FNC(const CFunctionsScopePtr &c, void *data) {	\
	CScriptVarDatePtr Date(c->getArgument("this"));											\
	if(!Date) scDateThrowTypeError(c, #FNC);													\
	CNumber v = c->getArgument(0)->toNumber().floor();										\
	if(v.isInt32() || v.isNegativeZero()) {													\
		double value = (double)Date->FNC(v.toInt32());										\
		if(Date->isValid()) {																		\
			c->setReturnVar(c->newScriptVar(value));											\
			return;																						\
		}																									\
	}																										\
	Date->setInvalide();																				\
	c->setReturnVar(c->constScriptVar(NaN));													\
}

DATE_PROTOTYPE_SET(setDate)
DATE_PROTOTYPE_GET(getDate)
DATE_PROTOTYPE_SET(setUTCDate)
DATE_PROTOTYPE_GET(getUTCDate)
DATE_PROTOTYPE_GET(getDay)
DATE_PROTOTYPE_GET(getUTCDay)

static void scDate_prototype_setFullYear(const CFunctionsScopePtr &c, void *data) {
	CScriptVarDatePtr Date(c->getArgument("this"));
	if(!Date) scDateThrowTypeError(c, "setFullYear");
	int ArgumentsLength = c->getArgumentsLength();
	int32_t args[3] = { 0, Date->getMonth(), Date->getDate() };
	double value;
	for(int i=0; i<std::max(std::min(ArgumentsLength,3),1); ++i) {
		CNumber v = c->getArgument(i)->toNumber().floor();
		if(v.isInt32() || v.isNegativeZero())
			args[i] = v.toInt32();
		else
			goto invalid;
	}
	value = (double)Date->setFullYear(args[0], args[1], args[2]);
	if(Date->isValid()) {
		c->setReturnVar(c->newScriptVar(value));
		return;
	}
invalid:
	Date->setInvalide();
	c->setReturnVar(c->constScriptVar(NaN));
}

DATE_PROTOTYPE_GET(getFullYear)

static void scDate_prototype_setUTCFullYear(const CFunctionsScopePtr &c, void *data) {
	CScriptVarDatePtr Date(c->getArgument("this"));
	if(!Date) scDateThrowTypeError(c, "setUTCFullYear");
	int ArgumentsLength = c->getArgumentsLength();
	int32_t args[3] = { 0, Date->getUTCMonth(), Date->getUTCDate() };
	double value;
	for(int i=0; i<std::max(std::min(ArgumentsLength,3),1); ++i) {
		CNumber v = c->getArgument(i)->toNumber().floor();
		if(v.isInt32() || v.isNegativeZero())
			args[i] = v.toInt32();
		else
			goto invalid;
	}
	value = (double)Date->setUTCFullYear(args[0], args[1], args[2]);
	if(Date->isValid()) {
		c->setReturnVar(c->newScriptVar(value));
		return;
	}
invalid:
	Date->setInvalide();
	c->setReturnVar(c->constScriptVar(NaN));
}

DATE_PROTOTYPE_GET(getUTCFullYear)
DATE_PROTOTYPE_SET(setYear)
DATE_PROTOTYPE_GET(getYear)
DATE_PROTOTYPE_SET(setHours)
DATE_PROTOTYPE_GET(getHours)
DATE_PROTOTYPE_SET(setUTCHours)
DATE_PROTOTYPE_GET(getUTCHours)
DATE_PROTOTYPE_SET(setMilliseconds)
DATE_PROTOTYPE_GET(getMilliseconds)
DATE_PROTOTYPE_SET(setUTCMilliseconds)
DATE_PROTOTYPE_GET(getUTCMilliseconds)
DATE_PROTOTYPE_SET(setMinutes)
DATE_PROTOTYPE_GET(getMinutes)
DATE_PROTOTYPE_SET(setUTCMinutes)
DATE_PROTOTYPE_GET(getUTCMinutes)
DATE_PROTOTYPE_SET(setMonth)
DATE_PROTOTYPE_GET(getMonth)
DATE_PROTOTYPE_SET(setUTCMonth)
DATE_PROTOTYPE_GET(getUTCMonth)
DATE_PROTOTYPE_SET(setSeconds)
DATE_PROTOTYPE_GET(getSeconds)
DATE_PROTOTYPE_SET(setUTCSeconds)
DATE_PROTOTYPE_GET(getUTCSeconds)
DATE_PROTOTYPE_SET(setTime)
DATE_PROTOTYPE_GET(getTime)
DATE_PROTOTYPE_GET(getTimezoneOffset)

// ----------------------------------------------- Register Functions
extern "C" void _registerDateFunctions(CTinyJS *tinyJS) {
	CScriptVarPtr var = tinyJS->addNative("function Date(year, month, day, hour, minute, second, millisecond)", scDate, 0, SCRIPTVARLINK_CONSTANT);
	CScriptVarPtr datePrototype = var->findChild("prototype");
	datePrototype->addChild("valueOf", tinyJS->objectPrototype_valueOf, SCRIPTVARLINK_BUILDINDEFAULT);
	datePrototype->addChild("toString", tinyJS->objectPrototype_toString, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Date.UTC()", scDate_UTC, 0, SCRIPTVARLINK_CONSTANT);
	tinyJS->addNative("function Date.now()", scDate_now, 0, SCRIPTVARLINK_CONSTANT);
	tinyJS->addNative("function Date.parse()", scDate_parse, 0, SCRIPTVARLINK_CONSTANT);
#define DATE_PROTOTYPE_NATIVE(FNC) tinyJS->addNative("function Date.prototype."#FNC"()", scDate_prototype_##FNC, 0, SCRIPTVARLINK_CONSTANT)
	DATE_PROTOTYPE_NATIVE(setDate);
	DATE_PROTOTYPE_NATIVE(getDate);
	DATE_PROTOTYPE_NATIVE(setUTCDate);
	DATE_PROTOTYPE_NATIVE(getUTCDate);
	DATE_PROTOTYPE_NATIVE(getDay);
	DATE_PROTOTYPE_NATIVE(getUTCDay);
	DATE_PROTOTYPE_NATIVE(setFullYear);
	DATE_PROTOTYPE_NATIVE(getFullYear);
	DATE_PROTOTYPE_NATIVE(setUTCFullYear);
	DATE_PROTOTYPE_NATIVE(getUTCFullYear);
	DATE_PROTOTYPE_NATIVE(setYear);
	DATE_PROTOTYPE_NATIVE(getYear);
	DATE_PROTOTYPE_NATIVE(setHours);
	DATE_PROTOTYPE_NATIVE(getHours);
	DATE_PROTOTYPE_NATIVE(setUTCHours);
	DATE_PROTOTYPE_NATIVE(getUTCHours);
	DATE_PROTOTYPE_NATIVE(setMilliseconds);
	DATE_PROTOTYPE_NATIVE(getMilliseconds);
	DATE_PROTOTYPE_NATIVE(setUTCMilliseconds);
	DATE_PROTOTYPE_NATIVE(getUTCMilliseconds);
	DATE_PROTOTYPE_NATIVE(setMinutes);
	DATE_PROTOTYPE_NATIVE(getMinutes);
	DATE_PROTOTYPE_NATIVE(setUTCMinutes);
	DATE_PROTOTYPE_NATIVE(getUTCMinutes);
	DATE_PROTOTYPE_NATIVE(setMonth);
	DATE_PROTOTYPE_NATIVE(getMonth);
	DATE_PROTOTYPE_NATIVE(setUTCMonth);
	DATE_PROTOTYPE_NATIVE(getUTCMonth);
	DATE_PROTOTYPE_NATIVE(setSeconds);
	DATE_PROTOTYPE_NATIVE(getSeconds);
	DATE_PROTOTYPE_NATIVE(setUTCSeconds);
	DATE_PROTOTYPE_NATIVE(getUTCSeconds);
	DATE_PROTOTYPE_NATIVE(setTime);
	DATE_PROTOTYPE_NATIVE(getTime);
	DATE_PROTOTYPE_NATIVE(getTimezoneOffset);

}

} /* namespace TinyJS */
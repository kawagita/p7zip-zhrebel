// Windows/TimeUtils.cpp

#include "StdAfx.h"

#include "Defs.h"
#include "TimeUtils.h"

namespace NWindows {
namespace NTime {

static const UInt32 kNumTimeQuantumsInSecond = 10000000;
static const UInt32 kFileTimeStartYear = 1601;
static const UInt32 kDosTimeStartYear = 1980;
static const UInt32 kUnixTimeStartYear = 1970;
static const UInt64 kUnixTimeOffset =
    (UInt64)60 * 60 * 24 * (89 + 365 * (kUnixTimeStartYear - kFileTimeStartYear));
static const UInt64 kNumSecondsInFileTime = (UInt64)(Int64)-1 / kNumTimeQuantumsInSecond;

bool DosTimeToFileTime(UInt32 dosTime, FILETIME &ft) throw()
{
  #if defined(_WIN32) && !defined(UNDER_CE)
  return BOOLToBool(::DosDateTimeToFileTime((UInt16)(dosTime >> 16), (UInt16)(dosTime & 0xFFFF), &ft));
  #else
  ft.dwLowDateTime = 0;
  ft.dwHighDateTime = 0;
  UInt64 res;
  if (!GetSecondsSince1601(kDosTimeStartYear + (dosTime >> 25), (dosTime >> 21) & 0xF, (dosTime >> 16) & 0x1F,
      (dosTime >> 11) & 0x1F, (dosTime >> 5) & 0x3F, (dosTime & 0x1F) * 2, res))
    return false;
  res *= kNumTimeQuantumsInSecond;
  ft.dwLowDateTime = (UInt32)res;
  ft.dwHighDateTime = (UInt32)(res >> 32);
  return true;
  #endif
}

static const UInt32 kHighDosTime = 0xFF9FBF7D;
static const UInt32 kLowDosTime = 0x210000;

#define PERIOD_4 (4 * 365 + 1)
#define PERIOD_100 (PERIOD_4 * 25 - 1)
#define PERIOD_400 (PERIOD_100 * 4 + 1)

bool FileTimeToDosTime(const FILETIME &ft, UInt32 &dosTime) throw()
{
  #if defined(_WIN32) && !defined(UNDER_CE)

  WORD datePart, timePart;
  if (!::FileTimeToDosDateTime(&ft, &datePart, &timePart))
  {
    dosTime = (ft.dwHighDateTime >= 0x01C00000) ? kHighDosTime : kLowDosTime;
    return false;
  }
  dosTime = (((UInt32)datePart) << 16) + timePart;

  #else

  unsigned year, mon, day, hour, min, sec;
  UInt64 v64 = ft.dwLowDateTime | ((UInt64)ft.dwHighDateTime << 32);
  Byte ms[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  unsigned temp;
  UInt32 v;
  v64 += (kNumTimeQuantumsInSecond * 2 - 1);
  v64 /= kNumTimeQuantumsInSecond;
  sec = (unsigned)(v64 % 60);
  v64 /= 60;
  min = (unsigned)(v64 % 60);
  v64 /= 60;
  hour = (unsigned)(v64 % 24);
  v64 /= 24;

  v = (UInt32)v64;

  year = (unsigned)(kFileTimeStartYear + v / PERIOD_400 * 400);
  v %= PERIOD_400;

  temp = (unsigned)(v / PERIOD_100);
  if (temp == 4)
    temp = 3;
  year += temp * 100;
  v -= temp * PERIOD_100;

  temp = v / PERIOD_4;
  if (temp == 25)
    temp = 24;
  year += temp * 4;
  v -= temp * PERIOD_4;

  temp = v / 365;
  if (temp == 4)
    temp = 3;
  year += temp;
  v -= temp * 365;

  if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
    ms[1] = 29;
  for (mon = 1; mon <= 12; mon++)
  {
    unsigned s = ms[mon - 1];
    if (v < s)
      break;
    v -= s;
  }
  day = (unsigned)v + 1;

  dosTime = kLowDosTime;
  if (year < kDosTimeStartYear)
    return false;
  year -= kDosTimeStartYear;
  dosTime = kHighDosTime;
  if (year >= 128)
    return false;
  dosTime = (year << 25) | (mon << 21) | (day << 16) | (hour << 11) | (min << 5) | (sec >> 1);
  #endif
  return true;
}

static void UnixSecondsToFileTime(UInt32 sec, UInt32 nsec, FILETIME &ft)
{
  UInt64 v = (kUnixTimeOffset + (UInt64)sec) * kNumTimeQuantumsInSecond + (nsec / 100);
  ft.dwLowDateTime = (DWORD)v;
  ft.dwHighDateTime = (DWORD)(v >> 32);
}

void UnixTimeToFileTime(UInt32 unixTime, FILETIME &ft) throw()
{
  UnixSecondsToFileTime(unixTime, 0, ft);
}

bool UnixTime64ToFileTime(Int64 unixTime, FILETIME &ft) throw()
{
  if (unixTime > kNumSecondsInFileTime - kUnixTimeOffset)
  {
    ft.dwLowDateTime = ft.dwHighDateTime = (UInt32)(Int32)-1;
    return false;
  }
  Int64 v = (Int64)kUnixTimeOffset + unixTime;
  if (v < 0)
  {
    ft.dwLowDateTime = ft.dwHighDateTime = 0;
    return false;
  }
  UInt64 v2 = (UInt64)v * kNumTimeQuantumsInSecond;
  ft.dwLowDateTime = (DWORD)v2;
  ft.dwHighDateTime = (DWORD)(v2 >> 32);
  return true;
}

Int64 FileTimeToUnixTime64(const FILETIME &ft) throw()
{
  UInt64 winTime = (((UInt64)ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
  return (Int64)(winTime / kNumTimeQuantumsInSecond) - kUnixTimeOffset;
}

static bool FileTimeToUnixSeconds(const FILETIME &ft, UInt32 *sec, UInt32 *nsec)
{
  UInt64 winTime = (((UInt64)ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
  UInt64 seconds = winTime / kNumTimeQuantumsInSecond;
  if (seconds < kUnixTimeOffset)
  {
    if (sec)
      *sec = 0;
    if (nsec)
      *nsec = 0xFFFFFFFF;
    return false;
  }
  seconds -= kUnixTimeOffset;
  if (seconds > 0xFFFFFFFF)
  {
    if (sec)
      *sec = 0xFFFFFFFF;
    if (nsec)
      *nsec = 0xFFFFFFFF;
    return false;
  }
  if (sec)
    *sec = (UInt32)seconds;
  if (nsec)
    *nsec = (UInt32)((winTime % kNumTimeQuantumsInSecond) * 100);
  return true;
}

bool FileTimeToUnixTime(const FILETIME &ft, UInt32 &unixTime) throw()
{
  return FileTimeToUnixSeconds(ft, &unixTime, NULL);
}

bool GetSecondsSince1601(unsigned year, unsigned month, unsigned day,
  unsigned hour, unsigned min, unsigned sec, UInt64 &resSeconds) throw()
{
  resSeconds = 0;
  if (year < kFileTimeStartYear || year >= 10000 || month < 1 || month > 12 ||
      day < 1 || day > 31 || hour > 23 || min > 59 || sec > 59)
    return false;
  UInt32 numYears = year - kFileTimeStartYear;
  UInt32 numDays = numYears * 365 + numYears / 4 - numYears / 100 + numYears / 400;
  Byte ms[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
    ms[1] = 29;
  month--;
  for (unsigned i = 0; i < month; i++)
    numDays += ms[i];
  numDays += day - 1;
  resSeconds = ((UInt64)(numDays * 24 + hour) * 60 + min) * 60 + sec;
  return true;
}

bool GetFileTimeSince1601(unsigned year, unsigned month, unsigned day,
  unsigned hour, unsigned min, unsigned sec, unsigned _100ns, FILETIME &ft) throw()
{
  UInt64 res;
  if (!GetSecondsSince1601(year, month, day, hour, min, sec, res))
  {
    ft.dwLowDateTime = ft.dwHighDateTime = 0;
    return false;
  }
  res = res * kNumTimeQuantumsInSecond + _100ns;
  ft.dwLowDateTime = (UInt32)res;
  ft.dwHighDateTime = (UInt32)(res >> 32);
  return true;
}

void GetCurUtcFileTime(FILETIME &ft) throw()
{
  // Both variants provide same low resolution on WinXP: about 15 ms.
  // But GetSystemTimeAsFileTime is much faster.

  #ifdef UNDER_CE
  SYSTEMTIME st;
  GetSystemTime(&st);
  SystemTimeToFileTime(&st, &ft);
  #else
  GetSystemTimeAsFileTime(&ft);
  #endif
}

void AddOffsetSecondsToFileTime(Int32 offset, FILETIME &ft) throw()
{
  if (offset != 0)
  {
    UInt64 winTime = (((UInt64)ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
    Int64 offsetTime = (Int64)offset * kNumTimeQuantumsInSecond;
    if (offset < 0)
    {
      offsetTime *= -1;
      if (winTime < offsetTime)
        winTime = 0;
      else
        winTime -= offsetTime;
    }
    else if (winTime > 0xFFFFFFFFFFFFFFFF - offsetTime)
      winTime = 0xFFFFFFFFFFFFFFFF;
    else
      winTime += offsetTime;
    ft.dwLowDateTime = (UInt32)winTime;
    ft.dwHighDateTime = (UInt32)(winTime >> 32);
  }
}

void TimespecToFileTime(const TIMESPEC &ts, FILETIME &ft) throw()
{
  UnixSecondsToFileTime(TIMESPEC_SECONDS(ts), TIMESPEC_NANO_SECONDS(ts), ft);
}

bool FileTimeToTimespec(const FILETIME &ft, TIMESPEC &ts) throw()
{
  UInt32 sec, nsec;
  bool ret = FileTimeToUnixSeconds(ft, &sec, &nsec);
  SET_TIMESPEC(ts, sec, nsec);
  return ret;
}

bool FileTimeToSystemTime2(const FILETIME &ft, SYSTEMTIME2 &systemTime) throw()
{
  SYSTEMTIME st;
  if (!BOOLToBool(FileTimeToSystemTime(&ft, &st)))
  {
    systemTime.wYear = systemTime.wMonth = systemTime.wDay = 0;
    systemTime.wHour = systemTime.wMinute = systemTime.wSecond = 0;
    #ifdef USE_NANO_SECONDS
    systemTime.dwNanoSeconds = 0;
    #else
    systemTime.wMilliseconds = 0;
    #endif
    return false;
  }
  systemTime.wYear = st.wYear;
  systemTime.wMonth = st.wMonth;
  systemTime.wDay = st.wDay;
  systemTime.wHour = st.wHour;
  systemTime.wMinute = st.wMinute;
  systemTime.wSecond = st.wSecond;
  #ifdef USE_NANO_SECONDS
  FileTimeToUnixSeconds(ft, NULL, &systemTime.dwNanoSeconds);
  #else
  systemTime.wMilliseconds = st.wMilliseconds;
  #endif
  return true;
}

}}

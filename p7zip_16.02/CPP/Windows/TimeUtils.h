// Windows/TimeUtils.h

#ifndef __WINDOWS_TIME_UTILS_H
#define __WINDOWS_TIME_UTILS_H

#include "../Common/MyTypes.h"
#include "../Common/MyWindows.h"

namespace NWindows {
namespace NTime {

bool DosTimeToFileTime(UInt32 dosTime, FILETIME &fileTime) throw();
bool FileTimeToDosTime(const FILETIME &fileTime, UInt32 &dosTime) throw();
void UnixTimeToFileTime(UInt32 unixTime, FILETIME &fileTime) throw();
bool UnixTime64ToFileTime(Int64 unixTime, FILETIME &fileTime) throw();
bool FileTimeToUnixTime(const FILETIME &fileTime, UInt32 &unixTime) throw();
Int64 FileTimeToUnixTime64(const FILETIME &ft) throw();
bool GetSecondsSince1601(unsigned year, unsigned month, unsigned day,
  unsigned hour, unsigned min, unsigned sec, UInt64 &resSeconds) throw();
bool GetFileTimeSince1601(unsigned year, unsigned month, unsigned day,
  unsigned hour, unsigned min, unsigned sec, unsigned _100ns, FILETIME &fileTime) throw();
void GetCurUtcFileTime(FILETIME &ft) throw();
void AddOffsetSecondsToFileTime(Int32 offset, FILETIME &fileTime) throw();

void TimespecToFileTime(const TIMESPEC &ts, FILETIME &fileTime) throw();
bool FileTimeToTimespec(const FILETIME &fileTime, TIMESPEC &ts) throw();
bool FileTimeToSystemTime2(const FILETIME &fileTime, SYSTEMTIME2 &systemTime) throw();

}}

#endif

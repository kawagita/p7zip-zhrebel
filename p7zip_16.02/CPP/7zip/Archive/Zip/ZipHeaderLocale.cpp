// ZipHeaderLocale.cpp

#include "StdAfx.h"

#include "../../../Common/MyString.h"
#include "../../../Windows/TimeUtils.h"

#include "ZipHeaderLocale.h"

#include <locale.h>

using namespace NWindows;

namespace NArchive {
namespace NZip {

CHeaderLocale::CHeaderLocale()
{
  char *name = setlocale(LC_CTYPE, NULL);
  if (name != NULL)
    _SystemLocaleName = name;
  _IsSystemLocale = true;
  InitLocaleSetting();
}

void CHeaderLocale::SetDecodingLocale(const wchar_t *localeName, bool forComment)
{
  if (forComment)
    m_DecodingCommentLocaleName.SetFromWStr_if_Ascii(localeName);
  else
    m_DecodingLocaleName.SetFromWStr_if_Ascii(localeName);
}

void CHeaderLocale::SetEncodingLocale(const wchar_t *localeName)
{
  m_EncodingLocaleName.SetFromWStr_if_Ascii(localeName);
  UString charset;
  bool readCharset = false;
  const wchar_t *p = localeName;
  while (*p != 0)
  {
    wchar_t c = *p++;
    if (readCharset)
    {
      if (c == L'@')
        readCharset = false;
      else
        charset += c;
    }
    else if (c == L'.')
      readCharset = true;
  }
  charset.MakeLower_Ascii();
  _IsUtf8Encoding = (charset == L"utf8" || charset == L"utf-8");
}

bool CHeaderLocale::HasDecodingCharset(bool forComment)
{
  if (forComment)
  {
    if (!m_DecodingCommentLocaleName.IsEmpty())
      return true;
  }
  return !m_DecodingLocaleName.IsEmpty();
}

bool CHeaderLocale::HasEncodingCharset()
{
  return !m_EncodingLocaleName.IsEmpty();
}

bool CHeaderLocale::SwitchDecodingLocale(bool forComment)
{
  if (forComment && !m_DecodingCommentLocaleName.IsEmpty())
  {
    if (setlocale(LC_CTYPE, m_DecodingCommentLocaleName.Ptr()) != NULL)
    {
      _IsSystemLocale = false;
      return true;
    }
  }
  else if (!m_DecodingLocaleName.IsEmpty())
  {
    if (setlocale(LC_CTYPE, m_DecodingLocaleName.Ptr()) != NULL)
    {
      _IsSystemLocale = false;
      return true;
    }
  }
  return false;
}

bool CHeaderLocale::SwitchEncodingLocale()
{
  if (!m_EncodingLocaleName.IsEmpty())
  {
    if (setlocale(LC_CTYPE, m_EncodingLocaleName.Ptr()) != NULL)
    {
      _IsSystemLocale = false;
      return true;
    }
  }
  return false;
}

void CHeaderLocale::ResetLocale()
{
  if (!_IsSystemLocale)
  {
    setlocale(LC_CTYPE, _SystemLocaleName.Ptr());
    _IsSystemLocale = true;
  }
}

void CHeaderLocale::AdjustTimestamp(CItemOut &item) const
{
  FILETIME *fileTimes[] = { &item.Ntfs_MTime, &item.Ntfs_ATime, &item.Ntfs_CTime };
  for (unsigned i = 0; i < TS_PARSIZE; i++)
  {
    const TimestampSetting &setting = TimestampSettings[i];
    if (setting.TimeSpecified)
    {
      fileTimes[i]->dwLowDateTime = setting.TimeUpdate.dwLowDateTime;
      fileTimes[i]->dwHighDateTime = setting.TimeUpdate.dwHighDateTime;
    }
    NTime::AddOffsetSecondsToFileTime(setting.TimeOffset, *fileTimes[i]);
  }
  FILETIME localTime = { 0, 0 };
  if (FileTimeToLocalFileTime(fileTimes[TS_MODTIME], &localTime))
    NTime::FileTimeToDosTime((const FILETIME)localTime, item.Time);
}

}}

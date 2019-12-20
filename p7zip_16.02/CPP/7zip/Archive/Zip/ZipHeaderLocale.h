// ZipHeaderLocale.h

#ifndef __ZIP_HEADER_LOCALE_H
#define __ZIP_HEADER_LOCALE_H

#include "../../../Common/MyString.h"

#include "ZipOut.h"

namespace NArchive {
namespace NZip {

struct TimestampSetting
{
  bool TimeSpecified;
  Int32 TimeOffset;
  FILETIME TimeUpdate;
};

class CHeaderLocale
{
public:
  TimestampSetting TimestampSettings[TS_PARSIZE];

private:
  AString _SystemLocaleName;
  bool _IsSystemLocale;

  AString m_DecodingLocaleName;
  AString m_DecodingCommentLocaleName;
  AString m_EncodingLocaleName;
  bool _IsUtf8Encoding;

public:
  CHeaderLocale();

  void InitLocaleSetting()
  {
    for (unsigned i = 0; i < TS_PARSIZE; i++)
    {
      TimestampSettings[i].TimeSpecified = false;
      TimestampSettings[i].TimeOffset = 0;
      TimestampSettings[i].TimeUpdate.dwLowDateTime = TimestampSettings[i].TimeUpdate.dwHighDateTime = 0;
    }
    ResetLocale();
    m_DecodingLocaleName.Empty();
    m_DecodingCommentLocaleName.Empty();
    m_EncodingLocaleName.Empty();
    _IsUtf8Encoding = false;
  }

  void SetDecodingLocale(const wchar_t *localeName, bool forComment);
  void SetDecodingLocale(const wchar_t *localeName)
  {
    SetDecodingLocale(localeName, false);
  }
  void SetEncodingLocale(const wchar_t *localeName);

  bool HasDecodingCharset(bool forComment);
  bool HasDecodingCharset()
  {
    return HasDecodingCharset(false);
  }
  bool HasEncodingCharset();

  bool IsUtf8Encoding()
  {
    return _IsUtf8Encoding;
  }

  bool SwitchDecodingLocale(bool forComment);
  bool SwitchDecodingLocale()
  {
    return SwitchDecodingLocale(false);
  }
  bool SwitchEncodingLocale();
  void ResetLocale();

  void UpdateTimestamp(CItemOut &item) const;
};

}}

#endif

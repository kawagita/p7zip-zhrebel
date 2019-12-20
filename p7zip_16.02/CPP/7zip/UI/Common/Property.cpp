// Property.cpp

#include "StdAfx.h"

#include "../../../Common/MyException.h"
#include "../../../Windows/TimeUtils.h"

#include "Property.h"

#include <locale.h>

using namespace NWindows;
using namespace NCOM;

enum
{
  kLocaleLanguage = 0,
  kLocaleCountry,
  kLocaleCharset,
  kLocaleModifier
};

static bool ParseFromLocaleFormat(const UString s, UString &res)
{
  unsigned len = s.Len();
  if (len < 2)
    return false;
  wchar_t *localeChars = new wchar_t[len + 1];
  unsigned readLength[] = { 0, 0, 0, 0 };
  unsigned readPos = kLocaleLanguage;
  for (unsigned i = 0; i < len; i++)
  {
    wchar_t c = *(s.Ptr(i));
    localeChars[i] = c;
    switch (c)
    {
      case L'_':
        if (readPos < kLocaleCountry)
        {
          if (i == 0)
            return false;
          readPos = kLocaleCountry;
          continue;
        }
        break;
      case L'.':
        if (readPos < kLocaleCharset)
        {
          if (i == 0 || localeChars[i - 1] == L'_')
            return false;
          readPos = kLocaleCharset;
          continue;
        }
        return false;
      case L'@':
        if (readPos < kLocaleModifier)
        {
          if (i == 0 || localeChars[i - 1] == L'_' || localeChars[i - 1] == L'.')
            return false;
          readPos = kLocaleModifier;
          continue;
        }
        return false;
      default:
        if (c < L'-'
            || (c > L'-' && c < L'0')
            || (c > L'9' && c < L'A')
            || (c > L'Z' && c < L'a')
            || c > L'z')
          return false;
        break;
    }
    readLength[readPos]++;
  }
  localeChars[len] = 0;
  if (readPos >= kLocaleCharset)
  {
    if (readLength[kLocaleLanguage] < 2 || readLength[kLocaleLanguage] > 3
        || readLength[kLocaleCountry] != 2 || readLength[kLocaleCharset] < 2)
      return false;
    // 2 or 3 letter language and 2 letter country code are parsed
    res = (const wchar_t*)localeChars;
  }
  else // no language and country is parsed
  {
    char *lc = setlocale(LC_CTYPE, NULL);
    if (lc == NULL)
      return false;
    UString locale;
    locale.SetFromAscii(lc);
    int charsetIndex = locale.Find(L'.');
    if (charsetIndex >= 0)
      locale.DeleteFrom(charsetIndex + 1);
    locale += (const wchar_t*)localeChars;
    res = locale;
  }
  return true;
}

static const unsigned kNumTimestampDateParts = 3;
static const unsigned kNumTimestampTimeParts = 2;

static const unsigned kTimestampDigitLengths[] = { 4, 2, 2, 2, 2 };
static const unsigned kTimestampDateLength = 8;
static const unsigned kTimestampTimeLength = 4;
static const unsigned kTimestampDateTimeLength = kTimestampDateLength + kTimestampTimeLength;
static const unsigned kTimestampSecondsLength = 2;
static const unsigned kTimestampISO8601DateTimeLength = kTimestampDateTimeLength + kTimestampSecondsLength + 5;
static const unsigned kTimestamp100NanoSecondsLength = 7;

static const wchar_t kTimestampMinDigit = L'0';
static const wchar_t kTimestampMaxDigit = L'9';
static const wchar_t kTimestampISO8601TimeSeparator = L':';
static const wchar_t kTimestampISO8601Separators[] = { 0, L'-', L'-', L'T', kTimestampISO8601TimeSeparator };
static const wchar_t kTimestampSecondSeparator = L'.';

static bool ParseTimestampDigit(wchar_t c, unsigned &t)
{
  if (c >= kTimestampMinDigit && c <= kTimestampMaxDigit)
  {
    t = (unsigned)(c - kTimestampMinDigit);
    return true;
  }
  return false;
}

static bool ParseFromTimestampFormat(const UString s, FILETIME &res)
{
  UString dateTimeStr = s;
  unsigned dateTimeLen = dateTimeStr.Len();
  unsigned seconds = 0;
  int secSeparatorPos = dateTimeStr.Find(kTimestampSecondSeparator);
  if (secSeparatorPos >= 0)
  {
    int secLen = dateTimeStr.Len() - (secSeparatorPos + 1);
    if (secSeparatorPos == kTimestampISO8601DateTimeLength)
    {
      if (secLen < 1)
        return false;
      secLen = kTimestamp100NanoSecondsLength;
    }
    else if (secSeparatorPos != kTimestampDateTimeLength || secLen != kTimestampSecondsLength)
      return false;
    const wchar_t *p = dateTimeStr.Ptr(secSeparatorPos + 1);
    for (unsigned i = 0; i < secLen; i++)
    {
      seconds *= 10;
      unsigned sec = 0;
      if (*p != 0)
      {
        if (!ParseTimestampDigit(*p, sec))
          return false;
        p++;
      }
      seconds += sec;
    }
    while (*p != 0)
    {
      unsigned sec;
      if (!ParseTimestampDigit(*p++, sec))
        return false;
    }
    dateTimeStr.DeleteFrom(secSeparatorPos);
    dateTimeLen = secSeparatorPos;
  }
  else if (dateTimeLen != kTimestampDateTimeLength
           && dateTimeLen != kTimestampISO8601DateTimeLength)
    return false;
  const wchar_t *p = dateTimeStr.Ptr();
  unsigned dt[] = { 0, 0, 0, 0, 0 };
  unsigned _100ns = 0;
  for (unsigned i = 0; i < kNumTimestampDateParts + kNumTimestampTimeParts; i++)
  {
    if (*p == kTimestampISO8601Separators[i])
    {
      dateTimeLen--;
      p++;
    }
    unsigned len = kTimestampDigitLengths[i];
    do
    {
      dt[i] *= 10;
      unsigned t;
      if (dateTimeLen <= 0 || !ParseTimestampDigit(*p++, t))
        return false;
      dt[i] += t;
      dateTimeLen--;
    }
    while (--len > 0);
  }
  if (dateTimeLen > 0)
  {
    if (--dateTimeLen != kTimestampSecondsLength || *p++ != kTimestampISO8601TimeSeparator)
      return false;
    _100ns = seconds;
    seconds = 0;
    for (unsigned i = 0; i < dateTimeLen; i++)
    {
      seconds *= 10;
      unsigned sec;
      if (!ParseTimestampDigit(*p++, sec))
        return false;
      seconds += sec;
    }
  }
  FILETIME localTime = { 0, 0 };
  if (!NTime::GetFileTimeSince1601(dt[0], dt[1], dt[2], dt[3], dt[4], seconds, _100ns, localTime)
      || !LocalFileTimeToFileTime(&localTime, &res))
    return false;
  return true;
}

static bool ParseFromTimeZoneFormat(const UString s, Int32 &res)
{
  const wchar_t *p = s.Ptr();
  unsigned len = s.Len();
  Int32 sign = 1;
  Int32 offset = 0;
  switch (*p)
  {
    case L'-':
      sign = -1;
    case L'+':
      len--;
      p++;
    default:
      if (len != kTimestampTimeLength + 1)
        return false;
      break;
  }
  for (unsigned i = 0; i < kNumTimestampTimeParts; i++)
  {
    if (i > 0)
    {
      if (*p++ != kTimestampISO8601TimeSeparator)
        return false;
    }
    unsigned time = 0;
    for (unsigned j = 0; j < kTimestampDigitLengths[kNumTimestampDateParts + i]; j++)
    {
      time *= 10;
      unsigned t;
      if (!ParseTimestampDigit(*p++, t))
        return false;
      time += t;
    }
    offset = (offset + time) * 60;
  }
  res = sign * offset;
  return true;
}

static const wchar_t kDataMinDigit = L'0';
static const wchar_t kDataMaxDigit = L'9';
static const wchar_t kDataMinHexCaptial = L'A';
static const wchar_t kDataMaxHexCaptial = L'F';
static const wchar_t kDataMinHexSmall = L'a';
static const wchar_t kDataMaxHexSmall = L'f';

static bool DataWCharToUInt(wchar_t c, UInt32 radix, UInt32 &res)
{
  if (c >= kDataMinDigit && c <= kDataMaxDigit)
  {
    res = (UInt32)(c - kDataMinDigit);
    return true;
  }
  else if (radix > 10)
  {
    if (c >= kDataMinHexCaptial && c <= kDataMaxHexCaptial)
    {
      res = (UInt32)(c - kDataMinHexCaptial);
      return true;
    }
    else if (c >= kDataMinHexSmall && c <= kDataMaxHexSmall)
    {
      res = (UInt32)(c - kDataMinHexSmall);
      return true;
    }
  }
  res = 0;
  return false;
}

static bool ParseFromDataFormat(const UString s, UInt64 &res, unsigned &len)
{
  UInt64 data = 0;
  UInt32 radix = 10;
  if (s[0] == L'0' && s[1] == L'x')
  {
    radix = 16;
    len = 2;
  }
  else
    len = 0;
  const wchar_t *p = s.Ptr(len);
  UInt32 val;
  if (!DataWCharToUInt(*p++, radix, val))
    return false;
  do
  {
    len++;
    data += val;
    if (data > 0xFFFFFFFF || !DataWCharToUInt(*p++, radix, val))
      break;
    data *= radix;
  }
  while (1);
  res = data;
  return true;
}

static bool ParseFromSwitchFormat(const UString s, CBoolPair &res)
{
  if (s == L"off")
    res.Def = true;
  else if (s == L"on")
    res.SetTrueTrue();
  return res.Def;
}


void CHeaderData::SetIntValue(Int32 intVal, int index)
{
  throw CSystemException(E_NOTIMPL);
}

void CHeaderData::SetUIntValue(UInt32 intVal, int index)
{
  throw CSystemException(E_NOTIMPL);
}

void CHeaderData::SetStringValue(const UString strVal, int index)
{
  throw CSystemException(E_NOTIMPL);
}

void CHeaderData::SetBoolValue(bool boolVal, int index)
{
  throw CSystemException(E_NOTIMPL);
}

void CHeaderData::SetFileTimeValue(const FILETIME &filetime, int index)
{
  throw CSystemException(E_NOTIMPL);
}

PARTYPE CHeaderData::GetValueType() const
{
  return VT_EMPTY;
}

PARSIZE CHeaderData::GetValueSize() const
{
  return 0;
}

void CHeaderData::GetValue(CPropVariant &value, int index) const
{
  throw CSystemException(E_FAIL);
}

struct CLocaleData: public CHeaderData
{
  UString Locales[LC_PARSIZE];

  CLocaleData() { }

  void SetStringValue(const UString strVal, int index)
  {
    if (index < 0)
    {
      for (unsigned i = 0; i < LC_PARSIZE; i++)
        Locales[i] = strVal;
    }
    else if (index < LC_PARSIZE)
      Locales[index] = strVal;
  }

  PARTYPE GetValueType() const { return VT_BSTR; }
  PARSIZE GetValueSize() const { return LC_PARSIZE; }

  void GetValue(CPropVariant &value, int index) const
  {
    if (index < 0 || index >= LC_PARSIZE)
      throw CSystemException(E_FAIL);
    else if (!Locales[index].IsEmpty())
      value = Locales[index];
  }
};

struct CTimestampData: public CHeaderData
{
  bool HasFileTimes[TS_PARSIZE];
  FILETIME FileTimes[TS_PARSIZE];

  CTimestampData()
  {
    for (unsigned i = 0; i < TS_PARSIZE; i++)
    {
      HasFileTimes[i] = false;
      FileTimes[i].dwLowDateTime = FileTimes[i].dwHighDateTime = 0;
    }
  }

  void SetFileTimeValue(const FILETIME &filetime, int index)
  {
    if (index < 0)
    {
      for (unsigned i = 0; i < TS_PARSIZE; i++)
      {
        HasFileTimes[i] = true;
        FileTimes[i] = filetime;
      }
    }
    else if (index < TS_PARSIZE)
    {
      HasFileTimes[index] = true;
      FileTimes[index] = filetime;
    }
  }

  PARTYPE GetValueType() const { return VT_FILETIME; }
  PARSIZE GetValueSize() const { return TS_PARSIZE; }

  void GetValue(CPropVariant &value, int index) const
  {
    if (index < 0 || index >= TS_PARSIZE)
      throw CSystemException(E_FAIL);
    else if (HasFileTimes[index])
      value = FileTimes[index];
  }
};

struct CTimeZoneData: public CHeaderData
{
  Int32 OffsetSeconds[TS_PARSIZE];

  CTimeZoneData()
  {
    for (unsigned i = 0; i < TS_PARSIZE; i++)
      OffsetSeconds[i] = 0;
  }

  void SetIntValue(Int32 intVal, int index)
  {
    if (index < 0)
    {
      for (unsigned i = 0; i < TS_PARSIZE; i++)
        OffsetSeconds[i] = intVal;
    }
    else if (index < TS_PARSIZE)
      OffsetSeconds[index] = intVal;
  }

  PARTYPE GetValueType() const { return VT_I4; }
  PARSIZE GetValueSize() const { return TS_PARSIZE; }

  void GetValue(CPropVariant &value, int index) const
  {
    if (index < 0 || index >= TS_PARSIZE)
      throw CSystemException(E_FAIL);
    else if (OffsetSeconds[index] != 0)
      value = OffsetSeconds[index];
  }
};

struct CInformationData: public CHeaderData
{
  UString InfoData;

  CInformationData() { }

  void SetStringValue(const UString strVal, int index) { InfoData = strVal; }

  PARTYPE GetValueType() const { return VT_BSTR; }
  PARSIZE GetValueSize() const { return 1; }

  void GetValue(CPropVariant &value, int index) const
  {
    if (index < 0 || index >= 1)
      throw CSystemException(E_FAIL);
    else if (!InfoData.IsEmpty())
      value = InfoData;
  }
};

struct CExtendedData: public CHeaderData
{
  CObjectVector<UInt32> HexDataValues;

  CExtendedData() { }

  void SetUIntValue(UInt32 uintVal, int index) { HexDataValues.Add(uintVal); }

  PARTYPE GetValueType() const { return VT_UI4; }
  PARSIZE GetValueSize() const { return HexDataValues.Size(); }

  void GetValue(CPropVariant &value, int index) const
  {
    if (index < 0 || index >= HexDataValues.Size())
      throw CSystemException(E_FAIL);
    value = HexDataValues[index];
  }
};

struct CExtendedDataSwitch: public CHeaderData
{
  bool SwitchOn;

  CExtendedDataSwitch(): SwitchOn(false) { }

  void SetBoolValue(bool boolVal, int index) { SwitchOn = boolVal; }

  PARTYPE GetValueType() const { return VT_BOOL; }
  PARSIZE GetValueSize() const { return 1; }

  void GetValue(CPropVariant &value, int index) const
  {
    if (index < 0 || index >= 1)
      throw CSystemException(E_FAIL);
    value = SwitchOn;
  }
};

CHeaderProperty::CHeaderProperty(UString name, CHeaderData *data)
{
  if (data == NULL)
    throw CSystemException(E_POINTER);
  Name = name;
  Data = data;
}

CHeaderPropertyParser::CHeaderPropertyParser(const CHeaderPropertyForm *forms, UInt32 size, CObjectVector<CHeaderProperty> *props)
{
  if (forms == NULL || props == NULL)
    throw CSystemException(E_POINTER);
  PropForms = forms;
  PropSize = size;
  Props = props;
}

bool CHeaderPropertyParser::ParseHeaderParameter(const CProperty &param)
{
  CBoolPair paramSwitch;
  UString name = param.Name;
  if (name.IsEmpty())
    return false;
  else if (param.Value.IsEmpty())
  {
    wchar_t c = name.Back();
    if (c == L'-')
      paramSwitch.Def = true;
    else if (c == L'+')
      paramSwitch.SetTrueTrue();
    if (paramSwitch.Def)
      name.DeleteBack();
  }
  const CHeaderPropertyForm *propForm = PropForms;
  for (unsigned i = 0; i < PropSize; i++)
  {
    unsigned paramIndex = -1;
    bool propFound = false;
    if (name == propForm->Name)
      propFound = true;
    else
    {
      while (++paramIndex < propForm->ParamSize)
      {
        if (name == propForm->ParamNames[paramIndex])
        {
          propFound = true;
          break;
        }
      }
    }
    if (propFound)
    {
      CHeaderData *data;
      bool dataFound = false;
      FOR_VECTOR (j, *Props)
      {
        CHeaderProperty &prop = (*Props)[j];
        if (prop.Name == propForm->Name)
        {
          data = prop.Data;
          dataFound = true;
          break;
        }
      }
      switch (propForm->Type)
      {
        case HT_LOCALE:
        {
          if (!dataFound)
            data = new CLocaleData();
          UString locale;
          if (!ParseFromLocaleFormat(param.Value, locale))
            return false;
          data->SetStringValue(locale, paramIndex);
          break;
        }
        case HT_TIMESTAMP:
        {
          if (!dataFound)
            data = new CTimestampData();
          FILETIME filetime = { 0, 0 };
          if (!ParseFromTimestampFormat(param.Value, filetime))
            return false;
          data->SetFileTimeValue(filetime, paramIndex);
          break;
        }
        case HT_TIME_ZONE:
        {
          if (!dataFound)
            data = new CTimeZoneData();
          Int32 offset = 0;
          if (!ParseFromTimeZoneFormat(param.Value, offset))
            return false;
          data->SetIntValue(offset, paramIndex);
          break;
        }
        case HT_EXTENDED_DATA:
        {
          if (!dataFound)
            data = new CExtendedData();
          UInt64 hexData = 0;
          UString value = param.Value;
          unsigned len;
          while (!value.IsEmpty())
          {
            if (!ParseFromDataFormat(value, hexData, len))
              return false;
            else if (value[len] == L',')
              len++;
            value.DeleteFrontal(len);
            data->SetUIntValue(hexData, paramIndex);
          }
          break;
        }
        case HT_EXTENDED_DATA_SWITCH:
        {
          if (!dataFound)
            data = new CExtendedDataSwitch();
          if (!ParseFromSwitchFormat(param.Value, paramSwitch))
            return false;
          data->SetBoolValue(paramSwitch.Val, paramIndex);
          break;
        }
        case HT_TIME_INFORMATION:
        case HT_EXTENDED_INFORMATION:
        {
          if (!dataFound)
            data = new CInformationData();
          data->SetStringValue(param.Value, paramIndex);
          break;
        }
      }
      if (!dataFound)
      {
        CHeaderProperty *prop = new CHeaderProperty(propForm->Name, data);
        Props->Add(*prop);
      }
      return true;
    }
    propForm++;
  }
  return false;
}

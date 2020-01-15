// Property.h

#ifndef __7Z_PROPERTY_H
#define __7Z_PROPERTY_H

#include "../../../Windows/PropVariant.h"

struct CProperty
{
  UString Name;
  UString Value;
};

struct CHeaderData
{
  CHeaderData() {}

  virtual void SetIntValue(Int32 intVal, int index);
  virtual void SetUIntValue(UInt32 uintVal, int index);
  virtual void SetCharValue(wchar_t c, int index);
  virtual void SetStringValue(const UString strVal, int index);
  virtual void SetBoolValue(bool boolVal, int index);
  virtual void SetFileTimeValue(const FILETIME &filetime, int index);

  virtual PARTYPE GetValueType() const;
  virtual PARSIZE GetValueSize() const;
  virtual void GetValue(NWindows::NCOM::CPropVariant &value, int index) const;
};

struct CHeaderProperty
{
  UString Name;
  CHeaderData *Data;

  CHeaderProperty(UString name, CHeaderData *data);
};

typedef enum tagHEADERTYPE
{
  HT_NOTHING = 0,
  HT_LOCALE,
  HT_TIME_INFORMATION,
  HT_TIME_ZONE,
  HT_TIMESTAMP,
  HT_FILE_INFORMATION,
  HT_FILE_ATTRIBUTE,
  HT_FILE_PERMISSIONS,
  HT_FILE_OWNERSHIP,
  HT_EXTENDED_INFORMATION,
  HT_EXTENDED_DATA,
  HT_EXTENDED_DATA_SWITCH
} HEADERTYPE;

struct CHeaderPropertyForm
{
  const wchar_t *Name;
  HEADERTYPE TypeForName;
  PARSIZE ParamSize;
  const wchar_t **ParamNames;
  HEADERTYPE ParamType;
};

class CHeaderPropertyParser
{
private:
  const CHeaderPropertyForm *PropForms;
  UInt32 PropSize;
  CObjectVector<CHeaderProperty> *HeaderProps;

public:
  CHeaderPropertyParser(const CHeaderPropertyForm *forms, UInt32 size,
      CObjectVector<CHeaderProperty> *headerProps);

  bool ParseHeaderParameter(const CProperty &param);
};

#endif

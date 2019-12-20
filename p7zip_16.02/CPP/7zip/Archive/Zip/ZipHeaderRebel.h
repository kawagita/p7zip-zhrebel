// ZipHeaderRebel.h

#ifndef __ZIP_HEADER_REBEL_H
#define __ZIP_HEADER_REBEL_H

#include "ZipHeaderLocale.h"

namespace NArchive {
namespace NZip {

static const CObjectVector<UInt16> kExtraNoExclusiveIDs;

class CHeaderExtraStorage
{
private:
  bool m_HasWzAesField;
  CWzAesExtra m_WzAesField;
  CUnixTimeExtra m_UnixTimeField;

  CExtraBlock m_LocalExtraUnknown;
  CExtraBlock m_CentralExtraUnknown;

public:
  CHeaderExtraStorage(): m_HasWzAesField(false) {}

  UInt32 GetLocalExtraSize(const CItemOut &item) const;

  void SetUnixTimeExtra(UInt32 index, UInt32 epochTime)
  {
    if (index >= 0 && index < NFileHeader::NUnixTime::kNumFields)
    {
      m_UnixTimeField.EpochTimes[index] = epochTime;
      m_UnixTimeField.IsTimePresent[index] = true;
    }
  }

  void CopyExtraSubBlocks(const CItem &item, const CObjectVector<UInt16> &exclusiveIDs, bool copyAll);

  void RestoreExtraSubBlocks(CItemOut &item) const;
  void RestoreExtraWzAes(CItemOut &item) const;
};

class CHeaderRebel
{
private:
  CHeaderLocale *m_HeaderLocale;

  UInt32 m_TimeType;

  bool m_WriteExtraAll;
  const CObjectVector<UInt16> *m_ExtraExclusiveIDs;

public:
  CHeaderRebel(CHeaderLocale *headerLocale, UInt32 timeType, const CObjectVector<UInt16> *extraAddedIDs)
  {
    m_HeaderLocale = headerLocale;
    m_TimeType = timeType;
    m_WriteExtraAll = false;
    if (extraAddedIDs != NULL)
      m_ExtraExclusiveIDs = extraAddedIDs;
    else
      m_ExtraExclusiveIDs = &kExtraNoExclusiveIDs;
  }

  void SwitchWriteExtraAll(const CObjectVector<UInt16> *extraDeletedIDs)
  {
    m_WriteExtraAll = true;
    if (extraDeletedIDs != NULL)
      m_ExtraExclusiveIDs = extraDeletedIDs;
    else
      m_ExtraExclusiveIDs = &kExtraNoExclusiveIDs;
  }

  void SetLocalHeaderLocale(CItemOut &item);
  void ChangeLocalHeader(CItemOut &item, CHeaderExtraStorage &extraStorage);

  void BackupHeaderExtra(const CItem &item, CHeaderExtraStorage &extraStorage)
  {
    extraStorage.CopyExtraSubBlocks(item, *m_ExtraExclusiveIDs, m_WriteExtraAll);
  }
};

}}

#endif

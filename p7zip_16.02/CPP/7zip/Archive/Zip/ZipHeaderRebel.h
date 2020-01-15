// ZipHeaderRebel.h

#ifndef __ZIP_HEADER_REBEL_H
#define __ZIP_HEADER_REBEL_H

#include "ZipHeaderLocale.h"

namespace NArchive {
namespace NZip {

class CHeaderExtraStorage
{
private:
  CUnixTimeExtra m_UnixTimeField;
  CUnixFileOwnershipExtra m_UnixFileOwnershipField;
  CWzAesExtra m_WzAesField;

  bool m_StoreUnixFileOwnershipField;
  bool m_HasWzAesField;

  CExtraBlock m_LocalExtraUnknown;
  CExtraBlock m_CentralExtraUnknown;

public:
  CHeaderExtraStorage(): m_StoreUnixFileOwnershipField(false), m_HasWzAesField(false) {}

  UInt32 GetLocalExtraSize(const CItemOut &item) const;

  void SetUnixTimeExtra(UInt32 index, UInt32 epochTime)
  {
    if (index >= 0 && index < TS_PARSIZE)
    {
      m_UnixTimeField.EpochTimes[index] = epochTime;
      m_UnixTimeField.IsTimePresent[index] = true;
    }
  }

  void SetUnixFileOwnershipExtra(UInt32 uid, UInt32 gid)
  {
    m_UnixFileOwnershipField.OwnerIDs[OWNER_UID] = uid;
    m_UnixFileOwnershipField.OwnerIDs[OWNER_GID] = gid;
    m_StoreUnixFileOwnershipField = true;
  }

  void CopyExtraSubBlocks(const CItem &item,
      const CObjectVector<UInt16> &exclusiveIDs, bool copyAll, bool copyUnixFileOwnership);

  void RestoreExtraSubBlocks(CItemOut &item) const;
  void RestoreExtraWzAes(CItemOut &item) const;
};

class CHeaderRebel
{
private:
  CHeaderLocale *m_HeaderLocale;

  UInt16 m_FileTimeType;
  UInt16 m_FileInfoType;
  UInt16 m_FileAttribSettingMask;
  UInt16 m_FileAttribUnsettingMask;
  UInt16 m_UnixModeBits;
  mode_t _UnixModeMask;

  bool m_SetTimestampFromModTime;
  bool m_SetIzAttrib;
  bool m_SetUnixModeBits;
  bool m_SetUnixFileOwnership;
  bool m_CopyUnixFileOwnership;
  bool m_CopyExtraAll;
  const CObjectVector<UInt16> *m_ExtraExclusiveIDs;

public:
  CHeaderRebel(
      CHeaderLocale *headerLocale,
      UInt16 timeType,
      UInt16 headerInfoType,
      const CFileHeaderInfo &headerInfo);

  void SetLocalHeaderLocale(CItemOut &item);
  void ChangeLocalHeader(CItemOut &item, CHeaderExtraStorage &extraStorage) const;

  void BackupHeaderExtra(const CItem &item, CHeaderExtraStorage &extraStorage) const
  {
    extraStorage.CopyExtraSubBlocks(item, *m_ExtraExclusiveIDs, m_CopyExtraAll, m_CopyUnixFileOwnership);
  }
};

}}

#endif

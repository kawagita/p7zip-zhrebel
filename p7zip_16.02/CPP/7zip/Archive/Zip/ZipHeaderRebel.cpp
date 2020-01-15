// ZipHeaderRebel.cpp

#include "StdAfx.h"

#include <sys/stat.h>

#include "../../../Common/MyLinux.h"
#include "../../../Common/StringConvert.h"
#include "../../../Windows/TimeUtils.h"

#include "ZipHeaderRebel.h"

using namespace NWindows;

namespace NArchive {
namespace NZip {

using namespace NFileHeader;

UInt32 CHeaderExtraStorage::GetLocalExtraSize(const CItemOut &item) const
{
  UInt32 extraSize = m_LocalExtraUnknown.GetSize();
  if (item.UnixTimeIsDefined)
  {
    extraSize += 4 + k_UnixTimeExtra_OnlyModTimeLength;
    if (item.UnixAcTimeIsDefined) extraSize += k_UnixTimeExtra_EpochTimeLength;
    if (item.UnixCrTimeIsDefined) extraSize += k_UnixTimeExtra_EpochTimeLength;
  }
  if (m_StoreUnixFileOwnershipField)
    extraSize += 4 + k_UnixFileOwnershipExtra_DataBlockLength;
  return extraSize;
}

static void SetExtraSubBlocks(const CObjectVector<CExtraSubBlock> &subBlocks,
    const CObjectVector<UInt16> &exclusiveIDs, bool copyAll, bool copyUnixFileOwnership, CExtraBlock &extra)
{
  if (!exclusiveIDs.IsEmpty() || copyAll)
  {
    FOR_VECTOR (i, subBlocks)
    {
      UInt16 id = subBlocks[i].ID;
      switch (id)
      {
        case NExtraID::kZip64:
        case NExtraID::kNTFS:
        case NExtraID::kStrongEncrypt:
        case NExtraID::kUnixTime:
        case NExtraID::kWzAES:
          break;
        case NExtraID::kUnixFileOwnership:
          if (!copyUnixFileOwnership)
            break;
        default:
        {
          bool specified = false;
          FOR_VECTOR (j, exclusiveIDs)
          {
            if (id == exclusiveIDs[j])
            {
              specified = true;
              break;
            }
          }
          if (copyAll)
          {
            if (!specified)
              extra.AddSubBlock(subBlocks[i]);
          }
          else if (specified)
            extra.AddSubBlock(subBlocks[i]);
          break;
        }
      }
    }
  }
}

void CHeaderExtraStorage::CopyExtraSubBlocks(const CItem &item,
    const CObjectVector<UInt16> &exclusiveIDs, bool copyAll, bool copyUnixFileOwnership)
{
  m_LocalExtraUnknown.Clear();
  m_CentralExtraUnknown.Clear();
  m_HasWzAesField = item.LocalExtra.GetWzAes(m_WzAesField);
  SetExtraSubBlocks(item.LocalExtra.SubBlocks, exclusiveIDs, copyAll, copyUnixFileOwnership, m_LocalExtraUnknown);
  SetExtraSubBlocks(item.LocalExtra.SubBlocks, exclusiveIDs, copyAll, copyUnixFileOwnership, m_CentralExtraUnknown);
}

void CHeaderExtraStorage::RestoreExtraSubBlocks(CItemOut &item) const
{
  item.LocalExtra.Clear();
  item.CentralExtra.Clear();
  if (item.UnixTimeIsDefined)
  {
    CExtraSubBlock sb;
    m_UnixTimeField.SetSubBlock(sb, true);
    item.LocalExtra.AddSubBlock(sb);
    m_UnixTimeField.SetSubBlock(sb, false);
    item.CentralExtra.AddSubBlock(sb);
  }
  if (m_StoreUnixFileOwnershipField)
  {
    CExtraSubBlock sb;
    m_UnixFileOwnershipField.SetSubBlock(sb);
    item.LocalExtra.AddSubBlock(sb);
    item.CentralExtra.AddSubBlock(sb);
  }
  item.LocalExtra.AddExtra(m_LocalExtraUnknown);
  item.CentralExtra.AddExtra(m_CentralExtraUnknown);
}

void CHeaderExtraStorage::RestoreExtraWzAes(CItemOut &item) const
{
  if (m_HasWzAesField)
  {
    CExtraSubBlock sb;
    m_WzAesField.SetSubBlock(sb);
    item.LocalExtra.AddSubBlock(sb);
    item.CentralExtra.AddSubBlock(sb);
  }
}

static const UInt16 kFileAttribNotSettingMask = 0x0000;
static const UInt16 kFileAttribNotUnsettingMask = 0xffff;

static const UInt16 kReadPermissionBits = MY_LIN_S_IRUSR | MY_LIN_S_IRGRP | MY_LIN_S_IROTH;
static const UInt16 kWritePermissionBits = MY_LIN_S_IWUSR | MY_LIN_S_IWGRP | MY_LIN_S_IWOTH;
static const UInt16 kExecutePermissionBits = MY_LIN_S_IXUSR | MY_LIN_S_IXGRP | MY_LIN_S_IXOTH;

CHeaderRebel::CHeaderRebel(
    CHeaderLocale *headerLocale,
    UInt16 timeType,
    UInt16 fileInfoType,
    const CFileHeaderInfo &headerInfo)
{
  m_HeaderLocale = headerLocale;
  m_FileTimeType = timeType;
  m_FileInfoType = fileInfoType;
  if ((m_FileInfoType & NFileInfoType::kWithAttribMask) != 0)
  {
    m_FileAttribSettingMask = headerInfo.AttribFlags[ATTR_SETTING];
    m_FileAttribUnsettingMask = ~headerInfo.AttribFlags[ATTR_UNSETTING];
  }
  else
  {
    m_FileAttribSettingMask = kFileAttribNotSettingMask;
    m_FileAttribUnsettingMask = kFileAttribNotUnsettingMask;
  }
  m_UnixModeBits = headerInfo.UnixModeBits & (~MY_LIN_S_IFMT);
  mode_t mask = umask(0);
  umask(mask);
  _UnixModeMask = ~mask;
  m_SetTimestampFromModTime = headerInfo.SetTimestampFromModTime;
  m_SetIzAttrib = headerInfo.SetIzAttrib;
  m_SetUnixModeBits = headerInfo.SetUnixModeBits;
  m_SetUnixFileOwnership = headerInfo.SetUnixFileOwnership;
  m_CopyUnixFileOwnership = headerInfo.CopyUnixFileOwnership;
  m_CopyExtraAll = headerInfo.CopyExtraAll;
  if (m_CopyExtraAll)
    m_ExtraExclusiveIDs = &headerInfo.ExtraDeletedIDs;
  else
    m_ExtraExclusiveIDs = &headerInfo.ExtraAddedIDs;
}

void CHeaderRebel::SetLocalHeaderLocale(CItemOut &item)
{
  if (m_HeaderLocale->HasEncodingCharset())
  {
    UString commentString;
    unsigned commentSize = item.Comment.Size();
    if (commentSize > 0)
    {
      AString srcComment;
      srcComment.SetFrom_CalcLen((const char *)(const Byte *)item.Comment, commentSize);
      if (m_HeaderLocale->HasDecodingCharset(true))
      {
        if (!m_HeaderLocale->SwitchDecodingLocale(true))
          throw CSystemException(E_LANG_NOT_FOUND);
        commentString = MultiByteToUnicodeString(srcComment);
      }
      else if (!ConvertUTF8ToUnicode(srcComment, commentString))
        throw CSystemException(E_LANG_NOT_FOUND);
    }
    if (!m_HeaderLocale->SwitchEncodingLocale())
      throw CSystemException(E_LANG_NOT_FOUND);
    UString nameString;
    ConvertUTF8ToUnicode(item.Name, nameString);  // never fail
    item.Name = UnicodeStringToMultiByte(nameString);
    if (commentSize > 0)
    {
      AString destComment = UnicodeStringToMultiByte(commentString);
      item.Comment.CopyFrom((const Byte *)destComment.Ptr(), destComment.Len());
    }
    item.SetUtf8(m_HeaderLocale->IsUtf8Encoding());
    m_HeaderLocale->ResetLocale();
  }
  else if (!item.IsUtf8() && m_HeaderLocale->HasDecodingCharset())
  {
    if (!m_HeaderLocale->SwitchDecodingLocale())
      throw CSystemException(E_LANG_NOT_FOUND);
    UString nameString;
    ConvertUTF8ToUnicode(item.Name, nameString);  // never fail
    item.Name = UnicodeStringToMultiByte(nameString);
    m_HeaderLocale->ResetLocale();
  }
}

static void SetUnixTimeExtraField(const FILETIME &ft, UInt32 index, CHeaderExtraStorage &extraStorage)
{
  UInt32 epochTime;
  NTime::FileTimeToUnixTime(ft, epochTime);
  extraStorage.SetUnixTimeExtra(index, epochTime);
}

void CHeaderRebel::ChangeLocalHeader(CItemOut &item, CHeaderExtraStorage &extraStorage) const
{
  bool NtfsTimeIsStored = false;
  bool UnixTimeIsStored = false;
  bool UnixAcTimeIsStored = false;
  bool UnixCrTimeIsStored = false;
  if (m_FileTimeType != NTimeType::kDosTimeZero)
  {
    if (m_FileTimeType == NTimeType::kDefault)
    {
      NtfsTimeIsStored = item.NtfsTimeIsDefined;
      UnixTimeIsStored = item.UnixTimeIsDefined;
      UnixAcTimeIsStored = item.UnixAcTimeIsDefined;
      UnixCrTimeIsStored = item.UnixCrTimeIsDefined;
    }
    else if (m_FileTimeType == NTimeType::kNtfsExtra)
      NtfsTimeIsStored = true;
    else if (m_FileTimeType == NTimeType::kUnixTimeExtra)
    {
      UnixTimeIsStored= true;
      UnixAcTimeIsStored= true;
    }
    m_HeaderLocale->AdjustTimestamp(item);
    if (NtfsTimeIsStored)
    {
      if (m_SetTimestampFromModTime)
        item.Ntfs_CTime = item.Ntfs_ATime = item.Ntfs_MTime;
      else if (!item.NtfsTimeIsDefined)
      {
        if (!item.UnixAcTimeIsDefined) item.Ntfs_ATime = item.Ntfs_MTime;
        if (!item.UnixCrTimeIsDefined) item.Ntfs_CTime = item.Ntfs_MTime;
      }
    }
    else if (UnixTimeIsStored)
    {
      if (m_SetTimestampFromModTime)
        item.Ntfs_CTime = item.Ntfs_ATime = item.Ntfs_MTime;
      else if (!item.NtfsTimeIsDefined && !item.UnixAcTimeIsDefined)
        item.Ntfs_ATime = item.Ntfs_MTime;
      SetUnixTimeExtraField(item.Ntfs_MTime, TS_MODTIME, extraStorage);
      if (UnixAcTimeIsStored) SetUnixTimeExtraField(item.Ntfs_ATime, TS_ACTIME, extraStorage);
      if (UnixCrTimeIsStored) SetUnixTimeExtraField(item.Ntfs_CTime, TS_CRTIME, extraStorage);
    }
  }
  else
    item.Time = 0;
  item.NtfsTimeIsDefined = NtfsTimeIsStored;
  item.UnixTimeIsDefined = UnixTimeIsStored;
  item.UnixAcTimeIsDefined = UnixAcTimeIsStored;
  item.UnixCrTimeIsDefined = UnixCrTimeIsStored;

  Byte hostOS = item.MadeByVersion.HostOS;
  if (m_FileInfoType == NFileInfoType::kWindows)
    hostOS = NHostOS::kFAT;
  else if ((m_FileInfoType & NFileInfoType::kUnixMask) != 0)
    hostOS = NHostOS::kUnix;
  item.MadeByVersion.HostOS = hostOS;

  bool changeAttrib = (m_FileInfoType & NFileInfoType::kWithAttribMask) != 0;
  UInt16 attrib = (item.ExternalAttrib | m_FileAttribSettingMask) & m_FileAttribUnsettingMask;
  UInt16 mode = item.ExternalAttrib >> 16;
  switch (hostOS)
  {
    case NHostOS::kFAT:
    case NHostOS::kNTFS:
      if (m_FileAttribSettingMask != kFileAttribNotSettingMask ||
          m_FileAttribUnsettingMask != kFileAttribNotUnsettingMask)
        changeAttrib = true;
      break;
    case NHostOS::kUnix:
      if ((mode & (~MY_LIN_S_IFMT)) == 0)
      {
        mode = kReadPermissionBits;
        if ((attrib & FILE_ATTRIBUTE_READONLY) == 0)
          mode |= kWritePermissionBits;
        if (item.IsDir())
          mode |= MY_LIN_S_IFDIR | kExecutePermissionBits;
        else
          mode |= MY_LIN_S_IFREG;
        mode &= _UnixModeMask;
      }
      if (m_SetUnixModeBits)
        mode &= MY_LIN_S_IFMT;  // set only bits except for file or device type
      mode |= m_UnixModeBits;
      if (m_SetUnixFileOwnership)
        extraStorage.SetUnixFileOwnershipExtra(item.UID, item.GID);
      break;
  }
  if (changeAttrib)
  {
    if (item.IsDir())
      attrib |= FILE_ATTRIBUTE_DIRECTORY;
    else if ((attrib & FILE_ATTRIBUTE_WIN_MASK) == 0)
      attrib |= FILE_ATTRIBUTE_NORMAL;
    if (m_FileInfoType == NFileInfoType::kWindows)
      mode = 0;
    else if (m_FileInfoType == NFileInfoType::kUnixWithAttrib)
      attrib |= FILE_ATTRIBUTE_UNIX_EXTENSION;
  }
  else if (m_FileInfoType == NFileInfoType::kUnix)
    attrib = 0;
  if (m_SetIzAttrib)
    attrib &= ~(FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_UNIX_EXTENSION);
  item.ExternalAttrib = attrib | ((UInt32)mode << 16);
}

}}

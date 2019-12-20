// ZipHeaderRebel.cpp

#include "StdAfx.h"

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
  return extraSize;
}

static void SetExtraSubBlocks(const CObjectVector<CExtraSubBlock> &subBlocks,
    const CObjectVector<UInt16> &exclusiveIDs, bool copyAll, CExtraBlock &extra)
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

void CHeaderExtraStorage::CopyExtraSubBlocks(const CItem &item, const CObjectVector<UInt16> &exclusiveIDs, bool copyAll)
{
  m_LocalExtraUnknown.Clear();
  m_CentralExtraUnknown.Clear();
  m_HasWzAesField = item.LocalExtra.GetWzAes(m_WzAesField);
  SetExtraSubBlocks(item.LocalExtra.SubBlocks, exclusiveIDs, copyAll, m_LocalExtraUnknown);
  SetExtraSubBlocks(item.LocalExtra.SubBlocks, exclusiveIDs, copyAll, m_CentralExtraUnknown);
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

void CHeaderRebel::ChangeLocalHeader(CItemOut &item, CHeaderExtraStorage &extraStorage)
{
  bool NtfsTimeIsStored = false;
  bool UnixTimeIsStored = false;
  bool UnixAcTimeIsStored = false;
  bool UnixCrTimeIsStored = false;
  if (m_TimeType != NTimeType::kDosTimeZero)
  {
    if (m_TimeType == NTimeType::kDefault)
    {
      NtfsTimeIsStored = item.NtfsTimeIsDefined;
      UnixTimeIsStored = item.UnixTimeIsDefined;
      UnixAcTimeIsStored = item.UnixAcTimeIsDefined;
      UnixCrTimeIsStored = item.UnixCrTimeIsDefined;
    }
    else if (m_TimeType == NTimeType::kNtfsExtra)
      NtfsTimeIsStored = true;
    else if (m_TimeType == NTimeType::kUnixTimeExtra)
    {
      UnixTimeIsStored= true;
      UnixAcTimeIsStored= true;
    }

    m_HeaderLocale->UpdateTimestamp(item);

    if (NtfsTimeIsStored)
    {
      if (!item.NtfsTimeIsDefined)
      {
        if (!item.UnixAcTimeIsDefined) item.Ntfs_ATime = item.Ntfs_MTime;
        if (!item.UnixCrTimeIsDefined) item.Ntfs_CTime = item.Ntfs_MTime;
      }
    }
    else if (UnixTimeIsStored)
    {
      if (!item.NtfsTimeIsDefined && !item.UnixAcTimeIsDefined)
        item.Ntfs_ATime = item.Ntfs_MTime;
      SetUnixTimeExtraField(item.Ntfs_MTime, NUnixTime::kMTime, extraStorage);
      if (UnixAcTimeIsStored) SetUnixTimeExtraField(item.Ntfs_ATime, NUnixTime::kATime, extraStorage);
      if (UnixCrTimeIsStored) SetUnixTimeExtraField(item.Ntfs_CTime, NUnixTime::kCTime, extraStorage);
    }
  }
  else
    item.Time = 0;
  item.NtfsTimeIsDefined = NtfsTimeIsStored;
  item.UnixTimeIsDefined = UnixTimeIsStored;
  item.UnixAcTimeIsDefined = UnixAcTimeIsStored;
  item.UnixCrTimeIsDefined = UnixCrTimeIsStored;
}

}}

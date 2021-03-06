// Archive/ZipItem.cpp

#include "StdAfx.h"

#include "../../../../C/CpuArch.h"
#include "../../../../C/7zCrc.h"

#ifdef ZIP_HEADER_REBEL
#include "../../../Common/IntToString.h"
#endif
#include "../../../Common/MyLinux.h"
#include "../../../Common/StringConvert.h"
#include "../Common/ItemNameUtils.h"

#include "ZipItem.h"

namespace NArchive {
namespace NZip {

using namespace NFileHeader;

bool CExtraSubBlock::ExtractNtfsTime(unsigned index, FILETIME &ft) const
{
  ft.dwHighDateTime = ft.dwLowDateTime = 0;
  UInt32 size = (UInt32)Data.Size();
  if (ID != NExtraID::kNTFS || size < 32)
    return false;
  const Byte *p = (const Byte *)Data;
  p += 4; // for reserved
  size -= 4;
  while (size > 4)
  {
    UInt16 tag = GetUi16(p);
    unsigned attrSize = GetUi16(p + 2);
    p += 4;
    size -= 4;
    if (attrSize > size)
      attrSize = size;
    
    if (tag == NNtfsExtra::kTagTime && attrSize >= 24)
    {
      p += 8 * index;
      ft.dwLowDateTime = GetUi32(p);
      ft.dwHighDateTime = GetUi32(p + 4);
      return true;
    }
    p += attrSize;
    size -= attrSize;
  }
  return false;
}

#ifndef ZIP_HEADER_REBEL
bool CExtraSubBlock::ExtractUnixTime(bool isCentral, unsigned index, UInt32 &res) const
{
  res = 0;
  UInt32 size = (UInt32)Data.Size();
  if (ID != NExtraID::kUnixTime || size < 5)
    return false;
  const Byte *p = (const Byte *)Data;
  Byte flags = *p++;
  size--;
  if (isCentral)
  {
    if (index != TS_MODTIME || (flags & (1 << TS_MODTIME)) == 0 || size < 4)
      return false;
    res = GetUi32(p);
    return true;
  }
  for (unsigned i = 0; i < 3; i++)
    if ((flags & (1 << i)) != 0)
    {
      if (size < 4)
        return false;
      if (index == i)
      {
        res = GetUi32(p);
        return true;
      }
      p += 4;
      size -= 4;
    }
  return false;
}
#else
bool CUnixTimeExtra::ParseFromSubBlock(const CExtraSubBlock &sb)
{
  if (sb.ID == NExtraID::kUnixTime && sb.Data.Size() >= 1)
  {
    unsigned needSize = k_UnixTimeExtra_OnlyModTimeLength;
    const Byte *p = (const Byte *)sb.Data;
    Byte flags = *p++;
    for (unsigned i = 0; i < TS_PARSIZE; i++)
    {
      bool isTimePresent = (flags & (1 << i)) != 0;
      if (isTimePresent && sb.Data.Size() >= needSize)
      {
        EpochTimes[i] = GetUi32(p);
        p += k_UnixTimeExtra_EpochTimeLength;
        needSize += k_UnixTimeExtra_EpochTimeLength;
      }
      else
        EpochTimes[i] = -1;
      IsTimePresent[i] = isTimePresent;
    }
  }
  else
    InitTimeFields();
  return IsTimePresent[TS_MODTIME] && EpochTimes[TS_MODTIME] >= 0;
}

void CUnixTimeExtra::SetSubBlock(CExtraSubBlock &sb, bool toLocal) const
{
  unsigned len = 1;
  Byte flags = 0;
  for (unsigned i = 0; i < TS_PARSIZE; i++)
  {
    if (IsTimePresent[i])
    {
      flags |= (Byte)(1 << i);
      if (toLocal || i == TS_MODTIME)
        len += k_UnixTimeExtra_EpochTimeLength;
    }
  }
  sb.ID = NExtraID::kUnixTime;
  sb.Data.Alloc(len);
  Byte *p = (Byte *)sb.Data;
  *p++ = flags;
  for (unsigned i = 0; i < TS_PARSIZE; i++)
  {
    for (unsigned j = 0; j < k_UnixTimeExtra_EpochTimeLength; j++)
      p[j] = (Byte)(EpochTimes[i] >> (8 * j));
    p += k_UnixTimeExtra_EpochTimeLength;
  }
}

bool CUnixFileOwnershipExtra::ParseFromSubBlock(const CExtraSubBlock &sb)
{
  if (sb.ID != NExtraID::kUnixFileOwnership || sb.Data.Size() < 2)
    return false;
  unsigned needSize = 2;
  const Byte *p = (const Byte *)sb.Data;
  *p++;  // Version
  for (unsigned i = 0; i < OWNER_PARSIZE; i++)
  {
    unsigned len = *p++;
    needSize += len;
    if (sb.Data.Size() < needSize || len > k_UnixFileOwnershipExtra_OwnerIDLength)
    {
      InitIDFields();
      return false;
    }
    UInt32 id = 0;
    for (int j = len - 1; j >= 0; j--)
      id = (id << 8) + p[j];
    OwnerIDs[i] = id;
    p += len;
    needSize++;
  }
  return true;
}

void CUnixFileOwnershipExtra::SetSubBlock(CExtraSubBlock &sb) const
{
  sb.ID = NExtraID::kUnixFileOwnership;
  sb.Data.Alloc(k_UnixFileOwnershipExtra_DataBlockLength);
  Byte *p = (Byte *)sb.Data;
  *p++ = 1;  // Version
  for (unsigned i = 0; i < OWNER_PARSIZE; i++)
  {
    *p++ = k_UnixFileOwnershipExtra_OwnerIDLength;
    for (unsigned j = 0; j < k_UnixFileOwnershipExtra_OwnerIDLength; j++)
      p[j] = (Byte)(OwnerIDs[i] >> (8 * j));
    p += k_UnixFileOwnershipExtra_OwnerIDLength;
  }
}
#endif


bool CExtraBlock::GetNtfsTime(unsigned index, FILETIME &ft) const
{
  FOR_VECTOR (i, SubBlocks)
  {
    const CExtraSubBlock &sb = SubBlocks[i];
    if (sb.ID == NFileHeader::NExtraID::kNTFS)
      return sb.ExtractNtfsTime(index, ft);
  }
  return false;
}

#ifndef ZIP_HEADER_REBEL
bool CExtraBlock::GetUnixTime(bool isCentral, unsigned index, UInt32 &res) const
{
  FOR_VECTOR (i, SubBlocks)
  {
    const CExtraSubBlock &sb = SubBlocks[i];
    if (sb.ID == NFileHeader::NExtraID::kUnixTime)
      return sb.ExtractUnixTime(isCentral, index, res);
  }
  return false;
}
#endif


bool CLocalItem::IsDir() const
{
  return NItemName::HasTailSlash(Name, GetCodePage());
}

bool CItem::IsDir() const
{
  if (NItemName::HasTailSlash(Name, GetCodePage()))
    return true;
  
  Byte hostOS = GetHostOS();

  if (Size == 0 && PackSize == 0 && !Name.IsEmpty() && Name.Back() == '\\')
  {
    // do we need to use CharPrevExA?
    // .NET Framework 4.5 : System.IO.Compression::CreateFromDirectory() probably writes backslashes to headers?
    // so we support that case
    switch (hostOS)
    {
      case NHostOS::kFAT:
      case NHostOS::kNTFS:
      case NHostOS::kHPFS:
      case NHostOS::kVFAT:
        return true;
    }
  }

  if (!FromCentral)
    return false;
  
  UInt16 highAttrib = (UInt16)((ExternalAttrib >> 16 ) & 0xFFFF);

  switch (hostOS)
  {
    case NHostOS::kAMIGA:
      switch (highAttrib & NAmigaAttrib::kIFMT)
      {
        case NAmigaAttrib::kIFDIR: return true;
        case NAmigaAttrib::kIFREG: return false;
        default: return false; // change it throw kUnknownAttributes;
      }
    case NHostOS::kFAT:
    case NHostOS::kNTFS:
    case NHostOS::kHPFS:
    case NHostOS::kVFAT:
      return ((ExternalAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0);
    case NHostOS::kAtari:
    case NHostOS::kMac:
    case NHostOS::kVMS:
    case NHostOS::kVM_CMS:
    case NHostOS::kAcorn:
    case NHostOS::kMVS:
      return false; // change it throw kUnknownAttributes;
    case NHostOS::kUnix:
      return MY_LIN_S_ISDIR(highAttrib);
    default:
      return false;
  }
}

UInt32 CItem::GetWinAttrib(
    #ifdef ZIP_HEADER_REBEL
    UInt16 fileInfoType
    #endif
    ) const
{
  #ifdef ZIP_HEADER_REBEL
  if (fileInfoType == NFileInfoType::kWindows)
    return ExternalAttrib & NAttrib::kWinAttribMask;
  else if (fileInfoType == NFileInfoType::kUnix)
    return ExternalAttrib & NAttrib::kUnixModeMask;
  else if (fileInfoType == NFileInfoType::kUnixWithAttrib)
    return ExternalAttrib;
  #endif
  UInt32 winAttrib = 0;
  switch (GetHostOS())
  {
    case NHostOS::kFAT:
    case NHostOS::kNTFS:
      if (FromCentral)
        winAttrib = ExternalAttrib;
      break;
#ifdef FILE_ATTRIBUTE_UNIX_EXTENSION
    case NFileHeader::NHostOS::kUnix:
        winAttrib = (ExternalAttrib & 0xFFFF0000) | FILE_ATTRIBUTE_UNIX_EXTENSION; 
        if (winAttrib & (MY_LIN_S_IFDIR << 16))
		winAttrib |= FILE_ATTRIBUTE_DIRECTORY;
        return winAttrib;
#endif
  }
  if (IsDir()) // test it;
    winAttrib |= FILE_ATTRIBUTE_DIRECTORY;
  return winAttrib;
}

bool CItem::GetPosixAttrib(UInt32 &attrib) const
{
  // some archivers can store PosixAttrib in high 16 bits even with HostOS=FAT.
  if (FromCentral && GetHostOS() == NHostOS::kUnix)
  {
    attrib = ExternalAttrib >> 16;
    return (attrib != 0);
  }
  attrib = 0;
  if (IsDir())
    attrib = MY_LIN_S_IFDIR;
  return false;
}

void CItem::GetUnicodeString(UString &res, const AString &s, bool isComment, bool useSpecifiedCodePage, UINT codePage) const
{
  bool isUtf8 = IsUtf8();
  bool ignore_Utf8_Errors = true;
  
  if (!isUtf8)
  {
    {
      const unsigned id = isComment ?
          NFileHeader::NExtraID::kIzUnicodeComment:
          NFileHeader::NExtraID::kIzUnicodeName;
      const CObjectVector<CExtraSubBlock> &subBlocks = GetMainExtra().SubBlocks;
      
      FOR_VECTOR (i, subBlocks)
      {
        const CExtraSubBlock &sb = subBlocks[i];
        if (sb.ID == id)
        {
          AString utf;
          if (sb.ExtractIzUnicode(CrcCalc(s, s.Len()), utf))
            if (ConvertUTF8ToUnicode(utf, res))
              return;
          break;
        }
      }
    }
    
    if (useSpecifiedCodePage)
      isUtf8 = (codePage == CP_UTF8);
    #ifdef _WIN32
    else if (GetHostOS() == NFileHeader::NHostOS::kUnix)
    {
      /* Some ZIP archives in Unix use UTF-8 encoding without Utf8 flag in header.
         We try to get name as UTF-8.
         Do we need to do it in POSIX version also? */
      isUtf8 = true;
      ignore_Utf8_Errors = false;
    }
    #endif
  }
  
  
  if (isUtf8)
    if (ConvertUTF8ToUnicode(s, res) || ignore_Utf8_Errors)
      return;
  
  MultiByteToUnicodeString2(res, s, useSpecifiedCodePage ? codePage : GetCodePage());
}

}}

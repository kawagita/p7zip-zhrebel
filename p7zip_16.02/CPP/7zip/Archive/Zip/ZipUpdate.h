// ZipUpdate.h

#ifndef __ZIP_UPDATE_H
#define __ZIP_UPDATE_H

#include "../../ICoder.h"
#include "../IArchive.h"

#include "../../Common/CreateCoder.h"

#include "ZipCompressionMode.h"
#include "ZipIn.h"
#ifdef ZIP_HEADER_REBEL
#include "ZipHeaderRebel.h"
#endif

namespace NArchive {
namespace NZip {

struct CUpdateRange
{
  UInt64 Position;
  UInt64 Size;
  
  // CUpdateRange() {};
  CUpdateRange(UInt64 position, UInt64 size): Position(position), Size(size) {};
};

struct CUpdateItem
{
  bool NewData;
  bool NewProps;
  bool IsDir;
  bool NtfsTimeIsDefined;
  #ifdef ZIP_HEADER_REBEL
  bool UnixTimeIsDefined;
  bool UnixAcTimeIsDefined;
  bool UnixCrTimeIsDefined;
  #endif
  bool IsUtf8;
  int IndexInArc;
  int IndexInClient;
  UInt32 UID;
  UInt32 GID;
  UInt32 Attrib;
  UInt32 Time;
  UInt64 Size;
  AString Name;
  // bool Commented;
  // CUpdateRange CommentRange;
  FILETIME Ntfs_MTime;
  FILETIME Ntfs_ATime;
  FILETIME Ntfs_CTime;

  CUpdateItem():
      NtfsTimeIsDefined(false),
      #ifdef ZIP_HEADER_REBEL
      UnixTimeIsDefined(false),
      UnixAcTimeIsDefined(false),
      UnixCrTimeIsDefined(false),
      #endif
      IsUtf8(false),
      Size(0)
      {}
};

HRESULT Update(
    DECL_EXTERNAL_CODECS_LOC_VARS
    const CObjectVector<CItemEx> &inputItems,
    CObjectVector<CUpdateItem> &updateItems,
    ISequentialOutStream *seqOutStream,
    CInArchive *inArchive, bool removeSfx,
    CCompressionMethodMode *compressionMethodMode,
    #ifdef ZIP_HEADER_REBEL
    CHeaderRebel *headerRebel,
    #endif
    IArchiveUpdateCallback *updateCallback);

}}

#endif

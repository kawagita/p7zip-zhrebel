// ZipHeader.h

#ifndef __ARCHIVE_ZIP_HEADER_H
#define __ARCHIVE_ZIP_HEADER_H

#ifdef ZIP_HEADER_REBEL
#include <sys/types.h>

#include "../../../Common/MyVector.h"
#include "../../../Common/MyWindows.h"
#endif
#include "../../../Common/MyTypes.h"

namespace NArchive {
namespace NZip {

const unsigned kMarkerSize = 4;

namespace NSignature
{
  const UInt32 kLocalFileHeader   = 0x04034B50;
  const UInt32 kDataDescriptor    = 0x08074B50;
  const UInt32 kCentralFileHeader = 0x02014B50;
  const UInt32 kEcd               = 0x06054B50;
  const UInt32 kEcd64             = 0x06064B50;
  const UInt32 kEcd64Locator      = 0x07064B50;
  const UInt32 kSpan              = 0x08074B50;
  const UInt32 kNoSpan            = 0x30304B50; // PK00, replaces kSpan, if there is only 1 segment
}

const unsigned kLocalHeaderSize = 4 + 26; // including signature
const unsigned kDataDescriptorSize = 4 + 12;  // including signature
const unsigned kCentralHeaderSize = 4 + 42; // including signature

const unsigned kEcdSize = 22; // including signature
const unsigned kEcd64_MainSize = 44;
const unsigned kEcd64_FullSize = 12 + kEcd64_MainSize;
const unsigned kEcd64Locator_Size = 20;

namespace NFileHeader
{
  namespace NCompressionMethod
  {
    enum EType
    {
      kStored = 0,
      kShrunk = 1,
      kReduced1 = 2,
      kReduced2 = 3,
      kReduced3 = 4,
      kReduced4 = 5,
      kImploded = 6,
      kReservedTokenizing = 7, // reserved for tokenizing
      kDeflated = 8,
      kDeflated64 = 9,
      kPKImploding = 10,
      
      kBZip2 = 12,
      kLZMA = 14,
      kTerse = 18,
      kLz77 = 19,
      
      kXz = 0x5F,
      kJpeg = 0x60,
      kWavPack = 0x61,
      kPPMd = 0x62,
      kWzAES = 0x63
    };

    const Byte kMadeByProgramVersion = 63;
    
    const Byte kExtractVersion_Default = 10;
    const Byte kExtractVersion_Dir = 20;
    const Byte kExtractVersion_ZipCrypto = 20;
    const Byte kExtractVersion_Deflate = 20;
    const Byte kExtractVersion_Deflate64 = 21;
    const Byte kExtractVersion_Zip64 = 45;
    const Byte kExtractVersion_BZip2 = 46;
    const Byte kExtractVersion_Aes = 51;
    const Byte kExtractVersion_LZMA = 63;
    const Byte kExtractVersion_PPMd = 63;
  }

  namespace NExtraID
  {
    enum
    {
      kZip64 = 0x01,
      kNTFS = 0x0A,
      kStrongEncrypt = 0x17,
      kUnixTime = 0x5455,
      kUnixFileOwnership = 0x7875,
      kIzUnicodeComment = 0x6375,
      kIzUnicodeName = 0x7075,
      kWzAES = 0x9901
    };
  }

  namespace NNtfsExtra
  {
    const UInt16 kTagTime = 1;
    #ifndef ZIP_HEADER_REBEL
    enum
    {
      kMTime = 0,
      kATime,
      kCTime
    };
    #endif
  }

  namespace NFlags
  {
    const unsigned kEncrypted = 1 << 0;
    const unsigned kLzmaEOS = 1 << 1;
    const unsigned kDescriptorUsedMask = 1 << 3;
    const unsigned kStrongEncrypted = 1 << 6;
    const unsigned kUtf8 = 1 << 11;

    const unsigned kImplodeDictionarySizeMask = 1 << 1;
    const unsigned kImplodeLiteralsOnMask     = 1 << 2;
    
    const unsigned kDeflateTypeBitStart = 1;
    const unsigned kNumDeflateTypeBits = 2;
    const unsigned kNumDeflateTypes = (1 << kNumDeflateTypeBits);
    const unsigned kDeflateTypeMask = (1 << kNumDeflateTypeBits) - 1;
  }
  
  namespace NHostOS
  {
    enum EEnum
    {
      kFAT      =  0,
      kAMIGA    =  1,
      kVMS      =  2,  // VAX/VMS
      kUnix     =  3,
      kVM_CMS   =  4,
      kAtari    =  5,  // what if it's a minix filesystem? [cjh]
      kHPFS     =  6,  // filesystem used by OS/2 (and NT 3.x)
      kMac      =  7,
      kZ_System =  8,
      kCPM      =  9,
      kTOPS20   = 10,  // pkzip 2.50 NTFS
      kNTFS     = 11,  // filesystem used by Windows NT
      kQDOS     = 12,  // SMS/QDOS
      kAcorn    = 13,  // Archimedes Acorn RISC OS
      kVFAT     = 14,  // filesystem used by Windows 95, NT
      kMVS      = 15,
      kBeOS     = 16,  // hybrid POSIX/database filesystem
      kTandem   = 17,
      kOS400    = 18,
      kOSX      = 19
    };
  }


  namespace NAmigaAttrib
  {
    const UInt32 kIFMT     = 06000;    // Amiga file type mask
    const UInt32 kIFDIR    = 04000;    // Amiga directory
    const UInt32 kIFREG    = 02000;    // Amiga regular file
    const UInt32 kIHIDDEN  = 00200;    // to be supported in AmigaDOS 3.x
    const UInt32 kISCRIPT  = 00100;    // executable script (text command file)
    const UInt32 kIPURE    = 00040;    // allow loading into resident memory
    const UInt32 kIARCHIVE = 00020;    // not modified since bit was last set
    const UInt32 kIREAD    = 00010;    // can be opened for reading
    const UInt32 kIWRITE   = 00004;    // can be opened for writing
    const UInt32 kIEXECUTE = 00002;    // executable image, a loadable runfile
    const UInt32 kIDELETE  = 00001;    // can be deleted
  }

  #ifdef ZIP_HEADER_REBEL
  namespace NAttrib
  {
    const UInt32 kUnixModeMask           = 0xFFFF0000;
    const UInt32 kWinAttribReadOnly      = FILE_ATTRIBUTE_READONLY;
    const UInt32 kWinAttribArchive       = FILE_ATTRIBUTE_ARCHIVE;
    const UInt32 kWinAttribSystem        = FILE_ATTRIBUTE_SYSTEM;
    const UInt32 kWinAttribHidden        = FILE_ATTRIBUTE_HIDDEN;
    const UInt32 kWinAttribNotIndexed    = FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
    const UInt32 kWinAttribDirectory     = FILE_ATTRIBUTE_DIRECTORY;
    const UInt32 kWinAttribNormal        = FILE_ATTRIBUTE_NORMAL;
    const UInt32 kWinAttribUnixExtension = FILE_ATTRIBUTE_UNIX_EXTENSION;
    const UInt32 kWinAttribMask          = ~(kUnixModeMask | kWinAttribUnixExtension);
    const UInt32 kWinAttribIzModeMask    = ~(kWinAttribNormal | kWinAttribUnixExtension);
  }

  namespace NTimeType
  {
    enum
    {
      kDefault = 0,
      kNtfsExtra,
      kUnixTimeExtra,
      kDosTimeOnly,
      kDosTimeZero
    };
  }

  namespace NFileInfoType
  {
    const unsigned kWithAttribMask = 1 << 0;
    const unsigned kUnixMask = 1 << 1;
    enum
    {
      kDefault = 0,
      kWindows = 1,
      kUnix = 2,
      kUnixWithAttrib = 3
    };
  }

  namespace NSetPropType
  {
    enum
    {
      kInitialization = 0,
      kLocale,
      kDecodingComment,
      kTimeType,
      kTimeZone,
      kTimestampFromModTime,
      kTimestamp,
      kFileInfoType,
      kFileInfoIzMode,
      kFileAttrib,
      kFilePermissions,
      kFileOwnership,
      kDataDescriptor,
      kCopyExtraAll,
      kExtraAddedID,
      kExtraDeletedID
    };
  }
  #endif
}

#ifdef ZIP_HEADER_REBEL
struct CFileHeaderInfo
{
  const UInt32 ProcUserID;
  const UInt32 ProcGroupID;

  UInt32 AttribFlags[ATTR_PARSIZE];
  UInt32 UnixModeBits;
  UInt32 UnixUID;
  UInt32 UnixGID;
  bool SetTimestampFromModTime;
  bool SetUnixModeBits;
  bool SetUnixUID;
  bool SetUnixGID;
  bool SetUnixFileOwnership;
  bool CopyUnixFileOwnership;
  bool CopyExtraAll;
  CObjectVector<UInt16> ExtraAddedIDs;
  CObjectVector<UInt16> ExtraDeletedIDs;

  void InitHeaderInfo()
  {
    for (unsigned i = 0; i < ATTR_PARSIZE; i++)
      AttribFlags[i] = 0;
    UnixModeBits = 0;
    UnixUID = 0;
    UnixGID = 0;
    SetTimestampFromModTime = false;
    SetUnixModeBits = false;
    SetUnixUID = false;
    SetUnixGID = false;
    SetUnixFileOwnership = false;
    CopyUnixFileOwnership = true;
    CopyExtraAll = false;
    ExtraAddedIDs.Clear();
    ExtraDeletedIDs.Clear();
  }

  CFileHeaderInfo(): ProcUserID(getuid()), ProcGroupID(getgid())
  {
    InitHeaderInfo();
  }
};
#endif

}}

#endif

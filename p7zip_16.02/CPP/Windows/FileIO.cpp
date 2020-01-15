// Windows/FileIO.cpp

#include "StdAfx.h"

#include "FileIO.h"
#include "Defs.h"
#include "TimeUtils.h"
#include "../Common/StringConvert.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define NEED_NAME_WINDOWS_TO_UNIX
#include "myPrivate.h"

#include <sys/types.h>
#include <utime.h>

#ifdef ENV_HAVE_LSTAT
#define FD_LINK (-2)
#endif

#define GENERIC_READ	0x80000000
#define GENERIC_WRITE	0x40000000

#ifdef ENV_HAVE_LSTAT
extern "C"
{
int global_use_lstat=1; // default behaviour : p7zip stores symlinks instead of dumping the files they point to
}
#endif

namespace NWindows {
namespace NFile {
namespace NIO {

CFileBase::~CFileBase()
{
  Close();
}

bool CFileBase::Create(CFSTR filename, DWORD dwDesiredAccess,
    DWORD dwShareMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,bool ignoreSymbolicLink)
{
  Close();
  
  int   flags = 0;
#ifdef USE_UNICODE_FSTRING
   AString astr = UnicodeStringToMultiByte(filename);
   const char * name = nameWindowToUnix((const char *)astr);
#else
  const char * name = nameWindowToUnix(filename);
#endif

#ifdef O_BINARY
  flags |= O_BINARY;
#endif

#ifdef O_LARGEFILE
  flags |= O_LARGEFILE;
#endif

  /* now use the umask value */
  int mask = umask(0);
  (void)umask(mask);
  int mode = 0666 & ~(mask & 066); /* keep the R/W for the user */

  if (dwDesiredAccess & GENERIC_WRITE) flags |= O_WRONLY;
  if (dwDesiredAccess & GENERIC_READ)  flags |= O_RDONLY;


  switch (dwCreationDisposition)
  {
    case CREATE_NEW    : flags |= O_CREAT | O_EXCL; break;
    case CREATE_ALWAYS : flags |= O_CREAT;          break;
    case OPEN_EXISTING :                            break;
    case OPEN_ALWAYS   : flags |= O_CREAT;          break;
    // case TRUNCATE_EXISTING : flags |= O_TRUNC;      break;
  }
  // printf("##DBG open(%s,0x%x,%o)##\n",name,flags,(unsigned)mode);

  _fd = -1;
#ifdef ENV_HAVE_LSTAT
   if ((global_use_lstat) && (ignoreSymbolicLink == false))
   {
     _size = readlink(name, _buffer, sizeof(_buffer)-1);
     if (_size > 0) {
       if (dwDesiredAccess & GENERIC_READ) {
         _fd = FD_LINK;
         _offset = 0;
         _buffer[_size]=0;
       } else if (dwDesiredAccess & GENERIC_WRITE) {
         // does not overwrite the file pointed by symbolic link
         if (!unlink(name)) return false;
       }
     }
  }
#endif

  if (_fd == -1) {
    _fd = open(name,flags, mode);
  }

  if ((_fd == -1) && (global_use_utf16_conversion)) {
    // bug #1204993 - Try to recover the original filename
    UString ustr = MultiByteToUnicodeString(AString(name), 0);
    AString resultString;
    int is_good = 1;
    for (int i = 0; i < ustr.Len(); i++)
    {
      if (ustr[i] >= 256) {
        is_good = 0;
        break;
      } else {
        resultString += char(ustr[i]);
      }
    }
    if (is_good) {
      _fd = open((const char *)resultString,flags, mode);
    }
  }

  if (_fd == -1) {
    /* !ENV_HAVE_LSTAT : an invalid symbolic link => errno == ENOENT */
    return false;
  } else {
    _unix_filename = name;
  }

  return true;
}

/* FIXME
bool CFileBase::Create(LPCWSTR fileName, DWORD desiredAccess,
    DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes,bool ignoreSymbolicLink)
{
  Close();
    return Create(UnicodeStringToMultiByte(fileName, CP_ACP), 
      desiredAccess, shareMode, creationDisposition, flagsAndAttributes,ignoreSymbolicLink);
}
*/

#ifdef USE_NANO_SECONDS
#define UTIME(n,t) utimes((n),(t))
#define UTIMBUF_ACTIME(buf)  (buf)[0]
#define UTIMBUF_MODTIME(buf) (buf)[1]
#define SET_UTIMBUF_ACTIME(buf,sec,ns)  UTIMBUF_ACTIME(buf).tv_sec=sec;  UTIMBUF_ACTIME(buf).tv_usec=ns/1000
#define SET_UTIMBUF_MODTIME(buf,sec,ns) UTIMBUF_MODTIME(buf).tv_sec=sec; UTIMBUF_MODTIME(buf).tv_usec=ns/1000
#else
#define UTIME(n,t) utime((n),&(t))
#define UTIMBUF_ACTIME(buf)  (buf).actime
#define UTIMBUF_MODTIME(buf) (buf).modtime
#define SET_UTIMBUF_ACTIME(buf,sec,ns)  (UTIMBUF_ACTIME(buf)=sec)
#define SET_UTIMBUF_MODTIME(buf,sec,ns) (UTIMBUF_MODTIME(buf)=sec)
#endif
#define COPY_UTIMBUF_ACTIME(buf,st) \
        SET_UTIMBUF_ACTIME(buf,TIMESPEC_SECONDS(STAT_ATIME(st)), TIMESPEC_NANO_SECONDS(STAT_ATIME(st)))
#define COPY_UTIMBUF_MODTIME(buf,st) \
        SET_UTIMBUF_ACTIME(buf,TIMESPEC_SECONDS(STAT_MTIME(st)), TIMESPEC_NANO_SECONDS(STAT_MTIME(st)))

bool CFileBase::Close()
{
  #ifdef USE_NANO_SECONDS
  struct timeval buf[2];
  #else
  struct utimbuf buf;
  #endif

  SET_UTIMBUF_ACTIME(buf, TIMESPEC_SECONDS(_lastAccessTime), TIMESPEC_NANO_SECONDS(_lastAccessTime));
  SET_UTIMBUF_MODTIME(buf, TIMESPEC_SECONDS(_lastWriteTime), TIMESPEC_NANO_SECONDS(_lastWriteTime));

  SET_TIMESPEC(_lastAccessTime, (time_t)-1, 0);
  SET_TIMESPEC(_lastWriteTime, (time_t)-1, 0);

  if(_fd == -1)
    return true;

#ifdef ENV_HAVE_LSTAT
  if(_fd == FD_LINK) {
    _fd = -1;
    return true;
  }
#endif

  int ret = ::close(_fd);
  if (ret == 0) {
    _fd = -1;

    /* On some OS (mingwin, MacOSX ...), you must close the file before updating times */
    if ((TIMESPEC_SECONDS(UTIMBUF_ACTIME(buf))  != (time_t)-1) ||
        (TIMESPEC_SECONDS(UTIMBUF_MODTIME(buf)) != (time_t)-1)) {
      struct stat    oldbuf;
      int ret = stat((const char*)(_unix_filename),&oldbuf);
      if (ret == 0) {
        if (TIMESPEC_SECONDS(UTIMBUF_ACTIME(buf))  == (time_t)-1) COPY_UTIMBUF_ACTIME(buf, oldbuf);
        if (TIMESPEC_SECONDS(UTIMBUF_MODTIME(buf)) == (time_t)-1) COPY_UTIMBUF_MODTIME(buf, oldbuf);
      } else {
        time_t current_time = time(0);
        if (TIMESPEC_SECONDS(UTIMBUF_ACTIME(buf))  == (time_t)-1) SET_UTIMBUF_ACTIME(buf, current_time, 0);
        if (TIMESPEC_SECONDS(UTIMBUF_MODTIME(buf)) == (time_t)-1) SET_UTIMBUF_MODTIME(buf, current_time, 0);
      }
      /* ret = */ UTIME((const char *)(_unix_filename), buf);
    }
    return true;
  }
  return false;
}

bool CFileBase::GetLength(UINT64 &length) const
{
  if (_fd == -1)
  {
     SetLastError( ERROR_INVALID_HANDLE );
     return false;
  }

#ifdef ENV_HAVE_LSTAT  
  if (_fd == FD_LINK) {
    length = _size;
    return true;
  }
#endif

  off_t pos_cur = ::lseek(_fd, 0, SEEK_CUR);
  if (pos_cur == (off_t)-1)
    return false;

  off_t pos_end = ::lseek(_fd, 0, SEEK_END);
  if (pos_end == (off_t)-1)
    return false;

  off_t pos_cur2 = ::lseek(_fd, pos_cur, SEEK_SET);
  if (pos_cur2 == (off_t)-1)
    return false;

  length = (UINT64)pos_end;

  return true;
}

bool CFileBase::Seek(INT64 distanceToMove, DWORD moveMethod, UINT64 &newPosition)
{
  if (_fd == -1)
  {
     SetLastError( ERROR_INVALID_HANDLE );
     return false;
  }

#ifdef ENV_HAVE_LSTAT
  if (_fd == FD_LINK) {
    INT64 offset;
    switch (moveMethod) {
    case STREAM_SEEK_SET : offset = distanceToMove; break;
    case STREAM_SEEK_CUR : offset = _offset + distanceToMove; break;
    case STREAM_SEEK_END : offset = _size + distanceToMove; break;
    default :  offset = -1;
    }
    if (offset < 0) {
      SetLastError( EINVAL );
      return false;
    }
    if (offset > _size) offset = _size;
    newPosition = _offset = offset;
    return true;
  }
#endif

  bool ret = true;

  off_t pos = (off_t)distanceToMove;

  off_t newpos = ::lseek(_fd,pos,moveMethod);

  if (newpos == ((off_t)-1)) {
    ret = false;
  } else {
    newPosition = (UINT64)newpos;
  }

  return ret;
}

bool CFileBase::Seek(UINT64 position, UINT64 &newPosition)
{
  return Seek(position, FILE_BEGIN, newPosition);
}

/////////////////////////
// CInFile

bool CInFile::Open(CFSTR fileName, DWORD shareMode, 
    DWORD creationDisposition,  DWORD flagsAndAttributes)
{
  return Create(fileName, GENERIC_READ, shareMode, 
      creationDisposition, flagsAndAttributes);
}

bool CInFile::Open(CFSTR fileName,bool ignoreSymbolicLink)
{
  return Create(fileName, GENERIC_READ , FILE_SHARE_READ, OPEN_EXISTING, 
     FILE_ATTRIBUTE_NORMAL,ignoreSymbolicLink);
}

// ReadFile and WriteFile functions in Windows have BUG:
// If you Read or Write 64MB or more (probably min_failure_size = 64MB - 32KB + 1) 
// from/to Network file, it returns ERROR_NO_SYSTEM_RESOURCES 
// (Insufficient system resources exist to complete the requested service).

// static UINT32 kChunkSizeMax = (1 << 24);

bool CInFile::ReadPart(void *data, UINT32 size, UINT32 &processedSize)
{
  // if (size > kChunkSizeMax)
  //  size = kChunkSizeMax;
  return Read(data,size,processedSize);
}

bool CInFile::Read(void *buffer, UINT32 bytesToRead, UINT32 &bytesRead)
{
  if (_fd == -1)
  {
     SetLastError( ERROR_INVALID_HANDLE );
     return false;
  }

  if (bytesToRead == 0) {
    bytesRead =0;
    return TRUE;
  }

#ifdef ENV_HAVE_LSTAT
  if (_fd == FD_LINK) {
    if (_offset >= _size) {
      bytesRead = 0;
      return TRUE;
    }
    int len = (_size - _offset);
    if (len > bytesToRead) len = bytesToRead;
    memcpy(buffer,_buffer+_offset,len);
    bytesRead = len;
    _offset += len;
    return TRUE;
  }
#endif

  ssize_t  ret;
  do {
    ret = read(_fd,buffer,bytesToRead);
  } while (ret < 0 && (errno == EINTR));

  if (ret != -1) {
    bytesRead = ret;
    return TRUE;
  }
  bytesRead =0;
  return FALSE;
}

bool CInFile::ReadSymLink(UString &linkName)
{
#ifdef ENV_HAVE_LSTAT
  if (_fd == FD_LINK) {
    MultiByteToUnicodeString2(linkName, AString(_buffer));
    return true;
  }
#endif
  return false;
}

/////////////////////////
// COutFile

bool COutFile::Open(CFSTR fileName, DWORD shareMode, 
    DWORD creationDisposition, DWORD flagsAndAttributes)
{
  return CFileBase::Create(fileName, GENERIC_WRITE, shareMode, 
      creationDisposition, flagsAndAttributes);
}

static inline DWORD GetCreationDisposition(bool createAlways)
  {  return createAlways? CREATE_ALWAYS: CREATE_NEW; }

bool COutFile::Open(CFSTR fileName, DWORD creationDisposition)
{
  return Open(fileName, FILE_SHARE_READ, 
      creationDisposition, FILE_ATTRIBUTE_NORMAL);
}

bool COutFile::Create(CFSTR fileName, bool createAlways)
{
  return Open(fileName, GetCreationDisposition(createAlways));
}

bool COutFile::CreateAlways(CFSTR fileName, DWORD /* flagsAndAttributes */ )
{
  return Open(fileName, true); // FIXME
}

bool COutFile::SetTime(const FILETIME *cTime, const FILETIME *aTime, const FILETIME *mTime) throw()
{
  if (_fd == -1) {
     SetLastError( ERROR_INVALID_HANDLE );
     return false;
  }

  /* On some OS (cygwin, MacOSX ...), you must close the file before updating times */
  if (aTime) {
     NTime::FileTimeToTimespec(*aTime, _lastAccessTime);
  }
  if (mTime) {
     NTime::FileTimeToTimespec(*mTime, _lastWriteTime);
  }

  return true;
}

bool COutFile::SetMTime(const FILETIME *mTime) throw()
{
  return SetTime(NULL, NULL, mTime);
}

bool COutFile::WritePart(const void *data, UINT32 size, UINT32 &processedSize) throw()
{
//  if (size > kChunkSizeMax)
//    size = kChunkSizeMax;

  return Write(data,size,processedSize);
}

bool COutFile::Write(const void *buffer, UINT32 bytesToWrite, UINT32 &bytesWritten) throw()
{
  if (_fd == -1)
  {
     SetLastError( ERROR_INVALID_HANDLE );
     return false;
  }

  ssize_t  ret;
  do {
    ret = write(_fd,buffer, bytesToWrite);
  } while (ret < 0 && (errno == EINTR));

  if (ret != -1) {
    bytesWritten = ret;
    return TRUE;
  }
  bytesWritten =0;
  return FALSE;
}

bool COutFile::SetEndOfFile() throw()
{
  if (_fd == -1)
  {
     SetLastError( ERROR_INVALID_HANDLE );
     return false;
  }

  bool bret = false;

  off_t pos_cur = lseek(_fd, 0, SEEK_CUR);
  if (pos_cur != (off_t)-1) {
    int iret = ftruncate(_fd, pos_cur);
    if (iret == 0) bret = true;
  }

  return bret;
}

bool COutFile::SetLength(UINT64 length) throw()
{
  UINT64 newPosition;
  if(!Seek(length, newPosition))
    return false;
  if(newPosition != length)
    return false;
  return SetEndOfFile();
}

}}}

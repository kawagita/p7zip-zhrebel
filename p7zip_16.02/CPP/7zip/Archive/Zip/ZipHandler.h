// Zip/Handler.h

#ifndef __ZIP_HANDLER_H
#define __ZIP_HANDLER_H

#include "../../../Common/DynamicBuffer.h"
#include "../../ICoder.h"
#include "../IArchive.h"

#include "../../Common/CreateCoder.h"

#include "ZipIn.h"
#include "ZipCompressionMode.h"
#ifdef ZIP_HEADER_REBEL
#include "ZipHeaderLocale.h"
#endif

namespace NArchive {
namespace NZip {

class CHandler:
  public IInArchive,
  public IOutArchive,
  public ISetProperties,
  PUBLIC_ISetCompressCodecsInfo
  public CMyUnknownImp
{
public:
  MY_QUERYINTERFACE_BEGIN2(IInArchive)
  MY_QUERYINTERFACE_ENTRY(IOutArchive)
  MY_QUERYINTERFACE_ENTRY(ISetProperties)
  QUERY_ENTRY_ISetCompressCodecsInfo
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE

  INTERFACE_IInArchive(;)
  INTERFACE_IOutArchive(;)

  STDMETHOD(SetProperties)(const wchar_t * const *names, const PROPVARIANT *values, UInt32 numProps);
  STDMETHOD(SetHeaderProperties)(const wchar_t * const *names, const HEADERPROP *props, UInt32 numProps);

  DECL_ISetCompressCodecsInfo

  CHandler();
private:
  CObjectVector<CItemEx> m_Items;
  CInArchive m_Archive;

  CBaseProps _props;

  int m_MainMethod;
  bool m_ForceAesMode;
  bool m_WriteNtfsTimeExtra;
  bool _removeSfxBlock;
  bool m_ForceLocal;
  bool m_ForceUtf8;
  bool _forceCodePage;
  UInt32 _specifiedCodePage;

  #ifdef ZIP_HEADER_REBEL
  UInt16 m_HeaderFileInfoType;
  UInt16 m_HeaderTimeType;
  bool m_HeaderIzMode;
  CBoolPair m_HeaderUseDescriptor;
  CFileHeaderInfo m_HeaderInfo;
  CHeaderLocale m_HeaderLocale;
  #endif

  DECL_EXTERNAL_CODECS_VARS

  void InitMethodProps()
  {
    _props.Init();
    m_MainMethod = -1;
    m_ForceAesMode = false;
    m_WriteNtfsTimeExtra = true;
    _removeSfxBlock = false;
    m_ForceLocal = false;
    m_ForceUtf8 = false;
    _forceCodePage = false;
    _specifiedCodePage = CP_OEMCP;
  }

  void InitHeaderProps()
  {
    #ifdef ZIP_HEADER_REBEL
    m_HeaderFileInfoType = NFileHeader::NFileInfoType::kDefault;
    m_HeaderTimeType = NFileHeader::NTimeType::kDefault;
    m_HeaderIzMode = false;
    m_HeaderUseDescriptor.Init();
    m_HeaderInfo.InitHeaderInfo();
    m_HeaderLocale.InitLocaleSetting();
    #endif
  }
};

}}

#endif

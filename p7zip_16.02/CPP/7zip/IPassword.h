// IPassword.h

#ifndef __IPASSWORD_H
#define __IPASSWORD_H

#include "../Common/MyTypes.h"
#include "../Common/MyUnknown.h"

#include "IDecl.h"

#define PASSWORD_INTERFACE(i, x) DECL_INTERFACE(i, 5, x)

PASSWORD_INTERFACE(ICryptoGetTextPassword, 0x10)
{
  STDMETHOD(CryptoGetTextPassword)(BSTR *password) PURE;
};

PASSWORD_INTERFACE(ICryptoGetTextPassword2, 0x11)
{
  STDMETHOD(CryptoGetTextPassword2)(Int32 *passwordIsDefined, BSTR *password) PURE;
};

namespace NUpdateAnswer
{
  enum EEnum
  {
    kYes,
    kYesToAll,
    kNo,
    kNoToAll,
    kCancel
  };
}

PASSWORD_INTERFACE(ICryptoAskHeaderChange, 0x12)
{
  STDMETHOD(CryptoAskHeaderChange)(const wchar_t *name, const FILETIME *time, Int32 *answer) PURE;
};

#endif

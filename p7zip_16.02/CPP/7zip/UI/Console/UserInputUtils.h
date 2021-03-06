// UserInputUtils.h

#ifndef __USER_INPUT_UTILS_H
#define __USER_INPUT_UTILS_H

#include "../../../Common/StdOutStream.h"

namespace NUserAnswerMode {

enum EEnum
{
  kUnknown = 0,
  kYes,
  kNo,
  kYesAll,
  kNoAll,
  kAutoRenameAll,
  kQuit
};
}

NUserAnswerMode::EEnum ScanUserYesNoAllQuit(CStdOutStream *outStream, bool rename = true);
UString GetPassword(CStdOutStream *outStream,bool verify = false);

#endif

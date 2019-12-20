// SetProperties.h

#ifndef __SETPROPERTIES_H
#define __SETPROPERTIES_H

#include "Property.h"

HRESULT SetProperties(IUnknown *unknown, const CObjectVector<CProperty> &properties);
HRESULT SetHeaderProperties(IUnknown *unknown, const CObjectVector<CHeaderProperty> &properties);

#endif

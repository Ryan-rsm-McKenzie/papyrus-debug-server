#include "GameRTTI.h"
#include "skse64_common/Relocation.h"

typedef void * (* _Runtime_DynamicCast_Internal)(void * srcObj, UInt32 arg1, const void * fromType, const void * toType, UInt32 arg4);
RelocAddr <_Runtime_DynamicCast_Internal> Runtime_DynamicCast_Internal(0x0134B900);	// __RTDynamicCast

void * Runtime_DynamicCast(void * srcObj, const void * fromType, const void * toType)
{
	uintptr_t fromTypeAddr = uintptr_t(fromType) + RelocationManager::s_baseAddr;
	uintptr_t toTypeAddr = uintptr_t(toType) + RelocationManager::s_baseAddr;

	return Runtime_DynamicCast_Internal(srcObj, 0, (void *)fromTypeAddr, (void *)toTypeAddr, 0);
}

#include "GameRTTI.inl"

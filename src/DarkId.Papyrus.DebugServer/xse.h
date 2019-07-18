#pragma once

#if F4SE
#include "f4se/PluginAPI.h"
#include "f4se_common/BranchTrampoline.h"
#include "f4se_common/f4se_version.h"
#include "f4se/GameThreads.h"
#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/PapyrusInterfaces.h"
#include "f4se/PapyrusStruct.h"
#include "f4se/GameReferences.h"

#include "f4se/PapyrusNativeFunctions.h"

typedef F4SEInterface XSEInterface;
typedef F4SETaskInterface XSETaskInterface;
typedef F4SEMessagingInterface XSEMessagingInterface;

typedef VirtualMachine::IdentifierItem VMIdentifierItem;
#endif

#if SKSE64
#include "skse64/PluginAPI.h"
#include "skse64_common/BranchTrampoline.h"
#include "skse64_common/skse_version.h"
#include "skse64/gamethreads.h"

#include "skse64/PapyrusNativeFunctions.h"
#include "skse64/PapyrusInterfaces.h"
#include "skse64/GameReferences.h"

typedef SKSEInterface XSEInterface;
typedef SKSETaskInterface XSETaskInterface;
typedef SKSEMessagingInterface XSEMessagingInterface;

typedef VMClassInfo VMObjectTypeInfo;
typedef VMClassRegistry VirtualMachine;

typedef TaskDelegate ITaskDelegate;
typedef UnkVMStackData2 VMIdentifierItem;

typedef void Logger;
typedef void SimpleAllocMemoryPagePolicy;
#endif
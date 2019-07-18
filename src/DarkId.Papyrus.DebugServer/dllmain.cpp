#include "xse.h"

#include <shlobj.h>

#include "DebugServer.h"
#include "RuntimeEvents.h"

using namespace DarkId::Papyrus::DebugServer;

const char* g_pluginName = "DarkId.Papyrus.DebugServer";

UInt32 g_version = 1;
PluginHandle g_pluginHandle = kPluginHandle_Invalid;

const XSEInterface* g_xse;
XSEMessagingInterface* g_messaging;
XSETaskInterface* g_tasks;

DebugServer* g_debugServer;

void XSEMessageHandler(XSEMessagingInterface::Message* msg)
{
    switch (msg->type)
    {
        case XSEMessagingInterface::kMessage_GameLoaded:
        {
            RuntimeEvents::Internal::CommitHooks();

            g_debugServer->Listen();
            _MESSAGE("Listening for connections from adapter messaging proxy...");

            break;
        }
    }
}

extern "C"
{
#if F4SE
    bool F4SEPlugin_Query(const XSEInterface* xse, PluginInfo* info)
#elif SKSE64
    bool SKSEPlugin_Query(const XSEInterface* xse, PluginInfo* info)
#endif
    {
        _MESSAGE("Registering debug server plugin...");

        gLog.OpenRelative(CSIDL_MYDOCUMENTS, ("\\My Games\\Fallout4\\XSE\\" + std::string(g_pluginName) + ".log").c_str());

        // populate info structure
        info->infoVersion = PluginInfo::kInfoVersion;
        info->name = g_pluginName;
        info->version = g_version;

        // store plugin handle so we can identify ourselves later
        g_pluginHandle = xse->GetPluginHandle();

        // Check game version
        if (xse->runtimeVersion > CURRENT_RELEASE_RUNTIME) {
            char versionMessage[512];
            sprintf_s(versionMessage, sizeof(versionMessage), "Your game version: v%d.%d.%d.%d\nExpected versions: v%d.%d.%d.%d-v%d.%d.%d.%d\n%s will be disabled.",
                GET_EXE_VERSION_MAJOR(xse->runtimeVersion),
                GET_EXE_VERSION_MINOR(xse->runtimeVersion),
                GET_EXE_VERSION_BUILD(xse->runtimeVersion),
                GET_EXE_VERSION_SUB(xse->runtimeVersion),
                GET_EXE_VERSION_MAJOR(RUNTIME_VERSION_1_10_130),
                GET_EXE_VERSION_MINOR(RUNTIME_VERSION_1_10_130),
                GET_EXE_VERSION_BUILD(RUNTIME_VERSION_1_10_130),
                GET_EXE_VERSION_SUB(RUNTIME_VERSION_1_10_130),
                GET_EXE_VERSION_MAJOR(CURRENT_RELEASE_RUNTIME),
                GET_EXE_VERSION_MINOR(CURRENT_RELEASE_RUNTIME),
                GET_EXE_VERSION_BUILD(CURRENT_RELEASE_RUNTIME),
                GET_EXE_VERSION_SUB(CURRENT_RELEASE_RUNTIME),
                g_pluginName
            );

            MessageBoxA(nullptr, versionMessage, g_pluginName, MB_OK | MB_ICONEXCLAMATION);
            return false;
        } 

        g_messaging = (XSEMessagingInterface*)xse->QueryInterface(kInterface_Messaging);
        g_tasks = (XSETaskInterface*)xse->QueryInterface(kInterface_Task);

        return true;
    }

#if F4SE
    bool F4SEPlugin_Load(const XSEInterface* xse)
#elif SKSE64
    bool SKSEPlugin_Load(const XSEInterface* xse)
#endif
    {
        _MESSAGE("Initializing debug server...");

        if (!g_branchTrampoline.Create(1024 * 64))
        {
            _ERROR("Couldn't create branch trampoline. This is fatal. Skipping remainder of init process.");
            return false;
        }

        if (!g_localTrampoline.Create(1024 * 64, nullptr))
        {
            _ERROR("Couldn't create codegen buffer. This is fatal. Skipping remainder of init process.");
            return false;
        }

        g_debugServer = new DebugServer(g_tasks);
        g_messaging->RegisterListener(g_pluginHandle, "XSE", XSEMessageHandler);

        _MESSAGE("Waiting for GameLoaded event...");

        return true;
    }

};
#pragma once
#include "xse.h"
#include "PapyrusDebugger.h"
#include "Protocol/vscodeprotocol.h"

namespace DarkId::Papyrus::DebugServer
{
    class DebugServerSession
    {
        PapyrusDebugger* m_debugger;
        VSCodeProtocol* m_protocol;

        HANDLE m_thread;

        static DWORD WINAPI CommandLoopThreadStart(void* param);
    public:
        DebugServerSession(std::function<void(std::string)> sendCallback, XSETaskInterface* tasks);
        
        void Receive(std::string message);
        void Close();

        ~DebugServerSession();
    };
}
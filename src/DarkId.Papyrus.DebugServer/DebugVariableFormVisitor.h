#pragma once

#include "FormVisitorBase.h"
#include "Protocol/protocol.h"

namespace DarkId::Papyrus::DebugServer
{
    class DebugVariableFormVisitor :
        public FormVisitorBase<std::vector<Variable>&>
    {
    public:
        DebugVariableFormVisitor()
        {

        }

        ~DebugVariableFormVisitor()
        {

        }

        virtual void DefaultVisit(TESForm* form, std::vector<Variable>& variables) override;
        virtual void VisitTESNPC(TESNPC* npc, std::vector<Variable>& variables) override;
    };
}

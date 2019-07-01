#include "DebugVariableFormVisitor.h"
#include "Utilities.h"

namespace DarkId::Papyrus::DebugServer
{
    void DebugVariableFormVisitor::DefaultVisit(TESForm* form, std::vector<Variable>& variables)
    {
        //Variable reference;
        //reference.name = "Full Name";
        //reference.value = form->GetFullName();

        //variables.push_back(reference);

        //Variable reference2;
        //reference2.name = "Editor ID";
        //reference2.value = form->GetEditorID();

        //variables.push_back(reference2);
    }

    void DebugVariableFormVisitor::VisitTESNPC(TESNPC* form, std::vector<Variable>& variables)
    {
        DefaultVisit(form, variables);
    }
}


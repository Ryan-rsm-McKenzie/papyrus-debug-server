#include "PapyrusDebugger.h"

#include <algorithm>
#include <functional>

#include "f4se/PluginAPI.h"
#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/PapyrusInterfaces.h"
#include "f4se/GameThreads.h"
#include "f4se/PapyrusStruct.h"
#include "f4se/GameReferences.h"

#include "champollion/Decompiler/PscCoder.hpp"
#include "champollion/Decompiler/StreamWriter.hpp"

#include "Tasks.h"

// TODO: Can use this if we want to change the behavior to only hit breakpoints once per line.
// typedef UInt32(*_GetInstructionIndex)(void* instructionMetadata, UInt32 position);
// RelocAddr<_GetInstructionIndex> GetInstructionIndex(0x0278a090);

template<typename... Args>
std::string string_format(const char* fmt, Args... args)
{
	size_t size = snprintf(nullptr, 0, fmt, args...);
	std::string buf;
	buf.reserve(size + 1);
	buf.resize(size);
	snprintf(&buf[0], size + 1, fmt, args...);
	return buf;
}

// TODO: Need to audit 64-bit value passing to the client due to JavaScript precision loss.
namespace DarkId::Papyrus::DebugServer
{
	PapyrusDebugger::PapyrusDebugger(Protocol* protocol, F4SETaskInterface* tasks)
	{
        m_protocol = protocol;

		m_vm = (*g_gameVM)->m_virtualMachine;
		m_pexCache = new PexCache();

        m_tasks = tasks;

		m_state = kState_Running;

		m_createStackEventHandle =
			RuntimeEvents::SubscribeToCreateStack(std::bind(&PapyrusDebugger::StackCreated, this, std::placeholders::_1));

		m_cleanupStackEventHandle =
			RuntimeEvents::SubscribeToCleanupStack(std::bind(&PapyrusDebugger::StackCleanedUp, this, std::placeholders::_1));

		m_instructionExecutionEventHandle =
			RuntimeEvents::SubscribeToInstructionExecution(
				std::bind(&PapyrusDebugger::InstructionExecution, this, std::placeholders::_1, std::placeholders::_2));
	}

	void PapyrusDebugger::StackCreated(Game::VMStackData* stackData)
	{
		UInt32 stackId = stackData->stackId;

		m_tasks->AddTask(new FuncTask([this, stackId]()
			{
				if (m_closed)
				{
					return;
				}

				m_protocol->EmitThreadEvent(ThreadEvent(ThreadReason::ThreadStarted, stackId));
			}));
	}

	void PapyrusDebugger::StackCleanedUp(UInt32 stackId)
	{
		if (m_deadStackIds.size() > 2024)
		{
			m_deadStackIds.erase(m_deadStackIds.begin());
		}

		m_deadStackIds.emplace(stackId);

		if (m_state == kState_Stepping && m_currentStepStackId == stackId)
		{
			m_state = kState_Running;
			m_currentStepStackId = 0;
			m_currentStepStackFrame = NULL;

			m_protocol->EmitContinuedEvent(ContinuedEvent());
		}

		m_tasks->AddTask(new FuncTask([this, stackId]()
			{
				if (m_closed)
				{
					return;
				}

				m_protocol->EmitThreadEvent(ThreadEvent(ThreadReason::ThreadExited, stackId));
			}));
	}

	void PapyrusDebugger::InstructionExecution(Game::CodeTasklet* tasklet, Pex::OpCode opCode)
	{
		IFunction* func = tasklet->stackFrame->function;

        // TODO: Excessive nesting.

		if (m_state != kState_Paused && !func->IsNative())
		{
			Game::ScriptFunction* asScriptFunction = (Game::ScriptFunction*)func;

			int sourceReference = m_pexCache->GetScriptReference(asScriptFunction->m_className.c_str());
			if (m_breakpoints.find(sourceReference) != m_breakpoints.end())
			{
				std::set<int> breakpointLines = m_breakpoints[sourceReference];
				if (breakpointLines.size() > 0)
				{
					UInt32 currentLine;
					if (asScriptFunction->GetLineNumber(tasklet->stackFrame->currentInstructionPosition, &currentLine))
					{
						if (breakpointLines.find(currentLine) != breakpointLines.end())
						{
							m_state = kState_Paused;
							m_protocol->EmitStoppedEvent(StoppedEvent(StopReason::StopBreakpoint, tasklet->stackData->stackId));
						}
					}
				}
			}
		}

		if (m_state == kState_Stepping && tasklet->stackData->stackId == m_currentStepStackId)
		{
			bool shouldPause = false;

			if (m_currentStepStackFrame)
			{
				std::vector<Game::VMStackFrame*> currentFrames;
				GetStackFrames(tasklet->stackData->stackId, currentFrames);

				if (currentFrames.size() != 0)
				{
					int stepFrameIndex = -1;
					auto stepFrameIter = std::find(currentFrames.begin(), currentFrames.end(), m_currentStepStackFrame);

					if (stepFrameIter != currentFrames.end())
					{
						stepFrameIndex = std::distance(currentFrames.begin(), stepFrameIter);
					}

					switch (m_currentStepType)
					{
					case StepType::STEP_IN:
						shouldPause = true;
						break;
					case StepType::STEP_OUT:
						if (stepFrameIndex == -1)
						{
							shouldPause = true;
						}
						break;
					case StepType::STEP_OVER:
						if (stepFrameIndex <= 0)
						{
							shouldPause = true;
						}
						break;
					}
				}
			}

			if (shouldPause)
			{
				m_state = kState_Paused;
				m_currentStepStackId = 0;
				m_currentStepStackFrame = NULL;
				m_protocol->EmitStoppedEvent(StoppedEvent(StopReason::StopStep, tasklet->stackData->stackId));
			}
		}

		while (m_state == kState_Paused && !m_closed)
		{
			Sleep(100);
		}
	}

	HRESULT PapyrusDebugger::SetBreakpoints(const Source& source, const std::vector<SourceBreakpoint>& srcBreakpoints, std::vector<Breakpoint>& breakpoints)
	{
		std::set<int> breakpointLines;
		Pex::Binary* binary = m_pexCache->GetScript(source.name.c_str());

		for (auto srcBreakpoint : srcBreakpoints)
		{
			bool foundLine = false;
			std::string foundInFunc;

			if (binary)
			{
				for (auto functionInfo : binary->getDebugInfo().getFunctionInfos())
				{
					for (auto lineNumber : functionInfo.getLineNumbers())
					{
						if (srcBreakpoint.line == lineNumber)
						{
							foundInFunc = functionInfo.getFunctionName().asString();
							foundLine = true;
						}
					}
				}
			}

			breakpointLines.emplace(srcBreakpoint.line);

			// TODO: Have proxy apply a message when local source can't be resolved?

			Breakpoint breakpoint;
			breakpoint.source = source;
			breakpoint.verified = foundLine;
			breakpoint.line = srcBreakpoint.line;

			breakpoints.push_back(breakpoint);
		}

		int sourceReference = m_pexCache->GetScriptReference(source.name.c_str());
		m_breakpoints[sourceReference] = breakpointLines;

		return 0;
	}

	HRESULT PapyrusDebugger::Initialize()
	{
		m_protocol->EmitInitializedEvent();
		return 0;
	}

	HRESULT PapyrusDebugger::Pause()
	{
		m_state = kState_Paused;

		m_protocol->EmitStoppedEvent(StoppedEvent(StopReason::StopPause));
		return 0;
	}

	HRESULT PapyrusDebugger::Continue()
	{
		m_state = kState_Running;

		m_protocol->EmitContinuedEvent(ContinuedEvent(0));
		return 0;
	}

	HRESULT PapyrusDebugger::GetSource(Source& source, std::string& output)
	{
		Pex::Binary* binary = m_pexCache->GetScript(source.name.c_str());
		if (!binary)
		{
			return 1;
		}

		std::basic_stringstream<char> pscStream;
		Decompiler::PscCoder coder(new Decompiler::StreamWriter(pscStream));

		coder.code(*binary);

		output = pscStream.str();

		return 0;
	}

	bool PapyrusDebugger::GetSourceData(const char* scriptName, Source& data, std::set<int>& seenScripts)
	{
		int sourceReference = m_pexCache->GetScriptReference(scriptName);

		if (seenScripts.find(sourceReference) != seenScripts.end())
		{
			return false;
		}

		seenScripts.emplace(sourceReference);

		Pex::Binary* binary = m_pexCache->GetScript(scriptName);
		if (strcmp(binary->getHeader().getSourceFileName().c_str(), "") == 0)
		{
			return false;
		}

		data = Source(scriptName, binary->getHeader().getSourceFileName(), sourceReference);

		return true;
	}

	HRESULT PapyrusDebugger::GetLoadedSources(std::vector<Source>& sources)
	{
		std::set<int> seenScripts;
		BSReadAndWriteLocker scriptLock(&m_vm->scriptsLock);

        auto forAttachedScript = [this, &sources, &seenScripts](VirtualMachine::IdentifierItem* identifierItem)
        {
            if (identifierItem->count > 1)
            {
                return true;
            }

            VMIdentifier* scriptObject = identifierItem->GetScriptObject(identifierItem->identifier.one);
            if (!scriptObject)
            {
                return true;
            }

            VMObjectTypeInfo* typeInfo = scriptObject->m_typeInfo;

            const char* scriptName = typeInfo->m_typeName.c_str();

            Source source;
            if (GetSourceData(scriptName, source, seenScripts))
            {
                sources.push_back(source);
            }

            return true;
        };

		m_vm->m_attachedScripts.ForEach(forAttachedScript);

		return 0;
	}

	void PapyrusDebugger::ToThreadData(Game::VMStackData* stack, Thread& thread)
	{
		thread.id = stack->stackId;

        Game::VMStackFrame* frame = stack->currentStackFrame;
		if (!frame)
		{
			return;
		}

        // Currently the "thread" name is based on the top of the stack, which isn't very useful.
        // This would be better represented by the object reference the stack is associated with
		auto name = frame->identifier ? frame->identifier->m_typeInfo->m_typeName.c_str() :
            frame->baseValueTypeInfo ? frame->baseValueTypeInfo->m_typeName.c_str() :
            "<unknown>";

		thread.name = string_format("%s (%d)", name, thread.id);

		thread.running = true;
	}

	HRESULT PapyrusDebugger::GetThreads(std::vector<Thread>& threads)
	{
        auto forStack = [this, &threads](VirtualMachine::StackTableItem* stackItem)
        {
            Game::VMStackData* data = (Game::VMStackData*)stackItem->data;

            if (!data->currentStackFrame || m_deadStackIds.find(stackItem->stackId) != m_deadStackIds.end())
            {
                return true;
            }

            Thread thread;
            ToThreadData(data, thread);

            threads.push_back(thread);

            return true;
        };

		m_vm->m_allStacks.ForEach(forStack);

		return 0;
	}

	bool FindPexFunction(Pex::Binary* scriptBinary, Game::ScriptFunction* scriptFunction, Pex::Function &function)
	{
		Pex::Object scriptObject = scriptBinary->getObjects().at(0);

		std::string stateName = scriptFunction->m_stateName.c_str();
		std::string funcName = scriptFunction->m_fnName.c_str();

		int localsCount = -1;
		Pex::States states = scriptObject.getStates();
		for (auto& state : states)
		{
			if (state.getName().asString() == stateName)
			{
				for (auto& pexFunction : state.getFunctions())
				{
					if (pexFunction.getName().asString() == funcName)
					{
						function = pexFunction;
						return true;
					}
				}

				return false;
			}
		}

		return false;
	}

	HRESULT PapyrusDebugger::GetScopes(uint64_t frameId, std::vector<Scope>& scopes)
	{
        Game::VMStackFrame* frame = GetFrame(frameId);
		if (!frame)
		{
			return 1;
		}

		Scope localScope(frameId, "Locals", frame->parameterCount);
		scopes.push_back(localScope);

		return 0;
	}

	UInt64 PapyrusDebugger::CacheValue(VMValue* value)
	{
		if (m_values.size() > 2048)
		{
			delete m_values.begin()->second;
			m_values.erase(m_values.begin());
		}

		m_currentValueId++;
		m_values.emplace(m_currentValueId, value);

		return m_currentValueId;
	}

	void PapyrusDebugger::ToVariableData(const char* name, VMValue* value, Variable& out)
	{
		out.name = std::string(name);

		UInt8 typeId = value->GetTypeEnum();

		// TODO: Handle Variables

		bool cacheValue = false;

		switch (typeId)
		{
		case VMValue::kType_Identifier:
		{
			VMObjectTypeInfo* typeInfo = (VMObjectTypeInfo*)value->GetComplexType();
			out.type = typeInfo->m_typeName.c_str();

			if (value->data.id)
			{
				out.value = out.type;
				cacheValue = true;
			}
			else
			{
				out.value = "None";
			}
		}
		break;
		case VMValue::kType_String:
		{
			out.type = "string";

			if (value->data.str)
			{
				out.value = value->data.GetStr()->c_str();
			}
			else
			{
				out.value = "None";
			}
		}
		break;
		case VMValue::kType_Int:
		{
			out.type = "int";
			out.value = string_format("%d", value->data.i);
		}
		break;
		case VMValue::kType_Float:
		{
			out.value = string_format("%f", value->data.f);
		}
		break;
		case VMValue::kType_Bool:
		{
			out.value = string_format(value->data.b ? "true" : "false");
		}
		break;
		case VMValue::kType_Struct:
		{
			VMStructTypeInfo* typeInfo = (VMStructTypeInfo*)value->GetComplexType();
			if (typeInfo)
			{
				out.type = typeInfo->m_typeName.c_str();
				cacheValue = true;
			}
		}
		break;
		// case VMValue::kType_Variable:
		// {

		// }
		// break;
		case VMValue::kType_StringArray:
		case VMValue::kType_IntArray:
		case VMValue::kType_FloatArray:
		case VMValue::kType_BoolArray:
		// case VMValue::kType_VariableArray:
		case VMValue::kType_IdentifierArray:
		case VMValue::kType_StructArray:
		{
			std::string elementTypeName = "";

			if (value->IsComplexArrayType())
			{
				IComplexType* typeInfo = value->GetComplexType();
				elementTypeName = typeInfo->m_typeName.c_str();
			}
			else
			{
				switch (typeId)
				{
					case VMValue::kType_StringArray:
						elementTypeName = "string";
						break;
					case VMValue::kType_IntArray:
						elementTypeName = "int";
						break;
					case VMValue::kType_FloatArray:
						elementTypeName = "float";
						break;
					case VMValue::kType_BoolArray:
						elementTypeName = "bool";
						break;
				}
			}

			if (!value->data.arr)
			{
				out.value = "None";
			}
			else if (value->data.arr->arr.count > 0)
			{
				cacheValue = true;
			}

			out.type = string_format("%s[]", elementTypeName.c_str());
			out.value = out.type;
		}
		break;
		}

		if (cacheValue)
		{
			out.variablesReference = CacheValue(new VMValue(*value));
		}
	}

    // TODO: Properly read/reflect on form state.
	void PapyrusDebugger::ToRefVariables(VMRefOrInventoryObj* ref, std::vector<Variable>& variables)
	{
		//TESObjectREFR* objReference = ref->GetObjectReference();
		//if (objReference)
		//{
		//	Variable reference;
		//	reference.name = "Reference";
		//	reference.value = objReference->GetFullName();

		//	variables.push_back(reference);
		//}

		//TESForm* baseForm;
		//ExtraDataList* extraData;

		//	if (ref->GetExtraData(&baseForm, &extraData))
		//	{
		//		Variable baseFormName;
		//		baseFormName.name = "Form";
		//		baseFormName.value = baseForm->GetFullName();

		//		variables.push_back(baseFormName);

		//		Variable baseFormEditorId;
		//		baseFormEditorId.name = "Editor ID";
		//		baseFormEditorId.value = baseForm->GetEditorID();

		//		variables.push_back(baseFormEditorId);
		//	}
		//}
	}

	HRESULT PapyrusDebugger::GetVariables(uint64_t variablesReference, VariablesFilter filter, int start, int count, std::vector<Variable> & variables)
	{
		// TODO: Can most likely get a lot of cleanup on this area by relying more on parsed Pex.

		auto it = m_values.find(variablesReference);
		if (it != m_values.end())
		{
			VMValue* value = it->second;

			if (value->IsArrayType() && filter != VariablesFilter::VariablesNamed)
			{
				if (!value->data.arr)
				{
					return 1;
				}

				for (int i = 0; i < value->data.arr->arr.count; i++)
				{
					Variable variable;

					VMValue elementValue;
					value->data.arr->arr.GetNthItem(i, elementValue);

					ToVariableData(string_format("%d", i).c_str(), &elementValue, variable);
					variables.push_back(variable);
				}
			}
			else if (value->IsComplexType() && filter != VariablesFilter::VariablesIndexed)
			{
				UInt8 typeId = value->GetTypeEnum();
				if (typeId == VMValue::kType_Struct)
				{
					if (!value->data.strct)
					{
						return 1;
					}

					VMStructTypeInfo* structType = (VMStructTypeInfo*)value->GetComplexType();

                    auto forStructMember = [this, &variables, value](VMStructTypeInfo::MemberItem* member)
                    {
                        VMValue* memberValue = &value->data.strct->GetStruct()[member->index];
                        Variable variable;

                        ToVariableData(member->name.c_str(), memberValue, variable);
                        variables.push_back(variable);

                        return true;
                    };

					structType->m_members.ForEach(forStructMember);
				}
				else if (typeId == VMValue::kType_Identifier)
				{
					if (!value->data.id)
					{
						return 1;
					}

					VMObjectTypeInfo* objectType = (VMObjectTypeInfo*)value->GetComplexType();

					if (objectType->m_parent)
					{
						VMObjectTypeInfo* parentType = (VMObjectTypeInfo*)objectType->m_parent;

						VMIdentifier* parentId = value->data.id;
						UInt64 unk = 0;
						m_vm->CastAs(&parentId, &parentType, &unk);

						VMValue* parentValue = new VMValue();

						VMValue	tempValue;
						tempValue.SetComplexType(parentType);
						CALL_MEMBER_FN(parentValue, Set)(&tempValue);
						parentValue->SetIdentifier(&parentId);

						Variable parentVariable;
						parentVariable.presentationHint.kind = "baseClass";

						ToVariableData("parent", parentValue, parentVariable);
						variables.push_back(parentVariable);
					}

					Pex::Binary* scriptBinary = m_pexCache->GetScript(objectType->m_typeName.c_str());
					
					Pex::Object object = scriptBinary->getObjects().at(0);
					for (Pex::Variable pexVariable : object.getVariables())
					{
						const char* variableName = pexVariable.getName().asString().c_str();
						if (variableName[0] == ':')
						{
							continue;
						}

						BSFixedString variableNameStr(variableName);

						VMPropertyInfo propertyInfo;
						(*GetVMPropertyInfo)(objectType, &propertyInfo, &variableNameStr, true);

						if (propertyInfo.index == -1)
						{
							continue;
						}

						VMValue* memberValue = value->data.id ? &value->data.id->properties[propertyInfo.index] : NULL;
						
						Variable variable;
						if (pexVariable.getConstFlag())
						{
							variable.presentationHint.attributes.push_back("const");
						}

						variable.presentationHint.visibility = "private";

						ToVariableData(variableName, memberValue, variable);
						variables.push_back(variable);
					}
					
					for (Pex::Property pexProperty : object.getProperties())
					{
						if (!pexProperty.hasAutoVar())
						{
							continue;
						}

						const char* propertyName = pexProperty.getName().asString().c_str();

						BSFixedString propertyNameStr(propertyName);

						VMPropertyInfo propertyInfo;
						(*GetVMPropertyInfo)(objectType, &propertyInfo, &propertyNameStr, true);

						if (propertyInfo.index == -1)
						{
							continue;
						}

						VMValue* memberValue = value->data.id ? &value->data.id->properties[propertyInfo.index] : NULL;

						Variable variable;

						variable.presentationHint.kind = "property";
						variable.presentationHint.visibility = "public";

						ToVariableData(propertyName, memberValue, variable);
						variables.push_back(variable);
					}
				}
			}
			else
			{
				return 1;
			}

			return 0;
		}

		auto frame = GetFrame(variablesReference);
		if (frame)
		{
			if (filter == VariablesFilter::VariablesIndexed)
			{
				return 0;
			}

			if (!frame->function)
			{
				return 0;
			}

			if (!frame->function->IsStatic())
			{
				VMIdentifier* identifier = frame->identifier;
				VMObjectTypeInfo* typeInfo = identifier->m_typeInfo;

				VMValue* selfValue = new VMValue();

				VMValue	tempValue;
				tempValue.SetComplexType(typeInfo);
				CALL_MEMBER_FN(selfValue, Set)(&tempValue);
				selfValue->SetIdentifier(&identifier);

				UInt64 reference = CacheValue(selfValue);

				Variable selfVariable;
				selfVariable.presentationHint.kind = "class";

				ToVariableData("self", selfValue, selfVariable);
				variables.push_back(selfVariable);
			}

			if (frame->function->IsNative())
			{
				return 0;
			}

			Game::ScriptFunction* function = (Game::ScriptFunction*)frame->function;

			VMState* state = (VMState*)frame;
			VMArgList* argList = state->argList;

			int offset = CALL_MEMBER_FN(argList, GetOffset)(state);
			for (int i = 0; i < frame->parameterCount; i++)
			{
				auto value = CALL_MEMBER_FN(argList, Get)(state, i, offset);
				if (value)
				{
					BSFixedString name = function->m_params.data[i].name;
					if (!name || name.c_str()[0] == ':')
					{
						continue;
					}

					Variable variable;
					ToVariableData(name.c_str(), value, variable);
					variables.push_back(variable);
				}
			}

			return 0;
		}

		return 1;
	}

	int PapyrusDebugger::GetNamedVariables(uint64_t variablesReference)
	{
		_MESSAGE("Named variables count request: %d", variablesReference);
		return 0;
	}

	void PapyrusDebugger::ToStackFrameData(Game::VMStackFrame * frame, UInt32 level, StackFrame & out)
	{
		Thread threadData;
		ToThreadData(frame->stackData, threadData);

		out = StackFrame(threadData.id, level);

		UInt32 lineNumber;
		frame->function->GetLineNumber(frame->currentInstructionPosition, &lineNumber);

		out.line = lineNumber;

		if (!frame->function->IsNative())
		{
			auto asScriptFunction = (Game::ScriptFunction*)frame->function;

			Source source;
			std::set<int> seenScripts;

			if (GetSourceData(asScriptFunction->m_className.c_str(), source, seenScripts))
			{
				out.source = source;
			}

			std::string fullName = string_format("%s:%s", asScriptFunction->m_className.c_str(), asScriptFunction->m_fnName.c_str());
			if (strcmp(asScriptFunction->m_stateName.c_str(), "") != 0)
			{
				fullName = string_format("%s (%s)", fullName.c_str(), asScriptFunction->m_stateName.c_str());
			}

			out.name = fullName.c_str();
		}
		else
		{
			out.name = string_format("%s:%s", frame->function->GetClassNameA()->c_str(), frame->function->GetName()->c_str()).c_str();
		}
	}

    Game::VMStackFrame* PapyrusDebugger::GetFrame(UInt64 frameId)
	{
		UInt32 stackId = StackFrame::GetThreadId(frameId);
		UInt32 level = StackFrame::GetLevel(frameId);

		std::vector<Game::VMStackFrame*> frames;
		GetStackFrames(stackId, frames);

		if (level > frames.size() - 1)
		{
			return NULL;
		}

		return frames.at(level);
	}

	void PapyrusDebugger::GetStackFrames(UInt32 stackId, std::vector<Game::VMStackFrame*> & frames)
	{
		auto tableItem = m_vm->m_allStacks.Find(&stackId);
		if (!tableItem)
		{
			return;
		}

		BSReadAndWriteLocker stackLock(&m_vm->stackLock);

        Game::VMStackData* data = (Game::VMStackData*)tableItem->data;

		auto frame = data->currentStackFrame;
		while (frame)
		{
			frames.push_back(frame);
			frame = frame->previousFrame;
		}
	}

	HRESULT PapyrusDebugger::GetStackTrace(int threadId, int startFrame, int levels, std::vector<StackFrame> & stackFrames, int& totalFrames)
	{
		std::vector<Game::VMStackFrame*> frames;
		GetStackFrames(threadId, frames);

		totalFrames = frames.size();

		for (int i = startFrame; i < frames.size() && i < levels + startFrame; i++)
		{
            Game::VMStackFrame* frame = frames.at(i);
			StackFrame frameData;

			ToStackFrameData(frame, i, frameData);
			stackFrames.push_back(frameData);
		}

		return 0;
	}

	HRESULT PapyrusDebugger::StepCommand(int threadId, StepType stepType)
	{
		if (m_state != kState_Paused)
		{
			return 1;
		}

		UInt32 stackId = threadId;

		if (!m_vm->HasStack(stackId) || m_deadStackIds.find(stackId) != m_deadStackIds.end())
		{
			return 1;
		}

		VirtualMachine::StackTableItem* stackTableItem = m_vm->m_allStacks.Find(&stackId);

        if (stackTableItem)
        {
            Game::VMStackData* data = (Game::VMStackData*)stackTableItem->data;

            if (data && data->currentStackFrame)
            {
                m_currentStepStackFrame = data->currentStackFrame;
            }
        }

		m_state = kState_Stepping;
		m_currentStepStackId = stackId;
		m_currentStepType = stepType;

		return 0;
	}

	PapyrusDebugger::~PapyrusDebugger()
	{
		m_closed = true;
		delete m_pexCache;

		RuntimeEvents::UnsubscribeFromInstructionExecution(m_instructionExecutionEventHandle);
		RuntimeEvents::UnsubscribeFromCreateStack(m_createStackEventHandle);
		RuntimeEvents::UnsubscribeFromCleanupStack(m_cleanupStackEventHandle);
	}
}
#define XBYAK_NO_OP_NAMES

#include "RuntimeEvents.h"

#include "f4se_common/BranchTrampoline.h"
#include "xbyak/xbyak.h"

#include "f4se_common/Relocation.h"

#include "xse.h"

#include "f4se_common/SafeWrite.h"

#include <cassert>
#include <mutex>

namespace DarkId::Papyrus::DebugServer

{
	namespace RuntimeEvents
	{
		eventpp::CallbackList<void(Game::CodeTasklet*, Pex::OpCode)> g_instructionExecutionEvent;

		InstructionExecutionEventHandle SubscribeToInstructionExecution(std::function<void(Game::CodeTasklet*, Pex::OpCode)> handler)
		{
			return g_instructionExecutionEvent.append(handler);
		}

		bool UnsubscribeFromInstructionExecution(InstructionExecutionEventHandle handle)
		{
			return g_instructionExecutionEvent.remove(handle);
		}

		eventpp::CallbackList<void(Game::VMStackData*)> g_createStackEvent;

		CreateStackEventHandle SubscribeToCreateStack(std::function<void(Game::VMStackData*)> handler)
		{
			return g_createStackEvent.append(handler);
		}

		bool UnsubscribeFromCreateStack(CreateStackEventHandle handle)
		{
			return g_createStackEvent.remove(handle);
		}

		eventpp::CallbackList<void(UInt32)> g_cleanupStackEvent;

		CleanupStackEventHandle SubscribeToCleanupStack(std::function<void(UInt32)> handler)
		{
			return g_cleanupStackEvent.append(handler);
		}

		bool UnsubscribeFromCleanupStack(CleanupStackEventHandle handle)
		{
			return g_cleanupStackEvent.remove(handle);
		}

		eventpp::CallbackList<void(TESInitScriptEvent*)> g_initScriptEvent;

		InitScriptEventHandle SubscribeToInitScript(std::function<void(TESInitScriptEvent*)> handler)
		{
			return g_initScriptEvent.append(handler);
		}

		bool UnsubscribeFromInitScript(InitScriptEventHandle handle)
		{
			return g_initScriptEvent.remove(handle);
		}

		eventpp::CallbackList<void(Game::BSScript::LogEvent*)> g_logEvent;

		LogEventHandle SubscribeToLog(std::function<void(Game::BSScript::LogEvent*)> handler)
		{
			return g_logEvent.append(handler);
		}

		bool UnsubscribeFromLog(LogEventHandle handle)
		{
			return g_logEvent.remove(handle);
		}

		class ScriptInitEventSink : public BSTEventSink<TESInitScriptEvent>
		{
			EventResult ReceiveEvent(TESInitScriptEvent* evn, void* dispatcher) override
			{
				g_initScriptEvent(evn);

				return kEvent_Continue;
			};
		};

		class LogEventSink : public BSTEventSink<Game::BSScript::LogEvent>
		{
			EventResult ReceiveEvent(Game::BSScript::LogEvent* evn, void* dispatcher) override
			{
				g_logEvent(evn);

				return kEvent_Continue;
			};
		};

#if F4SE
		RelocAddr<uintptr_t> InstructionExecute(0x0276e9bd);

		void InstructionExecute_Hook(Game::CodeTasklet* tasklet, Pex::OpCode opCode)
		{
			if (tasklet->stackFrame) {
				g_instructionExecutionEvent(tasklet, opCode);
			}
		}

		typedef bool (*_CreateStack)(VirtualMachine* vm, UInt64 unk1, UInt64 unk2, UInt64 unk3, Game::VMStackData** stackData);
		RelocAddr<_CreateStack> CreateStack(0x02741f50);
		_CreateStack CreateStack_Original = nullptr;

		bool CreateStack_Hook(VirtualMachine* vm, UInt64 unk1, UInt64 unk2, UInt64 unk3, Game::VMStackData** stackData)
		{
			bool result = CreateStack_Original(vm, unk1, unk2, unk3, stackData);

			if (result) {
				g_createStackEvent(*stackData);
			}

			return result;
		}

		typedef void (*_CleanupStack)(VirtualMachine* vm, Game::VMStackData* stack);
		RelocAddr<_CleanupStack> CleanupStack(0x027424f0);
		_CleanupStack CleanupStack_Original = nullptr;

		void CleanupStack_Hook(VirtualMachine* vm, Game::VMStackData* stack)
		{
			if (!stack) {
				return;
			}

			UInt32 stackId = stack->stackId;
			CleanupStack_Original(vm, stack);
			g_cleanupStackEvent(stackId);
		}

		//typedef UInt32(*_ScriptFunctionInvoke)(ScriptFunction* function, StackData** stackData, Logger* logger, VirtualMachine* vm, UInt32 unk);
		//RelocAddr<_ScriptFunctionInvoke> ScriptFunctionInvoke(0x027acb90);
		//_ScriptFunctionInvoke ScriptFunctionInvoke_Original = nullptr;

		//UInt32 ScriptFunctionInvoke_Hook(ScriptFunction* func, StackData** stackData, Logger* logger, VirtualMachine* virtualMachine, UInt32 unk)
		//{
		//    if ((*stackData)->currentStackFrame)
		//    {
		//        if (!func->IsStatic())
		//        {
		//            //(*stackData)->currentStackFrame->identifier->properties->

		//            ////UInt64 baseValueHandle = (*stackData)->currentStackFrame->identifier->GetHandle();
		//            ////virtualMachine->GetHandlePolicy()->Resolve(-1, baseValueHandle);

		//            ////val->SetIdentifier(&(*stackData)->currentStackFrame->identifier);
		//            ////Unpack

		//            ////_MESSAGE("%s", val->data.GetStr()->c_str());
		//        }

		//        if ((*stackData)->currentStackFrame->parameterCount > 0)
		//        {
		//            VMState* state = (VMState*)(*stackData)->currentStackFrame;
		//            UInt32    argOffset = CALL_MEMBER_FN(state->argList, GetOffset)(state);
		//            VMValue* arg0 = CALL_MEMBER_FN(state->argList, Get)(state, 0, argOffset);
		//            if (arg0->GetTypeEnum() == VMValue::kType_String)
		//            {
		//                _MESSAGE("%s", arg0->data.GetStr()->c_str());
		//            }
		//        }
		//    }

		//    UInt32 result = ScriptFunctionInvoke_Original(func, stackData, logger, virtualMachine, unk);
		//    
		//    //UInt32 argOffset = CALL_MEMBER_FN(state->argList, GetOffset)(state);

		//    //T_Base* base = NULL;

		//    //// extract base object pointer for non-static types
		//    //if (!IsStaticType <T_Base>::value)
		//    //{
		//    //    UnpackValue(&base, baseValue);
		//    //    if (!base) return false;
		//    //}

		//    return result;
		//}

		namespace Internal
		{
			void CommitHooks()
			{
				{
					struct InstructionExecute_Code : Xbyak::CodeGenerator
					{
						InstructionExecute_Code(void* buf, uintptr_t funcAddr) : Xbyak::CodeGenerator(4096, buf)
						{
							Xbyak::Label funcLabel;
							Xbyak::Label retnLabel;

							and_(edx, 0x3f);
							and_(eax, 0x3f);
							mov(dword[rbp + 0x30], edx);
							mov(qword[rbp - 0x38], rax);
							lea(rax, ptr[r9 + r8 * 8]);
							mov(qword[rbp - 0x40], rax);

							push(rax);
							push(rcx);
							push(rdx);
							push(r8);
							push(r9);
							push(r10);
							push(r11);
							push(r11);

							mov(rcx, rdi);
							mov(rdx, dword[rbp + 0x30]);

							sub(rsp, 0x20);
							call(ptr[rip + funcLabel]);
							add(rsp, 0x20);

							pop(r11);
							pop(r11);
							pop(r10);
							pop(r9);
							pop(r8);
							pop(rdx);
							pop(rcx);
							pop(rax);

							jmp(ptr[rip + retnLabel]);

							L(funcLabel);
							dq(funcAddr);

							L(retnLabel);
							dq(InstructionExecute + 0x15);
						}
					};

					void* codeBuf = g_localTrampoline.StartAlloc();
					InstructionExecute_Code code(codeBuf, (uintptr_t)InstructionExecute_Hook);
					g_localTrampoline.EndAlloc(code.getCurr());

					g_branchTrampoline.Write6Branch(InstructionExecute, uintptr_t(code.getCode()));
				}

				{
					struct CreateStack_Code : Xbyak::CodeGenerator
					{
						CreateStack_Code(void* buf) : Xbyak::CodeGenerator(4096, buf)
						{
							Xbyak::Label retnLabel;

							mov(ptr[rsp + 0x10], rbx);

							jmp(ptr[rip + retnLabel]);

							L(retnLabel);
							dq(CreateStack.GetUIntPtr() + 5);
						}
					};

					void* codeBuf = g_localTrampoline.StartAlloc();
					CreateStack_Code code(codeBuf);
					g_localTrampoline.EndAlloc(code.getCurr());

					CreateStack_Original = (_CreateStack)codeBuf;

					g_branchTrampoline.Write5Branch(CreateStack.GetUIntPtr(), (uintptr_t)CreateStack_Hook);
				}

				{
					struct CleanupStack_Code : Xbyak::CodeGenerator
					{
						CleanupStack_Code(void* buf) : Xbyak::CodeGenerator(4096, buf)
						{
							Xbyak::Label retnLabel;
							Xbyak::Label jmpLabel;

							jmp(ptr[rip + retnLabel]);

							L(retnLabel);
							dq(CleanupStack.GetUIntPtr() + 9);
						}
					};

					void* codeBuf = g_localTrampoline.StartAlloc();
					CleanupStack_Code code(codeBuf);
					g_localTrampoline.EndAlloc(code.getCurr());

					CleanupStack_Original = (_CleanupStack)codeBuf;

					g_branchTrampoline.Write5Branch(CleanupStack.GetUIntPtr(), (uintptr_t)CleanupStack_Hook);
				}

				GetEventDispatcher<TESInitScriptEvent>()->AddEventSink(new ScriptInitEventSink());
				(*g_gameVM)->m_virtualMachine->AddLogEventSink(new LogEventSink());

				//TSInit
				//TESInitScriptEvent* eventSink = (*g_gameVM)->m_eventSinks[GameVM::kEventSink_InitScript]

				//{
				//    struct ScriptFunctionInvoke_Code : Xbyak::CodeGenerator {
				//        ScriptFunctionInvoke_Code(void* buf) : Xbyak::CodeGenerator(4096, buf)
				//        {
				//            Xbyak::Label retnLabel;

				//            sub(rsp, 0x28);
				//            mov(rdx, ptr[rdx]);

				//            jmp(ptr[rip + retnLabel]);

				//            L(retnLabel);
				//            dq(ScriptFunctionInvoke.GetUIntPtr() + 0x07);
				//        }
				//    };

				//    void* codeBuf = g_localTrampoline.StartAlloc();
				//    ScriptFunctionInvoke_Code code(codeBuf);
				//    g_localTrampoline.EndAlloc(code.getCurr());

				//    ScriptFunctionInvoke_Original = (_ScriptFunctionInvoke)codeBuf;

				//    g_branchTrampoline.Write5Branch(ScriptFunctionInvoke.GetUIntPtr(), (uintptr_t)ScriptFunctionInvoke_Hook);
				//}
			}
		}
#endif

#if SKSE64
		void InstructionExecute_Hook(RE::BSScript::Internal::CodeTasklet* a_tasklet, RE::BSScript::Internal::CodeTasklet::OpCode a_opCode)
		{
			// do something
		}


		void CreateStack_Hook(RE::BSTSmartPointer<RE::BSScript::Stack>& a_stack)
		{
			// do something
		}


		using CleanupStack_t = void(RE::BSScript::Internal::VirtualMachine*, RE::BSScript::Stack*);
		CleanupStack_t* _CleanupStack = 0;


		void CleanupStack_Hook(RE::BSScript::Internal::VirtualMachine* a_vm, RE::BSScript::Stack* a_stack)
		{
			if (a_stack) {
				auto stackID = a_stack->stackID;
				_CleanupStack(a_vm, a_stack);
				// do something
			}
		}


		namespace Internal
		{
			void CommitHooks()
			{
				{
					// E8 ? ? ? ? 48 83 C3 08 48 3B DF 75 EF 48 83 7C 24 30 00
					constexpr std::uintptr_t FUNC_ADDR = 0x01277C60;	// 1_5_80
					constexpr std::size_t CAVE_START = 0x170;
					constexpr std::size_t CAVE_END = 0x176;
					constexpr std::size_t CAVE_SIZE = CAVE_END - CAVE_START;

					REL::Offset<std::uintptr_t> funcBase(FUNC_ADDR);

					struct Patch : Xbyak::CodeGenerator
					{
						Patch(void* a_buf, std::size_t a_callAddr, std::size_t a_retAddr) : Xbyak::CodeGenerator(1024, a_buf)
						{
							Xbyak::Label callLbl;
							Xbyak::Label retLbl;

							push(rax);	// save volatile registers
							push(rcx);
							push(rdx);
							push(r8);
							push(r9);
							push(r10);
							push(r11);

							mov(rcx, rdi);	// rdi == BSScript::Internal::CodeTasklet*
							mov(r8d, edx);	// edx == BSScript::Internal::CodeTasklet::OpCode
							xor_(rdx, rdx);
							mov(edx, r8d);

							sub(rsp, 0x20);	// pad the stack
							call(ptr[rip + callLbl]);	// make call
							add(rsp, 0x20);

							pop(r11);
							pop(r10);
							pop(r9);
							pop(r8);
							pop(rdx);
							pop(rcx);
							pop(rax);

							mov(rax, r8);	// execute overridden ops
							and_(eax, 0x3F);

							jmp(ptr[rip + retLbl]);	// resume execution

							L(callLbl);
							dq(a_callAddr);

							L(retLbl);
							dq(a_retAddr);
						}
					};

					void* patchBuf = g_localTrampoline.StartAlloc();
					Patch patch(patchBuf, unrestricted_cast<std::uintptr_t>(InstructionExecute_Hook), funcBase.GetAddress() + CAVE_END);
					g_localTrampoline.EndAlloc(patch.getCurr());

					assert(CAVE_SIZE == 6);

					g_branchTrampoline.Write6Branch(funcBase.GetAddress() + CAVE_START, reinterpret_cast<std::uintptr_t>(patch.getCode()));
				}

				{
					// E8 ? ? ? ? 48 8B 5C 24 38 48 85 DB 74 19
					constexpr std::uintptr_t FUNC_ADDR = 0x01263D40;	// 1_5_80
					constexpr std::size_t HOOK_TARGET = 0x1D4;

					REL::Offset<std::uintptr_t> funcBase(FUNC_ADDR);

					struct Patch : Xbyak::CodeGenerator
					{
						Patch(void* a_buf, std::size_t a_funcAddr) : Xbyak::CodeGenerator(1024, a_buf)
						{
							Xbyak::Label funcLbl;

							mov(rcx, ptr[rsp - 0x448]);	// rsp - 0x448 == BSTSmartPointer<BSScript::Stack>&
							jmp(ptr[rip + funcLbl]);

							L(funcLbl);
							dq(a_funcAddr);
						}
					};

					void* patchBuf = g_localTrampoline.StartAlloc();
					Patch patch(patchBuf, unrestricted_cast<std::uintptr_t>(CreateStack_Hook));
					g_localTrampoline.EndAlloc(patch.getCurr());

					g_branchTrampoline.Write5Branch(funcBase.GetAddress() + HOOK_TARGET, reinterpret_cast<std::uintptr_t>(patch.getCode()));
				}


				{
					// E8 ? ? ? ? 90 48 85 DB 74 24 41 8B C7
					constexpr std::uintptr_t FUNC_ADDR = 0x01264240;	// 1_5_80
					constexpr std::size_t CAVE_START = 0x0;
					constexpr std::size_t CAVE_END = 0x9;
					constexpr std::size_t CAVE_SIZE = CAVE_END - CAVE_START;
					constexpr UInt8 NOP = 0x90;

					REL::Offset<std::uintptr_t> funcBase(FUNC_ADDR);

					struct Patch : Xbyak::CodeGenerator
					{
						Patch(void* a_buf, std::size_t a_funcAddr) : Xbyak::CodeGenerator(1024, a_buf)
						{
							Xbyak::Label funcLbl;

							jmp(ptr[rip + funcLbl]);

							L(funcLbl);
							dq(a_funcAddr);
						}
					};

					void* patchBuf = g_localTrampoline.StartAlloc();
					Patch patch(patchBuf, unrestricted_cast<std::uintptr_t>(CleanupStack_Hook));
					g_localTrampoline.EndAlloc(patch.getCurr());

					assert(CAVE_SIZE >= 6);

					g_branchTrampoline.Write6Branch(funcBase.GetAddress() + CAVE_START, reinterpret_cast<std::uintptr_t>(patch.getCode()));

					for (UInt8 i = CAVE_START + 6; i < CAVE_END; ++i) {
						SafeWrite8(funcBase.GetAddress() + i, NOP);
					}

					_CleanupStack = reinterpret_cast<CleanupStack_t*>(funcBase.GetAddress() + CAVE_END);
				}
			}
		}
#endif
    }
}
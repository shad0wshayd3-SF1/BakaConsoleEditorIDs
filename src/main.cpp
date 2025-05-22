namespace Hooks
{
	namespace hkGetReferencedObject
	{
		static RE::TESForm* GetReferencedObject(RE::SCRIPT_WORD* a_currentWord)
		{
			auto form = RE::TESForm::LookupByEditorID(a_currentWord->text);
			if (form && form->formType != RE::FormType::kGMST)
			{
				return form;
			}
			else if (auto formID = std::strtoul(a_currentWord->text, nullptr, 16))
			{
				return RE::TESForm::LookupByID(formID);
			}

			return nullptr;
		}

		static void Install()
		{
			struct GetReferencedObjectPatch : Xbyak::CodeGenerator
			{
				GetReferencedObjectPatch(std::uintptr_t a_func, std::uintptr_t a_retn)
				{
					Xbyak::Label func;
					Xbyak::Label retn;

					mov(rcx, rsi);  // RE::SCRIPT_WORD
					sub(rsp, 0x20);
					call(ptr[rip + func]);
					add(rsp, 0x20);

					jmp(ptr[rip + retn]);

					L(func);
					dq(a_func);

					L(retn);
					dq(a_retn);
				}
			};

			static REL::Relocation target{ REL::ID(65832) };
			static constexpr auto  TARGET_ADDR{ 0x1FF };
			static constexpr auto  TARGET_RETN{ 0x232 };
			static constexpr auto  TARGET_FILL{ TARGET_RETN - TARGET_ADDR };
			target.write_fill<TARGET_ADDR>(REL::NOP, TARGET_FILL);

			auto code = GetReferencedObjectPatch{
				reinterpret_cast<std::uintptr_t>(GetReferencedObject),
				target.address() + TARGET_RETN
			};

			auto& trampoline = REL::GetTrampoline();
			trampoline.write_jmp<5>(target.address() + TARGET_ADDR, trampoline.allocate(code));
		}
	}

	static void Install()
	{
		hkGetReferencedObject::Install();
	}
}

namespace
{
	void MessageCallback(SFSE::MessagingInterface::Message* a_msg) noexcept
	{
		switch (a_msg->type)
		{
		case SFSE::MessagingInterface::kPostLoad:
			Hooks::Install();
			break;
		default:
			break;
		}
	}
}

SFSEPluginLoad(const SFSE::LoadInterface* a_sfse)
{
	SFSE::Init(a_sfse, { .trampoline = true, .trampolineSize = 64 });
	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);
	return true;
}

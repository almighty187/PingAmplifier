/*
PingAmplifier for SA:MP client
by urShadow
2016
*/

#include <string>

#include "lib/urmem.hpp"

using namespace std;
using m = urmem;

class PingAmplifier
{
public:

	static bool Init()
	{
		if (auto addr = reinterpret_cast<m::address_t>(GetProcAddress(GetModuleHandleA("User32.dll"), "TranslateMessage")))
		{
			_hook_translate_message = make_shared<m::hook>(addr, m::get_func_addr(&HOOK_TranslateMessage));

			return true;
		}

		return false;
	}

private:

	static BOOL WINAPI HOOK_TranslateMessage(const void *lpMsg)
	{
		_hook_translate_message->disable();

		auto result = m::call_function<m::calling_convention::stdcall, BOOL>(
			_hook_translate_message->get_original_addr(),
			lpMsg);

		if (auto module = reinterpret_cast<m::address_t>(GetModuleHandleA("samp.dll")))
		{
			m::sig_scanner scanner;

			if (scanner.init(module))
			{
				m::address_t
					addr_add_client_message{},
					addr_add_cmd_proc{},
					add_write_bits{};

				if (
					scanner.find("\x56\x8B\x74\x24\x0C\x8B\xC6\x57\x8B\xF9",
						"xxxxxxxxxx",
						addr_add_client_message)
					&&
					scanner.find("\x57\x8B\xB9\x00\x00\x00\x00\x81\xFF",
						"xxx????xx",
						addr_add_cmd_proc)
					&&
					scanner.find("\xE8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\xF8\x89\x7C\x24\x20",
						"x????x????xxxxxx",
						add_write_bits)
					)
				{
					_hook_cchatwindow__add_client_message = make_shared<m::hook>(
						addr_add_client_message,
						m::get_func_addr(&HOOK_CChatWindow__AddClientMessage));

					_hook_ccmdwindow__add_cmd_proc = make_shared<m::hook>(
						addr_add_cmd_proc,
						m::get_func_addr(&HOOK_CCmdWindow__AddCmdProc));

					_hook_bitstream__write_bits = make_shared<m::hook>(
						add_write_bits,
						m::get_func_addr(&HOOK_BitStream__WriteBits),
						m::hook::type::call);
				}
			}

			return result;
		}

		_hook_translate_message->enable();

		return result;
	}

	static void __thiscall HOOK_CCmdWindow__AddCmdProc(void *_this, const char *szCmdName, void *cmdHandler)
	{
		_hook_ccmdwindow__add_cmd_proc->disable();

		m::call_function<m::calling_convention::thiscall>(
			_hook_ccmdwindow__add_cmd_proc->get_original_addr(),
			_this, "setampl", reinterpret_cast<void *>(cmd_setampl));

		return m::call_function<m::calling_convention::thiscall>(
			_hook_ccmdwindow__add_cmd_proc->get_original_addr(),
			_this, szCmdName, cmdHandler);
	}

	static void __thiscall HOOK_CChatWindow__AddClientMessage(void *_this, DWORD dwColor, char *szStr)
	{
		_hook_cchatwindow__add_client_message->disable();

		_cchatwindow = _this;

		addClientMessage(0xD3D8DB00, "PingAmplifier 2.0 loaded. Use /setampl <value> for configure the amplifier.");
		addClientMessage(0xD3D8DB00, "https://gist.github.com/urShadow/57bb550b97ff0ebbf7ebab0ff51a73b7");

		addClientMessage(dwColor, szStr);
	}

	static void __thiscall HOOK_BitStream__WriteBits(void *_this, const unsigned char *input,
		int numberOfBitsToWrite, const bool rightAlignedBits)
	{
		m::hook::raii scope(*_hook_bitstream__write_bits);

		m::pointer(reinterpret_cast<m::address_t>(input)).field<unsigned int>(0) -= _amplification;

		return m::call_function<m::calling_convention::thiscall>(
			_hook_bitstream__write_bits->get_original_addr(),
			_this, input, numberOfBitsToWrite, rightAlignedBits);
	}

	static void cmd_setampl(const char *param)
	{
		try
		{
			_amplification = stoi(param);

			addClientMessage(0xD3D8DB00, (string("[PingAmplifier] Amplification set to ") + param).c_str());
		}
		catch (std::exception &e)
		{
			addClientMessage(0xD3D8DB00, e.what());
		}
	}

	static void addClientMessage(DWORD dwColor, const char *szStr)
	{
		if (_cchatwindow)
			m::call_function<m::calling_convention::thiscall>(
				_hook_cchatwindow__add_client_message->get_original_addr(),
				_cchatwindow,
				dwColor,
				szStr);
	}

	static std::shared_ptr<m::hook>
		_hook_translate_message,
		_hook_ccmdwindow__add_cmd_proc,
		_hook_cchatwindow__add_client_message,
		_hook_bitstream__write_bits;

	static void *_cchatwindow;
	static int _amplification;
};

shared_ptr<m::hook>
PingAmplifier::_hook_translate_message,
PingAmplifier::_hook_ccmdwindow__add_cmd_proc,
PingAmplifier::_hook_cchatwindow__add_client_message,
PingAmplifier::_hook_bitstream__write_bits;

void *PingAmplifier::_cchatwindow{};
int PingAmplifier::_amplification{};

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReasonForCall, LPVOID lpReserved)
{
	if (dwReasonForCall == DLL_PROCESS_ATTACH)
		return PingAmplifier::Init();

	return TRUE;
}
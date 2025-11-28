//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
// Project: MQ2Cursor.cpp
// Original Author: s0rCieR
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

#include "mq/Plugin.h"
#include "mq/base/Enum.h"

#include "moveitem.h"

#include <string_view>
#include <string>
#include <chrono>

using namespace std::chrono_literals;
using namespace std::chrono;

PreSetup("MQ2Cursor");
PLUGIN_VERSION(4.2);

constexpr milliseconds CURSOR_SPAM_DELAY = 15000ms;
constexpr milliseconds CURSOR_WAIT_DELAY = 250ms;

static bool s_initialized = false;        // Is the plugin initialized?
static bool s_cursorActive = false;     // Is cursor handling active?
static bool s_quiet = false;           // Silence most output if true
static int s_cursorWarnItemID = NOID;       // Cursor Warned Item ID
static int s_randomizeCursor = 0;       // Randomized wait time max in ms

class CursorKeepList;
static CursorKeepList* s_keepList = nullptr;   // Cursor Keeping List

static steady_clock::time_point s_cursorWarnTime{};
static steady_clock::time_point s_nextExecute{};
static steady_clock::time_point s_randomTimer{};

enum PluginFlags : uint16_t
{
	Flag_None = 0x0000,

	Flag_GuildTributeMasterWnd = 0x0001,
	Flag_TributeMasterWnd = 0x0002,
	Flag_GuildBankWnd = 0x0004,
	Flag_TradeWnd = 0x0008,
	Flag_MerchantWnd = 0x0010,
	Flag_BankWnd = 0x0020,
	Flag_GiveWnd = 0x0040,
	Flag_SpellBookWnd = 0x0080,

	Flag_LootWnd = 0x0200,
	Flag_InventoryWnd = 0x0400,

	Flag_CastingWnd = 0x1000,
	Flag_SpellCasting = 0x2000,
	Flag_SpellInProgress = 0x4000,
	Flag_Stunned = 0x0100,

	Flag_BusyInventory = Flag_LootWnd | Flag_InventoryWnd,
	Flag_BusyInventoryFlag = 0x0800,

	Flag_AutoPause = static_cast<uint16_t>(~Flag_BusyInventory),    // 0xF9FF - Plugin Auto-Pause Flags (see UpdateFlags)
};
constexpr bool has_bitwise_operations(PluginFlags) { return true; }

PluginFlags s_pluginFlags = Flag_None;         // Window Conditions and Character State

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

bool IsWndVisible(CXWnd* wnd)
{
	return wnd && wnd->IsVisible();
}

int GetStackSize(ItemClient* Item)
{
	return Item->GetType() == ITEMTYPE_NORMAL
		&& Item->IsStackable() ? Item->GetMaxItemCount() : 1;
}

int SetSettingInt(const char* Key, const char* Val, int maxValue)
{
	int result = GetIntFromString(Val, 0);
	if (result && result > maxValue)
	{
		result = maxValue;
	}

	if (Key[0])
	{
		WritePrivateProfileInt("MQ2Cursor", Key, result, INIFileName);
	}

	if (result == 0)
	{
		WriteChatf("MQ2Cursor::%s (\aroff\ax)", Key);
	}
	else
	{
		WriteChatf("MQ2Cursor::%s (\ag%d\ax)", Key, result);
	}

	return result;
}

int SetSettingBool(const char* Key, const char* Val, bool currentValue)
{
	bool result = GetIntFromString(Val, !currentValue);

	if (Key[0])
	{
		WritePrivateProfileBool("MQ2Cursor", Key, result, INIFileName);
	}

	WriteChatf("MQ2Cursor::%s (%s)", Key, result ? "\agon\ax" : "\agoff\ax");
	return result;
}

PluginFlags UpdateFlags()
{
	PluginFlags flags = Flag_None;

	if (IsWndVisible(FindMQ2Window("GuildTributeMasterWnd")))
		flags |= Flag_GuildTributeMasterWnd;
	if (IsWndVisible(FindMQ2Window("TributeMasterWnd")))
		flags |= Flag_TributeMasterWnd;
	if (IsWndVisible(FindMQ2Window("GuildBankWnd")))
		flags |= Flag_GuildBankWnd;
	if (IsWndVisible(pTradeWnd))
		flags |= Flag_TradeWnd;
	if (IsWndVisible(pMerchantWnd))
		flags |= Flag_MerchantWnd;
	if (IsWndVisible(pBankWnd))
		flags |= Flag_BankWnd;
	if (IsWndVisible(pGiveWnd))
		flags |= Flag_GiveWnd;
	if (IsWndVisible(pSpellBookWnd))
		flags |= Flag_SpellBookWnd;
	if (IsWndVisible(pLootWnd))
		flags |= Flag_LootWnd;
	if (IsWndVisible(pInventoryWnd))
		flags |= Flag_InventoryWnd;
	if (IsWndVisible(pCastingWnd))
		flags |= Flag_CastingWnd;
	if (pLocalPC->standstate == STANDSTATE_CASTING)
		flags |= Flag_SpellCasting;
	if (pLocalPlayer->CastingData.SpellSlot != 255)
		flags |= Flag_SpellInProgress;
	if (pLocalPC->Stunned)
		flags |= Flag_Stunned;

	// ???
	if ((flags & Flag_BusyInventory) != Flag_BusyInventory && flags & Flag_BusyInventory)
		flags |= Flag_BusyInventoryFlag;

	return flags;
}

ItemClient* GetCursorItem()
{
	return GetPcProfile()->GetInventorySlot(InvSlot_Cursor);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

class KeepRec
{
public:
	std::string name;
	int id;
	int qty;

	KeepRec(std::string_view name, int id, int qty)
		: name(name)
		, id(id)
		, qty(qty)
	{
	}

	KeepRec(std::string_view id_, const char* str)
	{
		char Buffer[MAX_STRING];
		name = GetArg(Buffer, str, 1, false, false, false, '|');
		qty = GetIntFromString(GetArg(Buffer, str, 2, false, false, false, '|'), 0);
		id = GetIntFromString(id_, 0);
	}

	enum DisplayAction
	{
		Display_Delete,
		Display_Insert,
		Display_Listing,
	};

	void Display(DisplayAction action) const
	{
		if (action != Display_Delete)
		{
			char buffer[32];
			if (qty < -1)
				strcpy_s(buffer, " \agPROTECTED\ax");
			else if (qty < 0)
				strcpy_s(buffer, " QTY[\agALL\ax]");
			else if (!qty)
				strcpy_s(buffer, " QTY[\agNONE\ax]");
			else
				sprintf_s(buffer, " QTY[\ag%d\ax]", qty);

			if (action == Display_Insert)
				WriteChatf("[\ag+\ax] ID[\ag%d\ax] NAME[\ag%s\ax]%s.", id, name.c_str(), buffer);
			else
				WriteChatf("[\ay=\ax] ID[\ag%d\ax] NAME[\ag%s\ax]%s FIND[\ag%d\ax].", id, name.c_str(), buffer, CountItemByID(id, BAG_SLOT_START));
		}
		else
		{
			WriteChatf("[\ar-\ax] ID[\ag%d\ax] NAME[\ag%s\ax].", id, name.c_str());
		}
	}
};

class CursorKeepList
{
	std::map<int, KeepRec> m_data;
	std::string m_section;

public:
	explicit CursorKeepList(std::string_view section)
		: m_section(section)
	{
	}

	KeepRec* Find(int key)
	{
		auto iter = m_data.find(key);
		if (iter == m_data.end())
			return nullptr;

		return &iter->second;
	}

	void Delete(int key, bool quiet)
	{
		auto iter = m_data.find(key);
		if (iter != m_data.end())
		{
			if (!quiet)
			{
				iter->second.Display(KeepRec::Display_Delete);
			}

			m_data.erase(iter);
		}
	}

	void Insert(KeepRec&& rec, bool quiet)
	{
		Delete(rec.id, true);
		auto [iter, _] = m_data.emplace(rec.id, std::move(rec));

		if (!quiet)
		{
			iter->second.Display(KeepRec::Display_Insert);
		}
	}

	void Import()
	{
		std::vector<std::string> keys = mq::GetPrivateProfileKeys<MAX_STRING * 10>(m_section, INIFileName);
		char Temp[MAX_STRING];

		for (const std::string& key : keys)
		{
			GetPrivateProfileString(m_section, key.c_str(), "", Temp, MAX_STRING, INIFileName);

			if (Temp[0])
			{
				Insert(KeepRec(key.c_str(), Temp), true);
			}
		}
	}

	void Export()
	{
		::WritePrivateProfileStringA(m_section.c_str(), nullptr, nullptr, INIFileName);

		char buf[MAX_STRING];
		for (const auto& [_, rec] : m_data)
		{
			if (rec.qty != 0)
				sprintf_s(buf, "%s|%d", rec.name.c_str(), rec.qty);
			else
				strcpy_s(buf, rec.name.c_str());

			WritePrivateProfileString(m_section, std::to_string(rec.id), buf, INIFileName);
		}
	}

	void PrintListing(const char* Search)
	{
		for (const auto& [_, rec] : m_data)
		{
			if (!Search[0] || find_substr(rec.name, Search))
			{
				rec.Display(KeepRec::Display_Listing);
			}
		}
	}
};

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

void DestroyCommand()
{
	if (s_pluginFlags & Flag_AutoPause)
		return;

	PcProfile* pcProfile = GetPcProfile();

	if (pcProfile->CursorPlat)
	{
		if (!s_quiet)
		{
			WriteChatColor("MQ2Cursor::\ayDESTROYING\ax <\arPlat\ax>.");
		}
	}
	else if (pcProfile->CursorGold)
	{
		if (!s_quiet)
		{
			WriteChatColor("MQ2Cursor::\ayDESTROYING\ax <\arGold\ax>.");
		}
	}
	else if (pcProfile->CursorSilver)
	{
		if (!s_quiet)
		{
			WriteChatColor("MQ2Cursor::\ayDESTROYING\ax <\arSilver\ax>.");
		}
	}
	else if (pcProfile->CursorCopper)
	{
		if (!s_quiet)
		{
			WriteChatColor("MQ2Cursor::\ayDESTROYING\ax <\arCopper\ax>.");
		}
	}
	else if (ItemClient* cursorItem = GetCursorItem())
	{
		if (!s_quiet)
		{
			WriteChatf("MQ2Cursor::\ayDESTROYING\ax <\ar%s\ax>.", cursorItem->GetName());
		}
	}
	else
	{
		return;
	}

	DoCommand("/destroy", false);
}

void SendWornClick(const char* screenID, unsigned int ulKeyState)
{
	if (pInventoryWnd)
	{
		if (CXWnd* wndInv = pInventoryWnd->GetChildItem(screenID))
		{
			int keyboardFlags = *reinterpret_cast<unsigned int*>(&pWndMgr->KeyboardFlags);
			*reinterpret_cast<unsigned int*>(&pWndMgr->KeyboardFlags) = ulKeyState;
			SendWndClick2(wndInv, "leftmouseup");
			*reinterpret_cast<unsigned int*>(&pWndMgr->KeyboardFlags) = keyboardFlags;
		}
	}
}

void KeepCommand(PlayerClient* = nullptr, const char* = nullptr)
{
	if (s_pluginFlags & Flag_AutoPause)
	{
		WriteChatf("MQ2Cursor:: Conditions prevent item movement.");
		return;
	}

	ItemClient* cursorItem = GetCursorItem();
	if (!cursorItem)
		return;

	int lSlot = 0;

	// if there is a pack on cursor
	if (cursorItem->IsContainer())
	{
		lSlot = FindSlotForPack();
		if (lSlot == NOID)
		{
			WriteChatf("MQ2Cursor:: No room for another non-empty pack");
			return;
		}
	}
	else
	{
		lSlot = FreeSlotForItem(cursorItem);
	}

	// if there is a main inv slot for this move
	if (lSlot)
	{
		if (!s_quiet)
		{
			WriteChatf("MQ2Cursor::\ayKEEPING\ax <\ag%s\ax>.", cursorItem->GetName());
		}

		if (lSlot == NOID) // if the slot is accessed via autoinventory
		{
			EzCommand("/autoinventory");
		}
		else
		{
			char szInvSlot[20];
			sprintf_s(szInvSlot, "InvSlot%d", lSlot);
			SendWornClick(szInvSlot, 1);

			// if we swapped something (bag on cursor) autoinv it
			if (CursorHasItem())
			{
				EzCommand("/autoinventory");
			}
		}
	}
}

void HandleCommand()
{
	if (s_pluginFlags & Flag_AutoPause)
		return;

	if (ItemClient* cursorItem = GetCursorItem())
	{
		int itemID = -1;

		while (cursorItem && cursorItem->GetID() != itemID)
		{
			itemID = cursorItem->GetID();

			if (KeepRec* LookUp = s_keepList->Find(itemID))
			{
				if (LookUp->qty < 0 || LookUp->qty - CountItemByID(itemID) > 0)
					KeepCommand();
				else
					DestroyCommand();
			}
			else if (!s_quiet)
			{
				steady_clock::time_point now = steady_clock::now();

				if (s_cursorWarnItemID != itemID || now > s_cursorWarnTime)
				{
					WriteChatf("MQ2Cursor::\ayREQUIRE\ax Instruction <\ag%s\ax>%s.", cursorItem->GetName(), cursorItem->IsStackable() ? " [\aySTACKABLE\ax]" : "");

					s_cursorWarnItemID = itemID;
					s_cursorWarnTime = now + CURSOR_SPAM_DELAY;
					return;
				}
			}

			cursorItem = GetCursorItem();
		}
	}
}

void CursorCommand(PlayerClient*, const char* zLine)
{
	bool showHelp = false;
	char param1[MAX_STRING];
	GetArg(param1, zLine, 1);
	char param2[MAX_STRING];
	GetArg(param2, zLine, 2);

	if ((!param1[0] && !CursorHasItem()) || ci_equals("help", param1))
	{
		showHelp = true;
	}
	else if (ci_equals("load", param1))
	{
		WriteChatColor("MQ2Cursor::\ayLOADING\ax Item List...");
		s_keepList->Import();
	}
	else if (ci_equals("save", param1))
	{
		WriteChatColor("MQ2Cursor::\aySAVING\ax Item List...");
		s_keepList->Export();
	}
	else if (ci_equals("list", param1))
	{
		WriteChatColor("MQ2Cursor::\ayLISTING\ax Item List...");
		WriteChatColor("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");

		s_keepList->PrintListing(param2);

		WriteChatColor("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");

	}
	else if (ci_equals("silent", param1) || ci_equals("quiet", param1))
	{
		s_quiet = SetSettingBool("Silent", param2, s_quiet);
	}
	else if (ci_equals("on", param1) || ci_equals("true", param1))
	{
		s_cursorActive = SetSettingBool("Active", "on", s_cursorActive);
	}
	else if (ci_equals("off", param1) || ci_equals("false", param1))
	{
		s_cursorActive = SetSettingBool("Active", "off", s_cursorActive);
	}
	else if (ci_equals("auto", param1))
	{
		s_cursorActive = SetSettingBool("Active", "", s_cursorActive);
	}
	else if (ci_equals("random", param1))
	{
		s_randomizeCursor = SetSettingInt("Random", param2, 15000);
	}
	else
	{
		ItemClient* cursorItem = GetCursorItem();

		if (ci_starts_with(param1, "rem") || ci_starts_with(param1, "del"))
		{
			if (IsNumber(param2))
			{
				WriteChatf("MQ2Cursor::\ayDELETING ENTRY\ax <\ag%s\ax>.", cursorItem->GetName());

				s_keepList->Delete(atol(param2), s_quiet);
				s_keepList->Export();
				return;
			}

			if (cursorItem)
			{
				WriteChatf("MQ2Cursor::\ayDELETING ENTRY\ax <\ag%s\ax>.", cursorItem->GetName());

				s_keepList->Delete(cursorItem->GetID(), s_quiet);
				s_keepList->Export();
				return;
			}

			showHelp = true;
		}

		if (cursorItem && !showHelp)
		{
			if (param1[0]
				&& (IsNumber(param1)
					|| ci_starts_with(param1, "al") || ci_starts_with(param1, "pro")))
			{
				int howMany = atol(param1);

				if (ci_starts_with(param1, "pro"))
					howMany = -2;
				else if (ci_starts_with(param1, "al"))
					howMany = -1;
				else if (ci_starts_with(param2, "st"))
					howMany *= GetStackSize(cursorItem);

				if (howMany < -1)
					WriteChatf("MQ2Cursor::\ayPROTECT\ax <\ag%s\ax> [\agALL\ax].", cursorItem->GetName());
				else if (howMany < 0)
					WriteChatf("MQ2Cursor::\ayKEEPING\ax <\ag%s\ax> [\agALL\ax].", cursorItem->GetName());
				else if (howMany > 0)
					WriteChatf("MQ2Cursor::\ayKEEPING\ax <\ag%s\ax> Up To [\ag%d\ax]", cursorItem->GetName(), howMany);
				else
					WriteChatf("MQ2Cursor::\ayDESTROYING\ax <\ag%s\ax> [\agALL\ax].", cursorItem->GetName());

				s_keepList->Insert(KeepRec(cursorItem->GetName(), cursorItem->GetID(), howMany), s_quiet);
				s_keepList->Export();
				return;
			}

			if (s_cursorActive)
			{
				HandleCommand();
				return;
			}

			showHelp = true;
		}
	}

	if (showHelp)
	{
		WriteChatColor("Usage:");
		WriteChatColor("    /cursor on|off");
		WriteChatColor("    /cursor silent on|off");
		WriteChatColor("    /cursor rem(ove)|del(ete) id|itemoncursor");
		WriteChatColor("    /cursor load|save|list|help");
		WriteChatColor("    /cursor al(l(ways))");
		WriteChatColor("    /cursor pro(tect)");
		WriteChatColor("    /cursor #[ st(acks)]");
		WriteChatColor("    /cursor random #");
	}
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

PLUGIN_API void SetGameState(DWORD GameState)
{
	if (GameState == GAMESTATE_INGAME)
	{
		if (!s_initialized && pLocalPC != nullptr)
		{
			s_initialized = true;
			sprintf_s(INIFileName, "%s\\%s_%s.ini", gPathConfig, GetServerShortName(), pLocalPC->Name);
			s_keepList = new CursorKeepList("MQ2Cursor_ItemList");
			s_keepList->Import();
			s_cursorActive = GetPrivateProfileBool("MQ2Cursor", "Active", false, INIFileName);
			s_quiet = GetPrivateProfileBool("MQ2Cursor", "Silent", false, INIFileName);
			s_randomizeCursor = GetPrivateProfileInt("MQ2Cursor", "Random", 0, INIFileName);
		}
	}
	else if (GameState != GAMESTATE_LOGGINGIN)
	{
		if (s_initialized)
		{
			delete s_keepList;
			s_keepList = nullptr;
			s_cursorActive = false;
			s_initialized = false;
		}
	}
}

PLUGIN_API void InitializePlugin()
{
	AddCommand("/cursor", CursorCommand);
	AddCommand("/keep", KeepCommand);
}

PLUGIN_API void ShutdownPlugin()
{
	delete s_keepList;
	s_keepList = nullptr;

	RemoveCommand("/cursor");
	RemoveCommand("/keep");
}

PLUGIN_API void OnPulse()
{
	if (!s_initialized || !gbInZone || !pLocalPlayer || !pLocalPC)
		return;

	s_pluginFlags = UpdateFlags();
	if (s_pluginFlags & Flag_AutoPause)
		return;

	if (s_cursorActive && GetCursorItem())
	{
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		if (now < s_nextExecute)
			return;

		if (s_randomTimer.time_since_epoch().count() == 0 && s_randomizeCursor != 0)
		{
			s_randomTimer = now + std::chrono::milliseconds(s_randomizeCursor * rand() / RAND_MAX);
		}

		if (now >= s_randomTimer || IsCasting())
		{
			HandleCommand();

			s_randomTimer = {};
			s_nextExecute = now + CURSOR_WAIT_DELAY;
		}
	}
}

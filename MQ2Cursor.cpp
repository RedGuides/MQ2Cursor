//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
// Project: MQ2Cursor.cpp   | Fix Random Humanish Delay that wasn't working!
// Author: s0rCieR          | Make it more user friendly for twisting bard!
// Updated: eqmule 12/15/14 | Updated to work with The Darkened Sea expansion
// 4.0 - Eqmule 07-22-2016 - Added string safety.
// 4.1 - Sym - 06-13-2017 - Removed forced 1 count for non-stackable keep counts
//                          and added notification when removing an entry
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

#include <mq/Plugin.h>
#include <moveitem.h>

PreSetup("MQ2Cursor");
PLUGIN_VERSION(4.1);

#define    PLUGIN_FLAG      0xF9FF      // Plugin Auto-Pause Flags (see InStat)
#define    CURSOR_SPAM       15000      // Cursor Spam Rate Instruction in ms.
#define    CURSOR_WAIT         250    // Cursor Wait After Manipulation

DWORD            Initialized =false;       // Plugin Initialized?
DWORD            Conditions  =false;       // Window Conditions and Character State
DWORD            SkipExecuted=false;       // Skip Executed Timer

PCONTENTS     InvCont     =NULL;           // ItemCounts/Locate/Search Contents
long          InvSlot     =NOID;           // ItemCounts/Locate/Search Slot ID

PCONTENTS     CursorContents();
long          InStat();
long          ItemCounts(DWORD ID, long B=0, long E=NUM_INV_SLOTS);
long          SetBOOL(long Cur, const char* Val, const char* Sec="", const char* Key="");
long          SetLONG(long Cur, const char* Val, const char* Sec="", const char* Key="", bool ZeroIsOff=false);
long          StackSize(PCONTENTS Item);
long          StackUnit(PCONTENTS Item);
bool          WinState(CXWnd *Wnd);

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

bool WinState(CXWnd *Wnd) {
	return (Wnd && Wnd->IsVisible());
}

long StackUnit(PCONTENTS Item) {
	PITEMINFO pItemInfo = GetItemFromContents(Item);
	return (pItemInfo && pItemInfo->Type == ITEMTYPE_NORMAL && Item->IsStackable()) ? Item->StackCount : 1;
}

long StackSize(PCONTENTS Item) {
	PITEMINFO pItemInfo = GetItemFromContents(Item);
	return (pItemInfo && pItemInfo->Type == ITEMTYPE_NORMAL && Item->IsStackable()) ? pItemInfo->StackSize : 1;
}

long SetLONG(long Cur, const char* Val, const char* Sec, const char* Key, bool ZeroIsOff, long Maxi) {
	char ToStr[16]; char Buffer[128]; long Result=atol(Val);
	if(Result && Result>Maxi) Result=Maxi;
	_itoa_s(Result,ToStr,10);
	if(Sec[0] && Key[0]) WritePrivateProfileString(Sec,Key,ToStr,INIFileName);
	sprintf_s(Buffer,"%s::%s (\ag%s\ax)",Sec,Key,(ZeroIsOff && !Result)?"\aroff":ToStr);
	WriteChatColor(Buffer);
	return Result;
}

long SetBOOL(long Cur, const char* Val, const char* Sec, const char* Key) {
	char buffer[128]; long result=0;
	if(!_strnicmp(Val,"false",5) || !_strnicmp(Val,"off",3) || !_strnicmp(Val,"0",1))    result=0;
	else if(!_strnicmp(Val,"true",4) || !_strnicmp(Val,"on",2) || !_strnicmp(Val,"1",1)) result=1;
	else result=(!Cur)&1;
	if(Sec[0] && Key[0]) WritePrivateProfileString(Sec,Key,result?"1":"0",INIFileName);
	sprintf_s(buffer,"%s::%s (%s)",Sec,Key,result?"\agon\ax":"\agoff\ax");
	WriteChatColor(buffer);
	return result;
}

long InStat() {
	Conditions=0x00000000;
	if(WinState(FindMQ2Window("GuildTributeMasterWnd")))                Conditions|=0x0001;
	if(WinState(FindMQ2Window("TributeMasterWnd")))                     Conditions|=0x0002;
	if(WinState(FindMQ2Window("GuildBankWnd")))                         Conditions|=0x0004;
	if(WinState((CXWnd*)pTradeWnd))                                     Conditions|=0x0008;
	if(WinState((CXWnd*)pMerchantWnd))                                  Conditions|=0x0010;
	if(WinState((CXWnd*)pBankWnd))                                      Conditions|=0x0020;
	if(WinState((CXWnd*)pGiveWnd))                                      Conditions|=0x0040;
	if(WinState((CXWnd*)pSpellBookWnd))                                 Conditions|=0x0080;
	if(WinState((CXWnd*)pLootWnd))                                      Conditions|=0x0200;
	if(WinState((CXWnd*)pInventoryWnd))                                 Conditions|=0x0400;
	if(WinState((CXWnd*)pCastingWnd))                                   Conditions|=0x1000;
	if(GetCharInfo()->standstate==STANDSTATE_CASTING)                   Conditions|=0x2000;
	if(((((PSPAWNINFO)pCharSpawn)->CastingData.SpellSlot)&0xFF)!=0xFF) Conditions|=0x4000;
	if(GetCharInfo()->Stunned)                                          Conditions|=0x0100;
	if((Conditions&0x0600)!=0x0600 && (Conditions&0x0600))            Conditions|=0x0800;
	return Conditions;
}

PCONTENTS CursorContents() {
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

	void Display(char action) const
	{
		if (action != '-')
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

			if (action == '+')
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

class ListRec
{
	std::map<int, KeepRec> data;
	std::string section;

public:
	explicit ListRec(std::string_view section)
		: section(section)
	{
	}

	KeepRec* Find(int key)
	{
		auto iter = data.find(key);
		if (iter == data.end())
			return nullptr;

		return &iter->second;
	}

	void Delete(int key, bool quiet)
	{
		auto iter = data.find(key);
		if (iter != data.end())
		{
			if (!quiet)
				iter->second.Display('-');

			data.erase(iter);
		}
	}

	void Insert(const KeepRec& rec, bool quiet)
	{
		Delete(rec.id, true);
		data.emplace(rec.id, rec);

		if (!quiet)
			rec.Display('+');
	}

	void Import(const char* Title)
	{
		if (Title[0])
			WriteChatColor(Title);

		std::vector<std::string> keys = mq::GetPrivateProfileKeys<MAX_STRING * 10>(section, INIFileName);
		char Temp[MAX_STRING];

		for (const std::string& key : keys)
		{
			GetPrivateProfileString(section, key.c_str(), "", Temp, MAX_STRING, INIFileName);
			if (Temp[0])
				Insert(KeepRec(key.c_str(), Temp), true);
		}
	}

	void Export(const char* Title)
	{
		if (Title[0])
			WriteChatColor(Title);

		WritePrivateProfileString(section, nullptr, nullptr, INIFileName);

		char BUF[MAX_STRING];
		for (const auto& [_, rec] : data)
		{
			if (rec.qty != 0)
				sprintf_s(BUF, "%s|%d", rec.name.c_str(), rec.qty);
			else
				strcpy_s(BUF, rec.name.c_str());

			WritePrivateProfileString(section, std::to_string(rec.id), BUF, INIFileName);
		}
	}

	void Listing(const char* Title, const char* Search)
	{
		if (Title[0])
			WriteChatColor(Title);

		WriteChatColor("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");

		for (const auto& [_, rec] : data)
		{
			if (!Search[0] || find_substr(rec.name, Search))
				rec.Display('=');
		}

		WriteChatColor("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");
	}
};

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

bool       CursorHandle      = false;    // Cursor Handle?
bool       CursorSilent      = false;    // Cursor Silent Operating?
long       CursorWarnItem    = NOID;     // Cursor Warned Item ID
long       CursorWarnTime    = 0;        // Cursor Warned Time ID
long       CursorRandom      = 0;        // Cursor Random Wait
DWORD      CursorTimer       = 0;        // Cursor Timer
ListRec*   CursorList        = nullptr;  // Cursor Keeping List

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

void DestroyCommand()
{
	if(!(PLUGIN_FLAG&(Conditions)))
	{
		char Buffers[128]; const char* Display=Buffers;
		if(GetPcProfile()->CursorPlat)        Display="MQ2Cursor::\ayDESTROYING\ax <\arPlat\ax>.";
		else if(GetPcProfile()->CursorGold)   Display="MQ2Cursor::\ayDESTROYING\ax <\arGold\ax>.";
		else if(GetPcProfile()->CursorSilver) Display="MQ2Cursor::\ayDESTROYING\ax <\arSilver\ax>.";
		else if(GetPcProfile()->CursorCopper) Display="MQ2Cursor::\ayDESTROYING\ax <\arCopper\ax>.";
		else if(PCONTENTS Cursor=CursorContents()) sprintf_s(Buffers,"MQ2Cursor::\ayDESTROYING\ax <\ar%s\ax>.", GetItemFromContents(Cursor)->Name);
		else return;
		if(!CursorSilent) WriteChatColor(Display);
		EQDestroyHeldItemOrMoney(GetCharInfo()->pSpawn,NULL);
	}
}

// dropping items on the ground is no longer possible
/*
void DropCommand() {
	if(!(PLUGIN_FLAG&(Conditions))) if(PCONTENTS Cursor=CursorContents()) if(Cursor->Item->NoDrop) {
		if(!CursorSilent) WriteChatf("MQ2Cursor::\ayDROPPING\ax <\ar%s\ax>.",Cursor->Item->Name);
		DropCmd(GetCharInfo()->pSpawn,NULL);
	}
}
*/

void SendWornClick(char* pcScreenID, unsigned long ulKeyState)
{
	if (pInventoryWnd)
	{
		if (CXWnd* wndInv = pInventoryWnd->GetChildItem(pcScreenID))
		{
			int KeyboardFlags[4] = { 0 };
			*(unsigned long*)&KeyboardFlags = *(unsigned long*)&pWndMgr->KeyboardFlags;
			*(unsigned long*)&pWndMgr->KeyboardFlags = ulKeyState;
			SendWndClick2(wndInv, "leftmouseup");
			*(unsigned long*)&pWndMgr->KeyboardFlags = *(unsigned long*)&KeyboardFlags;
		}
	}
}

void KeepCommand(PlayerClient*, const char* zLine)
{
	if (PLUGIN_FLAG&(Conditions=InStat()))
	{
		WriteChatf("MQ2Cursor:: Conditions prevent item movement.");
		return;
	}
	PCONTENTS Cursor = CursorContents();
	if (!Cursor) return;

	long fSLOT=0; long fSWAP=0; long fSIZE=10;

	long lSlot = 0;

	// if there is a pack on cursor
	if (TypePack(Cursor))
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
		lSlot = FreeSlotForItem(Cursor);
	}

	// if there is a main inv slot for this move
	if (lSlot)
	{
		if(!CursorSilent) WriteChatf("MQ2Cursor::\ayKEEPING\ax <\ag%s\ax>.", GetItemFromContents(Cursor)->Name);

		char szInvSlot[20];
		if (lSlot == NOID) // if the slot is accessed via autoinventory
		{
			EzCommand("/autoinventory");
		}
		else
		{
			sprintf_s(szInvSlot, "InvSlot%d", lSlot);
			SendWornClick(szInvSlot, 1);
			// if we swapped something (bag on cursor) autoinv it
			if (CursorHasItem())
				EzCommand("/autoinventory");
		}
	}
}

void HandleCommand()
{
	if (!(PLUGIN_FLAG&(Conditions))) if(PCONTENTS Cursor=CursorContents()) {
		DWORD LastNumber=0xFFFFFFFF;
		PITEMINFO pCursor = GetItemFromContents(Cursor);
		while (pCursor && pCursor->ItemNumber != LastNumber)
		{
			LastNumber = pCursor->ItemNumber;
			if (KeepRec *LookUp=CursorList->Find(LastNumber))
			{
				if (LookUp->qty<0 || LookUp->qty - (long)CountItemByID(LastNumber)>0)        KeepCommand(NULL,"");
				//else if(CursorDroping && Cursor->Item->NoDrop)                        DropCommand();
				else                                                                  DestroyCommand();
			}
			else if (!CursorSilent && ((DWORD)CursorWarnItem!=LastNumber || clock()>CursorWarnTime))
			{
				WriteChatf("MQ2Cursor::\ayREQUIRE\ax Instruction <\ag%s\ax>%s.", pCursor->Name, ItemIsStackable(Cursor) ? " [\aySTACKABLE\ax]" : "");
				CursorWarnItem=LastNumber;
				CursorWarnTime=clock()+CURSOR_SPAM;
				return;
			}
			Cursor=CursorContents();
		}
	}
}

void CursorCommand(PSPAWNINFO pCHAR, PCHAR zLine)
{
	bool NeedHelp=false;
	char Parm1[MAX_STRING]; GetArg(Parm1,zLine,1);
	char Parm2[MAX_STRING]; GetArg(Parm2,zLine,2);
	if((!Parm1[0] && !CursorHasItem()) || !_stricmp("help",Parm1)) NeedHelp=true;
	else if(!_stricmp("load",Parm1)) CursorList->Import("MQ2Cursor::\ayLOADING\ax Item List...");
	else if(!_stricmp("save",Parm1)) CursorList->Export("MQ2Cursor::\aySAVING\ax Item List...");
	else if(!_stricmp("list",Parm1)) CursorList->Listing("MQ2Cursor::\ayLISTING\ax Item List...",Parm2);
	//else if(!_stricmp("nodrop",Parm1) || !_stricmp("dropping",Parm1))
		//CursorDroping=SetBOOL(CursorDroping,Parm2,"MQ2Cursor","Droping");
	else if(!_stricmp("silent",Parm1) || !_stricmp("quiet",Parm1))
		CursorSilent=SetBOOL(CursorSilent ,Parm2,"MQ2Cursor","Silent");
	else if(!_stricmp("on",Parm1) || !_stricmp("true",Parm1))
		CursorHandle=SetBOOL(CursorHandle ,"on" ,"MQ2Cursor","Active");
	else if(!_stricmp("off",Parm1) || !_stricmp("false",Parm1))
		CursorHandle=SetBOOL(CursorHandle ,"off","MQ2Cursor","Active");
	else if(!_stricmp("auto",Parm1))
		CursorHandle=SetBOOL(CursorHandle ,""   ,"MQ2Cursor","Active");
	else if(!_stricmp("random",Parm1))
		CursorRandom=SetLONG(CursorRandom,Parm2 ,"MQ2Cursor","Random",true,15000);
	else {
		PCONTENTS Cursor=CursorContents();
		if (!_strnicmp("rem", Parm1, 3) || !_strnicmp("del", Parm1, 3))
		{
			if (IsNumber(Parm2))
			{
				WriteChatf("MQ2Cursor::\ayDELETING ENTRY\ax <\ag%s\ax>.", GetItemFromContents(Cursor)->Name);
				CursorList->Delete(atol(Parm2), CursorSilent);
				CursorList->Export("");
				return;
			}

			if (Cursor)
			{
				WriteChatf("MQ2Cursor::\ayDELETING ENTRY\ax <\ag%s\ax>.", Cursor->GetItemDefinition()->Name);
				CursorList->Delete(Cursor->GetItemDefinition()->ItemNumber, CursorSilent);
				CursorList->Export("");
				return;
			}
			NeedHelp = true;
		}
		if(Cursor && !NeedHelp)
		{
			if(Parm1[0] && (IsNumber(Parm1) || !_strnicmp(Parm1,"al",2) || !_strnicmp(Parm1,"pro",3)))
			{
				long HowMany=atol(Parm1);
				if(!_strnicmp(Parm1,"pro",3))        HowMany=-2;
				else if(!_strnicmp(Parm1,"al",2))    HowMany=-1;
				else if(!_strnicmp(Parm2,"st",2))    HowMany*=StackSize(Cursor);
				//if(HowMany>1 && !ItemIsStackable(Cursor)) HowMany=1;
				PITEMINFO pCursor = GetItemFromContents(Cursor);
				if(HowMany < -1)    WriteChatf("MQ2Cursor::\ayPROTECT\ax <\ag%s\ax> [\agALL\ax].", pCursor->Name);
				else if(HowMany <0) WriteChatf("MQ2Cursor::\ayKEEPING\ax <\ag%s\ax> [\agALL\ax].", pCursor->Name);
				else if(HowMany >0) WriteChatf("MQ2Cursor::\ayKEEPING\ax <\ag%s\ax> Up To [\ag%d\ax]", pCursor->Name, HowMany);
				else                WriteChatf("MQ2Cursor::\ayDESTROYING\ax <\ag%s\ax> [\agALL\ax].", pCursor->Name);
				CursorList->Insert(KeepRec(pCursor->Name, pCursor->ItemNumber, HowMany), CursorSilent);
				CursorList->Export("");
				return;
			}
			if(CursorHandle)
			{
				HandleCommand();
				return;
			}
			NeedHelp=true;
		}
	}
	if(NeedHelp)
	{
		WriteChatColor("Usage:");
		WriteChatColor("       /cursor on|off");
		WriteChatColor("       /cursor silent on|off");
		WriteChatColor("       /cursor rem(ove)|del(ete) id|itemoncursor");
		WriteChatColor("       /cursor load|save|list|help");
		WriteChatColor("       /cursor al(l(ways))");
		WriteChatColor("       /cursor pro(tect)");
		WriteChatColor("       /cursor #[ st(acks)]");
		WriteChatColor("       /cursor random #");
	}
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

PLUGIN_API VOID SetGameState(DWORD GameState) {
	if(GameState==GAMESTATE_INGAME) {
		if(!Initialized) {
			Initialized=true;
			sprintf_s(INIFileName,"%s\\%s_%s.ini", gPathConfig, GetServerShortName(), pLocalPC->Name);
			CursorList=new ListRec("MQ2Cursor_ItemList");
			CursorList->Import("");
			CursorHandle   =GetPrivateProfileInt("MQ2Cursor","Active"  ,0,INIFileName);
			CursorSilent   =GetPrivateProfileInt("MQ2Cursor","Silent"  ,0,INIFileName);
			CursorRandom   =GetPrivateProfileInt("MQ2Cursor","Random"  ,0,INIFileName);
			//CursorDroping  =GetPrivateProfileInt("MQ2Cursor","Droping" ,0,INIFileName);
		}
	} else if (GameState != GAMESTATE_LOGGINGIN) {
		if (Initialized) {
			if (CursorList) delete CursorList;
			CursorList = nullptr;
			CursorHandle = false;
			Initialized = 0;
		}
	}
}

PLUGIN_API VOID InitializePlugin() {
	AddCommand("/cursor",CursorCommand);
	AddCommand("/keep",KeepCommand);
}

PLUGIN_API VOID ShutdownPlugin() {
	if(CursorList) delete CursorList;
	RemoveCommand("/cursor");
	RemoveCommand("/keep");
}

PLUGIN_API VOID OnPulse()
{
	if(Initialized && gbInZone && pCharSpawn && GetPcProfile() && !(PLUGIN_FLAG&InStat()))
	{
		DWORD PulseTimer=(DWORD)clock();
		if(CursorHandle && CursorContents() && PulseTimer>SkipExecuted)
		{
			if (!CursorTimer && CursorRandom) CursorTimer = PulseTimer + (CursorRandom * rand() / RAND_MAX);
			if (PulseTimer >= CursorTimer || IsCasting())
			{
				HandleCommand();
				CursorTimer = 0;
				SkipExecuted = PulseTimer + CURSOR_WAIT;
			}
		}
	}
}
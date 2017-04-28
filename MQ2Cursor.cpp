//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
// Projet: MQ2Cursor.cpp    | Fix Random Humanish Delay that was'nt working!
// Author: s0rCieR          | Make it more user friendly for twisting bard!
// Updated: eqmule 12/15/14 | Updated to work with The Darkened Sea expansion
// 4.0 - Eqmule 07-22-2016 - Added string safety.
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

#define    PLUGIN_NAME  "MQ2Cursor"     // Plugin Name
#define    PLUGIN_DATE    20160725      // Plugin Date
#define    PLUGIN_VERS       4.0        // Plugin Version
#define    PLUGIN_FLAG      0xF9FF      // Plugin Auto-Pause Flags (see InStat)
#define    CURSOR_SPAM       15000      // Cursor Spam Rate Instruction in ms.
#define    CURSOR_WAIT         250    // Cursor Wait After Manipulation

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

#ifndef PLUGIN_API
  #include "../MQ2Plugin.h"
  PreSetup(PLUGIN_NAME);
  PLUGIN_VERSION(PLUGIN_VERS);
  #include <map>
#endif PLUGIN_API

#include "../moveitem.h"

DWORD            Initialized   =false;            // Plugin Initialized?
DWORD            Conditions   =false;            // Window Conditions and Character State
DWORD            SkipExecuted=false;            // Skip Executed Timer

PCONTENTS     InvCont     =NULL;           // ItemCounts/Locate/Search Contents
long          InvSlot     =NOID;           // ItemCounts/Locate/Search Slot ID

PCONTENTS     CursorContents();
long          InStat();
long          ItemCounts(DWORD ID, long B=0, long E=NUM_INV_SLOTS);
long          SetBOOL(long Cur, PCHAR Val, PCHAR Sec="", PCHAR Key="");
long          SetLONG(long Cur, PCHAR Val, PCHAR Sec="", PCHAR Key="", bool ZeroIsOff=false);
long          StackSize(PCONTENTS Item);
long          StackUnit(PCONTENTS Item);
bool          WinState(CXWnd *Wnd);

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
template <unsigned int _Size>LPSTR SafeItoa(int _Value,char(&_Buffer)[_Size], int _Radix)
{
	errno_t err = _itoa_s(_Value, _Buffer, _Radix);
	if (!err) {
		return _Buffer;
	}
	return "";
}
bool WinState(CXWnd *Wnd) {
    return (Wnd && ((PCSIDLWND)Wnd)->dShow);
}

long StackUnit(PCONTENTS Item) {
    PITEMINFO pItemInfo = GetItemFromContents(Item);
    return (pItemInfo && pItemInfo->Type == ITEMTYPE_NORMAL && ((EQ_Item*)Item)->IsStackable() == 1) ? Item->StackCount : 1;
}

long StackSize(PCONTENTS Item) {
    PITEMINFO pItemInfo = GetItemFromContents(Item);
    return (pItemInfo && pItemInfo->Type == ITEMTYPE_NORMAL && ((EQ_Item*)Item)->IsStackable() == 1) ? pItemInfo->StackSize : 1;
}

long SetLONG(long Cur,PCHAR Val, PCHAR Sec, PCHAR Key, bool ZeroIsOff,long Maxi) {
  char ToStr[16]; char Buffer[128]; long Result=atol(Val);
  if(Result && Result>Maxi) Result=Maxi;
  _itoa_s(Result,ToStr,10);
  if(Sec[0] && Key[0]) WritePrivateProfileString(Sec,Key,ToStr,INIFileName);
  sprintf_s(Buffer,"%s::%s (\ag%s\ax)",Sec,Key,(ZeroIsOff && !Result)?"\aroff":ToStr);
  WriteChatColor(Buffer);
  return Result;
}

long SetBOOL(long Cur, PCHAR Val, PCHAR Sec, PCHAR Key) {
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
  return GetCharInfo2()->pInventoryArray->Inventory.Cursor;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

class KeepRec {
public:
  char name[ITEM_NAME_LEN];
  long id;
  long qty;

  KeepRec(PCHAR n, long i, long q) {
    strcpy_s(name,n);
    id=i;
    qty=q;
  }

  KeepRec(PCHAR k, PCHAR d) {
    char Buffer[MAX_STRING];
    strcpy_s(name,GetArg(Buffer,d,1,FALSE,FALSE,FALSE,'|'));
    qty   =atol(GetArg(Buffer,d,2,FALSE,FALSE,FALSE,'|'));
    id    =atol(k);
  }

  void Display(char ACTION) {
    if(ACTION!='-') {
      char buffer[32];
      if(qty<-1)     strcpy_s(buffer," \agPROTECTED\ax");
      else if(qty<0) strcpy_s(buffer," QTY[\agALL\ax]");
      else if(!qty)  strcpy_s(buffer," QTY[\agNONE\ax]");
      else sprintf_s(buffer," QTY[\ag%d\ax]",qty);
      if(ACTION=='+') WriteChatf("[\ag+\ax] ID[\ag%d\ax] NAME[\ag%s\ax]%s.",id,name,buffer);
      else WriteChatf("[\ay=\ax] ID[\ag%d\ax] NAME[\ag%s\ax]%s FIND[\ag%d\ax].",id,name,buffer, (long)CountItemByID(id, BAG_SLOT_START));
    } else WriteChatf("[\ar-\ax] ID[\ag%d\ax] NAME[\ag%s\ax].",id,name);
  }
};

class ListRec {
  map<long,KeepRec>::iterator f; // data iterator
  map<long,KeepRec>        data; // data storages
  char              section[32]; // name inifile section
public:

  ListRec(PCHAR s) {
    strcpy_s(section,s);
    data.clear();
    f=data.end();
  }

  KeepRec* Find(long KEY) {
    return(data.end()==(f=data.find(KEY)))?NULL:&(*f).second;
  }

  void Delete(long KEY, long QUIET) {
    if(data.end()!=(f=data.find(KEY))) {
      if(!QUIET) (*f).second.Display('-');
      data.erase(f);
    }
  }

  void Insert(KeepRec REC, long QUIET) {
    Delete(REC.id,true);
    data.insert(map<long,KeepRec>::value_type(REC.id,REC));
    if(!QUIET) REC.Display('+');
  }

  void Import(PCHAR Title) {
    if(Title[0]) WriteChatColor(Title);
    char Keys[MAX_STRING*10]={0};
    if(GetPrivateProfileString(section,NULL,"",Keys,MAX_STRING*10,INIFileName)) {
      PCHAR pKeys=Keys; char Temp[MAX_STRING];
      while(pKeys[0]) {
        GetPrivateProfileString(section,pKeys,"",Temp,MAX_STRING,INIFileName);
        if(Temp[0]) Insert(KeepRec(pKeys,Temp),true);
        pKeys+=strlen(pKeys)+1;
      }
    }
  }
 
  void Export(PCHAR Title) {
    if(Title[0]) WriteChatColor(Title);
    char KEY[MAX_STRING]; char BUF[MAX_STRING];
    WritePrivateProfileString(section,NULL,NULL,INIFileName);
    for(f=data.begin(); f!=data.end(); f++) {
      KeepRec *DTA=&(*f).second;
      if(DTA->qty!=0) sprintf_s(BUF,"%s|%d",DTA->name,DTA->qty); else strcpy_s(BUF,DTA->name);
      WritePrivateProfileString(section,SafeItoa(DTA->id,KEY,10),BUF,INIFileName);
    }
  }
 
  void Listing(PCHAR Title, PCHAR Search) {
    if(Title[0]) WriteChatColor(Title);
    WriteChatColor("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");
    for(f=data.begin(); f!=data.end(); f++)
      if(!Search[0] || strstr((*f).second.name,Search))
        (*f).second.Display('=');
    WriteChatColor("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");
  }
};

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

long       CursorHandle      = false;    // Cursor Handle?
long       CursorSilent      = false;    // Cursor Silent Operating?
//long       CursorDroping     = false;    // Cursor Drop Droppable Stuff?
long       CursorWarnItem    = NOID;     // Cursor Warned Item ID
long       CursorWarnTime    = 0;        // Cursor Warned Time ID
long       CursorRandom      = 0;        // Cursor Random Wait
DWORD      CursorTimer       = 0;        // Cursor Timer
ListRec   *CursorList        = 0;        // Cursor Keeping List

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

void DestroyCommand()
{
    if(!(PLUGIN_FLAG&(Conditions)))
    {
        char Buffers[128]; PCHAR Display=Buffers;
        if(GetCharInfo2()->CursorPlat)        Display="MQ2Cursor::\ayDESTROYING\ax <\arPlat\ax>.";
        else if(GetCharInfo2()->CursorGold)   Display="MQ2Cursor::\ayDESTROYING\ax <\arGold\ax>.";
        else if(GetCharInfo2()->CursorSilver) Display="MQ2Cursor::\ayDESTROYING\ax <\arSilver\ax>.";
        else if(GetCharInfo2()->CursorCopper) Display="MQ2Cursor::\ayDESTROYING\ax <\arCopper\ax>.";
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

void KeepCommand(PSPAWNINFO pCHAR, PCHAR zLine)
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
            DoCommand((PSPAWNINFO)pCharSpawn,"/autoinventory");
        }
        else
        {
            sprintf_s(szInvSlot, "InvSlot%d", lSlot);
            SendWornClick(szInvSlot, SHIFTKEY);
            // if we swapped something (bag on cursor) autoinv it
            if (CursorHasItem())
				DoCommand((PSPAWNINFO)pCharSpawn,"/autoinventory");
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
    if(!_strnicmp("rem",Parm1,3) || !_strnicmp("del",Parm1,3)) {
      if(!Parm2[0] && Cursor) SafeItoa(GetItemFromContents(Cursor)->ItemNumber,Parm2,10);
      if(Parm2[0] && IsNumber(Parm2)) {
        CursorList->Delete((DWORD)atol(Parm2),CursorSilent);
        CursorList->Export("");
        return;
      }
      NeedHelp=true;
    }
    if(Cursor && !NeedHelp)
    {
      if(Parm1[0] && (IsNumber(Parm1) || !_strnicmp(Parm1,"al",2) || !_strnicmp(Parm1,"pro",3)))
      {
        long HowMany=atol(Parm1);
        if(!_strnicmp(Parm1,"pro",3))        HowMany=-2;
        else if(!_strnicmp(Parm1,"al",2))    HowMany=-1;
        else if(!_strnicmp(Parm2,"st",2))    HowMany*=StackSize(Cursor);
        if(HowMany>1 && !ItemIsStackable(Cursor)) HowMany=1;
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
      sprintf_s(INIFileName,"%s\\%s_%s.ini",gszINIPath,EQADDR_SERVERNAME,GetCharInfo()->Name);
      CursorList=new ListRec("MQ2Cursor_ItemList");
      CursorList->Import("");
      CursorHandle   =GetPrivateProfileInt("MQ2Cursor","Active"  ,0,INIFileName);
      CursorSilent   =GetPrivateProfileInt("MQ2Cursor","Silent"  ,0,INIFileName);
      CursorRandom   =GetPrivateProfileInt("MQ2Cursor","Random"  ,0,INIFileName);
      //CursorDroping  =GetPrivateProfileInt("MQ2Cursor","Droping" ,0,INIFileName);
    }
  } else if(GameState!=GAMESTATE_LOGGINGIN) {
     if(Initialized) {
        if(CursorList) delete CursorList;
        CursorList=0;
        CursorHandle=0;
        Initialized=0;
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

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

PLUGIN_API VOID OnPulse()
{
    if(Initialized && gbInZone && pCharSpawn && GetCharInfo2() && !(PLUGIN_FLAG&InStat()))
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
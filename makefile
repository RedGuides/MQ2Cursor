!include "../global.mak"

ALL : "$(OUTDIR)\MQ2Cursor.dll"

CLEAN :
	-@erase "$(INTDIR)\MQ2Cursor.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\MQ2Cursor.dll"
	-@erase "$(OUTDIR)\MQ2Cursor.exp"
	-@erase "$(OUTDIR)\MQ2Cursor.lib"
	-@erase "$(OUTDIR)\MQ2Cursor.pdb"


LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib $(DETLIB) ..\Release\MQ2Main.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\MQ2Cursor.pdb" /debug /machine:I386 /out:"$(OUTDIR)\MQ2Cursor.dll" /implib:"$(OUTDIR)\MQ2Cursor.lib" /OPT:NOICF /OPT:NOREF 
LINK32_OBJS= \
	"$(INTDIR)\MQ2Cursor.obj" \
	"$(OUTDIR)\MQ2Main.lib"

"$(OUTDIR)\MQ2Cursor.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) $(LINK32_FLAGS) $(LINK32_OBJS)


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("MQ2Cursor.dep")
!INCLUDE "MQ2Cursor.dep"
!ELSE 
!MESSAGE Warning: cannot find "MQ2Cursor.dep"
!ENDIF 
!ENDIF 


SOURCE=.\MQ2Cursor.cpp

"$(INTDIR)\MQ2Cursor.obj" : $(SOURCE) "$(INTDIR)"


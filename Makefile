CC      = cl
LD      = link
CFLAGS  = /O1 /Os /GS- /Gs9999999 /Gy /Gw /GF /GR- /EHa- /Zl
LDFLAGS = /NODEFAULTLIB /ENTRY:_start /SUBSYSTEM:CONSOLE /FILEALIGN:16 /ALIGN:16 /OPT:REF /OPT:ICF /INCREMENTAL:NO /MANIFEST:NO /DEBUG:NONE
LIBS    = kernel32.lib user32.lib
SOURCE  = keylogger.c
OBJECT  = keylogger.obj
EXE		= keylogger.exe

.PHONY: all build run clean

ifeq ($(OS),Windows_NT)
    PS    = powershell -NoProfile -Command
    SIZE  = $(PS) "Write-Host ('Size: ' + (Get-Item '$(EXE)').Length + ' bytes')"
    RUN   = $(PS) "& './$(EXE)'"
    RM    = $(PS) "Remove-Item -Force -ErrorAction SilentlyContinue '$(EXE)', '$(OBJECT)'"
else
    SIZE  = echo "Size: $$(wc -c < $(EXE)) bytes"
    RUN   = ./$(EXE)
    RM    = rm -f $(EXE) $(OBJECT)
endif

all: link

compile:
	$(CC) $(CFLAGS) /c $(SOURCE)

link: compile
	$(LD) $(LDFLAGS) $(OBJECT) $(LIBS)
	@$(SIZE)

run: link
	@$(RUN)

clean:
	@$(RM)
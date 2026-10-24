CC      = gcc
CFLAGS  = -Os -flto -ffunction-sections -fdata-sections -Wall -Wextra -fno-asynchronous-unwind-tables -fno-ident
LDFLAGS = -s -flto -Wl,--gc-sections -static-libgcc
SOURCE  = keylogger.c
EXE		= keylogger.exe

ifeq ($(OS),Windows_NT)
    PS    = powershell -NoProfile -Command
    SIZE  = $(PS) "Write-Host ('Size: ' + (Get-Item '$(EXE)').Length + ' bytes')"
    RUN   = $(PS) "& './$(EXE)'"
    RM    = $(PS) "Remove-Item -Force -ErrorAction SilentlyContinue '$(EXE)'"
else
    SIZE  = echo "Size: $$(wc -c < $(EXE)) bytes"
    RUN   = ./$(EXE)
    RM    = rm -f $(EXE)
endif

build:
	$(CC) $(CFLAGS) $(SOURCE) -o $(EXE) $(LDFLAGS)
	@$(SIZE)

run: build
	@$(RUN)

clean:
	@$(RM)

.PHONY: all clean
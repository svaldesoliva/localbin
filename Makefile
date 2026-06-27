CC      = gcc
CPPFLAGS = -I$(abspath $(dir $(lastword $(MAKEFILE_LIST))))/include
CFLAGS  = -Wall -Wextra -O2 -std=c11
LDFLAGS =
TARGET  = localbin
BINDIR  = $(HOME)/.localbin

SRCS = src/app/main.c \
       src/core/paths.c \
       src/core/utils.c \
       src/storage/metadata.c \
       src/security/checksum.c \
       src/cli/commands_install.c \
       src/cli/commands_list.c \
       src/cli/commands_manage.c \
       src/cli/commands_info.c \
       src/cli/commands_system.c \
       src/cli/commands_help.c \
       src/cli/commands_completions.c \
       src/cli/commands_self_update.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean install uninstall debug analyze format test help

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

install: $(TARGET)
	@mkdir -p $(BINDIR)
	@rm -f $(BINDIR)/$(TARGET)
	@cp $(TARGET) $(BINDIR)/$(TARGET)
	@chmod +x $(BINDIR)/$(TARGET)
	@$(BINDIR)/$(TARGET) setup || true
	@echo "Installed. Open a new terminal if PATH was just updated."

uninstall:
	@rm -f $(BINDIR)/$(TARGET)
	@echo "Uninstalled"

clean:
	@rm -f $(TARGET) $(OBJS)

debug: CFLAGS += -g -DDEBUG -O0
debug: clean $(TARGET)

analyze:
	clang --analyze $(CPPFLAGS) $(CFLAGS) $(SRCS)

format:
	clang-format -i $(SRCS) src/cli/commands_internal.h \
	    include/localbin/app/*.h include/localbin/cli/*.h \
	    include/localbin/core/*.h include/localbin/security/*.h \
	    include/localbin/storage/*.h

test: $(TARGET)
	@./$(TARGET) help
	@./$(TARGET) doctor

help:
	@echo "Targets: all install uninstall clean debug analyze format test"

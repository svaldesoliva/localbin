# Makefile para localbin v2.0 (Modular)

CC = gcc
PROJECT_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
CPPFLAGS = -I$(PROJECT_ROOT)/include
CFLAGS = -Wall -Wextra -O2 -std=c11
LDFLAGS = -framework Security -framework CoreFoundation
TARGET = localbin
INSTALL_DIR = $(HOME)/.localbin

# Archivos fuente
SRC_DIR = src
SRCS = $(SRC_DIR)/app/main.c \
       $(SRC_DIR)/core/paths.c \
       $(SRC_DIR)/core/utils.c \
       $(SRC_DIR)/storage/metadata.c \
       $(SRC_DIR)/security/checksum.c \
       $(SRC_DIR)/cli/commands.c \
       $(SRC_DIR)/cli/commands_install.c \
       $(SRC_DIR)/cli/commands_list.c \
       $(SRC_DIR)/cli/commands_manage.c \
       $(SRC_DIR)/cli/commands_info.c \
       $(SRC_DIR)/cli/commands_system.c \
       $(SRC_DIR)/cli/commands_help.c \
       $(SRC_DIR)/cli/commands_self_update.c

OBJS = $(SRCS:.c=.o)

# Detectar arquitectura en macOS
ARCH := $(shell uname -m)

.PHONY: all clean install uninstall help test debug

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS)
	@echo " Compilado: $(TARGET)"

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS) *.o
	@echo " Limpiado"

install: $(TARGET)
	@mkdir -p $(INSTALL_DIR)
	@cp $(TARGET) $(INSTALL_DIR)/$(TARGET)
	@chmod +x $(INSTALL_DIR)/$(TARGET)
	@echo " Instalado en $(INSTALL_DIR)/$(TARGET)"
	@echo ""
	@echo "Configurando PATH automaticamente..."
	@$(INSTALL_DIR)/$(TARGET) setup || true
	@echo ""
	@echo "Si tu shell no recargo la configuracion, abre una nueva terminal"

uninstall:
	@rm -f $(INSTALL_DIR)/$(TARGET)
	@echo "  Desinstalado $(TARGET)"

# Compilar con símbolos de debug
debug: CFLAGS += -g -DDEBUG -O0
debug: clean $(TARGET)
	@echo " Compilado en modo debug"

# Análisis estático
analyze:
	@echo " Ejecutando análisis estático..."
	clang --analyze $(CFLAGS) $(SRCS)
	@echo " Análisis completo"

# Formato de código
format:
	@echo " Formateando código..."
	clang-format -i $(SRCS) $(SRC_DIR)/cli/commands_internal.h include/localbin/app/*.h include/localbin/cli/*.h include/localbin/core/*.h include/localbin/security/*.h include/localbin/storage/*.h
	@echo " Código formateado"

test: $(TARGET)
	@echo " Probando localbin..."
	@./$(TARGET) help
	@echo ""
	@./$(TARGET) doctor

help:
	@echo "Makefile para localbin"
	@echo ""
	@echo "Objetivos disponibles:"
	@echo "  make           - Compila el programa"
	@echo "  make install   - Compila e instala en ~/.localbin"
	@echo "  make uninstall - Elimina el programa instalado"
	@echo "  make clean     - Elimina archivos compilados"
	@echo "  make debug     - Compila con símbolos de debug"
	@echo "  make test      - Prueba el programa compilado"
	@echo "  make analyze   - Ejecuta análisis estático"
	@echo "  make format    - Formatea el código"
	@echo "  make help      - Muestra esta ayuda"

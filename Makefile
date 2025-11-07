# Makefile para localbin v2.0 (Modular)

CC = clang
CFLAGS = -Wall -Wextra -O2 -std=c11 -I./include
LDFLAGS = -framework Security -framework CoreFoundation
TARGET = localbin
INSTALL_DIR = $(HOME)/.localbin

# Archivos fuente
SRC_DIR = src
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/core.c \
       $(SRC_DIR)/utils.c \
       $(SRC_DIR)/metadata.c \
       $(SRC_DIR)/checksum.c \
       $(SRC_DIR)/commands.c

OBJS = $(SRCS:.c=.o)

# Detectar arquitectura en macOS
ARCH := $(shell uname -m)

.PHONY: all clean install uninstall help test debug

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS)
	@echo "✅ Compilado: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
	@echo "🧹 Limpiado"

install: $(TARGET)
	@mkdir -p $(INSTALL_DIR)
	@cp $(TARGET) $(INSTALL_DIR)/$(TARGET)
	@chmod +x $(INSTALL_DIR)/$(TARGET)
	@echo "✅ Instalado en $(INSTALL_DIR)/$(TARGET)"
	@echo ""
	@echo "Para usar localbin, asegúrate de que $(INSTALL_DIR) esté en tu PATH"
	@echo "Agrega esto a tu ~/.zshrc o ~/.bash_profile:"
	@echo '  export PATH="$HOME/.localbin:$PATH"'

uninstall:
	@rm -f $(INSTALL_DIR)/$(TARGET)
	@echo "🗑️  Desinstalado $(TARGET)"

# Compilar con símbolos de debug
debug: CFLAGS += -g -DDEBUG -O0
debug: clean $(TARGET)
	@echo "🐛 Compilado en modo debug"

# Análisis estático
analyze:
	@echo "🔍 Ejecutando análisis estático..."
	clang --analyze $(CFLAGS) $(SRCS)
	@echo "✅ Análisis completo"

# Formato de código
format:
	@echo "🎨 Formateando código..."
	clang-format -i $(SRCS) include/*.h
	@echo "✅ Código formateado"

test: $(TARGET)
	@echo "🧪 Probando localbin..."
	@./$(TARGET) help
	@echo ""
	@./$(TARGET) doctor

help:
	@echo "Makefile para localbin v2.0"
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
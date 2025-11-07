# Makefile para localbin

CC = clang
CFLAGS = -Wall -Wextra -O2 -std=c11
TARGET = localbin
SRC = main.c
INSTALL_DIR = $(HOME)/.localbin

# Detectar arquitectura en macOS
ARCH := $(shell uname -m)

.PHONY: all clean install uninstall help test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)
	@echo "✅ Compilado: $(TARGET)"

clean:
	rm -f $(TARGET)
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
debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)
	@echo "🐛 Compilado en modo debug"

test: $(TARGET)
	@echo "🧪 Probando localbin..."
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
	@echo "  make help      - Muestra esta ayuda"
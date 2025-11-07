#!/bin/bash
# Script de instalación rápida para localbin v2.0

set -e

echo "🚀 Instalando localbin v2.0..."
echo ""

# Colores
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Verificar que estamos en el directorio correcto
if [ ! -f "Makefile" ] || [ ! -d "src" ]; then
    echo -e "${RED}❌ Error: Ejecuta este script desde el directorio raíz de localbin${NC}"
    exit 1
fi

# 1. Limpiar compilaciones previas
echo -e "${BLUE}[1/5]${NC} Limpiando compilaciones previas..."
make clean > /dev/null 2>&1 || true

# 2. Compilar
echo -e "${BLUE}[2/5]${NC} Compilando localbin..."
if make; then
    echo -e "${GREEN}✅ Compilación exitosa${NC}"
else
    echo -e "${RED}❌ Error en la compilación${NC}"
    exit 1
fi

# 3. Verificar binario
if [ ! -f "localbin" ]; then
    echo -e "${RED}❌ Error: No se generó el binario${NC}"
    exit 1
fi

echo -e "${BLUE}[3/5]${NC} Verificando binario..."
./localbin version
echo ""

# 4. Instalar
echo -e "${BLUE}[4/5]${NC} Instalando en ~/.localbin..."
if make install; then
    echo -e "${GREEN}✅ Instalado correctamente${NC}"
else
    echo -e "${RED}❌ Error en la instalación${NC}"
    exit 1
fi

# 5. Configurar PATH
echo -e "${BLUE}[5/5]${NC} Configurando PATH..."
if ~/.localbin/localbin setup; then
    echo -e "${GREEN}✅ PATH configurado${NC}"
else
    echo -e "${RED}⚠️  Advertencia: No se pudo configurar PATH automáticamente${NC}"
    echo "   Ejecuta manualmente: ~/.localbin/localbin setup"
fi

echo ""
echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}✅ Instalación completada exitosamente!${NC}"
echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo "Para completar la instalación:"
echo ""
echo "  1. Recarga tu shell:"
echo -e "     ${BLUE}source ~/.zshrc${NC}  # o ~/.bash_profile"
echo ""
echo "  2. Verifica la instalación:"
echo -e "     ${BLUE}localbin version${NC}"
echo -e "     ${BLUE}localbin doctor${NC}"
echo ""
echo "  3. Lee la documentación:"
echo -e "     ${BLUE}localbin help${NC}"
echo -e "     ${BLUE}cat README.md${NC}"
echo ""
echo "🎉 Disfruta de localbin v2.0!"

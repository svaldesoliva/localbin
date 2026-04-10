#!/usr/bin/env bash

set -euo pipefail

REPO_URL="https://github.com/svaldesoliva/localbin.git"
WORKDIR="$(pwd)"
TMPDIR=""

GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

cleanup() {
    if [ -n "${TMPDIR}" ] && [ -d "${TMPDIR}" ]; then
        rm -rf "${TMPDIR}"
    fi
}
trap cleanup EXIT

require_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo -e "${RED}Error: falta el comando '$1'${NC}"
        exit 1
    fi
}

echo "Instalando localbin..."
echo ""

if [ -f "${WORKDIR}/Makefile" ] && [ -d "${WORKDIR}/src" ] && [ -d "${WORKDIR}/include" ]; then
    echo -e "${BLUE}[1/5]${NC} Usando código local en ${WORKDIR}"
else
    require_cmd git
    TMPDIR="$(mktemp -d)"
    echo -e "${BLUE}[1/5]${NC} Descargando código fuente..."
    git clone --depth 1 "${REPO_URL}" "${TMPDIR}/localbin" >/dev/null 2>&1
    WORKDIR="${TMPDIR}/localbin"
fi

require_cmd make
require_cmd clang

echo -e "${BLUE}[2/5]${NC} Compilando localbin..."
make -C "${WORKDIR}" clean >/dev/null 2>&1 || true
make -C "${WORKDIR}"
echo -e "${GREEN}Compilación exitosa${NC}"

if [ ! -f "${WORKDIR}/localbin" ]; then
    echo -e "${RED}Error: no se generó el binario${NC}"
    exit 1
fi

echo -e "${BLUE}[3/5]${NC} Verificando binario..."
"${WORKDIR}/localbin" version
echo ""

echo -e "${BLUE}[4/5]${NC} Instalando en ~/.localbin..."
make -C "${WORKDIR}" install
echo -e "${GREEN}Instalado correctamente${NC}"

echo -e "${BLUE}[5/5]${NC} Configurando PATH..."
if "$HOME/.localbin/localbin" setup; then
    echo -e "${GREEN}PATH configurado${NC}"
else
    echo -e "${RED}Advertencia: no se pudo configurar PATH automáticamente${NC}"
fi

echo ""
echo -e "${GREEN}Instalación completada${NC}"
echo ""
echo "Siguiente paso:"
echo "  source ~/.zshrc   # o ~/.bash_profile"
echo ""
echo "Verificación rápida:"
echo "  localbin version"
echo "  localbin doctor"

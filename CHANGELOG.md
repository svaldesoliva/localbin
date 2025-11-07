# Changelog

Todos los cambios notables del proyecto localbin se documentarán en este archivo.

## [2.0.0] - 2025-11-07

### 🎉 Lanzamiento Mayor - Refactorización Completa

#### ✨ Nuevas Características

- **Arquitectura Modular**: Código reorganizado en módulos separados (core, utils, metadata, checksum, commands)
- **Sistema de Metadata**: Almacenamiento completo de información de cada programa en formato JSON
- **Checksums SHA256**: Cálculo y verificación automática de integridad usando CommonCrypto
- **Comando `info`**: Muestra información detallada de cualquier programa instalado
- **Comando `update`**: Actualiza programas existentes con:
  - Comparación de checksums
  - Backup automático de versión anterior
  - Actualización de metadata
  - Detección de cambios
- **Comando `verify`**: Verifica integridad de programas individuales o todos
- **Búsqueda mejorada**: `search <término>` para encontrar programas
- **Filtros avanzados**: `list --sort name|date|size`
- **Salida JSON**: Flag `--json` para scripting y automatización
- **Versionado de programas**: Soporte para `--version` al instalar

#### 🏗️ Arquitectura

Nueva estructura de directorios:
```
include/  - Headers (.h)
src/      - Implementación (.c)
tests/    - Tests unitarios (preparado)
```

Módulos:
- `core`: Funcionalidad central y estructuras de datos
- `utils`: Utilidades generales (formateo, filesystem, etc.)
- `metadata`: Sistema completo de metadata con serialización JSON
- `checksum`: Cálculo y verificación SHA256
- `commands`: Implementación de todos los comandos CLI

#### 📝 Documentación

- README.md completamente reescrito con:
  - Guía de uso completa
  - Ejemplos de cada comando
  - Arquitectura del código
  - Troubleshooting
  - Roadmap de features futuras
- CHANGELOG.md (este archivo)
- .gitignore apropiado para C/macOS

#### 🔧 Mejoras de Build

- Makefile actualizado para compilación modular
- Soporte para análisis estático (`make analyze`)
- Soporte para formateo de código (`make format`)
- Modo debug mejorado (`make debug`)
- Compilación incremental con archivos objeto

#### 🔐 Seguridad

- Checksums SHA256 automáticos para todos los programas
- Verificación de integridad en cualquier momento
- Backups automáticos al actualizar
- Detección de programas modificados

#### 🐛 Correcciones

- Manejo mejorado de errores en todas las operaciones
- Validación de rutas y archivos más robusta
- Gestión correcta de memoria (sin leaks conocidos)

### 📊 Estadísticas

- **Líneas de código**: ~2000+ (vs ~500 en v1.0)
- **Archivos fuente**: 6 módulos + 6 headers
- **Comandos disponibles**: 11+
- **Características nuevas**: 10+

---

## [1.0.0] - 2025-11-06

### 🎉 Lanzamiento Inicial

#### ✨ Características Básicas

- `install`: Instalar programas en ~/.localbin
- `remove`: Eliminar programas instalados
- `list`: Listar programas en formato tabla
- `doctor`: Verificar configuración del sistema
- `setup`: Configurar PATH automáticamente

#### 📝 Funcionalidad Core

- Detección automática de ejecutables
- Verificación de PATH
- Advertencias de configuración
- Soporte para zsh y bash
- Tabla formateada para listado

---

## Formato

Este changelog sigue el formato de [Keep a Changelog](https://keepachangelog.com/es/1.0.0/),
y este proyecto adhiere a [Semantic Versioning](https://semver.org/lang/es/).

### Tipos de cambios

- `✨ Nuevas Características` - para funcionalidad nueva
- `🔧 Cambios` - para cambios en funcionalidad existente
- `🐛 Correcciones` - para corrección de bugs
- `🗑️ Deprecado` - para características que serán removidas
- `❌ Removido` - para características removidas
- `🔐 Seguridad` - en caso de vulnerabilidades

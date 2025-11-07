# 📋 TODO - Roadmap de Funcionalidades

## ✅ Completado (v2.0.0)

- [x] Refactorización a arquitectura modular
- [x] Sistema de metadata con JSON
- [x] Checksums SHA256 automáticos
- [x] Comando `info` detallado
- [x] Comando `update` con backups
- [x] Comando `verify` individual y global
- [x] Búsqueda y filtros (`search`, `list --sort`)
- [x] Output JSON para scripting
- [x] Versionado básico (flag `--version`)
- [x] Documentación completa

## 🚀 Prioridad Alta (v2.1.0)

### Sistema de Versiones Múltiples
- [ ] Almacenar múltiples versiones del mismo programa
- [ ] Comando `switch <nombre> <version>` para cambiar entre versiones
- [ ] Comando `versions <nombre>` para listar versiones disponibles
- [ ] Limpieza automática de versiones antiguas (opcional)
- [ ] Directorio `~/.localbin/.versions/<programa>/<version>/`

### Backup y Restore
- [ ] Comando `backup [--output archivo.tar.gz]` - Crear backup completo
  - Incluir todos los binarios
  - Incluir toda la metadata
  - Incluir configuración
- [ ] Comando `restore <archivo.tar.gz>` - Restaurar desde backup
  - Validar integridad del backup
  - Opción para sobrescribir o skip conflictos
- [ ] Backups incrementales (opcional)

### Sistema de Dependencias
- [ ] Campo `dependencies` en metadata (ya preparado)
- [ ] Comando `deps add <programa> <dependencia>`
- [ ] Comando `deps remove <programa> <dependencia>`
- [ ] Comando `deps list <programa>`
- [ ] Comando `deps graph` - Mostrar árbol de dependencias
- [ ] Instalación automática de dependencias
- [ ] Verificación antes de eliminar programas con dependientes
- [ ] Detección de dependencias circulares

## 🎯 Prioridad Media (v2.2.0)

### Export/Import
- [ ] Comando `export [--output lista.txt]` - Exportar lista de programas
  - Formato: `nombre:version:checksum`
  - Compatible con git
  - Incluir metadata opcional
- [ ] Comando `import <lista.txt>` - Instalar desde lista
  - Verificar checksums
  - Instalar versiones específicas
  - Manejo de errores robusto

### Health Check Avanzado
- [ ] Mejorar `doctor` con más verificaciones:
  - Detectar programas rotos (enlaces simbólicos muertos)
  - Verificar permisos incorrectos
  - Identificar duplicados
  - Buscar metadata huérfana
  - Verificar integridad de todos los checksums
  - Sugerir reparaciones automáticas
- [ ] Comando `doctor --fix` - Reparar problemas automáticamente

### Autocompletado Shell
- [ ] Comando `completion zsh` - Generar script de autocompletado zsh
- [ ] Comando `completion bash` - Generar script de autocompletado bash
- [ ] Autocompletado de:
  - Comandos disponibles
  - Nombres de programas instalados
  - Opciones y flags
- [ ] Instalación automática en `~/.zsh/completions/` o `/etc/bash_completion.d/`

## 📦 Prioridad Baja (v2.3.0+)

### Integración con Homebrew
- [ ] Detectar si un programa existe en Homebrew
- [ ] Advertir sobre posibles conflictos
- [ ] Comando `conflicts` - Listar conflictos con brew
- [ ] Opción `--force` para instalar de todas formas
- [ ] Comparar versiones (localbin vs brew)

### Virtualenvs/Perfiles
- [ ] Comando `profile create <nombre>` - Crear nuevo perfil
- [ ] Comando `profile use <nombre>` - Activar perfil
- [ ] Comando `profile list` - Listar perfiles disponibles
- [ ] Comando `profile delete <nombre>` - Eliminar perfil
- [ ] Aislar programas por perfil (dev/prod/test)
- [ ] Variables de entorno por perfil

### Self-Update
- [ ] Comando `self-update` - Actualizar localbin desde GitHub
- [ ] Verificación de versión remota
- [ ] Descarga segura (HTTPS)
- [ ] Verificación de firma GPG (opcional)
- [ ] Rollback si falla la actualización
- [ ] Release notes automáticos

### Rollback
- [ ] Historial de operaciones en `~/.localbin/.history`
- [ ] Comando `rollback` - Deshacer última operación
- [ ] Comando `rollback <n>` - Deshacer n operaciones
- [ ] Comando `history` - Ver historial de operaciones
- [ ] Límite configurable de historial

## 🌟 Features Avanzados (v3.0.0+)

### Compresión
- [ ] Comprimir binarios grandes automáticamente
- [ ] Soporte para zstd, gzip, bzip2
- [ ] Descompresión transparente al ejecutar
- [ ] Configuración por programa o global

### Encriptación
- [ ] Encriptar binarios sensibles con AES-256
- [ ] Gestión de claves con Keychain (macOS)
- [ ] Desencriptación automática al ejecutar
- [ ] Comando `encrypt <programa>` / `decrypt <programa>`

### Links Simbólicos Inteligentes
- [ ] Detectar si programa necesita archivos auxiliares
- [ ] Mantener estructura de directorios
- [ ] Opción symlink vs copia (configurable por programa)
- [ ] Detección automática de tipo de programa

### Filtros Avanzados
- [ ] `list --after <fecha>` - Programas instalados después de fecha
- [ ] `list --smaller <tamaño>` - Programas menores a tamaño
- [ ] `list --modified` - Programas con checksum modificado
- [ ] `list --pattern <glob>` - Matching con wildcards
- [ ] Combinación de múltiples filtros

### Hooks
- [ ] Pre-install hooks
- [ ] Post-install hooks
- [ ] Pre-remove hooks
- [ ] Post-remove hooks
- [ ] Scripts customizables en `~/.localbin/.hooks/`

## 🧪 Calidad de Código

### Tests
- [ ] Suite de tests unitarios con Criterion
- [ ] Tests de integración
- [ ] Tests de memoria con Valgrind
- [ ] Coverage reports
- [ ] CI/CD con GitHub Actions

### Análisis y Performance
- [ ] Fuzzing con AFL o libFuzzer
- [ ] Memory profiling completo
- [ ] Benchmarks de operaciones críticas
- [ ] Optimización de operaciones I/O
- [ ] Caché de metadata en memoria

### Documentación
- [ ] Man pages (`man localbin`)
- [ ] Tutoriales interactivos
- [ ] Ejemplos de casos de uso avanzados
- [ ] API docs para extensibilidad
- [ ] Videos/GIFs demostrativos

## 💡 Ideas Adicionales

### Interface Alternativa
- [ ] TUI (Terminal UI) con ncurses
- [ ] Modo interactivo
- [ ] Dashboard visual de programas

### Sincronización
- [ ] Sync entre máquinas con rsync
- [ ] Sync con cloud storage (S3, Dropbox)
- [ ] Conflict resolution automático

### Estadísticas
- [ ] Comando `stats` - Estadísticas de uso
  - Programas más usados
  - Espacio total ocupado
  - Gráficos de crecimiento
  - Métricas de actualización

### Seguridad Avanzada
- [ ] Sandbox para programas desconocidos
- [ ] Análisis de virus/malware (integración con ClamAV)
- [ ] Firma digital de programas
- [ ] Whitelist/blacklist de checksums

---

## 📝 Notas

- Los items marcados con ✅ están completados
- Las versiones son estimadas y pueden cambiar
- Las prioridades se ajustan según feedback de usuarios
- Contribuciones son bienvenidas para cualquier feature

## 🤝 Contribuir

¿Quieres implementar alguna de estas features? 

1. Abre un issue discutiendo la feature
2. Fork el proyecto
3. Implementa la feature siguiendo las guías de estilo
4. Agrega tests
5. Actualiza documentación
6. Abre un Pull Request

---

Última actualización: 2025-11-07

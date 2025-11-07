# 📦 localbin v2.0

**Administrador de paquetes local** para mantener tus programas compilados separados de los instalados por Homebrew, MacPorts, APT, etc.

## 🎯 Características Principales

### ✅ Implementado en v2.0

- **Sistema de metadata completo**: Almacena información detallada de cada programa
- **Checksums SHA256**: Verificación de integridad automática
- **Comando `info`**: Muestra información detallada de cualquier programa
- **Comando `update`**: Actualiza programas con backup automático
- **Comando `verify`**: Verifica integridad de uno o todos los programas
- **Búsqueda y filtros**: `search`, `list --sort`, `--json`
- **Arquitectura modular**: Código organizado en módulos reutilizables

### 🚧 Roadmap (Próximas versiones)

- Sistema de versiones múltiples
- Backup y restore completo
- Export/import de configuraciones
- Sistema de dependencias
- Virtualenvs/perfiles
- Self-update desde GitHub
- Autocompletado para zsh/bash
- Integración con Homebrew

## 📋 Instalación

```bash
# Compilar
make

# Instalar en ~/.localbin
make install

# Configurar PATH automáticamente
./localbin setup
```

## 🚀 Uso

### Comandos Principales

```bash
# Instalar un programa
localbin install ./mi_programa
localbin install ./mi_programa --version 1.2.3

# Actualizar un programa existente
localbin update mi_programa ./nueva_version

# Eliminar un programa
localbin remove mi_programa

# Listar programas instalados
localbin list
localbin list --sort date
localbin list --sort size
localbin list --json

# Ver información detallada
localbin info mi_programa

# Buscar programas
localbin search term

# Verificar integridad
localbin verify mi_programa
localbin verify --all
```

### Comandos de Sistema

```bash
# Verificar configuración
localbin doctor

# Configurar PATH automáticamente
localbin setup

# Ver ayuda
localbin help

# Ver versión
localbin version
```

## 🏗️ Arquitectura del Código

```
localbin/
├── include/          # Headers (.h)
│   ├── core.h       # Definiciones centrales
│   ├── utils.h      # Utilidades generales
│   ├── metadata.h   # Sistema de metadata
│   ├── checksum.h   # Checksums SHA256
│   ├── commands.h   # Comandos del CLI
│   └── version.h    # Información de versión
├── src/             # Implementación (.c)
│   ├── main.c       # Punto de entrada
│   ├── core.c       # Lógica central
│   ├── utils.c      # Utilidades
│   ├── metadata.c   # Gestión de metadata
│   ├── checksum.c   # Cálculo de checksums
│   └── commands.c   # Implementación de comandos
├── tests/           # Tests unitarios (TODO)
├── main.c           # Versión legacy (deprecated)
└── Makefile         # Build system
```

## 📊 Sistema de Metadata

Cada programa instalado tiene metadata asociada almacenada en `~/.localbin/.metadata/<programa>.json`:

```json
{
  "name": "mi_programa",
  "version": "1.0.0",
  "source_path": "/ruta/absoluta/al/original",
  "checksum_sha256": "abc123...",
  "install_date": 1699392000,
  "update_date": 1699392000,
  "size_bytes": 1048576,
  "permissions": 755,
  "dependencies": ["dep1", "dep2"],
  "dep_count": 2
}
```

## 🔐 Seguridad

- **SHA256**: Todos los programas tienen checksums SHA256 calculados automáticamente
- **Verificación de integridad**: Detecta si un binario fue modificado
- **Backups automáticos**: Al actualizar, se crea backup de la versión anterior
- **Permisos seguros**: Los directorios se crean con permisos 0755

## 🛠️ Desarrollo

```bash
# Compilar en modo debug
make debug

# Ejecutar análisis estático
make analyze

# Formatear código
make format

# Limpiar archivos compilados
make clean

# Ejecutar tests
make test
```

## 📝 Ejemplos de Uso

```bash
# Instalar un programa con versión
localbin install ./myapp --version 2.1.0

# Ver información completa
localbin info myapp

# Actualizar desde nueva versión
localbin update myapp ./myapp_v2.2.0

# Verificar que no fue modificado
localbin verify myapp

# Listar ordenado por fecha (más recientes primero)
localbin list --sort date

# Buscar programas que contengan "server"
localbin search server

# Ver salida en formato JSON
localbin list --json

# Verificar integridad de todos los programas
localbin verify --all

# Diagnóstico completo del sistema
localbin doctor
```

## 🐛 Troubleshooting

### Los programas no se encuentran

```bash
# Verificar que PATH está configurado
localbin doctor

# Si no, configurar automáticamente
localbin setup

# Recargar shell
source ~/.zshrc  # o ~/.bash_profile
```

### Error de permisos

```bash
# Verificar permisos del directorio
ls -la ~/.localbin

# Si es necesario, corregir
chmod 755 ~/.localbin
```

## 📄 Licencia

MIT License - Ver archivo LICENSE

## 🤝 Contribuir

Las contribuciones son bienvenidas! Por favor:

1. Fork el proyecto
2. Crea una branch de features (`git checkout -b feature/nueva-funcionalidad`)
3. Commit tus cambios
4. Push a la branch
5. Abre un Pull Request

## 📧 Contacto

Para reportar bugs o sugerir features, abre un issue en GitHub.

---

**localbin v2.0** - Tu gestor de paquetes local personal 🚀

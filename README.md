# localbin

Administrador simple de binarios locales para macOS.

Con `localbin` puedes instalar ejecutables en `~/.localbin` y gestionarlos con comandos como `install`, `list`, `info`, `update`, `remove` y `verify`.

## Instalacion rapida

```bash
curl -fsSL https://raw.githubusercontent.com/svaldesoliva/localbin/main/install.sh | bash
```

Este script descarga el proyecto, compila, instala en `~/.localbin` e intenta configurar tu `PATH`.
La configuracion automatica cubre `zsh`, `bash`, `fish`, `csh/tcsh` y fallback en `~/.profile`.

## Instalacion manual

```bash
git clone https://github.com/svaldesoliva/localbin.git
cd localbin
make
make install
~/.localbin/localbin setup
```

## Uso basico

```bash
# Instalar un binario
localbin install ./mi_programa
localbin install ./mi_programa --version 1.2.3

# Instalar desde URL
localbin install https://url.com/binario --version 1.2.3

# Instalar con nombre distinto
localbin install ./mi_programa --as mi_programa_v2

# Crear alias (symlink)
localbin install ./mi_programa --alias mi-programa

# Hooks para update
localbin install ./mi_programa \
  --pre-update-hook ~/.localbin/hooks/pre_update.sh \
  --post-update-hook ~/.localbin/hooks/post_update.sh

# Ver programas instalados
localbin list

# Ver detalle de un programa
localbin info mi_programa

# Actualizar un programa
localbin update mi_programa ./mi_programa_nuevo

# Verificar integridad
localbin verify mi_programa
localbin verify --all

# Eliminar
localbin remove mi_programa
```

## Comandos utiles

```bash
localbin doctor
localbin setup
localbin version
localbin self-update
localbin self-update --manual
localbin help
```

## Desarrollo

```bash
make
make debug
make test
make analyze
make format
make clean
```


## Licencia

MIT

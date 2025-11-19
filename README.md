## TP2 Sistemas Operativos 2025 

Proyecto de Sistemas Operativos para x86_64 que implementa un sistema con kernel propio, programas de usuario y utilidades. La construcción y ejecución se realizan mediante scripts que generan una imagen booteable y la ejecutan en QEMU utilizando Docker.

### Estructura del proyecto
- `Bootloader/`: Pure64 + BMFS para inicialización y carga de módulos.
- `Kernel/`: Código del kernel (drivers, memoria, scheduler, syscalls, etc.).
- `Userland/`: Bibliotecas y programas de usuario (Shell, Snake, Tests).
- `Image/`: Artefactos generados (imagen de disco y formatos derivados).
- `Toolchain/`: Herramientas auxiliares (empaquetado de módulos).

### Requisitos
- Docker Desktop instalado y en ejecución.
- Make/GNU toolchain provistos dentro del contenedor (no se requieren localmente).

### Compilación
Ejecuta el script de compilación en la raíz del repo:

```bash
./compile.sh
```

Esto construye el toolchain, el kernel, los módulos de userland y genera la imagen booteable en `Image/`.

### Ejecución
Para lanzar el sistema en QEMU:

```bash
./run.sh
```

El script utiliza la imagen generada en `Image/` y abre la consola de la VM.

### Notas
- Si tuviste cambios grandes y querés recompilar desde cero, podés limpiar artefactos borrando los binarios generados en `Kernel/`, `Userland/` e `Image/`. (No hay comando de clean global expuesto; dependerá del flujo de cada subproyecto.)


### Licencia
Ver `License.txt`.

# Ship of Harkinian (Vulkan) para PS4

*[English version](README.md)*

Un port para PS4 de [Ship of Harkinian](https://github.com/HarbourMasters/Shipwright) 9.3.1, el port de PC de Ocarina of Time, para consolas con GoldHEN. El código del juego corre nativo en el procesador de la PS4 y se dibuja con Vulkan a través de RADV, el driver Radeon de Mesa portado a la GPU de la PS4. Está construido sobre el renderizador Vulkan del port de PS5, [damiantw/PS5_Shipwright](https://github.com/damiantw/PS5_Shipwright), y sobre el runtime de PS4 de los ports de [SMW](https://github.com/iHaiDeeZ/SMW) y [Wind Waker HD](https://github.com/Nabiscan58/WWHD-Recomp-PS4-Port).

**Aquí no hay juego, ROM ni datos del juego.** Cada quien pone su propia copia.

> **Estado (01.06):** va a 60 FPS en casi todo el juego en una PS4 Fat, incluso con packs grandes de texturas y modelos. Probado en una sola consola: PS4 Fat CUH-12xx, firmware 13.52, GoldHEN.

Se instala como un título propio (`SOHV00001`, "Ship of Harkinian (Vulkan)") y guarda sus datos en `/data/sohvk`, así que puede convivir con otros ports de Ship of Harkinian.

## Qué necesitas

- Una PS4 con GoldHEN.
- Tus propios datos de Ocarina of Time: un `oot.o2r` hecho a partir de tu ROM con Ship of Harkinian 9.3.1 en una PC. El `oot-mq.o2r` de Master Quest es opcional.
- Un cliente FTP, u otra forma de copiar archivos a `/data/sohvk` en la consola.

## Instalación

1. Descarga `SoH-Vulkan-PS4-<versión>.zip` de la página de Releases y descomprímelo. Trae un `.pkg`.
2. Instala el `.pkg` con el instalador de paquetes de GoldHEN.
3. Copia tu `oot.o2r` a `/data/sohvk/` (crea la carpeta si no existe).
4. Abre "Ship of Harkinian (Vulkan)". El primer arranque prepara los shaders y tarda un momento.

Con un `oot.o2r` hecho en PC sale una sola vez un aviso de que el port no lo generó. Es normal.

El port de PS5 también puede convertir en la consola una ROM (`.z64`, `.n64`, `.v64`) puesta en la carpeta de datos. Ese código sigue en este port, pero no se ha probado en una PS4.

**Para actualizar:** borra la versión instalada desde la pantalla de inicio y luego instala el `.pkg` nuevo. Las partidas, los ajustes, los mods y el `oot.o2r` de `/data/sohvk` se conservan.

## Controles y menú

Un clic del panel táctil abre y cierra el menú de ajustes. La distribución de botones por defecto es la del port de PS5: mira [su README](https://github.com/damiantw/PS5_Shipwright#controls). Todo se puede reasignar en **Settings > Controls**.

## Mods y packs de texturas

Pon los archivos `.o2r` u `.otr` en `/data/sohvk/mods/`. Se activan en el siguiente arranque y su orden se cambia en **Settings > Mod Menu**.

El desarrollo se hizo con el pack 3DS de texturas y modelos de Djipi (más de 30 archivos), que es el caso más pesado que se probó.

## Ajustes de PS4

En el menú de ajustes:

| Ajuste | Por defecto | Qué hace |
| --- | --- | --- |
| Keep Game Speed | activado | Cuando el procesador no alcanza a dibujar todos los frames, se dibujan menos (60, 50, 40, 30 o 20 FPS) para que el juego y el sonido mantengan su velocidad normal. |
| Skip Off-screen Geometry (experimental) | desactivado | Se salta los modelos cuya caja queda fuera de la pantalla, como hace el N64. |
| Anti-aliasing (MSAA) | 1 | 2x está probado. El valor 3 funciona como 2, y de 5 a 7 como 4. 4x y 8x no se han probado. |

En `/data/sohvk/env.txt` (una línea `NOMBRE=valor` por ajuste):

| Línea | Efecto |
| --- | --- |
| `SOHVK_MSAA_MAX=2` | Limita el MSAA, para una consola donde un valor más alto dé problemas. |
| `SOHVK_PS4_PROFILE=1` | Graba un minuto de muestras del procesador en `samples.log`, desde 45 segundos después de abrir el juego. |

## Rendimiento

Medido en una PS4 Fat con la versión 01.05, que tiene el mismo renderizador:

- La mayoría de las zonas: 60 FPS.
- Hyrule Field con el pack de Djipi: de 55 a 58 FPS en promedio.
- Cargas de zona con el pack: de 0.6 a 2.7 segundos.
- La memoria se mantuvo estable en una sesión de 95 minutos.

El límite es un solo núcleo del procesador convirtiendo la lista de dibujo del juego en llamadas de dibujo. La GPU pasa libre casi todo el tiempo.

## Problemas conocidos

- Congelamientos cortos de 0.1 a 0.3 segundos la primera vez que aparecen en pantalla modelos y texturas de un mod.
- En las vistas más pesadas el juego puede ir hasta un 4% más lento, porque Keep Game Speed mantiene los 60 FPS dentro de una tolerancia antes de bajar.
- El menú de ajustes puede dar pequeños tirones mientras está abierto.
- La resolución interna por encima de 100% no se ha probado.
- No hay controles de movimiento.
- Que el escenario lejano aparezca al acercarte es comportamiento del propio juego, no un error del port.

## Registros

Todo está en `/data/sohvk`: el registro del juego (en `logs/`), `boot-trace.log`, `crash.log` y `samples.log`. El registro del juego trae un reporte de rendimiento cada 10 segundos (FPS, tiempos de frame, cantidad de dibujos, memoria y lo que decidió Keep Game Speed).

## Compilar desde el código

Este repositorio solo tiene parches y código del port. La compilación toma cada proyecto de su propio repositorio:

| Proyecto | Versión |
| --- | --- |
| [Shipwright](https://github.com/HarbourMasters/Shipwright) | tag `9.3.1` (`f0599f96`) |
| libultraship (submódulo) | `da6daf1a50a5e46d6641408d52227e31e85e46df` |
| Torch (submódulo) | `2ab12fe9660aec04e02ee89fe81baed304a1a1d6` |
| [SDL](https://github.com/libsdl-org/SDL) | tag `release-2.32.10` |

Cada uno recibe dos capas de parches: `01-ps5-port-*` son los parches de PS5_Shipwright sin modificar (commit `a584b3c9`), y `02-ps4-*` son los cambios de este port encima.

Herramientas, en Linux: LLVM 21.1.8, OpenOrbis PS4 Toolchain v0.5.2 (PacBrew), el SDK de PS4 de SMW `sdk-1` (sysroot y RADV), Vulkan-Headers v1.4.357, CMake y Ninja.

Los comandos están en el [README en inglés](README.md#building-from-source). En resumen: clonar Shipwright 9.3.1 y SDL 2.32.10, aplicar los parches 01 y luego los 02, copiar `env.example.sh` a `env.sh` con tus rutas, compilar con CMake y Ninja, y empaquetar con `tools/package-soh.sh`.

El empaquetado necesita tres archivos en `pkgfiles/`: `soh.o2r`, `assets.zip` y `ps5_prompts.o2r`. Se sacan de un paquete publicado (`PkgTool.Core pkg_extract`) o se generan con `ps5/tools/assets.sh` de PS5_Shipwright.

## Versiones

- **01.06**: los valores de MSAA que no son 2, 4 u 8 ya no cierran el juego. Limpieza para la publicación.
- **01.05**: fondo del menú de pausa corregido. Keep Game Speed ya no reacciona a las cargas únicas. El compilador de shaders se prepara al arrancar. Búsqueda de recursos más rápida.
- **01.04**: lotes de dibujo indexados y caché de estado de triángulos. Las vistas pesadas de Hyrule Field pasaron de 24 ms a unos 17 ms de procesador por frame.
- **01.00 a 01.03**: primer arranque sobre RADV, sonido, Keep Game Speed y primeras optimizaciones del renderizador.

## Créditos y licencia

Este port se apoya en el trabajo de otras personas: mira [CREDITS.md](CREDITS.md). Los archivos propios del port están bajo licencia MIT ([LICENSE](LICENSE)). Los que vienen de otros proyectos conservan la licencia que indica su cabecera.

Sin relación con Nintendo, Sony ni el equipo de Ship of Harkinian.

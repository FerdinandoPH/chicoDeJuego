# Sacar el menú de depuración (y la terminal) del core

Continuación del split core/plataforma. Aquella primera pasada sacó vídeo, audio,
input y tiempo detrás de puertos, pero dejó dentro de `src/core/` dos cosas que no
pintaban nada ahí:

- **El menú de depuración**, que leía comandos de `stdin`.
- **`printf` / `std::cout`**, repartidos por el cargador de cartucho, el bus de
  memoria, la CPU y los savestates.

Las dos dan por hecho que hay un escritorio detrás. Una consola no tiene terminal
donde imprimir ni teclado con el que escribir una dirección, así que ninguna
sobreviviría a un port.

De paso se arregló un bug real: una carrera de datos entre el hilo de UI y el de
emulación (ver [Run_control](#3-run_control--el-handshake-de-pausa)).

---

## Lo nuevo de un vistazo

**10 ficheros nuevos, 606 líneas.** Nada de esto existía antes:

| Fichero | Líneas | Qué es |
| --- | ---: | --- |
| `src/ports/log.h` | 17 | **Puerto nuevo.** La interfaz `ILog`. |
| `src/core/core_log.h` | 13 | Fachada `log_info` / `log_warn` / `log_error`. |
| `src/core/core_log.cpp` | 46 | Implementación: formatea y reenvía al `ILog`. |
| `src/platform/sdl/sdl_log.cpp` | 18 | El `ILog` de escritorio. |
| `src/core/debug_api.h` | 63 | **Clase nueva.** `Debug_api` + helpers `parse_*`. |
| `src/core/debug_api.cpp` | 103 | Su implementación. |
| `src/core/run_control.h` | 35 | **Clase nueva.** `Run_control`, el handshake de pausa. |
| `src/core/run_control.cpp` | 36 | Su implementación. |
| `src/ports/menu.h` | 19 | **Puerto nuevo.** `platform_debug_menu()`. |
| `src/platform/sdl/console_menu.cpp` | 256 | El menú de terminal, ya en la plataforma. |

Y **cuatro conceptos nuevos** que conviene tener claros antes de tocar nada:

1. **`ILog`** — el core ya no imprime; entrega texto ya formateado y el backend
   decide dónde va.
2. **`Debug_api`** — todo lo que el menú puede hacer, como verbos y consultas que
   devuelven datos. Sin E/S de ningún tipo.
3. **`Run_control`** — pedir una parada y pararse son cosas distintas y las hacen
   hilos distintos.
4. **`platform_debug_menu()`** — el menú es un puerto más: el core lo declara, el
   backend lo implementa.

### Cómo encajan

```
                 src/core/                       src/ports/            src/platform/sdl/
                 ---------                       ----------            -----------------
  emulación  ->  Debugger, Cpu, Memory ...
                        |
                        |  log_info/warn/error
                        v
                   core_log.cpp        ---->     ILog          <----   Sdl_log
                                                (log.h)              (sdl_log.cpp)

  el menú    ->  Debug_api            <----+
                 (debug_api.h)              \
                        ^                    \
                        |                     +--- platform_debug_menu(api)
                   Run_control                        (menu.h)         console_menu.cpp
                 (run_control.h)
```

La regla es la misma que ya seguían los puertos: el core *declara* lo que
necesita, el backend lo *implementa*, y el linker los junta. Nada en `src/core/`
sabe que existe una terminal, y `CMakeLists.txt` sigue negándose a enlazar SDL
contra `cdj_core`, así que un `#include <SDL3/SDL.h>` despistado en el core rompe
la compilación a propósito.

---

# Parte 1 — Lo nuevo, en detalle

## 1. `ILog` — el puerto de log

### `src/ports/log.h` (nuevo)

```cpp
enum class Log_level { INFO, WARN, ERROR };

struct ILog {
    virtual ~ILog() = default;
    virtual void write(Log_level level, const char* msg) = 0;
};
```

### `src/core/core_log.h` (nuevo)

```cpp
void log_info(const char* fmt, ...);
void log_warn(const char* fmt, ...);
void log_error(const char* fmt, ...);
```

Esto es lo que llama el core en lugar de `printf`/`std::cout`. El mensaje se
formatea aquí (así cada backend recibe texto plano y nada más) y se entrega al
`ILog` que haya dado el backend.

### `src/ports/backend.h` (modificado)

Gana una cuarta factoría junto a las otras tres:

```cpp
ILog* create_log();
```

### `src/platform/sdl/sdl_log.cpp` (nuevo)

Escribe a `stdout`, y a `stderr` para `ERROR` (que es lo que ya hacía el código de
savestates). Un backend de consola puede tirarlo todo a la basura o escribir a un
fichero, y el core ni se entera.

### Tres detalles que no son obvios

- **El salto de línea es parte del mensaje.** Hay llamadas deliberadamente sin
  terminar para que la siguiente continúe la misma línea: `"Saving state... "`
  seguido de `"Done.\n"`. `ILog::write` **no** debe añadir ninguno por su cuenta.
- **Se llama `core_log.h` y no `log.h`.** Tanto `src/core` como `src/ports` están
  en el include path y `ports/log.h` ya ocupa ese nombre.
- **`vsnprintf` se llama dos veces.** La primera pasada solo pregunta cuánto mide
  el mensaje, así que un volcado de registros o una lista larga de breakpoints
  nunca se trunca en silencio por culpa de un buffer de tamaño fijo:

  ```cpp
  int size = std::vsnprintf(nullptr, 0, fmt, args_copy);
  std::string msg(size, '\0');
  std::vsnprintf(msg.data(), size + 1, fmt, args);
  ```

El `ILog` se resuelve la primera vez que alguien loguea, no en tiempo de inicio
estático, por si el backend necesita tener sus librerías levantadas antes de poder
entregar uno.

---

## 2. `Debug_api` — los comandos, sin E/S

### `src/core/debug_api.h` (nuevo)

Todo lo que el menú tiene permitido hacer. **Nada de esto imprime ni lee nada.**

```cpp
class Debug_api{
    public:
        Debug_api(Debugger& dbg, Cpu& cpu, Memory& mem, Host& host);

        // --- Estado de la máquina ---
        // Leerlos solo es seguro con el hilo de emulación parado,
        // que es justo cuando corre el menú.
        u8  read_mem(u16 addr);
        u16 read_reg(Reg reg);
        std::vector<u16> last_pc_values();

        // --- Breakpoints ---
        std::string breakpoints_toString();
        void add_pos_breakpoint(u16 pos);
        void add_opcode_breakpoint(u8 opcode);
        void add_mem_breakpoint(u16 addr, Dbg_cond cond, u8 value, u8 value2 = 0, bool current = false);
        void add_reg_breakpoint(Reg reg, Dbg_cond cond, u16 value, u16 value2 = 0, bool current = false);
        bool del_breakpoint(Breakpoint_type type, size_t index);
        void clear_breakpoints();

        // --- Vistas ---
        bool is_debug_window_active(DebugWindowType type);
        void toggle_debug_window(DebugWindowType type);
        void clear_main_screen();
        bool dump_memory();   // escribe mem.hexd y se lo pasa al visor del sistema

        // --- Control de ejecución ---
        // step y resume terminan el menú los dos; la diferencia es si la
        // siguiente instrucción vuelve a parar.
        void step();
        void resume();
        void quit();
};
```

**Añadir un comando al emulador = añadir un método aquí.** Cómo se invoque pasa a
ser problema de cada backend.

### Los helpers de parseo (nuevos, y a propósito fuera de la clase)

```cpp
bool parse_hex(const std::string& text, u16& out);
bool parse_cond(const std::string& text, Dbg_cond& out);   // "==", "!=", "><"...
bool parse_reg(const std::string& text, Reg& out);         // "A", "hl"...
```

Son funciones libres, no métodos, y eso es intencionado: **son el único sitio que
queda en el core que da por hecho que alguien ha tecleado algo.** Una UI sin
teclado se los salta y pasa los números directamente a `Debug_api`.

### Cosas nuevas que dependen de esto

- `toggle_debug_window` no abre la ventana: deja la petición en
  `host.debug_toggle_requested[]` porque crear y destruir ventanas le toca al hilo
  que posee el backend.
- `read_mem` / `read_reg` son nuevos y existen para que el menú pueda resolver por
  su cuenta la `X` del usuario ("el valor que haya ahora mismo") antes de crear el
  breakpoint. Antes eso lo hacía `Debugger` por dentro.

---

## 3. `Run_control` — el handshake de pausa

### El bug que arregla

`Host::on_break()` hacía esto:

```cpp
this->dbg->dbg_level = FULL_DBG;   // desde el hilo de UI
```

mientras el hilo de emulación leía `dbg_level` en cada instrucción. Eso es una
carrera de datos, y estaba ahí tanto para ESC como para Ctrl-C.

### `src/core/run_control.h` (nuevo)

Ahora el hilo de UI solo puede **pedir**:

```cpp
class Run_control{
    public:
        // --- Cualquier hilo ---
        void request_break();
        bool is_paused();
        void wait_until_paused();

        // --- Solo el hilo de emulación ---
        bool take_break_request();   // true una vez por petición, y la limpia
        void enter_paused();
        void leave_paused();
};
```

El hilo de emulación recoge la petición en un **límite de instrucción** — el único
punto donde la máquina está lo bastante consistente como para mirarla — y solo ahí
toca `dbg_level`.

Dos detalles de implementación:

- `take_break_request` usa `exchange` y no un load seguido de un store, porque ESC
  y Ctrl-C pueden llegar a la vez y la parada tiene que ocurrir exactamente una
  vez.
- `is_paused` / `wait_until_paused` **no los usa el backend SDL**, que corre su
  menú en el propio hilo de emulación. Están ahí para el backend que no puede: un
  menú en pantalla lo tiene que dibujar el hilo de render, y ese hilo necesita
  saber cuándo el emulador está realmente quieto.

---

## 4. `platform_debug_menu()` — el menú como puerto

### `src/ports/menu.h` (nuevo)

```cpp
class Debug_api;

void platform_debug_menu(Debug_api& api);
```

El contrato, que es lo importante:

- Se llama **desde el hilo de emulación**, ya parado en un límite de instrucción.
- **No debe volver hasta que el usuario termine** — volver es justo lo que reanuda
  la emulación.
- Lo que pase mientras tanto es asunto exclusivo del backend, incluido pasarle el
  trabajo a otro hilo y bloquearse aquí hasta que acabe.

### `src/platform/sdl/console_menu.cpp` (nuevo, pero es código mudado)

Es el menú de siempre, con las mismas teclas y los mismos textos; lo único que
cambia es que habla con `Debug_api` en vez de manosear `dbg`, `host`, `memory` y
`cpu` directamente. Viene de dos sitios: `debug_menu()` (estaba en `emu.cpp`) y
`add_breakpoint_menu()` / `del_breakpoint_menu()` (estaban en `debugger.cpp`).

**Dos mejoras que se colaron** porque el código se estaba reescribiendo de todos
modos, y que sí cambian el comportamiento en casos límite:

- Un índice de breakpoint no numérico ya no lanza una excepción desde `std::stoi`
  (antes reventaba el emulador). Hay un `parse_index` local que devuelve `false`.
- Las cuatro ramas casi idénticas del menú de borrado se fundieron en una sola.

Y hay una función auxiliar nueva, `read_cond_and_values`, que agrupa la cola
común de "condición + valor [+ segundo valor]" que comparten los breakpoints de
memoria y de registro, incluido el manejo de la `X`.

---

# Parte 2 — Cambios en ficheros que ya existían

`+114 / −336` en total, repartidos así:

## `src/core/emu.cpp` (+30 / −97)

`debug_menu()` desaparece. En su lugar:

```cpp
// Cede la máquina al menú del backend y espera ahí. Se llama desde el hilo de
// emulación y solo en un límite de instrucción, así que todo lo que el menú
// puede mirar es consistente; la emulación no avanza hasta que vuelve.
void enter_debug_menu(){
    memory->sync_mem_ui_copy();
    host->sync_video_buffer();
    run_control.enter_paused();
    platform_debug_menu(*debug_api);
    run_control.leave_paused();
    log_info("\n");
}
```

Los dos `sync_*` estaban antes al principio de `debug_menu` y era fácil olvidarse
de ellos; ahora forman parte de la pausa en sí.

En `cpu_run`, la petición de parada se consume **antes** de leer el nivel de
depuración:

```cpp
if(run_control.take_break_request()){
    dbg.dbg_level = FULL_DBG;
}
```

Globales nuevas en este fichero: `Run_control run_control;` y
`Debug_api* debug_api;`. Y `host->set_debugger(&dbg)` pasa a ser
`host->set_run_control(&run_control)`.

## `src/core/debugger.h` / `.cpp` (+42 / −206)

- `add_breakpoint_menu()` y `del_breakpoint_menu()` **borrados**; viven ahora en
  `console_menu.cpp`.
- Las sobrecargas `add_*_breakpoint(std::string, ...)` pasan a ser tipadas:

  ```cpp
  void add_pos_breakpoint(u16 pos);
  void add_opcode_breakpoint(u8 opcode);
  void add_mem_breakpoint(u16 addr, Dbg_cond cond, u8 value, u8 value2 = 0, bool current = false);
  void add_reg_breakpoint(Reg reg, Dbg_cond cond, u16 value, u16 value2 = 0, bool current = false);
  ```

  Ya no devuelven `bool`, porque con argumentos tipados no queda nada que pueda
  fallar: el parseo que fallaba ocurre antes de la llamada.
- `current` sigue significando lo mismo — el valor se capturó de la máquina tal y
  como estaba en ese momento, que es lo que hace que un breakpoint `!=` se rearme
  en cada cambio. Resolver la `X` del usuario a un valor concreto es ahora trabajo
  del menú, vía `Debug_api::read_mem` / `read_reg`.
- Los mensajes "Added ... breakpoint at ..." se van al menú (son respuesta a algo
  que el usuario ha pedido). Los "Reached ... breakpoint" se quedan en
  `check_breakpoints` y salen por `log_info`, porque esos son eventos, no
  respuestas.

## `src/core/host.h` / `.cpp` (+11 / −6)

`Host` ya no depende de `Debugger` **en absoluto** — solo lo usaba para esa única
línea de `on_break`. `Debugger* dbg` / `set_debugger` pasan a ser
`Run_control* run_control` / `set_run_control`.

`host.cpp` gana un `#include "ppu.h"` explícito: necesita `Sprite` para el visor de
OAM y antes lo conseguía por accidente a través de `debugger.h`.

## El resto

`cart.cpp`, `memory.cpp`, `cpu.cpp`, `controller.cpp`, `savestates.cpp`: `printf` y
`std::cout` sustituidos por `log_*`, y nada más. Los `fprintf(stderr, ...)` de
`savestates.cpp` van a `log_error`, que el backend SDL devuelve a `stderr`.

Siguen escribiendo a un `FILE*` a propósito, y está bien que lo hagan, porque son
E/S de fichero y no salida de consola:

- `Memory::dump()` → `mem.hexd`
- el log serie de `memory.cpp`
- la traza de `TRACEGEN` en `debugger.cpp`

---

# Parte 3 — Notas

## Lo que no cambió

El menú se comporta exactamente igual que antes: mismas teclas, mismos prompts,
mismos bucles de reintento, `X` para "valor actual", `><` para rangos, línea vacía
como step. Comprobado en ejecución contra `tetris.gb`: parada por SIGINT,
breakpoints de posición / memoria / registro, disparo, borrado, toggle de ventana,
continue y quit.

**El reset sigue desactivado.** El menú sigue imprimiendo
`"Resetting is dangerous rn..."` y la llamada sigue comentada. Con el handshake
puesto ya sería seguro hacerlo desde el menú (corre en el hilo de emulación, que
está parado), pero eso es un cambio de comportamiento y se dejó estar. Como
consecuencia, `Cpu_thread_args` y su semáforo quedan vestigiales — el semáforo
existía para pasarle el reset al hilo principal, y ya nadie lo lee. Hay un
comentario en `cpu_run` diciéndolo.

## Build

`CMakeLists.txt` hace glob de `src/core/*.cpp` y `src/platform/sdl/*.cpp`, así que
los ficheros nuevos entran solos — pero hay que volver a lanzar `cmake` después de
añadir uno.

Si el enlace falla con `bytecode stream ... generated with LTO version X instead of
the expected Y`, el directorio `build/` tiene objetos de un gcc más viejo. Bórralo
y reconfigura; LTO se activa siempre que el toolchain lo soporte.

## Lo que queda (el menú de configuración)

El plan original tenía un quinto paso que **no está hecho**: convertir `Prefs`
(todavía un único `bool`) en un registro de ajustes de verdad con descriptores —
`{id, label, kind, default, min/max, options}` — y un modelo de menú declarativo
que el backend *renderice* en lugar de llevarlo a fuego.

La razón es que un menú escrito por backend no escala: tres ports significan tres
menús desincronizándose. Si el core describe el menú como un árbol de
`{label, kind, id}`, la terminal lo pinta como una lista, una consola lo pinta como
un overlay navegable con el d-pad, y un core de libretro mapea las entradas
`TOGGLE`/`ENUM` casi 1:1 sobre `retro_variable`. Esos mismos descriptores dan
gratis la serialización a `.ini`/`.json`, los valores por defecto y la validación
de rangos, y el menú de depuración pasa a ser un submenú del de configuración en
vez de un sistema aparte.

`Debug_api` es el sustrato que lo hace posible: los comandos ya existen como
métodos que devuelven datos, así que el modelo de menú solo tiene que nombrarlos.

# robot-telemetria-sockets

Simulación de un robot que se desplaza en un mundo cuadriculado y expone telemetría a través de un protocolo ligero tipo solicitud/respuesta (RTTP/1.0). El proyecto incluye:

- Un servidor escrito en C (portable) que acepta clientes RTTP/1.0.
- Un cliente en C (CLI) multiplataforma.
- Un cliente en Python 3 (CLI) para ejecución rápida en cualquier sistema con Python.

El código está pensado para ser compilable y ejecutable en Linux, macOS y Windows (con herramientas adecuadas como MinGW/MSYS2 o Visual Studio para la parte en C). La versión en Python sólo requiere Python 3.

## Credenciales

Los usuarios se validan en el servidor:

- `victor` / `1234`
- `sebastian` / `1234`
- `admin` / `12345` (rol administrador; único que puede solicitar CO₂)

## Contribuidores

- Victor Hugo Chavez
- Sebastián Rodríguez Bayona

## Compilación

### Servidor (Linux / macOS / Windows)

El servidor está en `server/` y se compila con un compilador C con soporte pthreads/sockets.

Linux / macOS (ejemplo con gcc):

```bash
cd server
make
```

Windows (MSYS2 / MinGW):

- Abra una terminal MSYS2/MSYS or MinGW y ejecute `make` en la carpeta `server`.
- Alternativamente puede crear un proyecto en Visual Studio; el código usa sockets y pthreads (en Windows use la capa de compatibilidad o pthreads-w32).

### Cliente C (Linux / macOS / Windows)

El cliente en C está en la carpeta `cliente C`. Se puede compilar con `make` en entornos Unix-like o en MSYS2/MinGW en Windows:

```bash
cd "cliente C"
make
```

En Windows puede compilar con MinGW/MSYS2 o usar Visual Studio adaptando el proyecto.

### Cliente Python (cualquier plataforma con Python 3)

El cliente en Python está en `cliente_python/` y no requiere dependencias externas (usa la biblioteca estándar).

```bash
cd cliente_python
python3 client.py <host> <puerto>
```

En Windows puede usar `python` si `python3` no está disponible.

## Ejecución

1. Inicia el servidor (puerto por defecto 15000):

	 - En Linux/macOS:

		 ```bash
		 ./server 15000
		 ```

	 - En Windows (si compiló como ejecutable):

		 ```powershell
		 .\server.exe 15000
		 ```

2. Ejecuta un cliente apuntando a la IP/host donde corre el servidor y al mismo puerto:

	 - Cliente C (Unix-like):

		 ```bash
		 ./cliente 192.168.x.x 15000
		 ```

	 - Cliente C (Windows):

		 ```powershell
		 .\cliente.exe 192.168.x.x 15000
		 ```

	 - Cliente Python (cualquier SO con Python 3):

		 ```bash
		 python3 cliente_python/client.py 192.168.x.x 15000
		 ```

3. Autentícate con uno de los usuarios anteriores y utiliza los comandos interactivos:

- `move <dir> [pasos]` (dir: LEFT, RIGHT, UP, DOWN)
- `pos`
- `sense [lista_variables]` (ej.: `sense temp,hum`)
- `logout`
- `exit`

Las respuestas se entregan en JSON. El servidor valida colisiones con obstáculos, límites del mapa (10x10) y genera telemetría simulada (temperatura, humedad, presión y CO₂ —este último solamente disponible para el usuario `admin`).

Puedes usar cualquiera de los clientes (C o Python). El cliente en Python es la forma más rápida de probar la aplicación en cualquier sistema operativo; el cliente en C proporciona una alternativa nativa y compilable.

## Puertos y red

- El ejecutable del servidor acepta el puerto como primer argumento. Si tu profesor necesita usar otro valor, basta con arrancar `./server <puerto>` (o `server.exe <puerto>` en Windows) y pasar ese mismo puerto al cliente. No es necesario recompilar.
- El reenvío de puertos que tengas configurado para SSH (por ejemplo, `2222 -> 22`) es independiente: puedes crear otra regla para el puerto del servidor, por ejemplo `15000 -> 15000`, o elegir cualquier puerto libre del host.
- Si prefieres modo *bridged*, simplemente apunta el cliente a la IP real de la VM y al puerto elegido.
- Asegúrate de que el firewall de la VM/host permita el puerto seleccionado si está habilitado.

---

Si quieres, puedo también añadir una pequeña sección de "Pruebas rápidas" con comandos de ejemplo para validar la conexión localmente (server + cliente Python en la misma máquina), o generar un archivo `requirements.txt` (no necesario aquí) y un script para lanzar servidor+cliente para demos.
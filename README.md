# robot-telemetria-sockets

Simulación de un robot que se desplaza en un mundo cuadriculado y expone telemetría a través de un protocolo ligero tipo solicitud/respuesta (RTTP/1.0). El servidor corre en C (pensado para Ubuntu/Linux) y el cliente interactivo en C corre desde macOS.

## Credenciales

Los usuarios se validan en el servidor:

- `victor` / `1234`
- `sebastian` / `1234`
- `admin` / `12345` (rol administrador; único que puede solicitar CO₂)

## Compilación

### Servidor (Ubuntu/VM)

```bash
cd server
make
```

### Cliente (macOS)

```bash
cd "cliente C"
make
```

### Cliente Python (cualquier plataforma con Python 3)

```bash
cd cliente_python
python3 client.py <host> <puerto>
```

## Ejecución

1. Inicia el servidor en la VM (puerto por defecto 15000):

	```bash
	./server 15000
	```

2. Desde macOS ejecuta el cliente indicando la IP de la VM y el puerto:

	```bash
	./cliente 192.168.x.x 15000
	```

3. Autentícate con uno de los usuarios anteriores y utiliza los comandos interactivos:

	- `move <dir> [pasos]` (dir: LEFT, RIGHT, UP, DOWN)
	- `pos`
	- `sense [lista_variables]` (ej.: `sense temp,hum`)
	- `logout`
	- `exit`

Las respuestas se entregan en JSON. El servidor valida colisiones con obstáculos, límites del mapa (10x10) y genera telemetría simulada (temperatura, humedad, presión y CO₂ solo para administradores).
Puedes optar por el cliente en C o el cliente en Python: ambos hablan el mismo protocolo RTTP/1.0.

## Puertos y red

- El ejecutable del servidor acepta el puerto como primer argumento. Si necesita usar otro valor, basta con arrancar `./server <puerto>` y pasar ese mismo puerto al cliente. No es necesario recompilar.
- El reenvío de puertos que tengas configurado para SSH (por ejemplo, `2222 -> 22`) es independiente: puedes crear otra regla para el puerto del servidor, por ejemplo `15000 -> 15000`, o elegir cualquier puerto libre del host.
- Si prefieres modo *bridged*, simplemente apunta el cliente a la IP real de la VM y al puerto elegido.
- Asegúrate de que el firewall de la VM permita el puerto seleccionado (`ufw allow <puerto>/tcp`) si está habilitado.

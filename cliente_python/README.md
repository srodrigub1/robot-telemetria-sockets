# Cliente Python

Cliente interactivo en Python 3 para `robot-telemetria-sockets`. Permite autenticarse, enviar comandos de movimiento, consultar posición y solicitar telemetría, usando el mismo protocolo RTTP/1.0 que el cliente en C.

## Requisitos

- Python 3.8 o superior.

## Instalación

No requiere dependencias externas. Si deseas un entorno virtual:

```bash
cd cliente_python
python3 -m venv .venv
source .venv/bin/activate  # Linux/macOS
pip install --upgrade pip
```

## Uso

```bash
cd cliente_python
python3 client.py <host> <puerto>
```

Ejemplo:

```bash
python3 client.py 192.168.1.50 15000
```

Comandos disponibles en la interfaz interactiva:

- `move <LEFT|RIGHT|UP|DOWN> [pasos]`
- `pos`
- `sense [lista_variables]` (por defecto temperatura, humedad y presión)
- `logout`
- `exit`

Recuerda que el usuario `admin` tiene acceso al dato de CO₂.

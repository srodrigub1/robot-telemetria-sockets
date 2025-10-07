#!/usr/bin/env python3
import argparse
import json
import socket
import sys
from typing import Optional

BUF_SZ = 4096


def recv_line(sock: socket.socket) -> Optional[str]:
    data = bytearray()
    while True:
        ch = sock.recv(1)
        if not ch:
            return None
        data += ch
        if ch == b"\n":
            break
    return data.decode("utf-8", errors="replace")


def recv_n(sock: socket.socket, length: int) -> Optional[bytes]:
    data = bytearray()
    while len(data) < length:
        chunk = sock.recv(length - len(data))
        if not chunk:
            return None
        data += chunk
    return bytes(data)


def send_all(sock: socket.socket, data: bytes) -> None:
    total = 0
    while total < len(data):
        sent = sock.send(data[total:])
        if sent == 0:
            raise RuntimeError("socket connection broken")
        total += sent


def send_request(sock: socket.socket, method: str, resource: str, *,
                 client_id: str = "", token: str = "", vars_header: str = "",
                 body: str = "") -> None:
    lines = [f"{method} {resource} RTTP/1.0\r\n"]
    if client_id:
        lines.append(f"Client-Id: {client_id}\r\n")
    if token:
        lines.append(f"Token: {token}\r\n")
    if vars_header:
        lines.append(f"Vars: {vars_header}\r\n")
    body_bytes = body.encode("utf-8") if body else b""
    lines.append(f"Content-Length: {len(body_bytes)}\r\n\r\n")
    header = "".join(lines).encode("utf-8")
    send_all(sock, header)
    if body_bytes:
        send_all(sock, body_bytes)


def read_response(sock: socket.socket) -> tuple[int, str, dict]:
    first = recv_line(sock)
    if first is None:
        raise RuntimeError("Conexion cerrada por el servidor (sin encabezado)")
    first = first.strip()
    parts = first.split(maxsplit=2)
    if len(parts) < 2:
        raise RuntimeError(f"Encabezado invalido: {first}")
    code = int(parts[1])
    reason = parts[2] if len(parts) > 2 else ""
    content_length = 0
    while True:
        line = recv_line(sock)
        if line is None:
            raise RuntimeError("Conexion cerrada por el servidor (sin cuerpo)")
        line = line.strip()
        if not line:
            break
        if line.lower().startswith("content-length:"):
            value = line.split(":", 1)[1].strip()
            content_length = int(value)
    body_bytes = recv_n(sock, content_length) if content_length else b""
    body = body_bytes.decode("utf-8", errors="replace") if body_bytes else ""
    payload = {}
    if body:
        try:
            payload = json.loads(body)
        except json.JSONDecodeError:
            payload = {"raw": body}
    return code, reason, payload


def prompt(text: str) -> str:
    try:
        return input(text)
    except EOFError:
        return ""


def main() -> int:
    parser = argparse.ArgumentParser(description="Cliente Python para robot-telemetria-sockets")
    parser.add_argument("host", help="Dirección IP o nombre del servidor")
    parser.add_argument("port", type=int, help="Puerto del servidor")
    args = parser.parse_args()

    with socket.create_connection((args.host, args.port)) as sock:
        print(f"Conectado a {args.host}:{args.port}")
        user = prompt("Usuario: ").strip()
        passwd = prompt("Contraseña: ").strip()
        auth_body = json.dumps({"user": user, "pass": passwd})
        send_request(sock, "AUTH", "/login", client_id=user, body=auth_body)
        code, reason, payload = read_response(sock)
        print(f"[{code} {reason}] {payload}")
        if code != 200:
            return 1
        token = payload.get("token", "")
        if not token:
            print("No se recibió token válido")
            return 1

        while True:
            cmd = prompt("comando (move/pos/sense/logout/exit)> ").strip()
            if not cmd:
                continue
            if cmd.lower() in {"exit", "quit"}:
                break
            elif cmd.lower().startswith("move"):
                parts = cmd.split()
                if len(parts) < 2:
                    print("Uso: move <LEFT|RIGHT|UP|DOWN> [pasos]")
                    continue
                direction = parts[1].upper()
                steps = int(parts[2]) if len(parts) > 2 else 1
                move_body = json.dumps({"dir": direction, "steps": steps})
                send_request(sock, "MOVE", "/", client_id=user, token=token, body=move_body)
            elif cmd.lower() == "pos":
                send_request(sock, "GET", "/pos", client_id=user, token=token)
            elif cmd.lower().startswith("sense"):
                vars_header = cmd[5:].strip()
                send_request(sock, "GET", "/telemetry", client_id=user, token=token, vars_header=vars_header)
            elif cmd.lower() == "logout":
                send_request(sock, "LOGOUT", "/", client_id=user, token=token)
            else:
                print("Comando desconocido")
                continue

            code, reason, payload = read_response(sock)
            print(f"[{code} {reason}] {payload}")
            if cmd.lower() == "logout" and code == 200:
                break
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\nInterrumpido")
        sys.exit(130)

#!/usr/bin/env python3
import argparse
import hashlib
import socket
import struct
import sys

HOST = "127.0.0.1"
LOGIN = "user"
PASSWORD = "P@ssW0rd"


def receive_exact(sock, size):
    data = b""
    while len(data) < size:
        part = sock.recv(size - len(data))
        if not part:
            raise RuntimeError("Соединение закрыто до получения всех данных")
        data += part
    return data


def authenticate(sock):
    sock.sendall(LOGIN.encode("ascii"))
    salt = receive_exact(sock, 16).decode("ascii")
    if len(salt) != 16 or salt.upper() != salt:
        raise RuntimeError("Получена некорректная соль")

    digest = hashlib.sha256((salt + PASSWORD).encode("ascii")).hexdigest().upper()
    sock.sendall(digest.encode("ascii"))
    if receive_exact(sock, 2) != b"OK":
        raise RuntimeError("Сервер не подтвердил аутентификацию")


def send_vectors(sock, vectors):
    sock.sendall(struct.pack("=I", len(vectors)))
    results = []
    for vector in vectors:
        sock.sendall(struct.pack("=I", len(vector)))
        sock.sendall(struct.pack("=" + "h" * len(vector), *vector))
        results.append(struct.unpack("=h", receive_exact(sock, 2))[0])
    return results


def successful_calculation(port):
    vectors = [[1, 2, 3, 4], [800, 1000, 1200, 1200], [32767, 1], [-32768, -1]]
    expected = [10, 4200, 32767, -32768]
    with socket.create_connection((HOST, port), timeout=5) as sock:
        authenticate(sock)
        results = send_vectors(sock, vectors)
    if results != expected:
        raise RuntimeError("Получены неверные результаты: " + str(results))
    print("[PASS] Успешная аутентификация и расчёт: " + str(results))


def wrong_login(port):
    with socket.create_connection((HOST, port), timeout=5) as sock:
        sock.sendall(b"unknown_user")
        answer = receive_exact(sock, 3)
    if answer != b"ERR":
        raise RuntimeError("Вместо ERR получено: " + repr(answer))
    print("[PASS] Неизвестный логин: ERR")


def wrong_hash(port):
    with socket.create_connection((HOST, port), timeout=5) as sock:
        sock.sendall(LOGIN.encode("ascii"))
        salt = receive_exact(sock, 16).decode("ascii")
        digest = hashlib.sha1((salt + PASSWORD).encode("ascii")).hexdigest().upper()
        sock.sendall(digest.encode("ascii"))
        answer = receive_exact(sock, 3)
    if answer != b"ERR":
        raise RuntimeError("Вместо ERR получено: " + repr(answer))
    print("[PASS] SHA-1 вместо SHA-256: ERR")


def truncated_vector(port):
    with socket.create_connection((HOST, port), timeout=5) as sock:
        authenticate(sock)
        sock.sendall(struct.pack("=I", 1))
        sock.sendall(struct.pack("=I", 4))
        sock.sendall(struct.pack("=hh", 10, 20))
        sock.shutdown(socket.SHUT_WR)
        answer = sock.recv(1)
    if answer:
        raise RuntimeError("Сервер отправил данные для неполного вектора")
    print("[PASS] Неполный бинарный вектор: соединение закрыто")


def large_vector(port):
    vector = [1] * 4096
    with socket.create_connection((HOST, port), timeout=5) as sock:
        authenticate(sock)
        results = send_vectors(sock, [vector])
    if results != [4096]:
        raise RuntimeError("Получен неверный результат: " + str(results))
    print("[PASS] Вектор из 4096 значений: 4096")


def repeated_connections(port, count):
    for _ in range(count):
        with socket.create_connection((HOST, port), timeout=5) as sock:
            authenticate(sock)
            results = send_vectors(sock, [[1, 2, 3, 4]])
        if results != [10]:
            raise RuntimeError("Получен неверный результат: " + str(results))
    print("[PASS] Последовательных подключений: " + str(count))


def main():
    parser = argparse.ArgumentParser(description="Функциональные проверки vcalc")
    parser.add_argument("case", choices=["success", "wrong-login", "wrong-hash", "truncated-vector", "large-vector", "load"])
    parser.add_argument("--port", type=int, default=33333)
    parser.add_argument("--count", type=int, default=100)
    arguments = parser.parse_args()

    if arguments.case == "success":
        successful_calculation(arguments.port)
    elif arguments.case == "wrong-login":
        wrong_login(arguments.port)
    elif arguments.case == "wrong-hash":
        wrong_hash(arguments.port)
    elif arguments.case == "truncated-vector":
        truncated_vector(arguments.port)
    elif arguments.case == "large-vector":
        large_vector(arguments.port)
    else:
        repeated_connections(arguments.port, arguments.count)


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print("[FAIL] " + str(error), file=sys.stderr)
        sys.exit(1)

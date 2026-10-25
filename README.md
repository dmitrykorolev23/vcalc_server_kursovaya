Параметры варианта:

```text
Хеш-функция: SHA-256(SALT16 || PASSWORD)
Соль: серверная
Тип значений вектора: int16_t
```

## Сборка

Нужны `g++`, `make` и библиотека Crypto++.

```bash
sudo apt update
sudo apt install -y g++ make libcrypto++-dev
make
```

После сборки сервер находится здесь:

```text
bin/vcalc_server
```

## Запуск

```bash
mkdir -p logs
./bin/vcalc_server -d examples/vcalc.conf -l logs/vcalc.log -p 33333
```

Сервер ожидает подключения на TCP-порту `33333`. Остановка — `Ctrl+C`.

Параметры запуска:

```text
-d ФАЙЛ   файл базы LOGIN:PASSWORD
-l ФАЙЛ   файл журнала
-p ПОРТ   TCP-порт
-h        справка
```


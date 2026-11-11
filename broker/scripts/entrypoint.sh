#!/bin/sh
set -eu
# Private host files stay read-only. Copy only broker material to Linux tmpfs;
# this also gives reliable permissions with Docker Desktop/Windows bind mounts.
for file in server.crt server.key mosquitto.passwd acl ready; do
  if [ ! -s "/sentinel/private/$file" ]; then
    echo 'Falta material privado: ejecutar python3 broker/scripts/prepare.py --node-id ID antes de iniciar Compose.' >&2
    exit 1
  fi
done
for file in server.crt server.key mosquitto.passwd acl; do
  cp "/sentinel/private/$file" "/run/sentinel/$file"
  chmod 600 "/run/sentinel/$file"
done
chown -R 1883:1883 /run/sentinel /mosquitto/data
chmod 700 /run/sentinel /mosquitto/data
# Mosquitto drops root to its configured user before serving connections.
exec /usr/sbin/mosquitto -c /sentinel/config/mosquitto.conf

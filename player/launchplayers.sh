#!/bin/bash

set -e  # Salir si make falla

cd ~/realsuciedad/player/build
make

sleep 2

TEAM1="RealSuciedad"
TEAM2="RayoCayetano"

# Equipo izquierdo (puertos 7001..7011)
for PORT in $(seq 7001 7011); do
  echo "Lanzando $TEAM1 en puerto $PORT..."
  (
    cd /mnt/c || exit 1   # Evita problema UNC en cmd.exe
    cmd.exe /c start wt.exe wsl.exe -d Ubuntu -- bash -lc "cd ~/realsuciedad/player/build && ./player $TEAM1 $PORT"
  ) &
  sleep 0.5
done
echo "Todos los jugadores del equipo $TEAM1 lanzados."

#Equipo derecho (puertos 8001..8011)
for PORT in $(seq 8001 8011); do
  echo "Lanzando $TEAM2 en puerto $PORT..."
  (
    cd /mnt/c || exit 1
    cmd.exe /c start wt.exe wsl.exe -d Ubuntu -- bash -lc "cd ~/realsuciedad/player/build && ./player $TEAM2 $PORT"
  ) &
  sleep 0.5
done
echo "Todos los jugadores del equipo $TEAM2 lanzados."

echo "Lanzamiento de jugadores completado."

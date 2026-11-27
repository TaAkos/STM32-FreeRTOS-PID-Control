# STM32 FreeRTOS – Valós idejű PID szabályozórendszer

STM32 NUCLEO-F446RE fejlesztőpanelre készített, C nyelven implementált, FreeRTOS-alapú valós idejű szabályozórendszer, amely egy virtuális elsőrendű folyamatmodellt vezérel diszkrét PID szabályozóval.

## Főbb funkciók

- Periodikus PID szabályozás
- PID paraméterek és kimeneti korlátok futás közbeni módosítása
- Konfigurálható alapjel-, zavarjel- és zajgenerálás
- Moduláris, több taskból álló FreeRTOS architektúra
- Taskok közötti kommunikáció queue-k, task notificationök, EventGroupok és semaphore-ok segítségével
- Saját bináris UART protokoll DMA-alapú adatátvitellel, CRC16 ellenőrzéssel és sequence numberrel
- Heartbeat-alapú task-felügyelet és központi hibakezelés
- Futásidő-, jitter- és stackhasználat-monitorozás
- Hardveres watchdog (IWDG) és biztonságos hibaállapot kezelése
- Python-alapú valós idejű vizualizáció és CSV-naplózás

## Alkalmazott technológiák

C · STM32 · FreeRTOS · STM32Cube · PlatformIO · UART · DMA · Python

## A repository célja

A projekt szakmai portfólió és műszaki értékelés céljából került közzétételre. A fordításhoz szükséges külső FreeRTOS-Kernel szándékosan nem része a repositorynak.

## Szerzői jogok

Minden jog fenntartva. A felhasználási feltételeket a [COPYRIGHT.md](COPYRIGHT.md) fájl tartalmazza.
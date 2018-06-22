# System-Programming-Project

## TODO
- [x] Creare un parser per le opzioni. (Antonio)
- [x] Creare server Multi-Thread Multi-processo (pre thread e pre processo, pre forkato) (Il processo sotto unix deve essere Deamon sotto windows no).
- [x] Creare parser per i file. (Giacomino)
- [x] Trovare o Creare un decodere Base64 per il punto successivo. (Antonio)
- [x] Creare parser per le richieste HTTP. (Attenzione username e password sono dentro Authorization: )
- [x] Handler per sighup per la versione unix e per windows evento di console.
- [x] Creare creatore dei log secondo lo standard sul testo di Bernaschi.
- [x] Gestione command compreso di funzione per spawnare un processo dandogli come input un comando, ritorna come output l'output del comando Gesitsce anche la sincronizzazione. Progettare con la possibilità futura di passare argomenti.
- [x] Gestire la connessione sulla porta 8081.
- [x] Creare un funzione che cifra un file che mappa in memoria, la cifratura é: xor bitwise usando srand(ip-client).
- [x] Gestire download e upload dei file GET PUT. Se viene passato con la GET un nome di una director deve restituire l'albero della directori (non ricorsivo)
- [x] Gestire l'accesso esclusivo dei file mediante lock.


Gestire per bene gli errori delle fwrite e delle send.

## File Rivisti
- main.c
- signal.c
- server.c

## Funzioni Riviste operation.c
- send_file
- send_file_chipher
- put_file
- list_dir `Dobbiamo fare una scelta sulle realloc`
- http_log
- log_write
- m_sleep
- Send
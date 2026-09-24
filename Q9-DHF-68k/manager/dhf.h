#ifndef _DHF_H
#define _DHF_H

#include <types.h>
#include <io.h>

/* Kommandocodes für den Host-Simulator (Ihre 13 Aufrufe) */
#define DHF_CMD_CREATE   1
#define DHF_CMD_OPEN     2
#define DHF_CMD_CLOSE    3
#define DHF_CMD_READ     4
#define DHF_CMD_WRITE    5
#define DHF_CMD_SEEK     6
#define DHF_CMD_DELETE   7
#define DHF_CMD_MAKDIR   8
#define DHF_CMD_CHGDIR   9
#define DHF_CMD_READDIR 10
#define DHF_CMD_GETSTAT 11
#define DHF_CMD_SETSTAT 12
#define DHF_CMD_RENAME  13

/* 
 * Die Kommandostruktur, die an den Treiber/Emulator übergeben wird.
 * Da OS-9 68K standardmäßig auf 16-Bit/32-Bit aligned, 
 * halten wir die Variablen sauber strukturiert.
 */
typedef struct {
    u_int16      command;     /* Das Kommandobyte / Word */
    u_int16      status;      /* Rückgabewert/Fehlercode vom Host */
    u_int32      param1;      /* z.B. Dateizeiger, Modus oder Länge */
    u_int32      param2;      /* z.B. Seek-Position */
    void         *buffer;     /* Zeiger auf die Daten im OS-9 Speicher */
    char         *path_name;  /* Zeiger auf den Dateinamen/Pfadstring */
} DhfCmdStruct;

/* 
 * Unser Custom Pfaddeskriptor für den DHF-Manager.
 * Das erste Element MÜSSEN die Standard-OS-9-Pfadvariablen sein.
 */
typedef struct {
    /* 
     * Der OS-9 Standard-Pfad-Header (enthält Pfadnummer, UserID etc.).
     * 'path_desc' ist in <io.h> definiert.
     */
    struct path_desc  os9_path; 
    
    /* Ab hier erweitern wir den Pfaddeskriptor um unsere eigenen Daten */
    DhfCmdStruct      cmd;      /* Die eingebettete Kommandostruktur */
} DhfPath;

#endif


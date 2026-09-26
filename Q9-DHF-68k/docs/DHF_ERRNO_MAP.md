# DHF: Host-errno → OS-9-Fehlercode

Stand 26.09.2026. Das Status-Byte des Geräts geht **unverändert** als d1.w (mit Carry) an den
I$-Aufrufer, die Werte sind also die echten OS-9-Codes aus `MWOS/SRC/DEFS/errno.h`.
Quelle: `errno_to_dhf()` und `enum dhf_error` in `Q9-Flux-68k/src/devices/dhf/`.

| Host (errno / Lage) | OS-9 | Code |
|---|---|---|
| ENOENT | E$PNNF | $D8 |
| EACCES, EPERM | E$FNA | $D6 |
| EEXIST (Create auf vorhandene Datei) | E$CEF | $DA |
| EBADF, unbekannte Pfadnummer | E$BPNum | $C9 |
| ENOSPC, EDQUOT, EFBIG | E$Full | $F8 |
| EISDIR, ENOTDIR, Verzeichnis ohne/Datei mit Dir-Bit | E$FNA | $D6 |
| ENOTEMPTY (Verzeichnis löschen) | E$DNE | $EE |
| EROFS, nur lesbares Laufwerk, Rohgerät schreiben | E$WP | $F2 |
| EMFILE, ENFILE | E$PthFul | $C8 |
| ENAMETOOLONG, ELOOP, Name mit „/“ oder „..“ bei Rename | E$BPNam | $D7 |
| EBUSY, ETXTBSY | E$Share | $FD |
| Dateiende (Read, ReadLn, SS_EOF) | E$EOF | $D3 |
| unbekanntes Kommando | E$UnkSvc | $D0 |
| Gerät/Backend antwortet nicht | E$NotRdy | $F6 |
| alles andere | E$Write | $F5 |

Vom Manager selbst: unbekannter GetStt/SetStt-Code → E$UnkSvc ($D0).
Von IOMan (erreichen DHF gar nicht): Schreiben auf Lesepfad → E$BMode ($CB), Modus-Bit nicht
in M$Mode → E$BMode.

> **VERALTET (26.09.2026):** Aus der Frühphase von DHF, vom heutigen Stand überholt und von
> keinem aktiven Teil benutzt. Aktiv sind `manager/dhfmgr_68k.a`, `driver/dhfdrv_68k.a`,
> `descriptor/d0_dhf.a`/`d1_dhf.a` und das Gerät in `Q9-Flux-68k/src/devices/dhf/` –
> s. `Q9-OS/Q9-DHF-68k/README.md` und `docs/PROTOCOL.md`.

dhfdrv-68k

Driver skeleton that should implement mapping from Q9 filemanager calls to host filesystem calls (iOS, Linux, Windows).
Ensure path confinement so basepath cannot be escaped by chdir/cd operations.

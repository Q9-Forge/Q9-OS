> **VERALTET (26.09.2026):** Aus der Frühphase von DHF, vom heutigen Stand überholt und von
> keinem aktiven Teil benutzt. Aktiv sind `manager/dhfmgr_68k.a`, `driver/dhfdrv_68k.a`,
> `descriptor/d0_dhf.a`/`d1_dhf.a` und das Gerät in `Q9-Flux-68k/src/devices/dhf/` –
> s. `Q9-OS/Q9-DHF-68k/README.md` und `docs/PROTOCOL.md`.

Q9-DHF Manager (Q9-DHF-68k)

Skeleton manager that forwards Q9 filemanager calls to the dhf driver (dhfdrv-68k).
Implementations should perform validation, path confinement and mapping from Q9 semantics to host FS via the driver API.

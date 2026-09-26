> **VERALTET (26.09.2026):** Aus der Frühphase von DHF, vom heutigen Stand überholt und von
> keinem aktiven Teil benutzt. Aktiv sind `manager/dhfmgr_68k.a`, `driver/dhfdrv_68k.a`,
> `descriptor/d0_dhf.a`/`d1_dhf.a` und das Gerät in `Q9-Flux-68k/src/devices/dhf/` –
> s. `Q9-OS/Q9-DHF-68k/README.md` und `docs/PROTOCOL.md`.

Host simulator skeleton (file-backed memory backend)

Usage:
  make
  ./host_simulator <emulator_mem_file>

This simple skeleton maps the provided file and prints the dhf_shared_t fields.
Replace the file-backed backend with a real emulator memory reader (QEMU, gdb, etc.) later.

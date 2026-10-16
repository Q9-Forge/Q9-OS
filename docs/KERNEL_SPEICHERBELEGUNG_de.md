# Kernel-Speicherbelegung (absolute Zellen)

Diese Datei wird von `tools/q9_scratch_map.py --write` erzeugt und im Build
gegen die Quellen geprueft (`--check`). Nicht von Hand aendern.

**Regeln:** Jede Zelle gehoert genau einer Funktion. Zwei Namen auf derselben
Adresse sind nur zulaessig, wenn sie in `tools/q9_scratch_shared.txt` mit
Begruendung stehen (sie leben dann nie gleichzeitig). Wer eine Zelle
hinzufuegt, erzeugt diese Tabelle neu und prueft die Luecken unten.

| Adresse | Groesse | Name | Quelle | Bemerkung |
|---|---:|---|---|---|
| `$001000` | 4 | `Q9K_BOOTLIST_ADDR` | q9kernel_cinit.c |  |
| `$001200` | 4 | `Q9K_CPUCOUNT_ADDR` | q9kernel_cinit.c |  |
| `$001204` | 4 | `Q9K_PROCPOOL_BASE_ADDR` | q9kernel_firstproc.c |  |
| `$00120C` | 4 | `Q9K_PROCPOOL_FREE_ADDR` | q9kernel_firstproc.c | s. q9kernel_tables.c |
| `$001214` | 4 | `Q9K_PATHPOOL_BASE_ADDR` | q9kernel_entry.a; C: Q9K_FORK_PATHPOOL_BASE_ADDR, Q9K_PATHPOOL_BASE_ADDR, Q9K_PATHPOOL_BASE_ADDR |  |
| `$00121C` | 4 | `Q9K_PATHPOOL_FREE_ADDR` | q9kernel_entry.a; C: Q9K_PATHPOOL_FREE_ADDR, Q9K_PATHPOOL_FREE_ADDR, Q9K_PATHPOOL_FREE_ADDR |  |
| `$001220` | 4 | `Q9K_PROCPOOL_COUNT_ADDR` | q9kernel_procapi.c |  |
| `$001224` | 4 | `Q9K_BOOT_STATUS_ADDR` | q9kernel_cinit.c | 0 = ready, nonzero = fatal init stage |
| `$001230` | 4 | `Q9K_TrapHandlerScratch` | q9kernel_entry.a |  |
| `$001234` | 4 | `Q9K_MODDIR_FREE_ADDR` | q9kernel_moddir.c |  |
| `$001238` | 4 | `Q9K_MODDIR_HEAD_ADDR` | q9kernel_moddir.c |  |
| `$00123C` | 4 | `Q9K_SCHED_SLICE_ADDR` | q9kernel_sched.c |  |
| `$001240` | 4 | `Q9K_READYQ_SENTINEL_ADDR` | q9kernel_cinit.c |  |
| `$001278` | 2 | `Q9K_ForkScratch_TypeLang` | q9kernel_entry.a; C: Q9K_FORK_SCRATCH_TYPELANG | Q9_u16, d0.w EIN |
| `$00127C` | 4 | `Q9K_ForkScratch_AddMem` | q9kernel_entry.a; C: Q9K_FORK_SCRATCH_ADDMEM | Q9_u32, d1.l EIN |
| `$001280` | 4 | `Q9K_ForkScratch_ParamSize` | q9kernel_entry.a; C: Q9K_FORK_SCRATCH_PARAMSIZE | Q9_u32, d2.l EIN |
| `$001284` | 4 | `Q9K_ForkScratch_NumPaths` | q9kernel_entry.a; C: Q9K_FORK_SCRATCH_NUMPATHS, Q9K_FORK_SCRATCH_NUMPATHS | Q9_u16, d3.w EIN (bisher ungenutzt) |
| `$001288` | 2 | `Q9K_ForkScratch_Priority` | q9kernel_entry.a; C: Q9K_FORK_SCRATCH_PRIORITY | Q9_u16, d4.w EIN |
| `$00128C` | 4 | `Q9K_ForkScratch_NamePtr` | q9kernel_entry.a; C: Q9K_FORK_SCRATCH_NAMEPTR | Q9_u32, (a0) EIN/AUS |
| `$001290` | 4 | `Q9K_ForkScratch_ParamPtr` | q9kernel_entry.a; C: Q9K_FORK_SCRATCH_PARAMPTR | Q9_u32, (a1) EIN |
| `$001294` | 2 | `Q9K_ForkScratch_ChildPID` | q9kernel_entry.a; C: Q9K_FORK_SCRATCH_CHILDPID | Q9_u16, d0.w AUS (Erfolg) |
| `$001298` | 2 | `Q9K_ForkScratch_ErrorCode` | q9kernel_entry.a; C: Q9K_FORK_SCRATCH_ERRORCODE | Q9_u16, d1.w AUS (Fehlschlag) |
| `$00129C` | 2 | `Q9K_ForkScratch_Success` | q9kernel_entry.a; C: Q9K_FORK_SCRATCH_SUCCESS | Q9_u16, 0=Fehlschlag/1=Erfolg |
| `$0012A0` | 4 | `Q9K_WAITQ_SENTINEL_ADDR` | q9kernel_cinit.c |  |
| `$0012D8` | 2 | `Q9K_WaitScratch_ChildPID` | q9kernel_entry.a; C: Q9K_WAIT_SCRATCH_CHILDPID | Q9_u16, d0.w AUS (Erfolg) |
| `$0012DC` | 2 | `Q9K_WaitScratch_ExitStatus` | q9kernel_entry.a; C: Q9K_WAIT_SCRATCH_EXITSTATUS | Q9_u16, d1.w AUS (Erfolg) |
| `$0012E0` | 2 | `Q9K_WaitScratch_ErrorCode` | q9kernel_entry.a; C: Q9K_WAIT_SCRATCH_ERRORCODE | Q9_u16, d1.w AUS (E_NOCHLD) |
| `$0012E4` | 2 | `Q9K_WaitScratch_Outcome` | q9kernel_entry.a; C: Q9K_WAIT_SCRATCH_OUTCOME | Q9_u16, 0=blockieren/1=sofort erfolgreich/2=E_NOCHLD |
| `$0012E8` | 2 | `Q9K_ExitScratch_Status` | q9kernel_entry.a; C: Q9K_EXIT_SCRATCH_STATUS | Q9_u16, d1.w EIN |
| `$0012EC` | 4 | `Q9K_ExitScratch_Next` | q9kernel_entry.a; C: Q9K_EXIT_SCRATCH_NEXT | Q9_u32, naechster Deskriptor AUS |
| `$0012F0` | 4 | `Q9K_SLEEPQ_SENTINEL_ADDR` | q9kernel_cinit.c |  |
| `$001328` | 4 | `Q9K_SleepScratch_TicksIn` | q9kernel_entry.a; C: Q9K_SLEEP_SCRATCH_TICKSIN | Q9_u32, d0.l EIN |
| `$00132C` | 4 | `Q9K_SleepScratch_Next` | q9kernel_entry.a; C: Q9K_SLEEP_SCRATCH_NEXT | Q9_u32, naechster Deskriptor AUS |
| `$001330` | 4 | `Q9K_SRqMemScratch_SizeIn` | q9kernel_entry.a; C: Q9K_SRQMEM_SCRATCH_SIZEIN | Q9_u32, d0.l EIN (F$SRqMem) |
| `$001334` | 4 | `Q9K_SRqMemScratch_Addr` | q9kernel_entry.a; C: Q9K_SRQMEM_SCRATCH_ADDR | Q9_u32, (a2) AUS (Erfolg) |
| `$001338` | 4 | `Q9K_SRqMemScratch_SizeOut` | q9kernel_entry.a; C: Q9K_SRQMEM_SCRATCH_SIZEOUT | Q9_u32, d0.l AUS (Erfolg) |
| `$00133C` | 2 | `Q9K_SRqMemScratch_Error` | q9kernel_entry.a; C: Q9K_SRQMEM_SCRATCH_ERROR | Q9_u16, d1.w AUS (Fehlschlag) |
| `$001340` | 2 | `Q9K_SRqMemScratch_Success` | q9kernel_entry.a; C: Q9K_SRQMEM_SCRATCH_SUCCESS | Q9_u16, 0=Fehlschlag/1=Erfolg |
| `$001344` | 4 | `Q9K_SRtMemScratch_AddrIn` | q9kernel_entry.a; C: Q9K_SRTMEM_SCRATCH_ADDRIN | Q9_u32, (a2) EIN (F$SRtMem) |
| `$001348` | 4 | `Q9K_SRtMemScratch_SizeIn` | q9kernel_entry.a; C: Q9K_SRTMEM_SCRATCH_SIZEIN | Q9_u32, d0.l EIN |
| `$00134C` | 4 | `Q9K_SSvcScratch_TablePtr` | q9kernel_entry.a; C: Q9K_SSVC_SCRATCH_TABLEPTR | Q9_u32, (a1) EIN |
| `$001350` | 4 | `Q9K_SSvcScratch_DataPtr` | q9kernel_entry.a; C: Q9K_SSVC_SCRATCH_DATAPTR | Q9_u32, (a3) EIN |
| `$001358` | 4 | `Q9K_TrapCCRScratch` | q9kernel_entry.a |  |
| `$001360` | 1 | `Q9K_IOpenScratch_Mode` | q9kernel_entry.a; C: Q9K_IOpenScratch_Mode | Q9_u32, d0.b EIN (nur unteres Byte real genutzt) |
| `$001364` | 4 | `Q9K_IOpenScratch_NamePtr` | q9kernel_entry.a; C: Q9K_IOpenScratch_NamePtr | Q9_u32, (a0) EIN |
| `$001368` | 4 | `Q9K_IOpenScratch_PastName` | q9kernel_entry.a; C: Q9K_IOpenScratch_PastName | Q9_u32, (a0) AUS |
| `$00136C` | 2 | `Q9K_IOpenScratch_PathNum` | q9kernel_entry.a; C: Q9K_IOpenScratch_PathNum | Q9_u32, d0.w AUS (0 = Fehlschlag) |
| `$001370` | 2 | `Q9K_DispatchCallCode` (gemeinsam, s. shared) | q9kernel_entry.a | Q9_u16, Callcode des laufenden Traps: schreibt Q9K_TrapDispatch bei JE |
| `$001371` | 1 | `Q9DBG_A_DISPATCH_CODE_LOW` | q9kernel_dbg.c | 1 Byte, unteres Byte von Q9K_DispatchCallCode ($1370) |
| `$001374` | 2 | `Q9K_IOpenScratch_Error` | q9kernel_entry.a; C: Q9K_IOpenScratch_Error | Q9_u32, d1.w AUS bei Fehler |
| `$001380` | 2 | `Q9K_GProcPScratch_PID` | q9kernel_entry.a; C: Q9K_GPROCP_SCRATCH_PID | d0.w EIN |
| `$001384` | 4 | `Q9K_GProcPScratch_Desc` | q9kernel_entry.a; C: Q9K_GPROCP_SCRATCH_DESC | (a1) AUS |
| `$001388` | 2 | `Q9K_GProcPScratch_Error` | q9kernel_entry.a; C: Q9K_GPROCP_SCRATCH_ERROR | d1.w AUS bei Fehler |
| `$00138C` | 4 | `Q9K_GProcPScratch_Success` | q9kernel_entry.a; C: Q9K_GPROCP_SCRATCH_SUCCESS | 0/1 |
| `$001390` | 2 | `Q9K_IDScratch_PID` | q9kernel_entry.a; C: Q9K_ID_SCRATCH_PID | d0.w AUS |
| `$001394` | 4 | `Q9K_IDScratch_GroupUser` | q9kernel_entry.a; C: Q9K_ID_SCRATCH_GROUPUSER | d1.l AUS |
| `$001398` | 2 | `Q9K_IDScratch_Priority` | q9kernel_entry.a; C: Q9K_ID_SCRATCH_PRIORITY | d2.w AUS |
| `$00139C` | 2 | `Q9K_IDScratch_Error` | q9kernel_entry.a; C: Q9K_ID_SCRATCH_ERROR | d1.w AUS bei Fehler |
| `$0013A0` | 4 | `Q9K_IDScratch_Success` | q9kernel_entry.a; C: Q9K_ID_SCRATCH_SUCCESS | 0/1 |
| `$0013A4` | 4 | `Q9K_SRqMemFrameScratch` | q9kernel_entry.a | Q9_u32, Rahmenbasis oder 0 |
| `$0013A8` | 4 | `Q9K_TrapA4Save` | q9kernel_entry.a | A4-Zwischenablage im Trap-Dispatcher |
| `$0013AC` | 4 | `Q9K_AllPDScratch_DbtIn` | q9kernel_entry.a; C: Q9K_ALLPD_SCRATCH_DBTIN | Q9_u32, (a0) EIN  = DBT-Basis (F$AllPD) |
| `$0013B0` | 4 | `Q9K_AllPDScratch_Desc` | q9kernel_entry.a; C: Q9K_ALLPD_SCRATCH_DESC | Q9_u32, (a1) AUS = Deskriptorzeiger |
| `$0013B4` | 2 | `Q9K_AllPDScratch_Num` | q9kernel_entry.a; C: Q9K_ALLPD_SCRATCH_NUM | Q9_u32, d0.w AUS = Deskriptornummer |
| `$0013B8` | 2 | `Q9K_AllPDScratch_Error` | q9kernel_entry.a; C: Q9K_ALLPD_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$0013BC` | 4 | `Q9K_AllPDScratch_Success` | q9kernel_entry.a; C: Q9K_ALLPD_SCRATCH_SUCCESS | Q9_u32, 0 = Fehlschlag / 1 = Erfolg |
| `$0013C0` | 4 | `Q9K_AllPDFrameScratch` | q9kernel_entry.a | Rahmenbasis des Trampolin-Aufrufers (0 = kein Rahmen) |
| `$0013C4` | 4 | `Q9K_LinkFrameScratch` | q9kernel_entry.a | dito fuer F$Link |
| `$0013C8` | 4 | `Q9K_IRQScratch_Vector` | q9kernel_entry.a; C: Q9K_IRQ_SCRATCH_VECTOR | Q9_u32, d0.b EIN  = Vektornummer (F$IRQ) |
| `$0013CC` | 4 | `Q9K_IRQScratch_Prio` | q9kernel_entry.a; C: Q9K_IRQ_SCRATCH_PRIO | Q9_u32, d1.b EIN  = Prioritaet |
| `$0013D0` | 4 | `Q9K_IRQScratch_Isr` | q9kernel_entry.a; C: Q9K_IRQ_SCRATCH_ISR | Q9_u32, (a0) EIN  = Service-Routine (0 = entfernen) |
| `$0013D4` | 4 | `Q9K_IRQScratch_Static` | q9kernel_entry.a; C: Q9K_IRQ_SCRATCH_STATIC | Q9_u32, (a2) EIN  = statischer Treiberspeicher |
| `$0013D8` | 4 | `Q9K_IRQScratch_Port` | q9kernel_entry.a; C: Q9K_IRQ_SCRATCH_PORT | Q9_u32, (a3) EIN  = Geraete-Portadresse |
| `$0013DC` | 4 | `Q9K_IRQScratch_Error` | q9kernel_entry.a; C: Q9K_IRQ_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$0013E0` | 4 | `Q9K_IRQScratch_Success` | q9kernel_entry.a; C: Q9K_IRQ_SCRATCH_SUCCESS | Q9_u32, 0 = Fehlschlag / 1 = Erfolg |
| `$0013E4` | 4 | `Q9K_InTrapPath` | q9kernel_entry.a | 1 = Handler laeuft ueber Q9K_TrapDispatch, 0 = Trampolin |
| `$0013E8` | 4 | `Q9K_TrapCallerPC` | q9kernel_entry.a | echte Ruecksprung-PC des Aufrufers (fuer Herkunftspruefung) |
| `$0013EC` | 4 | `Q9K_PrsNamScratch_Path` | q9kernel_entry.a; C: Q9K_PRSNAM_SCRATCH_PATH | Q9_u32, (a0) EIN  = Pfadname (F$PrsNam) |
| `$0013F0` | 4 | `Q9K_PrsNamScratch_Name` | q9kernel_entry.a; C: Q9K_PRSNAM_SCRATCH_NAME | Q9_u32, (a1) AUS = Namensanfang |
| `$0013F4` | 4 | `Q9K_PrsNamScratch_Past` | q9kernel_entry.a; C: Q9K_PRSNAM_SCRATCH_PAST | Q9_u32, (a0) AUS = hinter dem Namen |
| `$0013F8` | 2 | `Q9K_PrsNamScratch_Len` | q9kernel_entry.a; C: Q9K_PRSNAM_SCRATCH_LEN | Q9_u32, d1.w AUS = Namenslaenge |
| `$0013FC` | 1 | `Q9K_PrsNamScratch_Delim` | q9kernel_entry.a; C: Q9K_PRSNAM_SCRATCH_DELIM | Q9_u32, d0.b AUS = Trennzeichen |
| `$001400` | 256 | `Tabelle:ssvcext` | q9_scratch_tables.txt; C: Q9K_SSvcExternalBase | q9kernel_ssvc.c: 256-Byte-Markierungstabelle (Q9K_SSVC_EXTERNAL_BASE) |
| `$001500` | 240 | `Tabelle:irqtab` | q9_scratch_tables.txt; C: Q9K_IRQTabBase | q9kernel_exctable.c: 12 x 20 Byte, endet bei $15F0 |
| `$001600` | 2 | `Q9K_PrsNamScratch_Error` | q9kernel_entry.a; C: Q9K_PRSNAM_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$001604` | 4 | `Q9K_PrsNamScratch_Ok` | q9kernel_entry.a; C: Q9K_PRSNAM_SCRATCH_OK | Q9_u32, 0 = Fehlschlag / 1 = Erfolg |
| `$001608` | 2 | `Q9K_SendScratch_Pid` | q9kernel_entry.a; C: Q9K_SEND_SCRATCH_PID | Q9_u32, d0.w EIN |
| `$00160C` | 2 | `Q9K_SendScratch_Signal` | q9kernel_entry.a; C: Q9K_SEND_SCRATCH_SIGNAL | Q9_u32, d1.w EIN |
| `$001610` | 2 | `Q9K_SendScratch_Error` | q9kernel_entry.a; C: Q9K_SEND_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$001614` | 4 | `Q9K_SendScratch_Success` | q9kernel_entry.a; C: Q9K_SEND_SCRATCH_SUCCESS | Q9_u32, 0 = Fehlschlag / 1 = Erfolg |
| `$001618` | 4 | `Q9K_SleepScratch_RetPC` | q9kernel_entry.a | Q9_u32, nur waehrend des Frame-Baus belegt |
| `$00161C` | 4 | `Q9K_RetPDScratch_DbtIn` | q9kernel_entry.a; C: Q9K_RETPD_SCRATCH_DBTIN | Q9_u32, (a0) EIN = DBT-Basis |
| `$001620` | 2 | `Q9K_RetPDScratch_NumIn` | q9kernel_entry.a; C: Q9K_RETPD_SCRATCH_NUMIN | Q9_u32, d0.w EIN = Deskriptornummer |
| `$001624` | 2 | `Q9K_RetPDScratch_Error` | q9kernel_entry.a; C: Q9K_RETPD_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$001628` | 4 | `Q9K_RetPDScratch_Success` | q9kernel_entry.a; C: Q9K_RETPD_SCRATCH_SUCCESS | Q9_u32, 0 = Fehlschlag / 1 = Erfolg |
| `$00162C` | 4 | `Q9K_VModulScratch_Hdr` | q9kernel_entry.a; C: Q9K_VMODUL_SCRATCH_HDR | Q9_u32, (a0) EIN = Modulzeiger |
| `$001630` | 4 | `Q9K_VModulScratch_Size` | q9kernel_entry.a | Q9_u32, d1.l EIN = Modulgroesse |
| `$001634` | 4 | `Q9K_VModulScratch_Entry` | q9kernel_entry.a; C: Q9K_VMODUL_SCRATCH_ENTRY | Q9_u32, (a2) AUS = Verzeichniseintrag |
| `$001638` | 2 | `Q9K_VModulScratch_Error` | q9kernel_entry.a; C: Q9K_VMODUL_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$00163C` | 4 | `Q9K_VModulScratch_Success` | q9kernel_entry.a; C: Q9K_VMODUL_SCRATCH_SUCCESS | Q9_u32, 0 = Fehlschlag / 1 = Erfolg |
| `$001640` | 4 | `Q9K_SRqCMemFrameScratch` | q9kernel_entry.a | Q9_u32, Rahmenbasis oder 0 |
| `$001644` | 4 | `Q9K_SetSysScratch_VarCode` | q9kernel_entry.a; C: Q9K_SetSysScratch_VarCode | Q9_u32, d0.l EIN |
| `$001648` | 4 | `Q9K_SetSysScratch_Flags` | q9kernel_entry.a; C: Q9K_SetSysScratch_Flags | Q9_u32, d1.l EIN |
| `$00164C` | 4 | `Q9K_SetSysScratch_Value` | q9kernel_entry.a; C: Q9K_SetSysScratch_Value | Q9_u32, d2.l EIN (Schreiben)/AUS (Lesen) |
| `$001650` | 4 | `Q9K_VMODUL_RETBUF` | q9kernel_moddir.c |  |
| `$001664` | 2 | `Q9K_SetSysScratch_Error` | q9kernel_entry.a; C: Q9K_SetSysScratch_Error | Q9_u32, d1.w AUS bei Fehlschlag |
| `$001668` | 4 | `Q9K_SetSysScratch_Success` | q9kernel_entry.a; C: Q9K_SetSysScratch_Success | Q9_u32, 0 = Fehlschlag / 1 = Erfolg |
| `$001690` | 2 | `Q9K_TLinkScratch_TrapNum` | q9kernel_entry.a; C: Q9K_TLINK_SCRATCH_TRAPNUM | Q9_u32, d0.w EIN |
| `$001694` | 4 | `Q9K_TLinkScratch_MemOvr` | q9kernel_entry.a; C: Q9K_TLINK_SCRATCH_MEMOVR | Q9_u32, d1.l EIN |
| `$001698` | 4 | `Q9K_TLinkScratch_NamePtr` | q9kernel_entry.a; C: Q9K_TLINK_SCRATCH_NAMEPTR | Q9_u32, (a0) EIN |
| `$00169C` | 4 | `Q9K_TLinkScratch_PastName` | q9kernel_entry.a; C: Q9K_TLINK_SCRATCH_PASTNAME | Q9_u32, (a0) AUS |
| `$0016A0` | 4 | `Q9K_TLinkScratch_ModPtr` | q9kernel_entry.a; C: Q9K_TLINK_SCRATCH_MODPTR | Q9_u32, (a2) AUS |
| `$0016A4` | 4 | `Q9K_TLinkScratch_ExecEntry` | q9kernel_entry.a; C: Q9K_TLINK_SCRATCH_EXECENTRY | Q9_u32, (a1) AUS |
| `$0016A8` | 4 | `Q9K_TLinkScratch_InitEntry` | q9kernel_entry.a; C: Q9K_TLINK_SCRATCH_INITENTRY | Q9_u32, AUS -- nur fuer den M$Init-Sprung |
| `$0016AC` | 4 | `Q9K_TLinkScratch_StaticPtr` | q9kernel_entry.a; C: Q9K_TLINK_SCRATCH_STATICPTR | Q9_u32, AUS -- wird a6 beim M$Init-Aufruf |
| `$0016B0` | 2 | `Q9K_TLinkScratch_Error` | q9kernel_entry.a; C: Q9K_TLINK_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$0016B4` | 4 | `Q9K_TLinkScratch_Success` | q9kernel_entry.a; C: Q9K_TLINK_SCRATCH_SUCCESS | Q9_u32, 0 = Fehlschlag / 1 = Erfolg |
| `$0016C0` | 4 | `Q9K_MemTraceEnabled` | q9kernel_entry.a; C: Q9K_MEMTRACE_ENABLED | Q9_u32, 0=disabled, 1=enabled |
| `$0016C4` | 4 | `Q9K_MemTraceOp` | q9kernel_entry.a; C: Q9K_MEMTRACE_OP | Q9_u32, 1=request, 2=return, 3=alloc, 4=free, 5=largest, 6=process rel |
| `$0016C8` | 4 | `Q9K_MemTraceRequest` | q9kernel_entry.a; C: Q9K_MEMTRACE_REQUEST | Q9_u32, requested size |
| `$0016CC` | 4 | `Q9K_MemTraceAddress` | q9kernel_entry.a; C: Q9K_MEMTRACE_ADDRESS | Q9_u32, allocated/returned address |
| `$0016D0` | 4 | `Q9K_MemTraceSize` | q9kernel_entry.a | Q9_u32, granted/returned size |
| `$0016D4` | 4 | `Q9K_MemTraceError` | q9kernel_entry.a; C: Q9K_MEMTRACE_ERROR | Q9_u32, error code or zero |
| `$0016D8` | 4 | `Q9K_MemTraceHead` | q9kernel_entry.a; C: Q9K_MEMTRACE_HEAD | Q9_u32, free-list head after event |
| `$0016DC` | 4 | `Q9K_MemTraceName` | q9kernel_entry.a; C: Q9K_MEMTRACE_NAME | Q9_u32, current process module name |
| `$0016E0` | 4 | `Q9K_MemTraceContextHdr` | q9kernel_entry.a; C: Q9K_MEMTRACE_CONTEXT_HDR | Q9_u32, temporary module context |
| `$0016E4` | 4 | `Q9K_MEMTRACE_CAPTURE_OP` | q9kernel_debug.c |  |
| `$0016E8` | 4 | `Q9K_MEMTRACE_CAPTURE_ADDR` | q9kernel_debug.c |  |
| `$0016F0` | 4 | `Q9K_MEMTRACE_CAPTURE_ERROR` | q9kernel_debug.c |  |
| `$0016F4` | 4 | `Q9K_MEMTRACE_CAPTURED` | q9kernel_debug.c |  |
| `$001710` | 384 | `Tabelle:memowner` | q9_scratch_tables.txt; C: Q9K_MEMOWNER_BASE | q9kernel_sysmem.c: 32 x 12 Byte |
| `$0018A0` | 2 | `Q9K_SPriorScratch_PID` | q9kernel_entry.a; C: Q9K_SPRIOR_SCRATCH_PID | Q9_u32, d0.w EIN |
| `$0018A4` | 2 | `Q9K_SPriorScratch_Priority` | q9kernel_entry.a; C: Q9K_SPRIOR_SCRATCH_PRIORITY | Q9_u32, d1.w EIN |
| `$0018A8` | 2 | `Q9K_SPriorScratch_Error` | q9kernel_entry.a; C: Q9K_SPRIOR_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$0018AC` | 4 | `Q9K_SPriorScratch_Success` | q9kernel_entry.a; C: Q9K_SPRIOR_SCRATCH_SUCCESS | Q9_u32, 0/1 |
| `$0018B0` | 4 | `Q9K_CmpNamScratch_Pattern` | q9kernel_entry.a; C: Q9K_CMPNAM_SCRATCH_PATTERN | Q9_u32, (a0) EIN = Muster |
| `$0018B4` | 4 | `Q9K_CmpNamScratch_Target` | q9kernel_entry.a; C: Q9K_CMPNAM_SCRATCH_TARGET | Q9_u32, (a1) EIN = Zielname |
| `$0018B8` | 2 | `Q9K_CmpNamScratch_PatLen` | q9kernel_entry.a; C: Q9K_CMPNAM_SCRATCH_PATLEN | Q9_u32, d1.w EIN = Musterlaenge |
| `$0018BC` | 2 | `Q9K_CmpNamScratch_Error` | q9kernel_entry.a; C: Q9K_CMPNAM_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$0018C0` | 4 | `Q9K_CmpNamScratch_Success` | q9kernel_entry.a; C: Q9K_CMPNAM_SCRATCH_SUCCESS | Q9_u32, 0 = ungleich / 1 = gleich |
| `$0018C4` | 4 | `Q9K_SUserScratch_GroupUser` | q9kernel_entry.a; C: Q9K_SUSER_SCRATCH_GROUPUSER | Q9_u32, d1.l EIN |
| `$0018C8` | 2 | `Q9K_SUserScratch_Error` | q9kernel_entry.a; C: Q9K_SUSER_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$0018CC` | 4 | `Q9K_SUserScratch_Success` | q9kernel_entry.a; C: Q9K_SUSER_SCRATCH_SUCCESS | Q9_u32, 0/1 |
| `$0018D0` | 2 | `Q9K_CpyMemScratch_PID` | q9kernel_entry.a; C: Q9K_CPYMEM_SCRATCH_PID | Q9_u32, d0.w EIN |
| `$0018D4` | 4 | `Q9K_CpyMemScratch_Count` | q9kernel_entry.a; C: Q9K_CPYMEM_SCRATCH_COUNT | Q9_u32, d1.l EIN |
| `$0018D8` | 4 | `Q9K_CpyMemScratch_Src` | q9kernel_entry.a; C: Q9K_CPYMEM_SCRATCH_SRC | Q9_u32, (a0) EIN |
| `$0018DC` | 4 | `Q9K_CpyMemScratch_Dst` | q9kernel_entry.a; C: Q9K_CPYMEM_SCRATCH_DST | Q9_u32, (a1) EIN |
| `$0018E0` | 2 | `Q9K_CpyMemScratch_Error` | q9kernel_entry.a; C: Q9K_CPYMEM_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$0018E4` | 4 | `Q9K_CpyMemScratch_Success` | q9kernel_entry.a; C: Q9K_CPYMEM_SCRATCH_SUCCESS | Q9_u32, 0/1 |
| `$0018E8` | 2 | `Q9K_UnloadScratch_TyLang` | q9kernel_entry.a; C: Q9K_UNLOAD_SCRATCH_TYLANG | Q9_u32, d0.w EIN |
| `$0018EC` | 4 | `Q9K_UnloadScratch_Name` | q9kernel_entry.a; C: Q9K_UNLOAD_SCRATCH_NAME | Q9_u32, (a0) EIN |
| `$0018F0` | 2 | `Q9K_UnloadScratch_Error` | q9kernel_entry.a; C: Q9K_UNLOAD_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$0018F4` | 4 | `Q9K_UnloadScratch_Success` | q9kernel_entry.a; C: Q9K_UNLOAD_SCRATCH_SUCCESS | Q9_u32, 0/1 |
| `$0018F8` | 4 | `Q9K_CrcScratch_Count` | q9kernel_entry.a; C: Q9K_CRC_SCRATCH_COUNT | Q9_u32, d0.l EIN |
| `$0018FC` | 4 | `Q9K_CrcScratch_Accum` | q9kernel_entry.a; C: Q9K_CRC_SCRATCH_ACCUM | Q9_u32, d1.l EIN/AUS |
| `$001900` | 4 | `Q9K_CrcScratch_Addr` | q9kernel_entry.a; C: Q9K_CRC_SCRATCH_ADDR | Q9_u32, (a0) EIN |
| `$001904` | 4 | `Q9K_SetCRCScratch_Addr` | q9kernel_entry.a; C: Q9K_SETCRC_SCRATCH_ADDR | Q9_u32, (a0) EIN |
| `$001908` | 2 | `Q9K_SetCRCScratch_Error` | q9kernel_entry.a; C: Q9K_SETCRC_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$00190C` | 4 | `Q9K_SetCRCScratch_Success` | q9kernel_entry.a; C: Q9K_SETCRC_SCRATCH_SUCCESS | Q9_u32, 0/1 |
| `$001910` | 4 | `Q9K_GModDrScratch_Buf` | q9kernel_entry.a; C: Q9K_GMODDR_SCRATCH_BUF | Q9_u32, (a0) EIN |
| `$001914` | 4 | `Q9K_GModDrScratch_Count` | q9kernel_entry.a; C: Q9K_GMODDR_SCRATCH_COUNT | Q9_u32, d1.l EIN/AUS |
| `$001918` | 2 | `Q9K_GPrDscScratch_PID` | q9kernel_entry.a; C: Q9K_GPRDSC_SCRATCH_PID | Q9_u32, d0.w EIN |
| `$00191C` | 2 | `Q9K_GPrDscScratch_Count` | q9kernel_entry.a; C: Q9K_GPRDSC_SCRATCH_COUNT | Q9_u32, d1.w EIN |
| `$001920` | 4 | `Q9K_GPrDscScratch_Buf` | q9kernel_entry.a; C: Q9K_GPRDSC_SCRATCH_BUF | Q9_u32, (a0) EIN |
| `$001924` | 2 | `Q9K_GPrDscScratch_Error` | q9kernel_entry.a; C: Q9K_GPRDSC_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$001928` | 4 | `Q9K_GPrDscScratch_Success` | q9kernel_entry.a; C: Q9K_GPRDSC_SCRATCH_SUCCESS | Q9_u32, 0/1 |
| `$001930` | 4 | `Q9K_DateScratch_Time` | q9kernel_entry.a; C: Q9K_DATE_SCRATCH_TIME | Q9_u32, d0.l EIN/AUS |
| `$001934` | 4 | `Q9K_DateScratch_Date` | q9kernel_entry.a; C: Q9K_DATE_SCRATCH_DATE | Q9_u32, d1.l EIN/AUS |
| `$001938` | 2 | `Q9K_DateScratch_Error` | q9kernel_entry.a; C: Q9K_DATE_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$00193C` | 4 | `Q9K_DateScratch_Success` | q9kernel_entry.a; C: Q9K_DATE_SCRATCH_SUCCESS | Q9_u32, 0/1 |
| `$001940` | 4 | `Q9K_AProcScratch_Desc` | q9kernel_entry.a; C: Q9K_APROC_SCRATCH_DESC | Q9_u32, (a0) EIN |
| `$001944` | 2 | `Q9K_AProcScratch_Error` | q9kernel_entry.a; C: Q9K_APROC_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$001948` | 4 | `Q9K_AProcScratch_Success` | q9kernel_entry.a; C: Q9K_APROC_SCRATCH_SUCCESS | Q9_u32, 0/1 |
| `$00194C` | 4 | `Q9K_GPrDBTScratch_Buf` | q9kernel_entry.a; C: Q9K_GPRDBT_SCRATCH_BUF | Q9_u32, (a0) EIN |
| `$001950` | 4 | `Q9K_GPrDBTScratch_Count` | q9kernel_entry.a; C: Q9K_GPRDBT_SCRATCH_COUNT | Q9_u32, d1.l EIN/AUS |
| `$001954` | 4 | `Q9K_AllPrcScratch_Desc` | q9kernel_entry.a; C: Q9K_ALLPRC_SCRATCH_DESC | Q9_u32, (a2) AUS |
| `$001958` | 2 | `Q9K_AllPrcScratch_Error` | q9kernel_entry.a; C: Q9K_ALLPRC_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$00195C` | 4 | `Q9K_AllPrcScratch_Success` | q9kernel_entry.a; C: Q9K_ALLPRC_SCRATCH_SUCCESS | Q9_u32, 0/1 |
| `$001960` | 2 | `Q9K_DelPrcScratch_PID` | q9kernel_entry.a; C: Q9K_DELPRC_SCRATCH_PID | Q9_u32, d0.w EIN |
| `$001964` | 2 | `Q9K_DelPrcScratch_Error` | q9kernel_entry.a; C: Q9K_DELPRC_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$001968` | 4 | `Q9K_DelPrcScratch_Success` | q9kernel_entry.a; C: Q9K_DELPRC_SCRATCH_SUCCESS | Q9_u32, 0/1 |
| `$00196C` | 4 | `Q9K_AProcScratch_Preempt` | q9kernel_entry.a; C: Q9K_APROC_SCRATCH_PREEMPT | Q9_u32, 1 = sofortiger Wechsel |
| `$001970` | 2 | `Q9K_FindPDScratch_Num` | q9kernel_entry.a; C: Q9K_FINDPD_SCRATCH_NUM | Q9_u32, d0.w EIN |
| `$001974` | 4 | `Q9K_FindPDScratch_Dbt` | q9kernel_entry.a; C: Q9K_FINDPD_SCRATCH_DBT | Q9_u32, (a0) EIN |
| `$001978` | 4 | `Q9K_FindPDScratch_Desc` | q9kernel_entry.a; C: Q9K_FINDPD_SCRATCH_DESC | Q9_u32, (a1) AUS |
| `$00197C` | 2 | `Q9K_FindPDScratch_Error` | q9kernel_entry.a; C: Q9K_FINDPD_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$001980` | 4 | `Q9K_FindPDScratch_Success` | q9kernel_entry.a; C: Q9K_FINDPD_SCRATCH_SUCCESS | Q9_u32, 0/1 |
| `$001988` | 4 | `Q9K_SigMaskScratch_Level` | q9kernel_entry.a; C: Q9K_SIGMASK_SCRATCH_LEVEL | Q9_u32, d1.l EIN |
| `$00198C` | 2 | `Q9K_SigMaskScratch_Error` | q9kernel_entry.a; C: Q9K_SIGMASK_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$001990` | 4 | `Q9K_SigMaskScratch_Success` | q9kernel_entry.a; C: Q9K_SIGMASK_SCRATCH_SUCCESS | Q9_u32, 0/1 |
| `$001998` | 4 | `Q9K_IcptScratch_Vec` | q9kernel_entry.a; C: Q9K_ICPT_SCRATCH_VEC | Q9_u32, (a0) EIN |
| `$00199C` | 4 | `Q9K_IcptScratch_Dat` | q9kernel_entry.a; C: Q9K_ICPT_SCRATCH_DAT | Q9_u32, (a6) EIN |
| `$0019A0` | 4 | `Q9K_IcptScratch_Pending` | q9kernel_entry.a; C: Q9K_ICPT_SCRATCH_PENDING | Q9_u32, d0.l AUS |
| `$0019A8` | 4 | `Q9K_TransScratch_Size` | q9kernel_entry.a | Q9_u32, d0.l EIN/AUS |
| `$0019AC` | 4 | `Q9K_TransScratch_Mode` | q9kernel_entry.a; C: Q9K_TRANS_SCRATCH_MODE | Q9_u32, d1.l EIN / Erfolgskennzeichen AUS |
| `$0019B0` | 4 | `Q9K_TransScratch_Addr` | q9kernel_entry.a; C: Q9K_TRANS_SCRATCH_ADDR | Q9_u32, (a0) EIN/AUS |
| `$0019B8` | 4 | `Q9K_DatModScratch_Size` | q9kernel_entry.a | Q9_u32, d0.l EIN / Typ+Sprache AUS |
| `$0019BC` | 2 | `Q9K_DatModScratch_AttRev` | q9kernel_entry.a; C: Q9K_DATMOD_SCRATCH_ATTREV | Q9_u32, d1.w EIN/AUS |
| `$0019C0` | 2 | `Q9K_DatModScratch_Access` | q9kernel_entry.a; C: Q9K_DATMOD_SCRATCH_ACCESS | Q9_u32, d2.w EIN |
| `$0019C4` | 2 | `Q9K_DatModScratch_TyLang` | q9kernel_entry.a; C: Q9K_DATMOD_SCRATCH_TYLANG | Q9_u32, d3.w EIN |
| `$0019C8` | 4 | `Q9K_DatModScratch_Name` | q9kernel_entry.a; C: Q9K_DATMOD_SCRATCH_NAME | Q9_u32, (a0) EIN/AUS |
| `$0019CC` | 4 | `Q9K_DatModScratch_Data` | q9kernel_entry.a; C: Q9K_DATMOD_SCRATCH_DATA | Q9_u32, (a1) AUS |
| `$0019D0` | 4 | `Q9K_DatModScratch_Hdr` | q9kernel_entry.a; C: Q9K_DATMOD_SCRATCH_HDR | Q9_u32, (a2) AUS |
| `$0019D4` | 2 | `Q9K_DatModScratch_Error` | q9kernel_entry.a; C: Q9K_DATMOD_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$0019D8` | 4 | `Q9K_DatModScratch_Success` | q9kernel_entry.a; C: Q9K_DATMOD_SCRATCH_SUCCESS | Q9_u32, 0/1 |
| `$0019E0` | 4 | `Q9K_SCHED_MINPTY_ADDR` | q9kernel_sched.c |  |
| `$001A00` | 228 | `Tabelle:alarm` | q9_scratch_tables.txt; C: Q9K_ALARM_BASE | q9kernel_alarm.c: 8 x 28 Byte + 4 Byte NEXTID direkt dahinter |
| `$001B00` | 2 | `Q9K_AlarmScratch_Func` | q9kernel_entry.a; C: Q9K_ALARM_SCRATCH_FUNC | Q9_u32, d1.w EIN = Funktionscode |
| `$001B04` | 4 | `Q9K_AlarmScratch_IdIn` | q9kernel_entry.a; C: Q9K_ALARM_SCRATCH_IDIN | Q9_u32, d0.l EIN / d0.l AUS = Alarm-ID |
| `$001B08` | 2 | `Q9K_AlarmScratch_Signal` | q9kernel_entry.a; C: Q9K_ALARM_SCRATCH_SIGNAL | Q9_u32, d2.w EIN |
| `$001B0C` | 4 | `Q9K_AlarmScratch_Ticks` | q9kernel_entry.a; C: Q9K_ALARM_SCRATCH_TICKS | Q9_u32, d3.l EIN |
| `$001B10` | 2 | `Q9K_AlarmScratch_Error` | q9kernel_entry.a; C: Q9K_ALARM_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$001B14` | 4 | `Q9K_AlarmScratch_Success` | q9kernel_entry.a; C: Q9K_ALARM_SCRATCH_SUCCESS | Q9_u32, 0/1 |
| `$001B18` | 4 | `Q9K_AlarmScratch_Date` | q9kernel_entry.a; C: Q9K_ALARM_SCRATCH_DATE | Q9_u32, d4.l EIN = Datum/Tageszahl |
| `$001B20` | 4 | `Q9K_ClockDay` | q9kernel_entry.a; C: Q9K_CLOCK_DAY | Q9_u32, julianische Tageszahl |
| `$001B24` | 4 | `Q9K_ClockSec` | q9kernel_entry.a; C: Q9K_CLOCK_SEC | Q9_u32, Sekunden seit Mitternacht |
| `$001B28` | 4 | `Q9K_ClockTickCell` | q9kernel_entry.a; C: Q9K_CLOCK_TICK | Q9_u32, Ticks innerhalb der Sekunde |
| `$001B2C` | 4 | `Q9K_ClockValid` | q9kernel_entry.a; C: Q9K_CLOCK_VALID | Q9_u32, 0 = nie gestellt |
| `$001B30` | 4 | `Q9K_STimeScratch_Time` | q9kernel_entry.a; C: Q9K_STIME_SCRATCH_TIME | Q9_u32, d0.l EIN (F$STime) / AUS (F$Time) |
| `$001B34` | 4 | `Q9K_STimeScratch_Date` | q9kernel_entry.a; C: Q9K_STIME_SCRATCH_DATE | Q9_u32, d1.l EIN (F$STime) / AUS (F$Time) |
| `$001B38` | 2 | `Q9K_STimeScratch_Error` | q9kernel_entry.a; C: Q9K_STIME_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$001B3C` | 4 | `Q9K_STimeScratch_Success` | q9kernel_entry.a; C: Q9K_STIME_SCRATCH_SUCCESS | Q9_u32, 0/1 |
| `$001B40` | 2 | `Q9K_BitmapScratch_Start` | q9kernel_entry.a; C: Q9K_BITMAP_SCRATCH_START | Q9_u32, d0.w EIN / AUS |
| `$001B44` | 2 | `Q9K_BitmapScratch_Count` | q9kernel_entry.a; C: Q9K_BITMAP_SCRATCH_COUNT | Q9_u32, d1.w EIN / AUS |
| `$001B48` | 4 | `Q9K_BitmapScratch_Base` | q9kernel_entry.a; C: Q9K_BITMAP_SCRATCH_BASE | Q9_u32, (a0) EIN |
| `$001B4C` | 4 | `Q9K_BitmapScratch_End` | q9kernel_entry.a; C: Q9K_BITMAP_SCRATCH_END | Q9_u32, (a1) EIN, nur F$SchBit |
| `$001B50` | 2 | `Q9K_BitmapScratch_Error` | q9kernel_entry.a; C: Q9K_BITMAP_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$001B54` | 4 | `Q9K_BitmapScratch_Ok` | q9kernel_entry.a; C: Q9K_BITMAP_SCRATCH_OK | Q9_u32, 0/1 |
| `$001B58` | 4 | `Q9K_RamSize` | q9kernel_entry.a; C: Q9K_RAMSIZE_ADDR | Q9_u32, Gesamt-RAM, beim Boot gesetzt |
| `$001B5C` | 4 | `Q9K_BlkMpScratch_Begin` | q9kernel_entry.a; C: Q9K_BLKMP_SCRATCH_BEGIN | Q9_u32, d0.l EIN = ab welcher Adresse berichtet wird |
| `$001B60` | 4 | `Q9K_BlkMpScratch_BufSize` | q9kernel_entry.a; C: Q9K_BLKMP_SCRATCH_BUFSIZE | Q9_u32, d1.l EIN = Puffergroesse |
| `$001B64` | 4 | `Q9K_BlkMpScratch_Buf` | q9kernel_entry.a; C: Q9K_BLKMP_SCRATCH_BUF | Q9_u32, (a0) EIN = Puffer |
| `$001B68` | 4 | `Q9K_BlkMpScratch_MinAlloc` | q9kernel_entry.a; C: Q9K_BLKMP_SCRATCH_MINALLOC | Q9_u32, d0.l AUS = kleinste Zuteilungsgroesse |
| `$001B6C` | 4 | `Q9K_BlkMpScratch_Frags` | q9kernel_entry.a; C: Q9K_BLKMP_SCRATCH_FRAGS | Q9_u32, d1.l AUS = Zahl der Fragmente |
| `$001B70` | 4 | `Q9K_BlkMpScratch_TotalRam` | q9kernel_entry.a; C: Q9K_BLKMP_SCRATCH_TOTALRAM | Q9_u32, d2.l AUS = Gesamt-RAM |
| `$001B74` | 4 | `Q9K_BlkMpScratch_FreeRam` | q9kernel_entry.a; C: Q9K_BLKMP_SCRATCH_FREERAM | Q9_u32, d3.l AUS = freier Speicher |
| `$001B78` | 2 | `Q9K_BlkMpScratch_Error` | q9kernel_entry.a; C: Q9K_BLKMP_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$001B7C` | 4 | `Q9K_BlkMpScratch_Ok` | q9kernel_entry.a; C: Q9K_BLKMP_SCRATCH_OK | Q9_u32, 0/1 |
| `$001B80` | 4 | `Q9K_SemaScratch_Ptr` | q9kernel_entry.a; C: Q9K_SEMA_SCRATCH_PTR | Q9_u32, d0.l EIN = Semaphorzeiger |
| `$001B84` | 2 | `Q9K_SemaScratch_Op` | q9kernel_entry.a; C: Q9K_SEMA_SCRATCH_OP | Q9_u32, d1.w EIN = 1 (P) / 2 (V) |
| `$001B88` | 2 | `Q9K_SemaScratch_Error` | q9kernel_entry.a; C: Q9K_SEMA_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$001B8C` | 4 | `Q9K_SemaScratch_Ok` | q9kernel_entry.a; C: Q9K_SEMA_SCRATCH_OK | Q9_u32, 0/1 |
| `$001B90` | 4 | `Q9K_SemaScratch_Sleep` | q9kernel_entry.a; C: Q9K_SEMA_SCRATCH_SLEEP | Q9_u32, 1 = Aufrufer muss warten |
| `$001B94` | 4 | `Q9K_SemaScratch_Next` | q9kernel_entry.a; C: Q9K_SEMA_SCRATCH_NEXT | Q9_u32, naechster Prozess |
| `$001BA0` | 2 | `Q9K_ChainScratch_TypeLang` | q9kernel_entry.a; C: Q9K_CHAIN_SCRATCH_TYPELANG | Q9_u32, d0.w EIN |
| `$001BA4` | 4 | `Q9K_ChainScratch_AddMem` | q9kernel_entry.a; C: Q9K_CHAIN_SCRATCH_ADDMEM | Q9_u32, d1.l EIN |
| `$001BA8` | 4 | `Q9K_ChainScratch_ParamSz` | q9kernel_entry.a; C: Q9K_CHAIN_SCRATCH_PARAMSZ | Q9_u32, d2.l EIN |
| `$001BAC` | 2 | `Q9K_ChainScratch_NumPaths` | q9kernel_entry.a; C: Q9K_CHAIN_SCRATCH_NUMPATHS | Q9_u32, d3.w EIN |
| `$001BB0` | 2 | `Q9K_ChainScratch_Priority` | q9kernel_entry.a; C: Q9K_CHAIN_SCRATCH_PRIORITY | Q9_u32, d4.w EIN |
| `$001BB4` | 4 | `Q9K_ChainScratch_Name` | q9kernel_entry.a; C: Q9K_CHAIN_SCRATCH_NAME | Q9_u32, (a0) EIN |
| `$001BB8` | 4 | `Q9K_ChainScratch_Param` | q9kernel_entry.a; C: Q9K_CHAIN_SCRATCH_PARAM | Q9_u32, (a1) EIN |
| `$001BBC` | 2 | `Q9K_ChainScratch_Error` | q9kernel_entry.a; C: Q9K_CHAIN_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$001BC0` | 4 | `Q9K_ChainScratch_Ok` | q9kernel_entry.a; C: Q9K_CHAIN_SCRATCH_OK | Q9_u32, 0/1 |
| `$001BC4` | 4 | `Q9K_ChainScratch_OldBlock` | q9kernel_entry.a; C: Q9K_CHAIN_SCRATCH_OLDBLOCK | Q9_u32, alter Block |
| `$001BC8` | 4 | `Q9K_ChainScratch_OldSize` | q9kernel_entry.a; C: Q9K_CHAIN_SCRATCH_OLDSIZE | Q9_u32, dessen Groesse |
| `$001BCC` | 4 | `Q9K_ChainScratch_OldMod` | q9kernel_entry.a; C: Q9K_CHAIN_SCRATCH_OLDMOD | Q9_u32, altes Primaermodul |
| `$001BD0` | 4 | `Q9K_IcptScratch_Ok` | q9kernel_entry.a; C: Q9K_ICPT_SCRATCH_OK | Q9_u32, 0/1 |
| `$001BD4` | 2 | `Q9K_IcptScratch_Error` | q9kernel_entry.a; C: Q9K_ICPT_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$001BD8` | 4 | `Q9K_IcptScratch_Depth` | q9kernel_entry.a | Q9_u32, verbliebene Verschachtelungstiefe |
| `$001BE0` | 4 | `Q9K_TickCount` | q9kernel_entry.a; C: Q9DBG_A_TICKS, Q9K_SETSYS_TICKCOUNT_ADDR | Q9_u32, Timer-Interrupts seit dem Boot |
| `$001BE4` | 4 | `Q9K_StrapScratch_Stack` | q9kernel_entry.a; C: Q9K_STRAP_SCRATCH_STACK | Q9_u32, (a0) EIN |
| `$001BE8` | 4 | `Q9K_StrapScratch_Table` | q9kernel_entry.a; C: Q9K_STRAP_SCRATCH_TABLE | Q9_u32, (a1) EIN |
| `$001BEC` | 2 | `Q9K_StrapScratch_Error` | q9kernel_entry.a; C: Q9K_STRAP_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$001BF0` | 4 | `Q9K_StrapScratch_Ok` | q9kernel_entry.a; C: Q9K_STRAP_SCRATCH_OK | Q9_u32, 0/1 |
| `$001BF4` | 4 | `Q9K_StrapScratch_Count` | q9kernel_entry.a; C: Q9K_STRAP_SCRATCH_COUNT | Q9_u32, eingetragene Behandler |
| `$001BF8` | 4 | `Q9K_StrapScratch_Vec` | q9kernel_entry.a; C: Q9K_STRAP_SCRATCH_VEC | Q9_u32, Vektornummer EIN |
| `$001BFC` | 4 | `Q9K_StrapScratch_Jump` | q9kernel_entry.a; C: Q9K_STRAP_SCRATCH_JUMP | Q9_u32, Sprungziel AUS (0 = keins) |
| `$001C00` | 2 | `Q9K_ExcNesting` | q9kernel_entry.a | Q9_u16 |
| `$001C10` | 512 | `Tabelle:event` | q9_scratch_tables.txt; C: Q9K_EVENT_BASE | q9kernel_event.c: 16 x 32 Byte |
| `$001E20` | 2 | `Q9K_EventScratch_Func` | q9kernel_entry.a; C: Q9K_EVENT_SCRATCH_FUNC | Q9_u32, d1.w EIN |
| `$001E24` | 4 | `Q9K_EventScratch_D0` | q9kernel_entry.a; C: Q9K_EVENT_SCRATCH_D0 | Q9_u32, d0.l EIN / AUS |
| `$001E28` | 4 | `Q9K_EventScratch_D1` | q9kernel_entry.a; C: Q9K_EVENT_SCRATCH_D1 | Q9_u32, d1.l AUS |
| `$001E2C` | 4 | `Q9K_EventScratch_D2` | q9kernel_entry.a; C: Q9K_EVENT_SCRATCH_D2 | Q9_u32, d2.l EIN |
| `$001E30` | 4 | `Q9K_EventScratch_D3` | q9kernel_entry.a; C: Q9K_EVENT_SCRATCH_D3 | Q9_u32, d3.l EIN |
| `$001E34` | 4 | `Q9K_EventScratch_A0` | q9kernel_entry.a; C: Q9K_EVENT_SCRATCH_A0 | Q9_u32, (a0) EIN |
| `$001E38` | 2 | `Q9K_EventScratch_Error` | q9kernel_entry.a; C: Q9K_EVENT_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$001E3C` | 4 | `Q9K_EventScratch_Ok` | q9kernel_entry.a; C: Q9K_EVENT_SCRATCH_OK | Q9_u32, 0/1 |
| `$001E40` | 4 | `Q9K_EventImplPtr` | q9kernel_entry.a | Q9_u32 |
| `$001E44` | 4 | `Q9K_EventScratch_Wait` | q9kernel_entry.a; C: Q9K_EVENT_SCRATCH_WAIT | Q9_u32, 1 = Aufrufer muss warten |
| `$001E48` | 4 | `Q9K_EventScratch_Next` | q9kernel_entry.a; C: Q9K_EVENT_SCRATCH_NEXT | Q9_u32, naechster Prozess |
| `$001E4C` | 4 | `Q9K_EventWaitImplPtr` | q9kernel_entry.a | Q9_u32, Einsprung Q9K_SysEventWaitImpl |
| `$001E50` | 4 | `Q9K_NProcScratch_Next` | q9kernel_entry.a; C: Q9K_NPROC_SCRATCH_NEXT | Q9_u32, naechster Prozess (F$NProc) |
| `$001E54` | 4 | `Q9K_NProcImplPtr` | q9kernel_entry.a | Q9_u32, Einsprung Q9K_SysNProcImpl |
| `$001E58` | 4 | `Q9K_StrapImplPtr` | q9kernel_entry.a | Q9K_SysSTrapImpl     (strap.c) |
| `$001E5C` | 4 | `Q9K_RTEImplPtr` | q9kernel_entry.a | Q9K_SysRTEImpl       (icpt.c) |
| `$001E60` | 4 | `Q9K_SigResetImplPtr` | q9kernel_entry.a | Q9K_SysSigResetImpl  (icpt.c) |
| `$001E64` | 4 | `Q9K_ChainImplPtr` | q9kernel_entry.a | Q9K_SysChainImpl     (chain.c) |
| `$001E68` | 4 | `Q9K_ChainReleaseImplPtr` | q9kernel_entry.a | Q9K_SysChainReleaseImpl |
| `$001E6C` | 4 | `Q9K_SemaImplPtr` | q9kernel_entry.a | Q9K_SysSemaImpl      (sema.c) |
| `$001E70` | 4 | `Q9K_SemaWaitImplPtr` | q9kernel_entry.a | Q9K_SysSemaWaitImpl |
| `$001E74` | 4 | `Q9K_MemScratch_Request` | q9kernel_entry.a; C: Q9K_MemScratch_Request | Q9_u32, d0.l EIN |
| `$001E78` | 4 | `Q9K_MemScratch_OutSize` | q9kernel_entry.a; C: Q9K_MemScratch_OutSize | Q9_u32, d0.l AUS |
| `$001E7C` | 4 | `Q9K_MemScratch_OutUpper` | q9kernel_entry.a; C: Q9K_MemScratch_OutUpper | Q9_u32, a1 AUS |
| `$001E80` | 2 | `Q9K_MemScratch_Error` | q9kernel_entry.a; C: Q9K_MemScratch_Error | Q9_u32, d1.w AUS bei Fehler |
| `$001E84` | 4 | `Q9K_MemScratch_Ok` | q9kernel_entry.a; C: Q9K_MemScratch_Ok | Q9_u32, 0/1 |
| `$001E88` | 4 | `Q9K_TLinkImplPtr` | q9kernel_entry.a | Q9K_SysTLinkImpl |
| `$001E8C` | 4 | `Q9K_SetSysImplPtr` | q9kernel_entry.a | Q9K_SysSetSysImpl |
| `$001E90` | 4 | `Q9K_JulianImplPtr` | q9kernel_entry.a | Q9K_SysJulianImpl |
| `$001E94` | 4 | `Q9K_GregorImplPtr` | q9kernel_entry.a | Q9K_SysGregorImpl |
| `$001E98` | 4 | `Q9K_AlarmImplPtr` | q9kernel_entry.a | Q9K_SysAlarmImpl |
| `$001E9C` | 4 | `Q9K_TimeImplPtr` | q9kernel_entry.a | Q9K_SysTimeImpl |
| `$001EA0` | 4 | `Q9K_STimeImplPtr` | q9kernel_entry.a | Q9K_SysSTimeImpl |
| `$001EA4` | 4 | `Q9K_SchBitImplPtr` | q9kernel_entry.a | Q9K_SysSchBitImpl |
| `$001EA8` | 4 | `Q9K_AllBitImplPtr` | q9kernel_entry.a | Q9K_SysAllBitImpl |
| `$001EAC` | 4 | `Q9K_DelBitImplPtr` | q9kernel_entry.a | Q9K_SysDelBitImpl |
| `$001EB0` | 4 | `Q9K_GBlkMpImplPtr` | q9kernel_entry.a | Q9K_SysGBlkMpImpl |
| `$001EB4` | 4 | `Q9K_SSvcImplPtr` | q9kernel_entry.a | Q9K_SysSSvcImpl |
| `$001EB8` | 4 | `Q9K_MemImplPtr` | q9kernel_entry.a | Q9K_SysFMemImpl      (mem.c) |
| `$001EBC` | 2 | `Q9K_DForkScratch_TypeLang` | q9kernel_entry.a | Q9_u16, d0.w EIN |
| `$001EC0` | 4 | `Q9K_DForkScratch_AddMem` | q9kernel_entry.a | Q9_u32, d1.l EIN |
| `$001EC4` | 4 | `Q9K_DForkScratch_ParamSize` | q9kernel_entry.a | Q9_u32, d2.l EIN |
| `$001EC8` | 2 | `Q9K_DForkScratch_NumPaths` | q9kernel_entry.a | Q9_u16, d3.w EIN |
| `$001ECC` | 2 | `Q9K_DForkScratch_Priority` | q9kernel_entry.a | Q9_u16, d4.w EIN |
| `$001ED0` | 4 | `Q9K_DForkScratch_Name` | q9kernel_entry.a | Q9_u32, (a0) EIN/AUS |
| `$001ED4` | 4 | `Q9K_DForkScratch_Param` | q9kernel_entry.a | Q9_u32, (a1) EIN |
| `$001ED8` | 4 | `Q9K_DForkScratch_RegBuf` | q9kernel_entry.a | Q9_u32, (a2) EIN |
| `$001EDC` | 2 | `Q9K_DForkScratch_ChildPID` | q9kernel_entry.a | Q9_u16, d0.w AUS |
| `$001EE0` | 2 | `Q9K_DForkScratch_Error` | q9kernel_entry.a | Q9_u16, d1.w AUS |
| `$001EE4` | 2 | `Q9K_DForkScratch_Ok` | q9kernel_entry.a | Q9_u16, 0/1 |
| `$001EE8` | 4 | `Q9K_DForkImplPtr` | q9kernel_entry.a | Q9K_SysDForkImpl       (firstproc.c) |
| `$001EEC` | 4 | `Q9K_DExitScratch_Pid` | q9kernel_entry.a; C: Q9K_DEXIT_SCRATCH_PID | Q9_u16, d0.w EIN |
| `$001EF0` | 4 | `Q9K_DExitScratch_Error` | q9kernel_entry.a; C: Q9K_DEXIT_SCRATCH_ERROR | Q9_u16, d1.w AUS |
| `$001EF4` | 4 | `Q9K_DExitScratch_Ok` | q9kernel_entry.a; C: Q9K_DEXIT_SCRATCH_SUCCESS | Q9_u16, 0/1 |
| `$001EF8` | 4 | `Q9K_DExitImplPtr` | q9kernel_entry.a | Q9K_SysDExitImpl |
| `$001EFC` | 4 | `Q9K_DExecImplPtr` | q9kernel_entry.a | Q9K_SysDExecImpl |
| `$001F00` | 2 | `Q9K_DExecScratch_Pid` | q9kernel_entry.a | Q9_u16, d0.w EIN |
| `$001F04` | 4 | `Q9K_DExecScratch_Count` | q9kernel_entry.a | Q9_u32, d1.l EIN |
| `$001F08` | 4 | `Q9K_DExecScratch_Next` | q9kernel_entry.a | Q9_u32, next descriptor AUS |
| `$001F0C` | 2 | `Q9K_DExecScratch_Error` | q9kernel_entry.a | Q9_u32, d1.w AUS |
| `$001F10` | 2 | `Q9K_DExecScratch_Ok` | q9kernel_entry.a | Q9_u16, 0/1 |
| `$001F14` | 4 | `Q9K_ConSSigPid` | q9kernel_entry.a; C: Q9K_CONSSIG_PID | Q9_u32, SS_SSig-Empfaenger der nativen Konsole (0 = keiner), s. q9kern |
| `$001F18` | 4 | `Q9K_ConSSigSignal` | q9kernel_entry.a; C: Q9K_CONSSIG_SIGNAL | Q9_u32, zugehoeriger Signalcode |
| `$001F1C` | 4 | `Q9K_ConsoleViaIOMan` | q9kernel_entry.a | Q9_u32, 1 = /term ueber IOMan/SCF statt nativ (echter Start) -- naechs |
| `$001F20` | 4 | `Q9K_FIRQScratch_Vector` | q9kernel_entry.a; C: Q9K_FIRQ_SCRATCH_VECTOR | Q9_u32, d0.l EIN = FIRQ-Vektor |
| `$001F24` | 4 | `Q9K_FIRQScratch_Reserved` | q9kernel_entry.a; C: Q9K_FIRQ_SCRATCH_RESERVED | Q9_u32, d1.b EIN (muss 0 sein) |
| `$001F28` | 4 | `Q9K_FIRQScratch_Isr` | q9kernel_entry.a; C: Q9K_FIRQ_SCRATCH_ISR | Q9_u32, a0 EIN = Fast-ISR |
| `$001F2C` | 4 | `Q9K_FIRQScratch_Static` | q9kernel_entry.a; C: Q9K_FIRQ_SCRATCH_STATIC | Q9_u32, a2 EIN = statische Daten |
| `$001F30` | 4 | `Q9K_FIRQScratch_Error` | q9kernel_entry.a; C: Q9K_FIRQ_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$001F34` | 4 | `Q9K_FIRQScratch_Success` | q9kernel_entry.a; C: Q9K_FIRQ_SCRATCH_SUCCESS | Q9_u32, 0 = Fehlschlag / 1 = Erfolg |
| `$001F38` | 4 | `Q9K_NATIVE_READ_SCRATCH_PATH` | q9kernel_entry.a; C: Q9K_NATIVE_READ_SCRATCH_PATH |  |
| `$001F3C` | 4 | `Q9K_NATIVE_READ_SCRATCH_BUF` | q9kernel_entry.a; C: Q9K_NATIVE_READ_SCRATCH_BUF |  |
| `$001F40` | 4 | `Q9K_NATIVE_READ_SCRATCH_COUNT` | q9kernel_entry.a; C: Q9K_NATIVE_READ_SCRATCH_COUNT |  |
| `$001F44` | 4 | `Q9K_NATIVE_READ_SCRATCH_DONE` | q9kernel_entry.a; C: Q9K_NATIVE_READ_SCRATCH_DONE |  |
| `$001F48` | 4 | `Q9K_NATIVE_READ_SCRATCH_ERROR` | q9kernel_entry.a; C: Q9K_NATIVE_READ_SCRATCH_ERROR |  |
| `$001F4C` | 4 | `Q9K_NATIVE_READ_SCRATCH_OK` | q9kernel_entry.a; C: Q9K_NATIVE_READ_SCRATCH_OK |  |
| `$001F50` | 4 | `Q9K_TrapForeignFlag` | q9kernel_entry.a | freier Scratch hinter dem nativen I$Read-Block |
| `$001F54` | 4 | `Q9K_FLinkSavedSP` | q9kernel_entry.a |  |
| `$001F58` | 4 | `Q9K_FLinkSavedD7` | q9kernel_entry.a |  |
| `$001F5C` | 4 | `Q9K_FLinkSavedA6` | q9kernel_entry.a |  |
| `$001F60` | 4 | `Q9K_FLinkSavedName` | q9kernel_entry.a |  |
| `$001F70` | 4 | `Q9K_FLinkSearchImplPtr` | q9kernel_entry.a | C-init-relocated Q9K_SysFLinkSearchImpl |
| `$001F74` | 4 | `Q9K_IOpenManagerRoutine` | q9kernel_entry.a; C: Q9K_SSVC_IOPEN_ROUTINE_ADDR | IOMan I$Open handler shadow; native path remains installed |
| `$001F78` | 4 | `Q9K_IReadManagerRoutine` | q9kernel_entry.a; C: Q9K_SSVC_IREAD_ROUTINE_ADDR | IOMan I$Read shadow; paths carry manager tag |
| `$001F7C` | 4 | `Q9K_ICloseManagerRoutine` | q9kernel_entry.a; C: Q9K_SSVC_ICLOSE_ROUTINE_ADDR | IOMan I$Close shadow; paths carry manager tag |
| `$001F80` | 4 | `Q9K_RetPDImplPtr` | q9kernel_entry.a | C-init-relocated Q9K_SysRetPDImpl |
| `$001F84` | 4 | `Q9K_FIRQHandlersPtr` | q9kernel_entry.a | C-init-relocated Q9K_FIRQHandlers VSECT base |
| `$001F88` | 4 | `Q9K_FIRQStaticsPtr` | q9kernel_entry.a | C-init-relocated Q9K_FIRQStatics VSECT base |
| `$001F8C` | 4 | `Q9K_IdleProcDesc` | q9kernel_entry.a; C: Q9K_IDLEPROCDESC_ADDR | Q9_u32, permanenter Idle-Prozessdeskriptor (s. Q9K_ProcCreateIdle, q9k |
| `$001F90` | 1 | `Q9K_FrameLoggedOnce` | q9kernel_entry.a | TEMPORAER (2026-10-02, Fortsetzung 102): Q9_u8, begrenzt die erweitert |
| `$001F94` | 76 | `Q9K_MgrRoutineTable` | q9kernel_entry.a; C: Q9K_SSVC_MGR_TABLE_ADDR | 19 x Q9_u32 ($1F94-$1FDF): IOMan-Benutzerroutine je I/O-Code $80..$92  |
| `$001FE0` | 32 | `Q9K_SysStartParam` | q9kernel_entry.a | 32 Byte, Startparameter + CR -- naechste freie Adresse $2000 |
| `$002000` | 4 | `Q9K_DbgTraceBufAddr` | q9kernel_entry.a; C: Q9K_INITIAL_SR, Q9DBG_A_BUF_ADDR, Q9K_INITIAL_SR | Q9_u32, Trace-Ringpuffer: per F$SRqMem reservierte Adresse (0 = keiner |
| `$002004` | 4 | `Q9K_DbgTraceBufSize` | q9kernel_entry.a | Q9_u32 |
| `$002008` | 4 | `Q9K_DbgTraceWriteOff` | q9kernel_entry.a | Q9_u32 |
| `$00200C` | 4 | `Q9K_DbgTraceReadOff` | q9kernel_entry.a | Q9_u32 |
| `$002010` | 4 | `Q9K_DbgTraceUsed` | q9kernel_entry.a; C: Q9DBG_A_USED | Q9_u32 |
| `$002014` | 4 | `Q9K_DbgTraceWritten` | q9kernel_entry.a; C: Q9DBG_A_WRITTEN | Q9_u32 |
| `$002018` | 4 | `Q9K_DbgTraceLost` | q9kernel_entry.a; C: Q9DBG_A_LOST | Q9_u32 |
| `$00201C` | 4 | `Q9K_DbgTraceMode` | q9kernel_entry.a; C: Q9DBG_A_MODE | Q9_u32 |
| `$002020` | 4 | `Q9K_DbgTraceEnabled` | q9kernel_entry.a; C: Q9DBG_A_ENABLED | Q9_u32, 0/1 -- Trace an/aus |
| `$002024` | 4 | `Q9K_DbgTraceOutMode` | q9kernel_entry.a; C: Q9DBG_A_OUTMODE | Q9_u32 |
| `$002028` | 32 | `Q9K_DbgSyscallMask` | q9kernel_entry.a | 32 Byte |
| `$002048` | 32 | `Q9K_DbgInternMask` | q9kernel_entry.a | 32 Byte |
| `$002068` | 4 | `Q9K_DbgFiltPid` | q9kernel_entry.a; C: Q9DBG_A_FILT_PID | Q9_u32 |
| `$00206C` | 4 | `Q9K_DbgFiltUser` | q9kernel_entry.a; C: Q9DBG_A_FILT_USER | Q9_u32 |
| `$002070` | 4 | `Q9K_DbgFiltPath` | q9kernel_entry.a; C: Q9DBG_A_FILT_PATH | Q9_u32 |
| `$002074` | 4 | `Q9K_DbgFiltFileId` | q9kernel_entry.a; C: Q9DBG_A_FILT_FILEID | Q9_u32 |
| `$002078` | 4 | `Q9K_DbgFiltErrOnly` | q9kernel_entry.a; C: Q9DBG_A_FILT_ERRONLY | Q9_u32 |
| `$00207C` | 4 | `Q9K_DbgFiltDetail` | q9kernel_entry.a; C: Q9DBG_A_FILT_DETAIL | Q9_u32 |
| `$002080` | 4 | `Q9K_DbgTrapDepth` | q9kernel_entry.a | Q9_u32, aktuelle Verschachtelungstiefe von Q9K_TrapDispatch |
| `$002084` | 32 | `Q9K_DbgTrapCode` | q9kernel_entry.a; C: Q9DBG_A_TRAP_CODE | 8 x Q9_u32 |
| `$0020A4` | 32 | `Q9K_DbgTrapPid` | q9kernel_entry.a; C: Q9DBG_A_TRAP_PID | 8 x Q9_u32 |
| `$0020C4` | 32 | `Q9K_DbgTrapTick` | q9kernel_entry.a; C: Q9DBG_A_TRAP_TICK | 8 x Q9_u32 |
| `$0020E4` | 4 | `Q9K_DbgRetCarry` | q9kernel_entry.a; C: Q9DBG_A_RET_CARRY | Q9_u32, vom Rueckkehr-Stub VOR dem C-Aufruf gesetzt (0/1) |
| `$0020E8` | 2 | `Q9K_DbgSvcFn` | q9kernel_entry.a; C: Q9DBG_A_SVC_FN | Q9_u32, d0.w des F$Q9Dbg-Aufrufers |
| `$0020EC` | 4 | `Q9K_DbgSvcD1` | q9kernel_entry.a; C: Q9DBG_A_SVC_D1 | Q9_u32 |
| `$0020F0` | 4 | `Q9K_DbgSvcD2` | q9kernel_entry.a; C: Q9DBG_A_SVC_D2 | Q9_u32 |
| `$0020F4` | 4 | `Q9K_DbgSvcD3` | q9kernel_entry.a; C: Q9DBG_A_SVC_D3 | Q9_u32 |
| `$0020F8` | 4 | `Q9K_DbgSvcA0` | q9kernel_entry.a; C: Q9DBG_A_SVC_A0 | Q9_u32, Zeigerwert |
| `$0020FC` | 4 | `Q9K_DbgSvcOutD0` | q9kernel_entry.a; C: Q9DBG_A_SVC_OUT_D0 | Q9_u32 |
| `$002100` | 4 | `Q9K_DbgSvcOutD1` | q9kernel_entry.a; C: Q9DBG_A_SVC_OUT_D1 | Q9_u32 |
| `$002104` | 4 | `Q9K_DbgSvcOutD2` | q9kernel_entry.a; C: Q9DBG_A_SVC_OUT_D2 | Q9_u32 |
| `$002108` | 4 | `Q9K_DbgSvcOutErr` | q9kernel_entry.a; C: Q9DBG_A_SVC_OUT_ERR | Q9_u32, 0 = Erfolg, sonst Fehlercode |
| `$00210C` | 4 | `Q9K_DbgLogEntryPtr` | q9kernel_entry.a | Q9_u32, Q9K_DbgLogEntryImpl |
| `$002110` | 4 | `Q9K_DbgLogReturnPtr` | q9kernel_entry.a | Q9_u32, Q9K_DbgLogReturnImpl |
| `$002114` | 4 | `Q9K_DbgSvcImplPtr` | q9kernel_entry.a | Q9_u32, Q9K_SysFQ9DbgImpl |
| `$002118` | 4 | `Q9K_DbgRegsPtr` | q9kernel_entry.a; C: Q9DBG_A_REGS | Q9_u32, von den Trace-Haken gesetzt: Stackadresse der gesicherten d0,d |
| `$002200` | 512 | `Q9K_DbgScratch` | q9kernel_entry.a | 512 Byte Arbeitsbereich der Trace-Haken (q9kernel_dbg.c, Q9DbgScratch) |
| `$002400` | 4 | `Q9K_TCallScratch_TrapNum` | q9kernel_entry.a | Q9_u32 |
| `$002404` | 4 | `Q9K_TCallScratch_ModPtr` | q9kernel_entry.a | Q9_u32 |
| `$002408` | 4 | `Q9K_TCallScratch_ExecEntry` | q9kernel_entry.a | Q9_u32 |
| `$00240C` | 4 | `Q9K_TCallScratch_StaticPtr` | q9kernel_entry.a | Q9_u32 |
| `$002410` | 4 | `Q9K_TCallScratch_RetPC` | q9kernel_entry.a | Q9_u32 |
| `$002414` | 2 | `Q9K_TCallScratch_VectorWord` | q9kernel_entry.a | Q9_u16 |
| `$002416` | 2 | `Q9K_TCallScratch_FuncCode` | q9kernel_entry.a | Q9_u16 |
| `$002418` | 4 | `Q9K_FModulScratch_TyLang` | q9kernel_entry.a; C: Q9K_FMODUL_SCRATCH_TYLANG | Q9_u32, d0.w EIN |
| `$00241C` | 4 | `Q9K_FModulScratch_Name` | q9kernel_entry.a; C: Q9K_FMODUL_SCRATCH_NAME | Q9_u32, (a0) EIN |
| `$002420` | 4 | `Q9K_FModulScratch_OutTy` | q9kernel_entry.a; C: Q9K_FMODUL_SCRATCH_OUTTY | Q9_u32, d0.w AUS |
| `$002424` | 4 | `Q9K_FModulScratch_OutAR` | q9kernel_entry.a; C: Q9K_FMODUL_SCRATCH_OUTAR | Q9_u32, d1.w AUS |
| `$002428` | 4 | `Q9K_FModulScratch_Entry` | q9kernel_entry.a; C: Q9K_FMODUL_SCRATCH_ENTRY | Q9_u32, a2 AUS |
| `$00242C` | 4 | `Q9K_FModulScratch_Error` | q9kernel_entry.a; C: Q9K_FMODUL_SCRATCH_ERROR | Q9_u32, d1.w AUS bei Fehler |
| `$002430` | 4 | `Q9K_FModulScratch_Ok` | q9kernel_entry.a; C: Q9K_FMODUL_SCRATCH_OK | Q9_u32, 0/1 |
| `$002440` | 60 | `Q9K_RegChkSnap` | q9kernel_entry.a | 15 x Q9_u32, d0-d7/a0-a6 des Aufrufers beim Handlereintritt |
| `$00247C` | 4 | `Q9K_RegChkSP` | q9kernel_entry.a | Q9_u32, sp beim Eintritt (= Adresse des Hardware-Frames; Schluessel ge |
| `$002480` | 2 | `Q9K_RegChkCode` | q9kernel_entry.a | Q9_u16, Callcode des Schnappschusses |
| `$002484` | 4 | `Q9K_RegChkCount` | q9kernel_entry.a | Q9_u32, Anzahl gemeldeter Abweichungen seit dem Boot |
| `$002488` | 4 | `Q9K_SchedNoSwitch` | q9kernel_entry.a; C: Q9K_SCHED_NOSWITCH_ADDR | Q9_u32, A7: <> 0, wenn der vom Timer Unterbrochene selbst eine Interru |
| `$002500` | 64 | `Q9K_MgrRefs` | q9kernel_entry.a; C: Q9K_FORK_MGRREF_ADDR, Q9K_MGRREF_ADDR | 64 Byte, A13: Ueberschuss-Zaehler je IOMan-Pfad (Index = PD-Nummer & $ |
| `$002540` | 4 | `Q9K_DExecScratch_Parent` | q9kernel_entry.a | Q9_u32, C2: Deskriptor des Debuggers (Aufrufer von F$DExec) |
| `$002544` | 2 | `Q9K_DExecScratch_BpCount` | q9kernel_entry.a | Q9_u32, C2: d2.w EIN = Anzahl Haltepunkte (im unteren Wort) |
| `$002548` | 4 | `Q9K_DExecScratch_BpList` | q9kernel_entry.a | Q9_u32, C2: (a0) EIN = Haltepunktliste |
| `$1540C0` | 4 | `Q9K_RaceRingIdx` | q9kernel_entry.a | Base + Slots*EntSz (8192*8=$10000), Literal wegen r68 |
| `$154100` | 4 | `Q9K_TLinkTrace_Proc` | q9kernel_entry.a |  |
| `$154104` | 4 | `Q9K_TLinkTrace_EntryA3` | q9kernel_entry.a |  |
| `$154108` | 4 | `Q9K_TLinkTrace_EntryA6` | q9kernel_entry.a |  |
| `$15410C` | 4 | `Q9K_TLinkTrace_PreInitA3` | q9kernel_entry.a |  |
| `$154110` | 4 | `Q9K_TLinkTrace_PreInitA6` | q9kernel_entry.a |  |
| `$154114` | 4 | `Q9K_TLinkTrace_PostInitA3` | q9kernel_entry.a |  |
| `$154118` | 4 | `Q9K_TLinkTrace_PostInitA6` | q9kernel_entry.a |  |
| `$15411C` | 4 | `Q9K_TLinkTrace_ModPtr` | q9kernel_entry.a |  |
| `$154120` | 4 | `Q9K_TLinkTrace_InitEntry` | q9kernel_entry.a |  |
| `$154124` | 4 | `Q9K_TLinkTrace_ExecEntry` | q9kernel_entry.a |  |
| `$154128` | 4 | `Q9K_TLinkTrace_StaticPtr` | q9kernel_entry.a |  |
| `$15412C` | 4 | `Q9K_TLinkTrace_Error` | q9kernel_entry.a |  |
| `$154130` | 4 | `Q9K_TLinkTrace_Success` | q9kernel_entry.a |  |
| `$154140` | 4 | `Q9K_FLinkTrace_Status` | q9kernel_entry.a |  |
| `$154144` | 4 | `Q9K_FLinkTrace_D0` | q9kernel_entry.a |  |
| `$154148` | 4 | `Q9K_FLinkTrace_D1` | q9kernel_entry.a |  |
| `$15414C` | 4 | `Q9K_FLinkTrace_A0` | q9kernel_entry.a |  |
| `$154150` | 4 | `Q9K_FLinkTrace_A1` | q9kernel_entry.a |  |
| `$154154` | 4 | `Q9K_FLinkTrace_A2` | q9kernel_entry.a |  |
| `$154158` | 4 | `Q9K_FLinkTrace_InputA0` | q9kernel_entry.a |  |
| `$154170` | 4 | `Q9K_FLinkSearch_TyLang` | q9kernel_entry.a |  |
| `$154174` | 4 | `Q9K_FLinkSearch_Name` | q9kernel_entry.a |  |
| `$154178` | 4 | `Q9K_FLinkSearch_Slot` | q9kernel_entry.a |  |
| `$154180` | 4 | `Q9K_FLINK_DIAG_ITER` | q9kernel_moddir.c |  |
| `$154184` | 4 | `Q9K_FLINK_DIAG_SLOT` | q9kernel_moddir.c |  |
| `$154188` | 4 | `Q9K_FLINK_DIAG_HEADER` | q9kernel_moddir.c |  |
| `$15418C` | 4 | `Q9K_FLINK_DIAG_GUARD` | q9kernel_moddir.c |  |
| `$154194` | 4 | `Q9K_FLinkFrame_Mode` | q9kernel_entry.a | Q9K_InTrapPath beim F$Link-Eintritt |
| `$154198` | 4 | `Q9K_FLinkFrame_EntrySP` | q9kernel_entry.a | SP beim Handler-Eintritt |
| `$15419C` | 4 | `Q9K_FLinkFrame_EntryRet` | q9kernel_entry.a | erstes Stackwort beim Handler-Eintritt |
| `$1541A0` | 4 | `Q9K_FLinkFrame_PostSP` | q9kernel_entry.a | SP direkt nach der C-Suche |
| `$1541A4` | 4 | `Q9K_FLinkFrame_Post0` | q9kernel_entry.a | erstes Stackwort nach der C-Suche |
| `$1541A8` | 4 | `Q9K_FLinkFrame_Post4` | q9kernel_entry.a | zweites Stackwort nach der C-Suche |
| `$1541AC` | 4 | `Q9K_FLinkFrame_EntryPC` | q9kernel_entry.a | PC-Feld des Frames beim Eintritt |
| `$1541B0` | 4 | `Q9K_FLinkFrame_PostPC` | q9kernel_entry.a | PC-Feld des Frames nach der C-Suche |
| `$1541B4` | 4 | `Q9K_FLinkFrame_RTEPC` | q9kernel_entry.a | PC-Feld unmittelbar vor RTE |
| `$1541B8` | 4 | `Q9K_FLinkFrame_RTESR` | q9kernel_entry.a | SR-Feld unmittelbar vor RTE |
| `$1541C0` | 4 | `Q9K_RACERING_HI` | q9kernel_cinit.c | Ring bis $1540BF, Indexzelle $1540C0, Trace-Zellen bis $1541BB |

## Freie Luecken (>= 16 Byte)

| Bereich | Byte | Fenster |
|---|---:|---|
| `$1541C4`-`$15FFFF` | 48700 | Diagnose |
| `$150000`-`$1540BF` | 16576 | Diagnose |
| `$00254C`-`$003FFF` | 6836 | Scratch |
| `$001004`-`$0011FF` | 508 | Scratch |
| `$00211C`-`$0021FF` | 228 | Scratch |
| `$00248C`-`$0024FF` | 116 | Scratch |
| `$1540C4`-`$1540FF` | 60 | Diagnose |
| `$0012F4`-`$001327` | 52 | Scratch |
| `$0012A4`-`$0012D7` | 52 | Scratch |
| `$001244`-`$001277` | 52 | Scratch |
| `$00166C`-`$00168F` | 36 | Scratch |
| `$001AE4`-`$001AFF` | 28 | Scratch |
| `$0019E4`-`$0019FF` | 28 | Scratch |
| `$0016F8`-`$00170F` | 24 | Scratch |
| `$15415C`-`$15416F` | 20 | Diagnose |
| `$001E10`-`$001E1F` | 16 | Scratch |
| `$001890`-`$00189F` | 16 | Scratch |
| `$001654`-`$001663` | 16 | Scratch |
| `$0015F0`-`$0015FF` | 16 | Scratch |

/* q9kernel_offsets.h -- Centralized scratch offsets used in the trap path.
 * This header lists the reserved scratch addresses between $3AC and $3FC
 * (inclusive) and their intended usage. Keep this in sync with the
 * assembler entry code (q9kernel_entry.a) to avoid overlap.
 */

#ifndef Q9KERNEL_OFFSETS_H
#define Q9KERNEL_OFFSETS_H

/* Scratch region base/end (byte offsets from frame base). */
#define Q9K_SCRATCH_START 0x3AC
#define Q9K_SCRATCH_END   0x3FC

/* Individual scratch slots (size in bytes) -- examples derived from
 * current trap-path. Adjust names/sizes to match the assembler layout. */
#define Q9K_SCRATCH_R_A7   0x3AC /* 4 bytes: saved A7 */
#define Q9K_SCRATCH_R_D0   0x3B0 /* 4 bytes: temp D0 */
#define Q9K_SCRATCH_R_D1   0x3B4 /* 4 bytes: temp D1 */
#define Q9K_SCRATCH_R_A4   0x3B8 /* 4 bytes: saved A4 */
#define Q9K_SCRATCH_R_A6   0x3BC /* 4 bytes: saved A6 */
#define Q9K_SCRATCH_TMP1   0x3C0 /* 4 bytes */
#define Q9K_SCRATCH_TMP2   0x3C4 /* 4 bytes */
#define Q9K_SCRATCH_TMP3   0x3C8 /* 4 bytes */
#define Q9K_SCRATCH_TMP4   0x3CC /* 4 bytes */
#define Q9K_SCRATCH_TMP5   0x3D0 /* 4 bytes */
#define Q9K_SCRATCH_TMP6   0x3D4 /* 4 bytes */
#define Q9K_SCRATCH_TMP7   0x3D8 /* 4 bytes */
#define Q9K_SCRATCH_TMP8   0x3DC /* 4 bytes */
#define Q9K_SCRATCH_ENDMARK 0x3E0

#endif /* Q9KERNEL_OFFSETS_H */

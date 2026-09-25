# DHF errno mapping

This document contains a minimal mapping from host errno values to Q9/OS-9 DHF result codes used in dhf_shared.result_code.

For now, host errno values are passed through directly as 32-bit integers. Later we may map them into a compact Q9 code space.

Examples:
- 0 -> OK
- EACCES -> 13
- ENOENT -> 2
- EFAULT -> 14
- ENOSYS -> 78

(These are host errno numbers on POSIX; final mapping for Q9 codes to be defined.)

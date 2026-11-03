# Source recovery

The local `pre` folder contained three C source variants. All three passed the initial Windows syntax check and contained the same 36 function names. They were substantially more complete than the 342-line `login.c` previously in this repository.

| Local source | Lines | Last modified in the local filesystem |
| --- | --- | --- |
| `moremore.c` | 2,847 | 13 December 2024, 16:27 |
| `morning.c` | 3,005 | 13 December 2024, 16:39 |
| `more2.c` | 3,002 | 13 December 2024, 17:14 |

These are filesystem timestamps, not a verified development history. The adjacent executable files were not used as source or run during recovery.

## Selection

`more2.c` was selected as the base. It contains account recovery, detailed project records, filtering, buyer/seller balances and the order confirmation/cancellation workflow. Compared with `moremore.c`, it has expanded order-status handling. Its differences from `morning.c` are concentrated in transaction saving, the account menu and order-status handling. It was the latest saved candidate with the complete feature set, although it still required repairs.

The recovered application keeps the existing repository filename, `login.c`, so build commands and portfolio links continue to work. The original local source files were left unchanged. Earlier repository versions remain available in Git history.

SHA-256 of the selected original `more2.c`:

```text
c1484d3ee7a6a155580a5b04244fcac26eb7d7691bb3477667b56d026a8e6cc0
```

## Repairs

- Replaced the unpersisted per-user order-status cache with checks against saved transactions. Reloading transactions now resets the in-memory count and checks capacity before insertion.
- Centralized purchase, confirmation, cancellation and acknowledgement rules. These check the acting account and current status before changing balances or credits. A completed order cannot settle twice.
- Corrected numeric input that wrote an integer into a one-byte character. Line-based input now rejects invalid, negative, non-finite and oversized values, and handles redirected input and EOF.
- Bounded record parsing to each destination field. Invalid or duplicate records stop loading before any files are saved. The original whitespace account format and quoted project CSV remain supported; transaction names can now contain commas.
- Corrected password reset so it updates the password without rewriting eight-field account records through the old six-field parser.
- Corrected inventory changes when editing and deleting listings. Cancelling an edit restores the original project without changing inventory. Pending and completed listings cannot be edited or deleted.
- Kept usernames fixed after registration so saved ownership references remain valid. Profile editing still supports passwords, recovery answers, email and simulated balances.
- Added terminal handling for Windows and POSIX systems, temporary-file replacement for saves, fictional demo records and automated regression checks.

The recovery and regression tests were completed with AI assistance in November 2026. They are repository maintenance work; the original coursework contribution is described separately in the README.

## Validation

The Windows GCC 6.3.0 build passed with `-std=c11 -Wall -Wextra -Wpedantic -Werror -O2`. The automated suite exercises purchase and refund accounting, authorization, repeated settlement attempts, restarts, full transaction capacity, CSV parsing and malformed input. CLI checks cover registration, listing creation, type and credit editing, cancelling edits, search, password recovery, deletion and a purchase completed across separate application processes.

The repository includes only synthetic example records. Original local accounts, project data, transactions and executable files were not copied into Git.

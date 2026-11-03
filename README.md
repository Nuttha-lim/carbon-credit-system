# Carbon Credit Trading System

[![C checks](https://github.com/Nuttha-lim/carbon-credit-system/actions/workflows/build.yml/badge.svg)](https://github.com/Nuttha-lim/carbon-credit-system/actions/workflows/build.yml)

A C console application built for a university programming project. Users can list carbon-credit projects, search available listings and manage purchase requests. Accounts, projects and orders are stored in local text files.

## Features

- Account registration, login and password recovery through security questions
- Project creation, editing and deletion, with credits reserved from the seller's demo inventory
- Search by project type, credit quantity, total price and duration
- Purchase requests with seller confirmation, cancellation and buyer acknowledgement
- Saved balances and order status across sessions

Each purchase covers an entire project listing. The balance and credit inventory are simulated; the application does not process payments or verify carbon credits.

## Run

With GCC installed, run from the repository root in PowerShell:

```powershell
New-Item -ItemType Directory -Path build -Force
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 login.c -o build/carbon-credit.exe
.\build\carbon-credit.exe
```

On Linux or macOS:

```sh
mkdir -p build
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 login.c -o build/carbon-credit
./build/carbon-credit
```

The first run starts with no accounts. Register a sample account or load the demo below. The application saves `users.txt`, `projects.txt` and `transaction.txt` in the directory where it is launched. These files are excluded from Git.

## Try the demo

For a fresh checkout, copy the fictional records into the repository root. These commands replace any existing local application data, so keep a backup if you have already used the program.

```powershell
Copy-Item demo/users.txt.example users.txt
Copy-Item demo/projects.txt.example projects.txt
Copy-Item demo/transaction.txt.example transaction.txt
.\build\carbon-credit.exe
```

For Linux or macOS, use `cp` in place of `Copy-Item`.

| Account | Password | Starting inventory | Starting balance |
| --- | --- | --- | --- |
| `seller` | `demo123` | 70 unlisted credits; 30 reserved in project 0001 | 0 |
| `buyer` | `demo123` | 0 credits | 1,000 |
| `observer` | `demo123` | 0 credits | 1,000 |

1. Log in as `buyer`. Open **Buyer**, choose **Start Search**, then request project `0001` for 200.
2. Log out and log in as `seller`. Open **Order status**, enter `0001` and confirm the order.
3. Log in as `buyer` again. The account now has 30 credits and a balance of 800. Acknowledge the completed order under **Order status**.

Funds are reserved when the buyer requests a purchase. Confirmation transfers the listed credits to the buyer and the reserved funds to the seller. Either participant can cancel a pending order for a refund. After a seller cancellation, the buyer must acknowledge it before the listing can be purchased or edited again.

## Implementation and contribution

My coursework contribution covered registration, login and project management using C structs, dynamic arrays and file handling.

`login.c` is the single application entry point. This repository now uses the fuller local `more2.c` version, which includes the buyer/seller order flow. The recovery also corrected input handling, file loading and order-state errors. [Source comparison and recovery notes](docs/SOURCE_REVIEW.md) record the selected version and subsequent changes.

Usernames stay fixed after registration to preserve project and order ownership. Account fields use single words; underscores can replace spaces in security answers. Project descriptions and types support spaces and commas. Amounts are limited to 1,000,000 and saved to two decimal places.

This academic prototype stores passwords and recovery answers in plain text and uses floating-point balances. Use fictional details. Run one instance at a time. Individual files are replaced through temporary files, but saving all three files is not a database transaction; keep a consistent backup if retaining local records.

## Tests

With Python 3 and GCC available:

```sh
python tests/run_tests.py
```

The runner builds the application and order tests with warnings treated as errors. All test records are created in temporary directories. Checks cover order ownership, settlement and refunds, repeated file loading, transaction limits, malformed data, and CLI registration, project editing, password recovery and restarts. GitHub Actions also runs the suite with AddressSanitizer and UndefinedBehaviorSanitizer on Linux.

[Portfolio](https://github.com/Nuttha-lim/portfolio)

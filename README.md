# Carbon Credit Trading System

A C console program for registering users and managing carbon-credit project listings. Sellers can add and edit projects, while buyers can search listings by project type. User and project records are stored in local text files.

## My contribution

I developed the registration, login and project-management functions using structs, dynamic memory allocation and file handling.

## Features

- Register an account and log in
- Add project listings with credit quantity, price, dates and seller name
- Search listings by project type
- Edit an existing project
- Save records to disk and load them in later sessions

## Run on Windows

Install GCC, then run these commands from the repository root in PowerShell:

```powershell
New-Item -ItemType Directory -Path build -Force
gcc -std=c11 -Wall -Wextra -Wpedantic login.c -o build/carbon-credit.exe
.\build\carbon-credit.exe
```

The program creates `user_data.txt` and `data.txt` in its working directory. Project types must be single words, such as `forest`, because input uses whitespace-separated fields.

## Implementation

`login.c` contains the user and project structs, account functions, search and editing logic, and file operations. Arrays grow with `realloc` when they reach their current capacity.

This is an academic console prototype. Use sample accounts: passwords are stored as plain text, and project editing checks the username entered at the prompt.

## Checks

The program compiled with GCC 6.3.0 on Windows. Registration, project creation, login, search, editing and reloading saved data were checked using temporary sample records. The compiler reports an unused `username` parameter in `edit_project`.

[Portfolio](https://github.com/Nuttha-lim/portfolio)

# Purpose: Simple Makefile for Contact Manager. Author: Sagar Biswas
CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2

SQLITE_CFLAGS := $(shell pkg-config --cflags sqlite3 2>/dev/null)
SQLITE_LIBS := $(shell pkg-config --libs sqlite3 2>/dev/null)
SODIUM_CFLAGS := $(shell pkg-config --cflags libsodium 2>/dev/null)
SODIUM_LIBS := $(shell pkg-config --libs libsodium 2>/dev/null)
ARGON2_CFLAGS := $(shell pkg-config --cflags libargon2 2>/dev/null)
ARGON2_LIBS := $(shell pkg-config --libs libargon2 2>/dev/null)

SRC = src/main.c src/db.c src/auth.c src/contacts.c src/csv.c src/util.c
INC = -Iinclude

all: contacts

contacts: $(SRC)
	@if [ -n "$(SODIUM_LIBS)" ]; then \
		$(CC) $(CFLAGS) $(INC) $(SQLITE_CFLAGS) $(SODIUM_CFLAGS) -DHAVE_LIBSODIUM -o $@ $(SRC) $(SQLITE_LIBS) $(SODIUM_LIBS); \
	elif [ -n "$(ARGON2_LIBS)" ]; then \
		$(CC) $(CFLAGS) $(INC) $(SQLITE_CFLAGS) $(ARGON2_CFLAGS) -DHAVE_ARGON2 -o $@ $(SRC) $(SQLITE_LIBS) $(ARGON2_LIBS); \
	else \
		echo "Missing libsodium or libargon2"; exit 1; \
	fi

clean:
	rm -f contacts

.PHONY: all clean

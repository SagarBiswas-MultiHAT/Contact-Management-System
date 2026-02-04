# Security Notes

## Purpose

This document explains password storage, threat model, and operational recommendations.

## Password Storage

- Passwords are never stored in plaintext.
- The application uses Argon2id via libsodium (preferred) or libargon2.
- The encoded hash includes salt and parameters; verification uses constant-time routines.

## Threat Model

- Protects against offline hash cracking by using Argon2id with reasonable defaults.
- Does not defend against a fully compromised host or runtime memory inspection.
- SQLite database file is stored on disk; access controls depend on OS permissions.

## Recommendations

- Use OS file permissions to restrict database access.
- Store the database on encrypted storage if disk confidentiality is required.
- Back up the database regularly and store backups securely.
- Use strong, unique passwords for the contact manager.
- Avoid passing passwords on the command line in shared environments (command history/process lists).

## Database Encryption

This project does not enable built-in encryption for SQLite. If encryption-at-rest is required, use full-disk encryption or a virtual encrypted volume. Documented optional adapters can be added later.

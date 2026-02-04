// Purpose: Password hashing and verification. Author: GitHub Copilot
#ifndef CONTACTS_AUTH_H
#define CONTACTS_AUTH_H

#include "db.h"

#ifdef __cplusplus
extern "C" {
#endif

    int auth_set_password(Db* db, const char* password);
    int auth_verify_password(Db* db, const char* password);

#ifdef __cplusplus
}
#endif

#endif

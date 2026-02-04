// Purpose: Robust CSV parsing and writing. Author: GitHub Copilot
#ifndef CONTACTS_CSV_H
#define CONTACTS_CSV_H

#include "contacts.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

    int csv_write_contacts(Db* db, FILE* out);
    int csv_import_contacts(Db* db, FILE* in, int strict, int dry_run, int* out_imported, int* out_failed);

#ifdef __cplusplus
}
#endif

#endif

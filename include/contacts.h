// Purpose: Contact data model and business logic. Author: GitHub Copilot
#ifndef CONTACTS_CONTACTS_H
#define CONTACTS_CONTACTS_H

#include "db.h"
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONTACT_NAME_MAX 200
#define CONTACT_PHONE_MAX 50
#define CONTACT_ADDRESS_MAX 200
#define CONTACT_EMAIL_MAX 200
#define CONTACT_DUE_DATE_MAX 50

    typedef struct {
        int64_t id;
        char name[CONTACT_NAME_MAX];
        char phone[CONTACT_PHONE_MAX];
        char address[CONTACT_ADDRESS_MAX];
        char email[CONTACT_EMAIL_MAX];
        double due_amount;
        char due_date[CONTACT_DUE_DATE_MAX];
    } Contact;

    typedef struct {
        int total_contacts;
        int due_contacts;
        int no_due_contacts;
        int overdue_contacts;
        int due_today_contacts;
        int due_soon_contacts;
        int due_later_contacts;
        int due_date_present;
        int due_date_missing;
        int due_date_invalid;
        int missing_phone;
        int missing_email;
        int missing_address;
        double total_due_amount;
        double avg_due_amount;
        double min_due_amount;
        double max_due_amount;
        char min_due_name[CONTACT_NAME_MAX];
        char max_due_name[CONTACT_NAME_MAX];
        char earliest_due_date[CONTACT_DUE_DATE_MAX];
        char latest_due_date[CONTACT_DUE_DATE_MAX];
        int by_letter[27];
    } ContactStats;

    int contacts_add(Db* db, const Contact* c, int64_t* out_id);
    int contacts_update(Db* db, const Contact* c);
    int contacts_delete(Db* db, int64_t id);
    int contacts_get_by_id(Db* db, int64_t id, Contact* out);
    int contacts_list(Db* db, int json, FILE* out);
    int contacts_search_by_name(Db* db, const char* name, int json, FILE* out);
    int contacts_stats(Db* db, ContactStats* out);
    int contacts_set_sort_mode(Db* db, const char* mode);
    int contacts_get_sort_mode(Db* db, char* mode, size_t mode_len);

#ifdef __cplusplus
}
#endif

#endif

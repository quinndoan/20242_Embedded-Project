#ifndef GLOBAL_H
#define GLOBAL_H


// Khai báo biến global
extern char g_uid[20];
extern char g_atqa[10];
extern char g_sak[10];

#define CONFIG_LOG_MAXIMUM_LEVEL 3

// NVS namespace và keys
#define NVS_NAMESPACE "rfid_data"
#define NVS_KEY_UID "uid"
#define NVS_KEY_ATQA "atqa"
#define NVS_KEY_SAK "sak"

#endif
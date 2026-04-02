#include "sqlite3.h"
#include "../Dem/Dem.h"
#include "../Dem/Dem_Cfg.h"
#include <stdio.h>

sqlite3 *db;

void NvM_Sql_Init(void) {
    /* Create a local database file in the current directory */
    int rc = sqlite3_open("smocip_diag.db", &db);
    
    if (rc != SQLITE_OK) {
        printf("[SQL ERROR] %s\n", sqlite3_errmsg(db));
        return;
    }

    const char *sql_s1 = "CREATE TABLE IF NOT EXISTS FaultLogs ("
                         "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                         "DID TEXT, Status TEXT, Timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";

    const char *sql_s2 = "CREATE TABLE IF NOT EXISTS OperationalSnapshots ("
                         "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                         "DID TEXT, OccurrenceTime TEXT);";

    sqlite3_exec(db, sql_s1, 0, 0, 0);
    sqlite3_exec(db, sql_s2, 0, 0, 0);
    printf("[SYSTEM] SQLite Database Initialized (smocip_diag.db)\n");
}

void NvM_WriteFaultLog(Dem_EventIdType id, Dem_EventStatusType status) {
    char query[128];
    sprintf(query, "INSERT INTO FaultLogs (DID, Status) VALUES ('0x%04X', '%s');", 
            id, (status == 0x01 ? "FAILED" : "PASSED"));
    sqlite3_exec(db, query, 0, 0, 0);
    printf("[SQL] Logged Fault: DID 0x%04X [cite: 6, 12]\n", id);
}

void NvM_WriteSnapshotRecord(Dem_EventIdType id, uint32_t timestamp) {
    char query[128];
    sprintf(query, "INSERT INTO OperationalSnapshots (DID, OccurrenceTime) VALUES ('0x%04X', '%u');", 
            id, timestamp);
    sqlite3_exec(db, query, 0, 0, 0);
    printf("[SQL] Recorded Snapshot: DID 0x%04X [cite: 7, 17]\n", id);
}

#include <stdio.h>
#include <sys/unistd.h>
#include "database.h"
#include "sqlite3.h"
#include <stdlib.h>
#include "time.h"
#include "esp_timer.h"

const char *data = "Callback function called";
char *callbackdata;

sqlite3 *farmbox;
int rc;
char *zErrMsg = 0;

static int callback(void *data, int argc, char **argv, char **azColName) //data is empty, argv is the data of col, argc is col
{
    int i;
    // printf("%s: ", (const char *)data);
    //printf("This is Argv:%s, \n This is argc: %d \n This is azColName: %s", argv[1], argc, azColName[1]);
    for (i = 0; i < argc; i++)
    {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    printf("\n");
    return 0;
}

int db_open(const char *filename, sqlite3 **db)
{
    int rc = sqlite3_open(filename, db);
    if (rc)
    {
        printf("Can't open database: %s\n", sqlite3_errmsg(*db));
        return rc;
    }
    else
    {
        printf("Opened database successfully\n");
    }
    return rc;
}

int db_exec(sqlite3 *db, const char *sql)
{
    printf("%s\n", sql);
    int64_t start = esp_timer_get_time();
    int rc = sqlite3_exec(db, sql, callback, (void *)data, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        printf("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        printf("Operation done successfully\n");
    }
    printf("Time taken: %lld\n", esp_timer_get_time() - start);
    return rc;
}
int db_create(char *query)
{

    rc = db_exec(farmbox, query);
    if (rc != SQLITE_OK)
    {
        sqlite3_close(farmbox);
        return 1;
    }
    else
    {
        return rc;
    }
}
int db_insert(char *query)
{
    rc = db_exec(farmbox, query);
    if (rc != SQLITE_OK)
    {
        sqlite3_close(farmbox);
        return 1;
    }
    else
    {
        return rc;
    }
}
int db_select(char *query)
{
    rc = db_exec(farmbox, query);
    if (rc != SQLITE_OK)
    {
        sqlite3_close(farmbox);
        return 1;
    }
    else
    {
        return rc;
    }
}

void database_init(void)
{

    // remove existing file
    unlink("/spiffs/farmbox.db");

    sqlite3_initialize();

    if (db_open("/spiffs/farmbox.db", &farmbox))
        return;
}

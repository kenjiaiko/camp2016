// DB.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"
#include "DB.h"

#include "sqlite3.h"

sqlite3 *db;
char result[8192];
char *pp;

DB_API int db_open()
{
	int rc = sqlite3_open(":memory:", &db);
	if(rc){
		return -1;
	}
	return 0;
}

static int callback(void *data, int argc, char **argv, char **azColName)
{
   int i;
   //wsprintfA(result, "%s: ", (const char*)data);
   for(i=0; i<argc; i++){
      wsprintfA(pp, "%s=%s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	  pp += strlen(pp);
   }
   return 0;
}

DB_API int db_exec(char *sql, char *out)
{
	pp = result;
	char *zErrMsg = 0;
   /* Execute SQL statement */
   int rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
      sqlite3_free(zErrMsg);
	  return -1;
   }
   Sleep(500);
   memcpy(out, result, strlen(result));
   return 0;
}

DB_API int db_close()
{
	sqlite3_close(db);
	return 0;
}

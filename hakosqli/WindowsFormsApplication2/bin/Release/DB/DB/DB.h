// 以下の ifdef ブロックは DLL からのエクスポートを容易にするマクロを作成するための 
// 一般的な方法です。この DLL 内のすべてのファイルは、コマンド ラインで定義された DB_EXPORTS
// シンボルを使用してコンパイルされます。このシンボルは、この DLL を使用するプロジェクトでは定義できません。
// ソースファイルがこのファイルを含んでいる他のプロジェクトは、 
// DB_API 関数を DLL からインポートされたと見なすのに対し、この DLL は、このマクロで定義された
// シンボルをエクスポートされたと見なします。
#ifdef DB_EXPORTS
#define DB_API __declspec(dllexport)
#else
#define DB_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
              // used by C++ source code
#endif


DB_API int db_open();
DB_API int db_close();
DB_API int db_exec(char *sql, char *out);

#ifdef __cplusplus
}
#endif


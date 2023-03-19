#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include "sqlite3.h"

const char SETUP_PRAGMA[] = 
	"PRAGMA foreign_keys = ON;"
	"PRAGMA journal_mode = MEMORY;"
	"PRAGMA synchronous = OFF;"
	"PRAGMA temp_store = MEMORY;"
	"PRAGMA mmap_size = 2000000000;";

const char CREATE_SCHEMA[] = 
	"CREATE TABLE source(id INT PRIMARY KEY NOT NULL, path VARCHAR(256) NOT NULL, name VARCHAR(64) NOT NULL, size INT NOT NULL);"
	"CREATE TABLE adjacency(parent INT NOT NULL, child INT NOT NULL, depth INT NOT NULL, FOREIGN KEY(parent) REFERENCES source(id), FOREIGN KEY(child) REFERENCES source(id));"
	"CREATE UNIQUE INDEX adj_parent ON adjacency(parent, child);"
	"CREATE INDEX adj_child ON adjacency(child);";

const char INSERT_SOURCE[] = "INSERT INTO source VALUES(?1, ?2, ?3, ?4);";
const char INSERT_ADJACENCY[] = 
	"INSERT INTO adjacency SELECT ?1, ?2, 1 "
	"UNION SELECT t1.parent, t2.child, t1.depth+t2.depth+1 FROM (SELECT * FROM adjacency WHERE child = ?1) AS t1 CROSS JOIN (SELECT * FROM adjacency WHERE parent = ?2) AS t2 "
	"UNION SELECT parent, ?2, depth+1 FROM adjacency WHERE child = ?1 "
	"UNION SELECT ?1, child, depth+1 FROM adjacency WHERE parent = ?2 "
	"ON CONFLICT(parent, child) DO UPDATE SET depth = min(depth, EXCLUDED.depth);";
/*
const char INSERT_ADJACENCY[] = 
	"INSERT OR IGNORE INTO adjacency SELECT ?1, ?2, 1 "
	"UNION SELECT parent, ?2, depth+1 FROM adjacency WHERE child = ?1 "
	"UNION SELECT ?1, child, depth+1 FROM adjacency WHERE parent = ?2 "
	"UNION SELECT t1.parent, t2.child, t1.depth+t2.depth+1 FROM (SELECT * FROM adjacency WHERE child = ?1) AS t1 CROSS JOIN (SELECT * FROM adjacency WHERE parent = ?2) AS t2;";
*/
const char SELECT_SOURCE[] = "SELECT * FROM source;";
const char SELECT_SOURCE_LIKE[] = "SELECT * FROM source WHERE path LIKE ?1;";

const char TOP_INCLUDING[] = "SELECT child, path, COUNT(*) AS count, SUM(size) AS sum FROM adjacency JOIN source ON child = id GROUP BY child ORDER BY sum DESC LIMIT 1000;";
const char TOP_INCLUDED[] = "SELECT parent, path, COUNT(*) AS count, SUM(size) AS sum FROM adjacency JOIN source ON parent = id GROUP BY parent ORDER BY sum DESC LIMIT 1000;";
const char FILE_INCLUDING[] = "SELECT child, path, size, depth FROM adjacency JOIN source ON child = id WHERE parent = 68 AND depth < 3;";
const char FILE_INCLUDED[] = "SELECT parent, path, size, depth FROM adjacency JOIN source ON parent = id WHERE child = 3075 AND depth < 3;";

enum State
{
	STATE_GLOBAL = 0,
	STATE_COMMENT_C,
	STATE_COMMENT_CPP,
	STATE_INCLUDE,
	INCLUDE_GLOBAL,
	INCLUDE_SYSTEM,
	INCLUDE_CURRENT
};

const char TOK_C_BEGIN[] = "/*";
const char TOK_C_END[] = "*/";
const char TOK_CPP_BEGIN[] = "//";
const char TOK_INCLUDE[] = "#include";

typedef void (*include_cb)(const char *_name, void *_data);

bool walk_source(const char *_buffer, include_cb _callback, void *_data)
{
	char text[512];
	const char *begin;
	const char *ptr = _buffer;
	State state = STATE_GLOBAL;
	State substate = INCLUDE_GLOBAL;
	
	while(ptr)
	{
		switch(state)
		{
			case STATE_GLOBAL:
				if(*ptr == 0) { ptr = 0; }
				else if(strncmp(ptr, TOK_C_BEGIN, sizeof(TOK_C_BEGIN)-1) == 0) { state = STATE_COMMENT_C; ptr += sizeof(TOK_C_BEGIN)-1; }
				else if(strncmp(ptr, TOK_CPP_BEGIN, sizeof(TOK_CPP_BEGIN)-1) == 0) { state = STATE_COMMENT_CPP; ptr += sizeof(TOK_CPP_BEGIN)-1; }
				else if(strncmp(ptr, TOK_INCLUDE, sizeof(TOK_INCLUDE)-1) == 0) { state = STATE_INCLUDE; substate = INCLUDE_GLOBAL; ptr += sizeof(TOK_INCLUDE)-1; }
				else { ++ptr; }
				break;
			case STATE_COMMENT_C:
				ptr = strstr(ptr, TOK_C_END);
				if(ptr) { state = STATE_GLOBAL; ptr += sizeof(TOK_C_END)-1; }
				break;
			case STATE_COMMENT_CPP:
				ptr = strchr(ptr, '\n');
				if(ptr) { state = STATE_GLOBAL; ++ptr; }
				break;
			case STATE_INCLUDE:
				switch(substate)
				{
					case INCLUDE_GLOBAL:
						if(*ptr == 0) { ptr = 0; }
						else if(*ptr == '<') { substate = INCLUDE_SYSTEM; ++ptr; }
						else if(*ptr == '"') { substate = INCLUDE_CURRENT; ++ptr; }
						else if(*ptr == ' ' || *ptr == '\t' || *ptr == '\n') { ++ptr; }
						else { state = STATE_GLOBAL; }
						break;
					case INCLUDE_SYSTEM:
						begin = ptr;
						ptr = strchr(ptr, '>');
						if(ptr) { state = STATE_GLOBAL; strncpy(text, begin, ptr-begin); text[ptr-begin] = 0; _callback(text, _data); }
						break;
					case INCLUDE_CURRENT:
						begin = ptr;
						ptr = strchr(ptr, '"');
						if(ptr) { state = STATE_GLOBAL; strncpy(text, begin, ptr-begin); text[ptr-begin] = 0; _callback(text, _data); }
						break;
				}
				break;
		}
	}

	return true;
}


typedef void (*file_cb)(const char *_path, const char *_name, size_t _size, void *_data);

bool walk_directory(const char *_root, const char *_path, bool _recursive, file_cb _callback, void *_data)
{
	bool ret = false;

	char directory[512];
	strcpy(directory, _root); strcat(directory, _path);
	
	DIR *dir = opendir(directory);
	if(dir != 0)
	{
		dirent *entry = readdir(dir);
		while(entry != 0)
		{
			char path[512];
			if(entry->d_type == DT_DIR && entry->d_name[0] != '.' && _recursive)
			{
				strcpy(path, _path); strcat(path, entry->d_name); strcat(path, "/" );
				walk_directory(_root, path, _recursive, _callback, _data);
			}
			else if(entry->d_type == DT_REG)
			{
				struct stat st;
				strcpy(path, directory); strcat(path, entry->d_name);
				lstat(path, &st);

				strcpy(path, _path); strcat(path, entry->d_name);
				_callback(path, entry->d_name, st.st_size, _data);
			}
			entry = readdir(dir);
		}

		closedir(dir);
		ret = true;
	}
	
	return ret;
}


struct Source
{
	const char *root;
	sqlite3_stmt *insert_source;
	sqlite3_stmt *insert_adjacency;
	sqlite3_stmt *select_source;
	const char **extfilter;
	int id;
	int parent;
};


static void includecb(const char *_name, void *_data)
{
	int ret;
	char like[512];
	Source *source = (Source *) _data;

	strcpy(like, "%");
	strcat(like, _name);
	ret = sqlite3_reset(source->select_source);
	ret = sqlite3_bind_text(source->select_source, 1, like, -1, SQLITE_TRANSIENT);
	ret = sqlite3_step(source->select_source);

	int child = -1;
	if(ret == SQLITE_ROW)
	{
		// TODO: pick the best matching row instead of the first one
		child = sqlite3_column_int(source->select_source, 0);
	}
	else
	{
		// We didn't find a matching include source
		// We assume it's an external one and insert it to be able to reference it
		child = source->id;
		ret = sqlite3_reset(source->insert_source);
		ret = sqlite3_bind_int(source->insert_source, 1, source->id++);
		ret = sqlite3_bind_text(source->insert_source, 2, _name, -1, SQLITE_TRANSIENT);
		ret = sqlite3_bind_text(source->insert_source, 3, _name, -1, SQLITE_TRANSIENT);
		ret = sqlite3_bind_int(source->insert_source, 4, 0);
		ret = sqlite3_step(source->insert_source);
	}

	ret = sqlite3_reset(source->insert_adjacency);
	ret = sqlite3_bind_int(source->insert_adjacency, 1, source->parent);
	ret = sqlite3_bind_int(source->insert_adjacency, 2, child);
	ret = sqlite3_step(source->insert_adjacency);
}


static void filecb(const char *_path, const char *_name, size_t _size, void *_data)
{
	int ret;
	Source *source = (Source *) _data;

	const char **ext = source->extfilter;
	while(*ext) 
	{
		// TODO: optimize by removing the strlen to avoid doing it every iterations
		int len = strlen(*ext);
		const char *ptr = strstr(_name, *ext);
		if(ptr != 0 && ptr[len] == 0) break;
		else ++ext;
	}

	if(*ext)
	{
		ret = sqlite3_reset(source->insert_source);
		ret = sqlite3_bind_int(source->insert_source, 1, source->id++);
		ret = sqlite3_bind_text(source->insert_source, 2, _path, -1, SQLITE_TRANSIENT);
		ret = sqlite3_bind_text(source->insert_source, 3, _name, -1, SQLITE_TRANSIENT);
		ret = sqlite3_bind_int(source->insert_source, 4, _size);
		ret = sqlite3_step(source->insert_source);
	}
}


static int sourcecb(void *_data, int _count, char **_values, char **_headers)
{
	char path[512];
	Source *source = (Source *) _data;

	strcpy(path, source->root);
	strcat(path, _values[1]);

	printf("%s\n", path);

	// TODO: remove atoi by using sqlite3_step instead of sqlite3_exec
	source->parent = atoi(_values[0]);

	FILE *fp = fopen(path, "r");
	if(fp)
	{
		fseek(fp, 0, SEEK_END);
		size_t size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		char *buffer = (char *) malloc(size + 1); buffer[size] = 0;
		size_t ret = fread(buffer, 1, size, fp);
		fclose(fp);
		walk_source(buffer, includecb, _data);
		free(buffer);
	}

	return 0; 
}


int main(int argc, char *argv[])
{
	int ret;
	char *error;

	printf("SQLite %s\n", sqlite3_libversion());

	sqlite3 *db;
	ret = sqlite3_open("test.db", &db);

	ret = sqlite3_exec(db, SETUP_PRAGMA, 0, 0, &error);
	if(error) { printf("%s\n", error); sqlite3_free(error); }

	ret = sqlite3_exec(db, CREATE_SCHEMA, 0, 0, &error);
	if(error) { printf("%s\n", error); sqlite3_free(error); }
	
	sqlite3_stmt *stmt_insert_source;
	ret = sqlite3_prepare_v2(db, INSERT_SOURCE, -1, &stmt_insert_source, 0);
	if(ret != SQLITE_OK) printf("INSERT_SOURCE %s\n%s\n", sqlite3_errmsg(db), INSERT_SOURCE);

	sqlite3_stmt *stmt_insert_adjacency;
	ret = sqlite3_prepare_v2(db, INSERT_ADJACENCY, -1, &stmt_insert_adjacency, 0);
	if(ret != SQLITE_OK) printf("INSERT_ADJACENCY %s\n%s\n", sqlite3_errmsg(db), INSERT_ADJACENCY);

	sqlite3_stmt *stmt_select_source;
	ret = sqlite3_prepare_v2(db, SELECT_SOURCE_LIKE, -1, &stmt_select_source, 0);
	if(ret != SQLITE_OK) printf("SELECT_SOURCE_LIKE %s\n%s\n", sqlite3_errmsg(db), SELECT_SOURCE_LIKE);

	const char *extfilter[] = { ".h", ".c", ".hpp", ".cpp", ".inl", ".cxx", 0 };

	Source source;
	source.root = "/media/data/windows/games/UE_5.1/Engine/Source/Runtime/";
	source.insert_source = stmt_insert_source;
	source.insert_adjacency = stmt_insert_adjacency;
	source.select_source = stmt_select_source;
	source.extfilter = extfilter;
	source.id = 1;
	walk_directory(source.root, "", true, &filecb, &source);

	ret = sqlite3_exec(db, SELECT_SOURCE, &sourcecb, &source, &error);
	if(error) { printf("%s\n", error); sqlite3_free(error); }

	ret = sqlite3_finalize(stmt_insert_source);
	ret = sqlite3_finalize(stmt_insert_adjacency);
	ret = sqlite3_finalize(stmt_select_source);
	ret = sqlite3_close(db);

	return 0;
}


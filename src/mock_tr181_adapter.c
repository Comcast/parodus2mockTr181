/**
 * Copyright 2018 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "mock_tr181_client.h"
#include "mock_tr181_adapter.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define P2M_DB_FILE      "../../mock_tr181.json"

/* Declares unsigned char mock_tr181_json[] created from mock_tr181.json
 via xxd in the make file.
 */
#include "mock_tr181_json.h"

static char* g_mock_tr181_db_name = NULL;

//cache data instead of reading every time from filesystem
// as per requirement I am making the filesystem db Read only
//
//static int g_tr181_dirty_flag = 0;
static cJSON* g_mock_tr181_db_cache = NULL;
/*----------------------------------------------------------------------------*/
/*                 External interface functions                               */
/*----------------------------------------------------------------------------*/
/*
 * returns NULL - failure
 *         Address of cached db - success
 *
 * Init will set the db file name
 *
 */

cJSON *mock_tr181_db_init(char* db_name)
{
	if(g_mock_tr181_db_name)
	{
		free(g_mock_tr181_db_name);
		g_mock_tr181_db_name = NULL;
	}

	if(g_mock_tr181_db_cache)
	{
		//  allocated by cJSON_Parse
		cJSON_Delete(g_mock_tr181_db_cache);
		g_mock_tr181_db_cache = NULL;
	}

	if(db_name)
	{
		Info("MockTR181 DB file: \"%s\"\n", db_name);
		g_mock_tr181_db_name = strdup(db_name);
	}
	else
	{
        // Using built-in data base file
        g_mock_tr181_db_cache = cJSON_Parse((char *) &mock_tr181_json[0]);
	}

	return g_mock_tr181_db_cache;
}

cJSON* mock_tr181_db_get_instance()
{
	if(g_mock_tr181_db_cache == NULL)
	{
		char *dbData = NULL;

		if (mock_tr181_db_read(&dbData))
		{
			Info("Data from DB: %s\n", dbData);
			g_mock_tr181_db_cache = cJSON_Parse(dbData); //paramList contains TR181 db
		}
		else
		{
			Error("Failed to read from DB! creating an empty db instance.\n");
			g_mock_tr181_db_cache = cJSON_CreateArray();
		}
	}

	return g_mock_tr181_db_cache; //TODO: How to protect this from clients accidently deleting/corrupting?
}


/*
 * returns 0 - failure
 *         1 - success
 */
int mock_tr181_db_read(char **data)
{
	Info("mock_tr181_db_read() Entered\n");
	FILE *fp;
	int ch_count = 0;
	if(g_mock_tr181_db_name)
	{
		fp = fopen(g_mock_tr181_db_name, "r");
		if (fp == NULL)
		{
			Error("Failed to open db file \"%s\" !\n", g_mock_tr181_db_name);
			return 0;
		}

		Info("Opened db file \"%s\"\n", g_mock_tr181_db_name);
	}
	else
	{
		Error("No db file found or specified!\n");
		return 0;
	}

	fseek(fp, 0, SEEK_END);  //set file position to end
	ch_count = ftell(fp);    // get the file position
	fseek(fp, 0, SEEK_SET);  //set file position to start

	Info("DB file size : %d\n", ch_count);
	*data = (char *) malloc(sizeof(char) * (ch_count + 1)); //allocate memory for file size
	if (*data != NULL)
	{
		memset(*data, 0, ch_count+1);
		size_t sz = fread(*data, 1, ch_count, fp);  // copy file to memory
		if(sz == 0)
		{
			Error("Error: Read file size: %d\n", sz);
		}

		Info("Read %d bytes from db file\n", sz);
		(*data)[ch_count] = '\0';
	}
	else
	{
		Error("Failed to allocate memory of %d bytes\n", ch_count);
		return 0;
	}

	fclose(fp);
	Print("mock_tr181_db_read() Returned Success\n");

	return 1;
}

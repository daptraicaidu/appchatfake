// DbConnect.h

#pragma once
#include "sqlite3.h"

extern sqlite3* g_db;
bool ConnectDatabase();
void DisconnectDatabase();

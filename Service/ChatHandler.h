// ChatHandler.h
#pragma once

#include <string>
#include <winsock2.h>
#include "sqlite3.h" // Cần để tương tác DB


int GetUserId(const std::string& username);

void ProcessClientMessage(SOCKET senderSocket, const std::string& senderUsername, const std::string& messageLine);

void HandleGetUsersList(SOCKET senderSocket, const std::string& currentUsername);

void HandleGetHistory(SOCKET senderSocket, const std::string& senderUsername, const std::string& otherUsername);

void HandleDownloadRequest(SOCKET senderSocket, const std::string& senderUsername, const std::string& otherUsername);
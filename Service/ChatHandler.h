// ChatHandler.h
#pragma once

#include <string>
#include <winsock2.h>
#include "sqlite3.h" // Cần để tương tác DB


int GetUserId(const std::string& username);

void ProcessClientMessage(SOCKET senderSocket, const std::string& senderUsername, const std::string& messageLine);

void SendOfflineMessages(SOCKET clientSocket, const std::string& username);

void HandleGetUsersList(SOCKET senderSocket);

void HandleGetHistory(SOCKET senderSocket, const std::string& senderUsername, const std::string& otherUsername);
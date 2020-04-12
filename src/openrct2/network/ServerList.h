/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../common.h"

#include <future>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

struct json_t;
struct INetworkEndpoint;

struct ServerListEntry
{
    std::string address;
    std::string name;
    std::string description;
    std::string version;
    bool requiresPassword{};
    bool favourite{};
    uint8_t players{};
    uint8_t maxplayers{};
    bool local{};

    int32_t CompareTo(const ServerListEntry& other) const;
    bool IsVersionValid() const;

    static std::optional<ServerListEntry> FromJson(const json_t* root);
};

class ServerList
{
private:
    std::vector<ServerListEntry> _serverEntries;

    void Sort();
    std::vector<ServerListEntry> ReadFavourites() const;
    bool WriteFavourites(const std::vector<ServerListEntry>& entries) const;
    std::future<std::vector<ServerListEntry>> FetchLocalServerListAsync(const INetworkEndpoint& broadcastEndpoint) const;

public:
    ServerListEntry& GetServer(size_t index);
    size_t GetCount() const;
    void Add(const ServerListEntry& entry);
    void AddRange(const std::vector<ServerListEntry>& entries);
    void Clear();

    void ReadAndAddFavourites();
    void WriteFavourites() const;

    std::future<std::vector<ServerListEntry>> FetchLocalServerListAsync() const;
    std::future<std::vector<ServerListEntry>> FetchOnlineServerListAsync() const;
    uint32_t GetTotalPlayerCount() const;
};

class MasterServerException : public std::exception
{
public:
    rct_string_id StatusText;

    MasterServerException(rct_string_id statusText)
        : StatusText(statusText)
    {
    }
};

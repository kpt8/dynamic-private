// Copyright (c) 2018 Duality Blockchain Solutions Developers
// TODO: Add License

#ifndef DYNAMIC_DHT_SESSION_H
#define DYNAMIC_DHT_SESSION_H

#include "libtorrent/alert.hpp"
#include "libtorrent/alert_types.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/session_status.hpp"

class CChainParams;
class CConnman;
class CKeyEd25519;

static constexpr int DHT_GET_ALERT_TYPE_CODE = 75;
static constexpr int DHT_PUT_ALERT_TYPE_CODE = 76;
static constexpr int BOOTSTRAP_ALERT_TYPE_CODE = 62;

void Bootstrap();
bool LoadSessionState(libtorrent::session* dhtSession);
int SaveSessionState(libtorrent::session* dhtSession);
std::string GetSessionStatePath();

/** Start the DHT libtorrent network threads */
void StartTorrentDHTNetwork(const CChainParams& chainparams, CConnman& connman);
/** Stop the DHT libtorrent network threads */
void StopTorrentDHTNetwork();
/** Get a mutable entry in the libtorrent DHT */

void GetDHTStats(libtorrent::session_status& stats, std::vector<libtorrent::dht_lookup>& vchDHTLookup, std::vector<libtorrent::dht_routing_bucket>& vchDHTBuckets);

extern libtorrent::session *pTorrentDHTSession;

#endif // DYNAMIC_DHT_SESSION_H
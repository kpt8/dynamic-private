// Copyright (c) 2019 Duality Blockchain Solutions Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dht/session.h"

#include "dht/sessionevents.h"
#include "chainparams.h"
#include "dht/settings.h"
#include "dynode-sync.h"
#include "net.h"
#include "spork.h"
#include "util.h"
#include "utiltime.h" // for GetTimeMillis
#include "validation.h"

#include "libtorrent/hex.hpp" // for to_hex
#include "libtorrent/alert_types.hpp"
#include "libtorrent/bencode.hpp" // for bencode()
#include "libtorrent/kademlia/ed25519.hpp"
#include "libtorrent/span.hpp"

#include <boost/filesystem.hpp>

#include <cstdio> // for snprintf
#include <cinttypes> // for PRId64 et.al.
#include <cstdlib>
#include <fstream>
#include <thread>

using namespace libtorrent;

static std::shared_ptr<std::thread> pDHTTorrentThread;

static bool fShutdown;
static bool fStarted;

session *pTorrentDHTSession = NULL;

static void empty_public_key(std::array<char, 32>& public_key)
{
    for( unsigned int i = 0; i < sizeof(public_key); i++) {
        public_key[i] = 0;
    }
}

alert* WaitForResponse(session* dhtSession, const int alert_type, const std::array<char, 32>& public_key, const std::string& strSalt)
{
    LogPrint("dht", "DHTTorrentNetwork -- WaitForResponse start.\n");
    alert* ret = nullptr;
    bool found = false;
    std::array<char, 32> emptyKey;
    empty_public_key(emptyKey);
    std::string strEmpty = aux::to_hex(emptyKey);
    std::string strPublicKey = aux::to_hex(public_key);
    while (!found)
    {
        dhtSession->wait_for_alert(seconds(1));
        std::vector<alert*> alerts;
        dhtSession->pop_alerts(&alerts);
        for (std::vector<alert*>::iterator iAlert = alerts.begin(), end(alerts.end()); iAlert != end; ++iAlert)
        {
            if (!(*iAlert))
               continue;
 
            std::string strAlertMessage = (*iAlert)->message();
            int iAlertType = (*iAlert)->type();
            if ((*iAlert)->category() == 0x1) {
                LogPrint("dht", "DHTTorrentNetwork -- error alert message = %s, alert_type =%d\n", strAlertMessage, iAlertType);
            }
            else if ((*iAlert)->category() == 0x80) {
                LogPrint("dht", "DHTTorrentNetwork -- progress alert message = %s, alert_type =%d\n", strAlertMessage, iAlertType);
            }
            else if ((*iAlert)->category() == 0x200) {
                LogPrint("dht", "DHTTorrentNetwork -- performance warning alert message = %s, alert_type =%d\n", strAlertMessage, iAlertType);
            }
            else if ((*iAlert)->category() == 0x400) {
                LogPrint("dht", "DHTTorrentNetwork -- dht alert message = %s, alert_type =%d\n", strAlertMessage, iAlertType);
            }
            else {
                LogPrint("dht", "DHTTorrentNetwork -- dht other alert message = %s, alert_type =%d\n", strAlertMessage, iAlertType);
            }
            if (iAlertType != alert_type)
            {
                continue;
            }
            
            size_t posKey = strAlertMessage.find("key=" + strPublicKey);
            size_t posSalt = strAlertMessage.find("salt=" + strSalt);
            if (strPublicKey == strEmpty || (posKey != std::string::npos && posSalt != std::string::npos)) {
                LogPrint("dht", "DHTTorrentNetwork -- wait alert complete. message = %s, alert_type =%d\n", strAlertMessage, iAlertType);
                ret = *iAlert;
                found = true;
            }
        }
        if (fShutdown)
            return ret;
    }
    return ret;
}

bool Bootstrap()
{
    LogPrintf("dht", "DHTTorrentNetwork -- bootstrapping.\n");
    const int64_t timeout = 30000; // 30 seconds
    const int64_t startTime = GetTimeMillis();
    while (timeout > GetTimeMillis() - startTime)
    {
        std::vector<CEvent> events;
        MilliSleep(1500);
        if (GetLastTypeEvent(DHT_BOOTSTRAP_ALERT_TYPE_CODE, startTime, events)) 
        {
            if (events.size() > 0 ) {
                LogPrint("dht", "DHTTorrentNetwork -- Bootstrap successful.\n");
                return true;
            }
        }
    }
    LogPrint("dht", "DHTTorrentNetwork -- Bootstrap failed after 30 second timeout.\n");
    return false;
}

std::string GetSessionStatePath()
{
    boost::filesystem::path path = GetDataDir() / "dht_state.dat";
    return path.string();
}

int SaveSessionState(session* dhtSession)
{
    entry torrentEntry;
    dhtSession->save_state(torrentEntry, session::save_dht_state);
    std::vector<char> state;
    bencode(std::back_inserter(state), torrentEntry);
    std::fstream f(GetSessionStatePath().c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
    f.write(state.data(), state.size());
    LogPrint("dht", "DHTTorrentNetwork -- SaveSessionState complete.\n");
    return 0;
}

bool LoadSessionState(session* dhtSession)
{
    std::fstream f(GetSessionStatePath().c_str(), std::ios_base::in | std::ios_base::binary | std::ios_base::ate);

    auto const size = f.tellg();
    if (static_cast<int>(size) <= 0) return false;
    f.seekg(0, std::ios_base::beg);

    std::vector<char> state;
    state.resize(static_cast<std::size_t>(size));

    f.read(state.data(), state.size());
    if (f.fail())
    {
        LogPrint("dht", "DHTTorrentNetwork -- LoadSessionState failed to read dht-state.log\n");
        return false;
    }

    bdecode_node e;
    error_code ec;
    bdecode(state.data(), state.data() + state.size(), e, ec);
    if (ec) {
        LogPrint("dht", "DHTTorrentNetwork -- LoadSessionState failed to parse dht-state.log file: (%d) %s\n", ec.value(), ec.message());
        return false;
    }
    else
    {
        LogPrint("dht", "DHTTorrentNetwork -- LoadSessionState load dht state from dht-state.log\n");
        dhtSession->load_state(e);
    }
    return true;
}

void static DHTTorrentNetwork(const CChainParams& chainparams, CConnman& connman)
{
    LogPrint("dht", "DHTTorrentNetwork -- starting\n");
    SetThreadPriority(THREAD_PRIORITY_LOWEST);
    RenameThread("dht-session");
    
    try {
        CDHTSettings settings;
        // Busy-wait for the network to come online so we get a full list of Dynodes
        do {
            bool fvNodesEmpty = connman.GetNodeCount(CConnman::CONNECTIONS_ALL) == 0;
            if (!fvNodesEmpty && !IsInitialBlockDownload() && dynodeSync.IsSynced() && 
                dynodeSync.IsBlockchainSynced() && sporkManager.IsSporkActive(SPORK_30_ACTIVATE_BDAP))
                    break;

            MilliSleep(1000);
            if (fShutdown)
                return;

        } while (true);
        
        fStarted = true;
        LogPrintf("DHTTorrentNetwork -- started\n");
        // with current peers and Dynodes
        settings.LoadSettings();
        pTorrentDHTSession = settings.GetSession();
        
        if (!pTorrentDHTSession)
            throw std::runtime_error("DHT Torrent network bootstraping error.");
        
        StartEventListener(pTorrentDHTSession);
    }
    catch (const std::runtime_error& e)
    {
        fShutdown = true;
        LogPrintf("DHTTorrentNetwork -- runtime error: %s\n", e.what());
        return;
    }
}

void StopTorrentDHTNetwork()
{
    LogPrintf("DHTTorrentNetwork -- StopTorrentDHTNetwork begin.\n");
    fShutdown = true;
    MilliSleep(300);
    StopEventListener();
    MilliSleep(30);
    if (pDHTTorrentThread != NULL)
    {
        LogPrint("dht", "DHTTorrentNetwork -- StopTorrentDHTNetwork trying to stop.\n");
        if (fStarted) { 
            libtorrent::session_params params;
            params.settings.set_bool(settings_pack::enable_dht, false);
            params.settings.set_int(settings_pack::alert_mask, 0x0);
            pTorrentDHTSession->apply_settings(params.settings);
            pTorrentDHTSession->abort();
        }
        pDHTTorrentThread->join();
        LogPrint("dht", "DHTTorrentNetwork -- StopTorrentDHTNetwork abort.\n");
    }
    else {
        LogPrint("dht", "DHTTorrentNetwork --StopTorrentDHTNetwork pDHTTorrentThreads is null.  Stop not needed.\n");
    }
    pDHTTorrentThread = NULL;
    LogPrintf("DHTTorrentNetwork -- Stopped.\n");
}

void StartTorrentDHTNetwork(const CChainParams& chainparams, CConnman& connman)
{
    LogPrint("dht", "DHTTorrentNetwork -- Log file = %s.\n", GetSessionStatePath());
    fShutdown = false;
    fStarted = false;
    if (pDHTTorrentThread != NULL)
         StopTorrentDHTNetwork();

    pDHTTorrentThread = std::make_shared<std::thread>(std::bind(&DHTTorrentNetwork, std::cref(chainparams), std::ref(connman)));
}

void GetDHTStats(session_status& stats, std::vector<dht_lookup>& vchDHTLookup, std::vector<dht_routing_bucket>& vchDHTBuckets)
{
    LogPrint("dht", "DHTTorrentNetwork -- GetDHTStats started.\n");

    if (!pTorrentDHTSession) {
        return;
    }

    if (!pTorrentDHTSession->is_dht_running()) {
        return;
        //LogPrint("dht", "DHTTorrentNetwork -- GetDHTStats Restarting DHT.\n");
        //if (!LoadSessionState(pTorrentDHTSession)) {
        //    LogPrint("dht", "DHTTorrentNetwork -- GetDHTStats Couldn't load previous settings.  Trying to bootstrap again.\n");
        //    Bootstrap();
        //}
        //else {
        //    LogPrint("dht", "DHTTorrentNetwork -- GetDHTStats setting loaded from file.\n");
        //}
    }
    else {
        LogPrint("dht", "DHTTorrentNetwork -- GetDHTStats DHT already running.  Bootstrap not needed.\n");
    }

    pTorrentDHTSession->post_dht_stats();
    //get alert from map
    //alert* dhtAlert = WaitForResponse(pTorrentDHTSession, dht_stats_alert::alert_type);
    //dht_stats_alert* dhtStatsAlert = alert_cast<dht_stats_alert>(dhtAlert);
    //vchDHTLookup = dhtStatsAlert->active_requests;
    //vchDHTBuckets = dhtStatsAlert->routing_table;
    stats = pTorrentDHTSession->status();
}
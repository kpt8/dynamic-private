// Copyright (c) 2016-2018 Duality Blockchain Solutions Developers
// Copyright (c) 2017-2018 The Particl Core developers
// Copyright (c) 2014-2018 The Dash Core Developers
// Copyright (c) 2009-2018 The Bitcoin Developers
// Copyright (c) 2009-2018 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DYNAMIC_WALLET_WALLETDB_H
#define DYNAMIC_WALLET_WALLETDB_H

#include "amount.h"
#include "hdchain.h"
#include "key.h"
#include "wallet/db.h"

#include <list>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

static const bool DEFAULT_FLUSHWALLET = true;

class CAccount;
class CAccountingEntry;
class CKeyEd25519;
struct CBlockLocator;
class CKeyPool;
class CMasterKey;
class CScript;
class CStealthAddressIndexed;
class CStealthKeyMetadata;
class CWallet;
class CWalletTx;
class uint160;
class uint256;

/** Error statuses for the wallet database */
enum DBErrors {
    DB_LOAD_OK,
    DB_CORRUPT,
    DB_NONCRITICAL_ERROR,
    DB_TOO_NEW,
    DB_LOAD_FAIL,
    DB_NEED_REWRITE
};

class CKeyMetadata
{
public:
    static const int CURRENT_VERSION = 1;
    int nVersion;
    int64_t nCreateTime; // 0 means unknown

    CKeyMetadata()
    {
        SetNull();
    }
    CKeyMetadata(int64_t nCreateTime_)
    {
        SetNull();
        nCreateTime = nCreateTime_;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(this->nVersion);
        READWRITE(nCreateTime);
    }

    void SetNull()
    {
        nVersion = CKeyMetadata::CURRENT_VERSION;
        nCreateTime = 0;
    }
};

/** Access to the wallet database */
class CWalletDB : public CDB
{
public:
    CWalletDB(const std::string& strFilename, const char* pszMode = "r+", bool fFlushOnClose = true) : CDB(strFilename, pszMode, fFlushOnClose)
    {
    }

    bool WriteName(const std::string& strAddress, const std::string& strName);
    bool EraseName(const std::string& strAddress);

    bool WritePurpose(const std::string& strAddress, const std::string& purpose);
    bool ErasePurpose(const std::string& strAddress);

    bool WriteTx(const CWalletTx& wtx);
    bool EraseTx(uint256 hash);

    bool WriteDHTKey(const CKeyEd25519& key, const std::vector<unsigned char>& vchPubKey, const CKeyMetadata& keyMeta);

    bool WriteKey(const CPubKey& vchPubKey, const CPrivKey& vchPrivKey, const CKeyMetadata& keyMeta);
    bool WriteCryptedKey(const CPubKey& vchPubKey, const std::vector<unsigned char>& vchCryptedSecret, const CKeyMetadata& keyMeta);
    bool WriteCryptedDHTKey(const std::vector<unsigned char>& vchPubKey, const std::vector<unsigned char>& vchCryptedSecret, const CKeyMetadata& keyMeta);
    bool WriteMasterKey(unsigned int nID, const CMasterKey& kMasterKey);

    bool WriteCScript(const uint160& hash, const CScript& redeemScript);

    bool WriteWatchOnly(const CScript& script, const CKeyMetadata& keymeta);
    bool EraseWatchOnly(const CScript& script);

    bool WriteBestBlock(const CBlockLocator& locator);
    bool ReadBestBlock(CBlockLocator& locator);

    bool WriteOrderPosNext(int64_t nOrderPosNext);

    bool WriteDefaultKey(const CPubKey& vchPubKey);

    bool ReadPool(int64_t nPool, CKeyPool& keypool);
    bool WritePool(int64_t nPool, const CKeyPool& keypool);
    bool ErasePool(int64_t nPool);

    bool WriteMinVersion(int nVersion);

    /// This writes directly to the database, and will not update the CWallet's cached accounting entries!
    /// Use wallet.AddAccountingEntry instead, to write *and* update its caches.
    bool WriteAccountingEntry(const uint64_t nAccEntryNum, const CAccountingEntry& acentry);
    bool WriteAccountingEntry_Backend(const CAccountingEntry& acentry);
    bool ReadAccount(const std::string& strAccount, CAccount& account);
    bool WriteAccount(const std::string& strAccount, const CAccount& account);

    /// Write destination data key,value tuple to database
    bool WriteDestData(const std::string& address, const std::string& key, const std::string& value);
    /// Erase destination data tuple from wallet database
    bool EraseDestData(const std::string& address, const std::string& key);

    CAmount GetAccountCreditDebit(const std::string& strAccount);
    void ListAccountCreditDebit(const std::string& strAccount, std::list<CAccountingEntry>& acentries);

    DBErrors LoadWallet(CWallet* pwallet);
    DBErrors FindWalletTx(CWallet* pwallet, std::vector<uint256>& vTxHash, std::vector<CWalletTx>& vWtx);
    DBErrors ZapWalletTx(CWallet* pwallet, std::vector<CWalletTx>& vWtx);
    DBErrors ZapSelectTx(CWallet* pwallet, std::vector<uint256>& vHashIn, std::vector<uint256>& vHashOut);
    static bool Recover(CDBEnv& dbenv, const std::string& filename, bool fOnlyKeys);
    static bool Recover(CDBEnv& dbenv, const std::string& filename);

    //! write the hdchain model (external chain child index counter)
    bool WriteHDChain(const CHDChain& chain);
    bool WriteCryptedHDChain(const CHDChain& chain);
    bool WriteHDPubKey(const CHDPubKey& hdPubKey, const CKeyMetadata& keyMeta);

    static void IncrementUpdateCounter();
    static unsigned int GetUpdateCounter();
    //! Begin add for stealth address transactions
    bool TxnBegin(); //! Begin a new transaction
    bool ReadExtKey(const CKeyID& identifier, CStoredExtKey& ek32);
    bool WriteExtKey(const CKeyID& identifier, const CStoredExtKey& ek32);
    bool ReadNamedExtKeyId(const std::string& name, CKeyID& identifier);
    bool WriteNamedExtKeyId(const std::string& name, const CKeyID& identifier);
    bool ReadExtAccount(const CKeyID& identifier, CExtKeyAccount& ekAcc);
    bool WriteExtAccount(const CKeyID& identifier, const CExtKeyAccount& ekAcc);
    bool ReadExtKeyIndex(uint32_t id, CKeyID& identifier);
    bool WriteExtKeyIndex(uint32_t id, const CKeyID& identifier);
    bool ReadFlag(const std::string& name, int32_t& nValue);
    bool WriteFlag(const std::string& name, const int32_t& nValue);
    bool ReadExtStealthKeyPack(const CKeyID& identifier, const uint32_t nPack, std::vector<CEKAStealthKeyPack>& aksPak);
    bool WriteExtStealthKeyPack(const CKeyID& identifier, const uint32_t nPack, const std::vector<CEKAStealthKeyPack>& aksPak);
    bool WriteStealthKeyMeta(const CKeyID& keyId, const CStealthKeyMetadata& sxKeyMeta);
    bool EraseStealthKeyMeta(const CKeyID& keyId);
    bool ReadStealthAddressIndex(uint32_t id, CStealthAddressIndexed& sxi);
    bool WriteStealthAddressIndex(const uint32_t id, const CStealthAddressIndexed& sxi);
    bool ReadStealthAddressIndexReverse(const uint160& hash, uint32_t& id);
    bool WriteStealthAddressIndexReverse(const uint160& hash, const uint32_t id);
    bool ReadStealthAddressLink(const CKeyID& keyId, uint32_t& id);
    bool WriteStealthAddressLink(const CKeyID& keyId, const uint32_t id);
    bool ReadExtStealthKeyChildPack(const CKeyID& identifier, const uint32_t nPack, std::vector<CEKASCKeyPack>& asckPak);
    bool WriteExtStealthKeyChildPack(const CKeyID& identifier, const uint32_t nPack, const std::vector<CEKASCKeyPack>& asckPak);
    bool ReadExtKeyPack(const CKeyID& identifier, const uint32_t nPack, std::vector<CEKAKeyPack>& ekPak);
    bool WriteExtKeyPack(const CKeyID& identifier, const uint32_t nPack, const std::vector<CEKAKeyPack>& ekPak);
    bool LoadExtKeyAccounts(std::vector<std::pair<CKeyID, CExtKeyAccount*>>& vExtKeyAccount, int64_t& nTimeFirstKey);
    bool LoadExtKeyPacks(std::vector<std::pair<CKeyID, std::vector<CEKAKeyPack>>>& vExtKeyAccount);
    bool LoadExtStealthKeyPacks(std::vector<std::pair<CKeyID, std::vector<CEKAStealthKeyPack>>>& vExtStealthKeyPacks);
    bool LoadSharedStealthKeyPacks(std::vector<std::pair<CKeyID, std::vector<CEKASCKeyPack>>>& vStealthSharedKeyPacks);
    bool LoadStealthKeyAddresses(std::vector<std::pair<CKeyID, CStealthAddress>>& vStealthAddresses);
    //! End add for stealth address transactions

private:
    CWalletDB(const CWalletDB&);
    void operator=(const CWalletDB&);
};

void ThreadFlushWalletDB();

#endif // DYNAMIC_WALLET_WALLETDB_H

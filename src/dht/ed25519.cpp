
// Copyright (c) 2018 Duality Blockchain Solutions Developers
// TODO: Add License

#include "ed25519.h"

#include "hash.h"
#include "key.h"
#include "random.h"
#include "util.h"

#include <libtorrent/ed25519.hpp>
#include <libtorrent/hex.hpp>
#include <libtorrent/kademlia/ed25519.hpp>
#include <libtorrent/kademlia/types.hpp>

#include <array>
#include <assert.h>
#include <tuple>

using namespace libtorrent;

static ed25519_context* ed25519_context_sign = NULL;

// TODO (BDAP): Implement check Ed25519 keys

CKeyEd25519::CKeyEd25519(const std::array<char, 32>& _seed)
{
    seed = _seed;
    std::tuple<dht::public_key, dht::secret_key> keyPair = dht::ed25519_create_keypair(seed);
    {
        dht::secret_key sk = std::get<1>(keyPair);
        privateKey = sk.bytes;
    }
    {
        dht::public_key pk = std::get<0>(keyPair);
        publicKey = pk.bytes;
    }
}

CKeyEd25519::CKeyEd25519(const CKey& key)
{
    // TODO (DHT): Better way to convert secp256k1 to ed25519 key.
    CPrivKey privKey = key.GetPrivKey();
    assert(privKey.size() >= 190);
    
    unsigned int counter = 8;
    for(unsigned int i = 0; i < 24; i++) {
        seed[i] = privKey[counter];
        counter++;
        //LogPrintf("CKeyEd25519::CKeyEd25519 key size = %u. value = %d \n", i, privKey[counter]);
    }
    counter = 182;
    for(unsigned int i = 0; i < 8; i++) {
        seed[i + 24] = privKey[counter];
        counter++;
        //LogPrintf("CKeyEd25519::CKeyEd25519 key size = %u. value = %d \n", i + 24, privKey[counter]);
    }

    std::tuple<dht::public_key, dht::secret_key> keyPair = dht::ed25519_create_keypair(seed);
    {
        dht::secret_key sk = std::get<1>(keyPair);
        privateKey = sk.bytes;
    }
    {
        dht::public_key pk = std::get<0>(keyPair);
        publicKey = pk.bytes;
    }
}

static std::string StringFromVch(const std::vector<unsigned char>& vch) {
    std::string res;
    std::vector<unsigned char>::const_iterator vi = vch.begin();
    while (vi != vch.end()) {
        res += (char) (*vi);
        vi++;
    }
    return res;
}

CKeyEd25519::CKeyEd25519(const std::vector<unsigned char>& _seed)
{
    std::string strSeed = StringFromVch(_seed);
    aux::from_hex(strSeed, seed.data());
    if (sizeof(_seed) == 32) {
        for(unsigned int i = 0; i < sizeof(_seed); i++) {
            seed[i] = _seed[i];
        }
    }
    
    std::tuple<dht::public_key, dht::secret_key> keyPair = dht::ed25519_create_keypair(seed);
    {
        dht::secret_key sk = std::get<1>(keyPair);
        privateKey = sk.bytes;
    }
    {
        dht::public_key pk = std::get<0>(keyPair);
        publicKey = pk.bytes;
    }
}

//! Generate a new private key using LibTorrent's Ed25519
void CKeyEd25519::MakeNewKeyPair()
{
    // Load seed
    seed = dht::ed25519_create_seed();
    // Load the new ed25519 private key
    std::tuple<dht::public_key, dht::secret_key> newKeyPair = dht::ed25519_create_keypair(seed); 
    {
        dht::secret_key sk = std::get<1>(newKeyPair);
        privateKey = sk.bytes;
    }
    // Load the new ed25519 public key
    {
        dht::public_key pk = std::get<0>(newKeyPair);
        publicKey = pk.bytes;
    }
}

std::string CKeyEd25519::GetPrivKeyString() const
{
    return aux::to_hex(privateKey);
}

std::string CKeyEd25519::GetPubKeyString() const
{
    return aux::to_hex(publicKey);
}

std::string CKeyEd25519::GetPrivSeedString() const
{
    return aux::to_hex(seed);
}

std::vector<unsigned char> CKeyEd25519::GetPrivKey() const
{
    std::string strPrivateKey = GetPrivKeyString();
    return std::vector<unsigned char>(strPrivateKey.begin(), strPrivateKey.end());
}

std::vector<unsigned char> CKeyEd25519::GetPubKey() const
{
    std::string strPublicKey = GetPubKeyString();
    return std::vector<unsigned char>(strPublicKey.begin(), strPublicKey.end());
}

std::vector<unsigned char> CKeyEd25519::GetPrivSeed() const
{
    std::string strPrivateSeedKey = GetPrivSeedString();
    return std::vector<unsigned char>(strPrivateSeedKey.begin(), strPrivateSeedKey.end());
}

void ECC_Ed25519_Start() 
{
    assert(ed25519_context_sign == NULL);
    ed25519_context* ctx = new ed25519_context();
    assert(ctx != NULL);
    {
        ctx->seed = dht::ed25519_create_seed();
        std::string strSeed = aux::to_hex(ctx->seed);
    }
    ed25519_context_sign = ctx;
}

/*
bool ECC_Ed25519_InitSanityCheck() 
{
    CKeyEd25519 key;
    key.MakeNewKey(true);
    CPubKeyEd25519 pubkey = key.GetPubKey();
    return key.VerifyPubKey(pubkey);
}
*/

void ECC_Ed25519_Stop() 
{
    ed25519_context *ctx = ed25519_context_sign;
    std::string strSeed = aux::to_hex(ctx->seed);
    ctx->SetNull();
    ed25519_context_sign = NULL;
    strSeed = aux::to_hex(ctx->seed);
    assert(ed25519_context_sign == NULL);
}
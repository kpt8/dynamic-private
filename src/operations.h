// Copyright (c) 2017 Duality Blockchain Solutions Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <string>
#include <vector>

class CDynamicAddress;

class HexFunctions
{
public:
    std::string StringToHex(std::string input);
    std::string HexToString(std::string hex);

    void ConvertToHex(std::string& input);
    void ConvertToString(std::string& input);

};

void ScrubString(std::string& input, bool forInteger = false);
void SeparateString(std::string input, std::vector<std::string>& output, bool subDelimiter = false);
void SeparateFluidOpString(std::string input, std::vector<std::string>& output);
std::string StitchString(std::string stringOne, std::string stringTwo, bool subDelimiter = false);
std::string StitchString(std::string stringOne, std::string stringTwo, std::string stringThree, bool subDelimiter = false);
std::string GetRidOfScriptStatement(std::string input, int position = 1);

extern std::string PrimaryDelimiter;
extern std::string SubDelimiter;
extern std::string SignatureDelimiter;

class COperations : public HexFunctions
{
public:
    bool VerifyAddressOwnership(CDynamicAddress dynamicAddress);
    bool SignTokenMessage(CDynamicAddress address, std::string unsignedMessage, std::string& stitchedMessage, bool stitch = true);
    bool GenericSignMessage(const std::string message, std::string& signedString, CDynamicAddress signer);
};

#endif // OPERATIONS_H

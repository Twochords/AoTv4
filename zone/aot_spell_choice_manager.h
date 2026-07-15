#pragma once

#include <vector>
#include <cstdint>

class Client;

class SpellChoiceManager
{
public:

    static SpellChoiceManager& Instance();

    void CreateOffer(
        Client* client,
        const std::vector<uint32_t>& spells
    );

    bool HasPendingChoice(
        Client* client
    );

    bool ChooseSpell(
        Client* client,
        uint32_t spell_id
    );

    bool IsValidChoice(
        Client* client,
        uint32_t spell_id
    );

    void Clear(
        Client* client
    );

private:

    SpellChoiceManager() = default;

};
/*
 * This file is part of the CMaNGOS Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Common.h"
#include "Server/Opcodes.h"
#include "Log.h"
#include "Entities/Player.h"
#include "Server/WorldPacket.h"
#include "Server/WorldSession.h"

void WorldSession::HandleLearnTalentOpcode(WorldPacket& recv_data)
{
    uint32 talent_id, requested_rank;
    recv_data >> talent_id >> requested_rank;

    _player->LearnTalent(talent_id, requested_rank);
    _player->SendTalentsInfoData(false);

    // if player has a pet, update owner talent auras
    if (_player->GetPet())
        _player->GetPet()->CastOwnerTalentAuras();
}

void WorldSession::HandleLearnPreviewTalents(WorldPacket& recvPacket)
{
    DEBUG_LOG("CMSG_LEARN_PREVIEW_TALENTS");

    uint32 talentsCount;
    recvPacket >> talentsCount;

    uint32 talentId, talentRank;

    for (uint32 i = 0; i < talentsCount; ++i)
    {
        recvPacket >> talentId >> talentRank;

        _player->LearnTalent(talentId, talentRank);
    }

    _player->SendTalentsInfoData(false);

    // if player has a pet, update owner talent auras
    if (_player->GetPet())
        _player->GetPet()->CastOwnerTalentAuras();
}

void WorldSession::HandleTalentWipeConfirmOpcode(WorldPacket& recv_data)
{
    DETAIL_LOG("MSG_TALENT_WIPE_CONFIRM");
    ObjectGuid guid;
    recv_data >> guid;

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_TRAINER);
    if (!unit)
    {
        DEBUG_LOG("WORLD: HandleTalentWipeConfirmOpcode - %s not found or you can't interact with him.", guid.GetString().c_str());
        return;
    }

    if (!unit->CanTrainAndResetTalentsOf(_player))
        return;

    if (!(_player->resetTalents()))
    {
        WorldPacket data(MSG_TALENT_WIPE_CONFIRM, 8 + 4);   // you have not any talent
        data << uint64(0);
        data << uint32(0);
        SendPacket(data);
        return;
    }

    _player->SendTalentsInfoData(false);
    unit->CastSpell(_player, 14867, TRIGGERED_OLD_TRIGGERED);                  // spell: "Untalent Visual Effect"

    if (_player->GetPet())
        _player->GetPet()->CastOwnerTalentAuras();
}

void WorldSession::HandleUnlearnSkillOpcode(WorldPacket& recv_data)
{
    uint32 skill_id;
    recv_data >> skill_id;

    Player* player = GetPlayer();

    // Check if unlearnable
    if (!player->GetSkillInfo(uint16(skill_id), ([] (SkillRaceClassInfoEntry const& entry) { return (entry.flags & SKILL_FLAG_CAN_UNLEARN); })))
        return;

    player->SetSkillStep(uint16(skill_id), 0);
}

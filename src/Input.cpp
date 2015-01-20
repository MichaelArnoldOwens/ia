#include "Init.h"

#include <memory>

#include "ActorPlayer.h"
#include "Log.h"
#include "MapTravel.h"
#include "Input.h"
#include "Reload.h"
#include "Kick.h"
#include "Render.h"
#include "Close.h"
#include "JamWithSpike.h"
#include "Pickup.h"
#include "InventoryHandling.h"
#include "Marker.h"
#include "Map.h"
#include "DungeonMaster.h"
#include "PlayerSpellsHandling.h"
#include "Manual.h"
#include "CharacterDescr.h"
#include "Query.h"
#include "SaveHandling.h"
#include "ItemFactory.h"
#include "ActorFactory.h"
#include "ActorMon.h"
#include "PlayerBon.h"
#include "CreateCharacter.h"
#include "Disarm.h"
#include "SdlWrapper.h"
#include "Popup.h"
#include "Look.h"
#include "Attack.h"
#include "Throwing.h"
#include "Utils.h"

using namespace std;

namespace Input
{

namespace
{

SDL_Event* sdlEvent_ = nullptr;

void queryQuit()
{
    const vector<string> quitChoices = vector<string> {"yes", "no"};
    const int QUIT_CHOICE = Popup::showMenuMsg(
                                "Save and highscore are not kept.",
                                false, quitChoices, "Quit the current game?");
    if (QUIT_CHOICE == 0)
    {
        Init::quitToMainMenu = true;
        Render::clearScreen();
        Render::updateScreen();
    }
}

} //Namespace

void init()
{
    if (!sdlEvent_) {sdlEvent_ = new SDL_Event;}
    setKeyRepeatDelays();
}

void cleanup()
{
    if (sdlEvent_)
    {
        delete sdlEvent_;
        sdlEvent_ = nullptr;
    }
}

void handleMapModeKeyPress(const KeyData& d)
{
    //----------------------------------- MOVEMENT
    if (d.sdlKey == SDLK_RIGHT            || d.key == '6' || d.key == 'l')
    {
        if (Map::player->isAlive())
        {
            Log::clearLog();
            if (d.isShiftHeld)
            {
                Map::player->moveDir(Dir::upRight);
            }
            else if (d.isCtrlHeld)
            {
                Map::player->moveDir(Dir::downRight);
            }
            else
            {
                Map::player->moveDir(Dir::right);
            }
        }
        clearEvents();
        return;
    }
    else if (d.sdlKey == SDLK_DOWN      || d.key == '2' || d.key == 'j')
    {
        if (Map::player->isAlive())
        {
            Log::clearLog();
            Map::player->moveDir(Dir::down);
        }
        clearEvents();
        return;
    }
    else if (d.sdlKey == SDLK_LEFT      || d.key == '4' || d.key == 'h')
    {
        if (Map::player->isAlive())
        {
            Log::clearLog();
            if (d.isShiftHeld)
            {
                Map::player->moveDir(Dir::upLeft);
            }
            else if (d.isCtrlHeld)
            {
                Map::player->moveDir(Dir::downLeft);
            }
            else
            {
                Map::player->moveDir(Dir::left);
            }
        }
        clearEvents();
        return;
    }
    else if (d.sdlKey == SDLK_UP        || d.key == '8' || d.key == 'k')
    {
        if (Map::player->isAlive())
        {
            Log::clearLog();
            Map::player->moveDir(Dir::up);
        }
        clearEvents();
        return;
    }
    else if (d.sdlKey == SDLK_PAGEUP    || d.key == '9' || d.key == 'u')
    {
        if (Map::player->isAlive())
        {
            Log::clearLog();
            Map::player->moveDir(Dir::upRight);
        }
        clearEvents();
        return;
    }
    else if (d.sdlKey == SDLK_PAGEDOWN  || d.key == '3' || d.key == 'n')
    {
        if (Map::player->isAlive())
        {
            Log::clearLog();
            Map::player->moveDir(Dir::downRight);
        }
        clearEvents();
        return;
    }
    else if (d.sdlKey == SDLK_END       || d.key == '1' || d.key == 'b')
    {
        if (Map::player->isAlive())
        {
            Log::clearLog();
            Map::player->moveDir(Dir::downLeft);
        }
        clearEvents();
        return;
    }
    else if (d.sdlKey == SDLK_HOME      || d.key == '7' || d.key == 'y')
    {
        if (Map::player->isAlive())
        {
            Log::clearLog();
            Map::player->moveDir(Dir::upLeft);
        }
        clearEvents();
        return;
    }
    else if (d.key == '5' || d.key == '.')
    {
        if (Map::player->isAlive())
        {
            Log::clearLog();

            if (PlayerBon::traits[int(Trait::steadyAimer)])
            {
                PropHandler& propHlr = Map::player->getPropHandler();

                int nrTurnsAimingOld = 0;

                if (PlayerBon::traits[int(Trait::sharpShooter)])
                {
                    Prop* const propAimingOld = propHlr.getProp(PropId::aiming, PropSrc::applied);
                    if (propAimingOld)
                    {
                        nrTurnsAimingOld = static_cast<PropAiming*>(propAimingOld)->nrTurnsAiming;
                    }
                }

                PropAiming* const aiming = new PropAiming(PropTurns::specific, 1);
                aiming->nrTurnsAiming += nrTurnsAimingOld;
                propHlr.tryApplyProp(aiming);
            }
            Map::player->moveDir(Dir::center);
        }
        clearEvents();
        return;
    }

    //----------------------------------- MANUAL
    else if (d.key == '?')
    {
        Log::clearLog();
        Manual::run();
        clearEvents();
        return;
    }

    //----------------------------------- OPTIONS
    else if (d.key == '=')
    {
        Log::clearLog();
        Config::runOptionsMenu();
        clearEvents();
        return;
    }

    //----------------------------------- RELOAD
    else if (d.key == 'r')
    {
        Log::clearLog();
        if (Map::player->isAlive())
        {
            Reload::reloadWieldedWpn(*(Map::player));
        }
        clearEvents();
        return;
    }

    //----------------------------------- KICK
    else if ((d.key == 'q'))
    {
        Log::clearLog();
        if (Map::player->isAlive())
        {
            Kick::playerKick();
            Render::drawMapAndInterface();
        }
        clearEvents();
        return;
    }

    //----------------------------------- CLOSE
    else if (d.key == 'c')
    {
        Log::clearLog();
        if (Map::player->isAlive())
        {
            Close::playerClose();
        }
        clearEvents();
        return;
    }

    //----------------------------------- JAM
    else if (d.key == 'D')
    {
        Log::clearLog();
        if (Map::player->isAlive())
        {
            JamWithSpike::playerJam();
        }
        clearEvents();
        return;
    }

    //----------------------------------- DISARM
    else if (d.key == 'd')
    {
        Log::clearLog();
        if (Map::player->isAlive())
        {
            Disarm::playerDisarm();
        }
        clearEvents();
        return;
    }

    //----------------------------------- UNLOAD AMMO FROM GROUND
    else if (d.key == 'G')
    {
        Log::clearLog();
        if (Map::player->isAlive())
        {
            ItemPickup::tryUnloadWpnOrPickupAmmo();
        }
        clearEvents();
        return;
    }

    //----------------------------------- AIM/FIRE FIREARM
    else if (d.key == 'f')
    {
        Log::clearLog();

        if (Map::player->isAlive() && Map::player->getPropHandler().allowAttackRanged(true))
        {
            auto* const item = Map::player->getInv().getItemInSlot(SlotId::wielded);

            if (item)
            {
                const ItemDataT& itemData = item->getData();

                if (itemData.ranged.isRangedWpn)
                {
                    auto* wpn = static_cast<Wpn*>(item);

                    //TODO: Quick hack for the Mi-go gun, this should not be here (refactor)
                    if (
                        wpn->getData().id == ItemId::migoGun  &&
                        wpn->nrAmmoLoaded == 0                &&
                        Map::player->getHp() > 1)
                    {
                        const string wpnName = wpn->getName(ItemRefType::plain,
                                                            ItemRefInf::none);

                        Log::addMsg("The " + wpnName + "draws power from my essence!",
                                    clrMsgBad);

                        Render::drawMapAndInterface();
                        ++wpn->nrAmmoLoaded;
                        Map::player->hit(1, DmgType::pure);
                        return;
                    }

                    if (wpn->nrAmmoLoaded >= 1 || itemData.ranged.hasInfiniteAmmo)
                    {
                        auto onMarkerAtPos = [&](const Pos & p)
                        {
                            Log::clearLog();
                            Look::printLocationInfoMsgs(p);

                            auto* const actor = Utils::getActorAtPos(p);

                            if (actor && !actor->isPlayer())
                            {
                                bool tgtProps[size_t(PropId::END)];
                                actor->getPropHandler().getPropIds(tgtProps);

                                const bool GETS_UNDEAD_BANE_BON =
                                    PlayerBon::getsUndeadBaneBon(*Map::player, actor->getData());

                                if (
                                    !tgtProps[int(PropId::ethereal)] ||
                                    GETS_UNDEAD_BANE_BON)
                                {
                                    RangedAttData data(*Map::player, *wpn, actor->pos,
                                                       actor->pos);
                                    Log::addMsg(toStr(data.hitChanceTot) +
                                                "% hit chance.");
                                }
                            }

                            Log::addMsg("[f] to fire" + cancelInfoStr);
                        };

                        auto onKeyPress = [&](const Pos & p, const KeyData & d_)
                        {
                            if (d_.key == 'f')
                            {
                                if (p != Map::player->pos)
                                {
                                    Log::clearLog();
                                    Render::drawMapAndInterface();

                                    Actor* const actor = Utils::getActorAtPos(p);
                                    if (actor) {Map::player->tgt_ = actor;}

                                    Attack::ranged(*Map::player, *wpn, p);

                                    return MarkerDone::yes;
                                }
                            }
                            else if (d_.sdlKey == SDLK_SPACE || d_.sdlKey == SDLK_ESCAPE)
                            {
                                Log::clearLog();
                                return MarkerDone::yes;
                            }
                            return MarkerDone::no;
                        };

                        Marker::run(MarkerDrawTail::yes, MarkerUsePlayerTgt::yes, onMarkerAtPos,
                                    onKeyPress, wpn->getData().ranged.effectiveRange);
                    }
                    else /* Not enough ammo loaded */ if (Config::isRangedWpnAutoReload())
                    {
                        Reload::reloadWieldedWpn(*(Map::player));
                    }
                    else // Not enough ammo loaded, and auto reloading is disabled
                    {
                        Log::addMsg("There is no ammo loaded.");
                    }
                }
                else //Wielded item is not a ranged weapon
                {
                    Log::addMsg("I am not wielding a firearm.");
                }
            }
            else //Not wielding any item
            {
                Log::addMsg("I am not wielding a weapon.");
            }
        }
        clearEvents();
        return;
    }

    //----------------------------------- GET
    else if (d.key == 'g')
    {
        Log::clearLog();
        if (Map::player->isAlive())
        {
            const Pos& p = Map::player->pos;
            Item* const itemAtPlayer = Map::cells[p.x][p.y].item;
            if (itemAtPlayer)
            {
                if (itemAtPlayer->getData().id == ItemId::trapezohedron)
                {
                    DungeonMaster::winGame();
                    Init::quitToMainMenu = true;
                }
            }
            if (!Init::quitToMainMenu) {ItemPickup::tryPick();}
        }
        clearEvents();
        return;
    }

    //----------------------------------- INVENTORY SCREEN
    else if (d.key == 'w')
    {
        Log::clearLog();
        if (Map::player->isAlive())
        {
            InvHandling::runInvScreen();
        }
        clearEvents();
        return;
    }

    //----------------------------------- SWAP TO PREPARED ITEM
    else if (d.key == 'z')
    {
        Log::clearLog();
        if (Map::player->isAlive())
        {

            const bool IS_FREE_TURN = PlayerBon::getBg() == Bg::warVet;

            const string swiftStr = IS_FREE_TURN ? " swiftly" : "";

            Inventory& inv = Map::player->getInv();

            Item* const wielded   = inv.getItemInSlot(SlotId::wielded);
            Item* const alt       = inv.getItemInSlot(SlotId::wieldedAlt);
            const string ALT_NAME = alt ? alt->getName(ItemRefType::a) : "";
            if (wielded || alt)
            {
                if (wielded)
                {
                    if (alt)
                    {
                        Log::addMsg(
                            "I" + swiftStr + " swap to my prepared weapon (" + ALT_NAME + ").");
                    }
                    else
                    {
                        const string NAME = wielded->getName(ItemRefType::a);
                        Log::addMsg("I" + swiftStr + " put away my weapon (" + NAME + ").");
                    }
                }
                else
                {
                    Log::addMsg("I" + swiftStr + " wield my prepared weapon (" + ALT_NAME + ").");
                }
                inv.swapWieldedAndPrepared(IS_FREE_TURN);
            }
            else
            {
                Log::addMsg("I have neither a wielded nor a prepared weapon.");
            }
        }
        clearEvents();
        return;
    }

    //----------------------------------- WAIT/SEARCH
    else if (d.key == 's')
    {
        Log::clearLog();
        if (Map::player->isAlive())
        {
            vector<Actor*> seenMon;
            Map::player->getSeenFoes(seenMon);
            if (seenMon.empty())
            {
                const int     TURNS_TO_APPLY  = 5;
                const string  TURNS_STR       = toStr(TURNS_TO_APPLY);
                Log::addMsg("I pause for a while (" + TURNS_STR + " turns)...");
                Map::player->waitTurnsLeft = TURNS_TO_APPLY - 1;
                GameTime::tick();
            }
            else
            {
                Log::addMsg(msgMonPreventCmd);
                Render::drawMapAndInterface();
            }
        }
        clearEvents();
        return;
    }

    else if (d.key == 'e')
    {
        //----------------------------------- QUICK WALK
        Log::clearLog();
        if (Map::player->isAlive())
        {
            vector<Actor*> seenMon;
            Map::player->getSeenFoes(seenMon);

            if (!seenMon.empty())
            {
                //Monster is seen, prevent quick move
                Log::addMsg(msgMonPreventCmd);
                Render::drawMapAndInterface();
            }
            else if (!Map::player->getPropHandler().allowSee())
            {
                //Player is blinded
                Log::addMsg("Not while blind.");
                Render::drawMapAndInterface();
            }
            else
            {
                bool props[size_t(PropId::END)];
                Map::player->getPropHandler().getPropIds(props);
                if (props[int(PropId::poisoned)])
                {
                    //Player is poisoned
                    Log::addMsg("Not while poisoned.");
                    Render::drawMapAndInterface();
                }
                else if (props[int(PropId::confused)])
                {
                    //Player is confused
                    Log::addMsg("Not while confused.");
                    Render::drawMapAndInterface();
                }
                else
                {
                    Log::addMsg("Which direction?" + cancelInfoStr);
                    Render::drawMapAndInterface();
                    const Dir dir = Query::dir();
                    Log::clearLog();
                    if (dir == Dir::center)
                    {
                        Render::updateScreen();
                    }
                    else
                    {
                        Map::player->setQuickMove(dir);
                    }

                }
            }
        }
        clearEvents();
        return;
    }

    //----------------------------------- THROW ITEM
    else if (d.key == 't')
    {
        Log::clearLog();
        if (Map::player->isAlive())
        {

            if (Map::player->activeExplosive)
            {
                auto onMarkerAtPos = [](const Pos & p)
                {
                    Log::clearLog();
                    Look::printLocationInfoMsgs(p);
                    Log::addMsg("[t] to throw." + cancelInfoStr);
                };

                auto onKeyPress = [](const Pos & p, const KeyData & d_)
                {
                    if (d_.sdlKey == SDLK_RETURN || d_.key == 't')
                    {
                        Log::clearLog();
                        Render::drawMapAndInterface();
                        Throwing::playerThrowLitExplosive(p);
                        return MarkerDone::yes;
                    }
                    else if (d_.sdlKey == SDLK_SPACE || d_.sdlKey == SDLK_ESCAPE)
                    {
                        Log::clearLog();
                        return MarkerDone::yes;
                    }
                    return MarkerDone::no;
                };

                Marker::run(MarkerDrawTail::yes, MarkerUsePlayerTgt::no, onMarkerAtPos,
                            onKeyPress);
            }
            else //Not holding explosive
            {
                if (Map::player->getPropHandler().allowAttackRanged(true))
                {
                    Inventory& playerInv  = Map::player->getInv();
                    Item* itemStack       = playerInv.getItemInSlot(SlotId::thrown);

                    if (itemStack)
                    {
                        Item* itemToThrow     = ItemFactory::copyItem(itemStack);
                        itemToThrow->nrItems_ = 1;

                        auto onMarkerAtPos = [&](const Pos & p)
                        {
                            Log::clearLog();
                            Look::printLocationInfoMsgs(p);

                            auto* const actor = Utils::getActorAtPos(p);

                            if (actor && !actor->isPlayer())
                            {
                                bool tgtProps[size_t(PropId::END)];
                                actor->getPropHandler().getPropIds(tgtProps);

                                const bool GETS_UNDEAD_BANE_BON =
                                    PlayerBon::getsUndeadBaneBon(*Map::player, actor->getData());

                                if (!tgtProps[int(PropId::ethereal)] || GETS_UNDEAD_BANE_BON)
                                {
                                    ThrowAttData data(*Map::player, *itemToThrow, actor->pos, actor->pos);
                                    Log::addMsg(toStr(data.hitChanceTot) + "% hit chance.");
                                }
                            }

                            Log::addMsg("[t] to throw");
                        };

                        auto onKeyPress = [&](const Pos & p, const KeyData & d_)
                        {
                            if (d_.sdlKey == SDLK_RETURN || d_.key == 't')
                            {
                                if (p == Map::player->pos)
                                {
                                    Log::addMsg("I think I can persevere a little longer.");
                                }
                                else
                                {
                                    Log::clearLog();
                                    Render::drawMapAndInterface();

                                    Actor* const actor = Utils::getActorAtPos(p);
                                    if (actor) {Map::player->tgt_ = actor;}

                                    Throwing::throwItem(*Map::player, p, *itemToThrow);
                                    playerInv.decrItemInSlot(SlotId::thrown);
                                }
                                return MarkerDone::yes;
                            }
                            else if (d_.sdlKey == SDLK_SPACE || d_.sdlKey == SDLK_ESCAPE)
                            {
                                delete itemToThrow;
                                itemToThrow = nullptr;
                                Log::clearLog();
                                return MarkerDone::yes;
                            }
                            return MarkerDone::no;
                        };

                        Marker::run(MarkerDrawTail::yes, MarkerUsePlayerTgt::yes, onMarkerAtPos,
                                    onKeyPress, itemToThrow->getData().ranged.effectiveRange);
                    }
                    else //No item equipped
                    {
                        Log::addMsg("I have no missiles chosen for throwing (press 'w').");
                    }
                }
            }
        }
        clearEvents();
        return;
    }

    //-----------------------------------  VIEW DESCRIPTIONS
    else if (d.key == 'v')
    {
        Log::clearLog();
        if (Map::player->isAlive())
        {
            if (Map::player->getPropHandler().allowSee())
            {
                auto onMarkerAtPos = [&](const Pos & p)
                {
                    Log::clearLog();
                    Look::printLocationInfoMsgs(p);

                    auto* const actor = Utils::getActorAtPos(p);

                    if (actor && actor != Map::player)
                    {
                        Log::addMsg("[v] for description");
                    }

                    Log::addMsg(cancelInfoStrNoSpace);
                };

                auto onKeyPress = [&](const Pos & p, const KeyData & d_)
                {
                    if (d_.key == 'v')
                    {
                        Log::clearLog();

                        Look::printDetailedActorDescr(p);

                        Render::drawMapAndInterface();

                        onMarkerAtPos(p);

                    }
                    else if (d_.sdlKey == SDLK_SPACE || d_.sdlKey == SDLK_ESCAPE)
                    {
                        Log::clearLog();
                        return MarkerDone::yes;
                    }
                    return MarkerDone::no;
                };

                Marker::run(MarkerDrawTail::yes, MarkerUsePlayerTgt::yes, onMarkerAtPos,
                            onKeyPress);
            }
            else
            {
                Log::addMsg("I cannot see.");
            }
        }
        clearEvents();
        return;
    }

    //----------------------------------- AUTO MELEE
    else if (d.sdlKey == SDLK_TAB)
    {
        Log::clearLog();
        if (Map::player->isAlive())
        {
            Map::player->autoMelee();
        }
        clearEvents();
        return;
    }

    //----------------------------------- RE-CAST PREVIOUS MEMORIZED SPELL
    else if (d.key == 'x')
    {
        Log::clearLog();
        if (Map::player->isAlive())
        {
            PlayerSpellsHandling::tryCastPrevSpell();
        }
        clearEvents();
        return;
    }

    //----------------------------------- MEMORIZED SPELLS
    else if (d.key == 'X')
    {
        Log::clearLog();
        if (Map::player->isAlive())
        {
            PlayerSpellsHandling::playerSelectSpellToCast();
        }
        clearEvents();
        return;
    }

    //----------------------------------- CHARACTER INFO
    else if (d.key == '@')
    {
        CharacterDescr::run();
        clearEvents();
        return;
    }
    //----------------------------------- LOG HISTORY
    else if (d.key == 'm')
    {
        Log::displayHistory();
        clearEvents();
        return;
    }

    //----------------------------------- ITEM SHORTCUTS
    else if (d.key == 'a')
    {
        Inventory&  inv         = Map::player->getInv();
        Item*       medicalBag  = inv.getFirstItemInBackpackWithId(ItemId::medicalBag);
        Item*       lantern     = inv.getFirstItemInBackpackWithId(ItemId::electricLantern);

        Log::clearLog();

        if (!medicalBag && !lantern)
        {
            Log::addMsg("No item with shortcut carried.");
            Render::drawMapAndInterface();
            clearEvents();
            return;
        }

        Log::addMsg("Use which item?",          clrWhiteHigh);

        if (medicalBag)
        {
            Log::addMsg("[a] Medical Bag",        clrWhiteHigh);
        }
        if (lantern)
        {
            Log::addMsg("[e] Electric Lantern",   clrWhiteHigh);
        }

        Log::addMsg(cancelInfoStrNoSpace,       clrWhiteHigh);

        Render::drawMapAndInterface();

        while (true)
        {
            const KeyData shortcutKeyData = Query::letter(true);

            if (shortcutKeyData.sdlKey == SDLK_ESCAPE || shortcutKeyData.sdlKey == SDLK_SPACE)
            {
                Log::clearLog();
                Render::drawMapAndInterface();
                break;
            }
            else if (medicalBag && shortcutKeyData.key == 'a')
            {
                Log::clearLog();

                for (Item* const item : Map::player->getInv().general_)
                {
                    if (item->getId() == ItemId::medicalBag)
                    {
                        item->activate(Map::player);
                        break;
                    }
                }
                break;
            }
            else if (lantern && shortcutKeyData.key == 'e')
            {
                Log::clearLog();

                for (Item* const item : Map::player->getInv().general_)
                {
                    if (item->getId() == ItemId::electricLantern)
                    {
                        item->activate(Map::player);
                        break;
                    }
                }
                break;
            }
        }
        clearEvents();
        return;
    }

    //----------------------------------- MENU
    else if (d.sdlKey == SDLK_ESCAPE)
    {
        if (Map::player->isAlive())
        {
            Log::clearLog();

            const vector<string> choices {"Options", "Tome of Wisdom", "Quit", "Cancel"};
            const int CHOICE = Popup::showMenuMsg("", true, choices);

            if (CHOICE == 0)
            {
                //---------------------------- Options
                Config::runOptionsMenu();
                Render::drawMapAndInterface();
            }
            else if (CHOICE == 1)
            {
                //---------------------------- Manual
                Manual::run();
                Render::drawMapAndInterface();
            }
            else if (CHOICE == 2)
            {
                //---------------------------- Quit
                queryQuit();
            }
        }
        else
        {
            Init::quitToMainMenu = true;
        }
        clearEvents();
        return;
    }

    //----------------------------------- QUIT
    else if (d.key == 'Q' /*&& IS_DEBUG_MODE*/)
    {
        queryQuit();
        clearEvents();
        return;
    }

    //----------------------------------- DESCEND CHEAT
    else if (d.sdlKey == SDLK_F2)
    {
        if (IS_DEBUG_MODE)
        {
            MapTravel::goToNxt();
            clearEvents();
        }
        return;
    }

    //----------------------------------- XP CHEAT
    else if (d.sdlKey == SDLK_F3)
    {
        if (IS_DEBUG_MODE)
        {
            DungeonMaster::playerGainXp(100);
            clearEvents();
        }
        return;
    }

    //----------------------------------- VISION CHEAT
    else if (d.sdlKey == SDLK_F4)
    {
        if (IS_DEBUG_MODE)
        {
            if (Init::isCheatVisionEnabled)
            {
                for (int x = 0; x < MAP_W; ++x)
                {
                    for (int y = 0; y < MAP_H; ++y)
                    {
                        Map::cells[x][y].isSeenByPlayer = false;
                        Map::cells[x][y].isExplored     = false;
                    }
                }
                Init::isCheatVisionEnabled = false;
            }
            else
            {
                Init::isCheatVisionEnabled = true;
            }
            Map::player->updateFov();
            Render::drawMapAndInterface();
        }
        clearEvents();
    }

    //----------------------------------- INSANITY CHEAT
    else if (d.sdlKey == SDLK_F5)
    {
        if (IS_DEBUG_MODE)
        {
            Map::player->incrShock(50, ShockSrc::misc);
            clearEvents();
        }
        return;
    }

    //----------------------------------- DROP ITEMS AROUND PLAYER
    else if (d.sdlKey == SDLK_F6)
    {
        if (IS_DEBUG_MODE)
        {
            ItemFactory::mkItemOnFloor(ItemId::gasMask, Map::player->pos);

            for (int i = 0; i < int(ItemId::END); ++i)
            {
                const auto* const data = ItemData::data[i];

                if (data->value != ItemValue::normal && data->allowSpawn)
                {
                    ItemFactory::mkItemOnFloor(ItemId(i), Map::player->pos);
                }
            }
            clearEvents();
        }
        return;
    }

    //----------------------------------- TELEPORT
    else if (d.sdlKey == SDLK_F7)
    {
        if (IS_DEBUG_MODE)
        {
            Log::clearLog();
            Map::player->teleport();
            clearEvents();
        }
        return;
    }

    //----------------------------------- INFECTED
    else if (d.sdlKey == SDLK_F8)
    {
        if (IS_DEBUG_MODE)
        {
            Map::player->getPropHandler().tryApplyProp(new PropInfected(PropTurns::std));
            clearEvents();
        }
        return;
    }

    //----------------------------------- UNDEFINED COMMANDS
    else if (d.key != -1)
    {
        string cmdTried = " ";
        cmdTried[0] = d.key;
        Log::clearLog();
        Log::addMsg("Unknown command '" + cmdTried + "'.");
        clearEvents();
        return;
    }
}

void setKeyRepeatDelays()
{
    TRACE_FUNC_BEGIN;
    const int DELAY     = Config::getKeyRepeatDelay();
    const int INTERVAL  = Config::getKeyRepeatInterval();
    SDL_EnableKeyRepeat(DELAY, INTERVAL);
    TRACE_FUNC_END;
}

void mapModeInput()
{
    if (sdlEvent_)
    {
        const KeyData& d = getInput();

        if (!Init::quitToMainMenu)
        {
            handleMapModeKeyPress(d);
        }
    }
}

void clearEvents()
{
    if (sdlEvent_) {while (SDL_PollEvent(sdlEvent_)) {}}
}

KeyData getInput(const bool IS_O_RETURN)
{
    if (!sdlEvent_)
    {
        return KeyData();
    }

    while (true)
    {
        SdlWrapper::sleep(1);

        while (SDL_PollEvent(sdlEvent_))
        {
            if (sdlEvent_->type == SDL_QUIT)
            {
                return KeyData(SDLK_ESCAPE);
            }
            else if (sdlEvent_->type == SDL_KEYDOWN)
            {
                // ASCII char entered?
                // Decimal unicode:
                // '!' = 33
                // '~' = 126
                Uint16 unicode = sdlEvent_->key.keysym.unicode;

                if ((unicode == 'o' || unicode == 'O') && IS_O_RETURN)
                {
                    return KeyData(-1, SDLK_RETURN, unicode == 'O', false);
                }
                else if (unicode >= 33 && unicode < 126)
                {
                    clearEvents();
                    return KeyData(char(unicode));
                }
                else
                {
                    //Other key pressed? (escape, return, space, etc)
                    const SDLKey sdlKey = sdlEvent_->key.keysym.sym;

                    //Do not return shift, control or alt as separate key events
                    if (
                        sdlKey == SDLK_LSHIFT ||
                        sdlKey == SDLK_RSHIFT ||
                        sdlKey == SDLK_LCTRL  ||
                        sdlKey == SDLK_RCTRL  ||
                        sdlKey == SDLK_LALT   ||
                        sdlKey == SDLK_RALT)
                    {
                        continue;
                    }

                    SDLMod      mod           = SDL_GetModState();
                    const bool  IS_SHIFT_HELD = mod & KMOD_SHIFT;
                    const bool  IS_CTRL_HELD  = mod & KMOD_CTRL;
                    const bool  IS_ALT_HELD   = mod & KMOD_ALT;

                    KeyData ret(-1, sdlKey, IS_SHIFT_HELD, IS_CTRL_HELD);

                    if (sdlKey >= SDLK_F1 && sdlKey <= SDLK_F15)
                    {
                        //F-keys
                        return ret;
                    }
                    else
                    {
                        switch (sdlKey)
                        {
                        case SDLK_RETURN:
                        case SDLK_KP_ENTER:
                            if (IS_ALT_HELD)
                            {
                                Config::toggleFullscreen();
                                clearEvents();
                                continue;
                            }
                            else
                            {
                                ret.sdlKey = SDLK_RETURN;
                                return ret;
                            }
                            break;
                        case SDLK_MENU:         continue;   break;
                        case SDLK_PAUSE:        continue;   break;
                        case SDLK_SPACE:        return ret; break;
                        case SDLK_BACKSPACE:    return ret; break;
                        case SDLK_TAB:          return ret; break;
                        case SDLK_PAGEUP:       return ret; break;
                        case SDLK_PAGEDOWN:     return ret; break;
                        case SDLK_END:          return ret; break;
                        case SDLK_HOME:         return ret; break;
                        case SDLK_INSERT:       return ret; break;
                        case SDLK_DELETE:       return ret; break;
                        case SDLK_LEFT:         return ret; break;
                        case SDLK_RIGHT:        return ret; break;
                        case SDLK_UP:           return ret; break;
                        case SDLK_DOWN:         return ret; break;
                        case SDLK_ESCAPE:       return ret; break;
                        default:                continue;   break;
                        }
                    }
                }
            }
        }
    }
}

bool isKeyHeld(const SDLKey key)
{
    Uint8* keystate = SDL_GetKeyState(nullptr);
    return keystate[key];
}

} //Input


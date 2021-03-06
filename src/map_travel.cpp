#include "map_travel.hpp"

#include "init.hpp"

#include <list>

#ifndef NDEBUG
#include <chrono>
#endif // NDEBUG

#include "map.hpp"
#include "mapgen.hpp"
#include "populate_items.hpp"
#include "io.hpp"
#include "msg_log.hpp"
#include "feature_rigid.hpp"
#include "saving.hpp"

namespace map_travel
{

std::vector<MapData> map_list;

namespace
{

void mk_lvl(const MapType& map_type)
{
    TRACE_FUNC_BEGIN;

    bool map_ok = false;

#ifndef NDEBUG
    int   nr_attempts  = 0;
    auto  start_time   = std::chrono::steady_clock::now();
#endif

    // TODO: When the map is invalid, any unique items spawned are lost forever.
    //       Currently, the only effect of this should be that slightly fewever
    //       unique items are found by the player. It is bad design however, and
    //       should be fixed.

    while (!map_ok)
    {
#ifndef NDEBUG
        ++nr_attempts;
#endif

        switch (map_type)
        {
        case MapType::intro:
            map_ok = mapgen::mk_intro_lvl();
            break;

        case MapType::std:
            map_ok = mapgen::mk_std_lvl();
            break;

        case MapType::egypt:
            map_ok = mapgen::mk_egypt_lvl();
            break;

        case MapType::leng:
            map_ok = mapgen::mk_leng_lvl();
            break;

        case MapType::rat_cave:
            map_ok = mapgen::mk_rat_cave_level();
            break;

        case MapType::trapez:
            map_ok = mapgen::mk_trapez_lvl();
            break;

        case MapType::boss:
            map_ok = mapgen::mk_boss_lvl();
            break;
        }
    }

#ifndef NDEBUG
    auto diff_time = std::chrono::steady_clock::now() - start_time;

    const double duration =
        std::chrono::duration<double, std::milli>(diff_time).count();

    TRACE << "map built after   " << nr_attempts << " attempt(s). " << std::endl
          << "Total time taken: " <<  duration << " ms" << std::endl;
#endif

    TRACE_FUNC_END;
}

} //namespace

void init()
{
    //Forest + dungeon + boss + trapezohedron
    const size_t nr_lvl_tot = dlvl_last + 3;

    map_list =
        std::vector<MapData>(nr_lvl_tot, {MapType::std, IsMainDungeon::yes});

    //Forest intro level
    map_list[0] = {MapType::intro, IsMainDungeon::yes};

    //Occasionally set rats-in-the-walls level as intro to first late game level
    if (rnd::one_in(3))
    {
        map_list[dlvl_first_late_game - 1] =
        {
            MapType::rat_cave,
            IsMainDungeon::yes
        };
    }

    //"Pharaoh chamber" is the first late game level
    map_list[dlvl_first_late_game] = {MapType::egypt,  IsMainDungeon::yes};

    map_list[dlvl_last + 1] = {MapType::boss,          IsMainDungeon::yes};
    map_list[dlvl_last + 2] = {MapType::trapez,        IsMainDungeon::yes};
}

void save()
{
    saving::put_int(map_list.size());

    for (const auto& map_data : map_list)
    {
        saving::put_int(int(map_data.type));
        saving::put_int(int(map_data.is_main_dungeon));
    }
}

void load()
{
    const int nr_maps = saving::get_int();

    map_list.resize(size_t(nr_maps));

    for (auto& map_data : map_list)
    {
        map_data.type               = MapType(saving::get_int());
        map_data.is_main_dungeon    = IsMainDungeon(saving::get_int());
    }
}

void go_to_nxt()
{
    TRACE_FUNC_BEGIN;

    map_list.erase(map_list.begin());

    const auto& map_data = map_list.front();

    if (map_data.is_main_dungeon == IsMainDungeon::yes)
    {
        ++map::dlvl;
    }

    mk_lvl(map_data.type);

    map::player->prop_handler().end_prop(PropId::descend, false);

    game_time::is_magic_descend_nxt_std_turn = false;

    map::player->restore_shock(999, true);

    map::player->tgt_ = nullptr;

    map::update_vision();

    if (map_data.is_main_dungeon == IsMainDungeon::yes &&
        map::dlvl == (dlvl_last - 1))
    {
        msg_log::add("An ominous voice thunders in my ears.",
                     clr_white,
                     false,
                     MorePromptOnMsg::yes);

        audio::play(SfxId::boss_voice2);
    }

    audio::try_play_amb(1);

    if (insanity::has_sympt(InsSymptId::phobia_deep))
    {
        msg_log::add("I am plagued by my phobia of deep places!");

        map::player->prop_handler().try_add(
            new PropTerrified(PropTurns::std));

        return;
    }

    TRACE_FUNC_END;
}

MapType map_type()
{
    return map_list.front().type;
}

} // map_travel

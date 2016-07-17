#include "inventory_handling.hpp"

#include "init.hpp"

#include <vector>

#include "item_scroll.hpp"
#include "actor_player.hpp"
#include "item_potion.hpp"
#include "msg_log.hpp"
#include "render_inventory.hpp"
#include "menu_input.hpp"
#include "render.hpp"
#include "drop.hpp"
#include "query.hpp"
#include "item_factory.hpp"
#include "audio.hpp"
#include "map.hpp"

namespace inv_handling
{

Inv_scr     scr_to_open_on_new_turn          = Inv_scr::none;
Inv_slot*   equip_slot_to_open_on_new_turn   = nullptr;
int         browser_idx_to_set_on_new_turn   = 0;

namespace
{

std::vector<size_t> backpack_indexes_to_show_;

//Index can mean Slot index or Backpack Index (both start from zero)
bool run_drop_query(const Inv_type inv_type, const size_t idx)
{
    TRACE_FUNC_BEGIN;

    Inventory&  inv     = map::player->inv();
    Item*       item    = nullptr;

    if (inv_type == Inv_type::slots)
    {
        ASSERT(idx < int(Slot_id::END));
        item = inv.slots_[idx].item;
    }
    else //Backpack
    {
        ASSERT(idx < inv.backpack_.size());
        item = inv.backpack_[idx];
    }

    if (!item)
    {
        TRACE_FUNC_END;
        return false;
    }

    const Item_data_t& data = item->data();

    msg_log::clear();

    if (data.is_stackable && item->nr_items_ > 1)
    {
        TRACE << "Item is stackable and more than one" << std::endl;

        render::draw_map_state(Update_screen::no);

        const std::string nr_str    = "1-" + to_str(item->nr_items_);
        const std::string drop_str  = "Drop how many (" + nr_str + ")?:";

        render::draw_text(drop_str,
                          Panel::screen,
                          P(0, 0),
                          clr_white_high);

        render::update_screen();

        const P   nr_query_pos(drop_str.size() + 1, 0);

        const int   max_digits      = 3;
        const P   done_inf_pos    = nr_query_pos + P(max_digits + 2, 0);

        render::draw_text("[enter] to drop" + cancel_info_str,
                          Panel::screen,
                          done_inf_pos,
                          clr_white_high);

        const int nr_to_drop = query::number(nr_query_pos,
                                             clr_white_high,
                                             0, 3,
                                             item->nr_items_,
                                             false);

        if (nr_to_drop <= 0)
        {
            TRACE << "Nr to drop <= 0, nothing to be done" << std::endl;
            TRACE_FUNC_END;
            return false;
        }
        else //Number to drop is at least one
        {
            item_drop::try_drop_item_from_inv(*map::player, inv_type, idx, nr_to_drop);
            TRACE_FUNC_END;
            return true;
        }
    }
    else //Not a stack
    {
        TRACE << "Item not stackable, or only one item" << std::endl;
        item_drop::try_drop_item_from_inv(*map::player, inv_type, idx);
        TRACE_FUNC_END;
        return true;
    }

    TRACE_FUNC_END;
    return false;
}

void filter_player_backpack_equip(const Slot_id slot_to_equip)
{
    ASSERT(slot_to_equip != Slot_id::END);

    const auto& inv         = map::player->inv();
    const auto& backpack    = inv.backpack_;

    backpack_indexes_to_show_.clear();

    for (size_t i = 0; i < backpack.size(); ++i)
    {
        const auto* const   item    = backpack[i];
        const auto&         data    = item->data();

        switch (slot_to_equip)
        {
        case Slot_id::wpn:
            if (data.melee.is_melee_wpn || data.ranged.is_ranged_wpn)
            {
                backpack_indexes_to_show_.push_back(i);
            }
            break;

        case Slot_id::wpn_alt:
            if (data.melee.is_melee_wpn || data.ranged.is_ranged_wpn)
            {
                backpack_indexes_to_show_.push_back(i);
            }
            break;

        case Slot_id::thrown:
            if (data.ranged.is_throwable_wpn)
            {
                backpack_indexes_to_show_.push_back(i);
            }
            break;

        case Slot_id::body:
            if (data.type == Item_type::armor)
            {
                backpack_indexes_to_show_.push_back(i);
            }
            break;

        case Slot_id::head:
            if (data.type == Item_type::head_wear)
            {
                backpack_indexes_to_show_.push_back(i);
            }
            break;

        case Slot_id::neck:
            if (data.type == Item_type::amulet)
            {
                backpack_indexes_to_show_.push_back(i);
            }
            break;

        case Slot_id::END:
            break;
        }
    }
}

void filter_player_backpack_apply()
{
    auto& backpack = map::player->inv().backpack_;

    backpack_indexes_to_show_.clear();

    const size_t nr_gen = backpack.size();

    for (size_t i = 0; i < nr_gen; ++i)
    {
        const Item* const   item    = backpack[i];
        const Item_data_t&  d       = item->data();

        if (d.has_std_activate)
        {
            backpack_indexes_to_show_.push_back(i);
        }
    }
}

} //namespace

void init()
{
    scr_to_open_on_new_turn         = Inv_scr::none;
    equip_slot_to_open_on_new_turn  = nullptr;
    browser_idx_to_set_on_new_turn  = 0;
}

void activate(const size_t GENERAL_ITEMS_element)
{
    Inventory&  player_inv  = map::player->inv();
    Item*       item        = player_inv.backpack_[GENERAL_ITEMS_element];

    if (item->activate(map::player) == Consume_item::yes)
    {
        player_inv.decr_item_in_backpack(GENERAL_ITEMS_element);
    }
}

void run_inv_screen()
{
    TRACE_FUNC_BEGIN_VERBOSE;

    scr_to_open_on_new_turn = Inv_scr::none;
    render::draw_map_state();

    Inventory& inv = map::player->inv();

    inv.sort_backpack();

    const int gen_size = (int)inv.backpack_.size();

    Menu_browser browser((int)Slot_id::END + gen_size,
                         render_inv::inv_h);

    browser.set_y(browser_idx_to_set_on_new_turn);

    browser_idx_to_set_on_new_turn = 0;

    render_inv::draw_inv(browser);

    auto inv_type = [&]()
    {
        return browser.y() < int(Slot_id::END) ?
               Inv_type::slots : Inv_type::backpack;
    };

    while (true)
    {
        inv.sort_backpack();

        const Menu_action action = menu_input::action(browser);

        switch (action)
        {
        case Menu_action::moved:
            render_inv::draw_inv(browser);
            break;

        case Menu_action::selected:
        {
            Inv_type cur_inv_type = inv_type();

            if (cur_inv_type == Inv_type::slots)
            {
                const size_t    browser_y   = browser.y();
                Inv_slot&       slot        = inv.slots_[browser_y];

                if (slot.item)
                {
                    msg_log::clear();

                    const Unequip_allowed unequip_allowed = inv.try_unequip_slot(slot.id);

                    if (unequip_allowed == Unequip_allowed::yes)
                    {
                        game_time::tick();
                    }

                    scr_to_open_on_new_turn         = Inv_scr::inv;
                    browser_idx_to_set_on_new_turn  = browser.y();

                    TRACE_FUNC_END_VERBOSE;
                    return;
                }
                else //No item in slot
                {
                    //Forbid equipping armor while burning
                    if (
                        slot.id == Slot_id::body &&
                        map::player->prop_handler().has_prop(Prop_id::burning))
                    {
                        msg_log::add("Not while burning.",
                                     clr_white,
                                     false,
                                     More_prompt_on_msg::yes);

                        render_inv::draw_inv(browser);

                        continue;
                    }

                    const bool did_equip_item = run_equip_screen(slot);

                    if (did_equip_item)
                    {
                        scr_to_open_on_new_turn         = Inv_scr::inv;
                        browser_idx_to_set_on_new_turn  = browser.y();

                        render::draw_map_state();

                        TRACE_FUNC_END_VERBOSE;
                        return;
                    }
                    else //No item equipped
                    {
                        render_inv::draw_inv(browser);
                    }
                }
            }
            else //In backpack inventory
            {
                const size_t browser_y = browser.y() - int(Slot_id::END);

                activate(browser_y);

                render::draw_map_state();

                TRACE_FUNC_END_VERBOSE;
                return;
            }
        } break;

        case Menu_action::selected_shift:
        {
            Inv_type cur_inv_type = inv_type();

            const int browser_y = browser.y();

            const size_t idx = cur_inv_type == Inv_type::slots ?
                               browser_y : (browser_y - int(Slot_id::END));

            if (run_drop_query(cur_inv_type, idx))
            {
                browser_idx_to_set_on_new_turn  = browser_y;
                scr_to_open_on_new_turn         = Inv_scr::inv;

                TRACE_FUNC_END_VERBOSE;
                return;
            }

            render_inv::draw_inv(browser);
        } break;

        case Menu_action::esc:
        case Menu_action::space:
            render::draw_map_state();

            TRACE_FUNC_END_VERBOSE;
            return;
        }
    }

    TRACE_FUNC_END_VERBOSE;
}

void run_apply_screen()
{
    TRACE_FUNC_BEGIN_VERBOSE;

    scr_to_open_on_new_turn = Inv_scr::none;
    render::draw_map_state();

    Inventory& inv = map::player->inv();

    inv.sort_backpack();

    filter_player_backpack_apply();

    Menu_browser browser(backpack_indexes_to_show_.size(), render_inv::inv_h);

    browser.set_y(browser_idx_to_set_on_new_turn);

    browser_idx_to_set_on_new_turn = 0;

    render_inv::draw_apply(browser, backpack_indexes_to_show_);

    while (true)
    {
        inv.sort_backpack();

        const Menu_action action = menu_input::action(browser);

        switch (action)
        {
        case Menu_action::moved:
            render_inv::draw_apply(browser, backpack_indexes_to_show_);
            break;

        case Menu_action::selected:
        {
            if (!backpack_indexes_to_show_.empty())
            {
                const size_t idx = backpack_indexes_to_show_[browser.y()];

                activate(idx);

                render::draw_map_state();

                TRACE_FUNC_END_VERBOSE;
                return;
            }
        } break;

        case Menu_action::selected_shift:
        {
            if (!backpack_indexes_to_show_.empty())
            {
                const Inv_type  inv_type    = Inv_type::backpack;
                const size_t    idx         = backpack_indexes_to_show_[browser.y()];

                if (run_drop_query(inv_type, idx))
                {
                    browser_idx_to_set_on_new_turn   = browser.y();
                    scr_to_open_on_new_turn          = Inv_scr::apply;

                    TRACE_FUNC_END_VERBOSE;
                    return;
                }

                render_inv::draw_apply(browser, backpack_indexes_to_show_);
            }
        } break;

        case Menu_action::esc:
        case Menu_action::space:
            render::draw_map_state();

            TRACE_FUNC_END_VERBOSE;
            return;
        }
    }

    TRACE_FUNC_END_VERBOSE;
}

bool run_equip_screen(Inv_slot& slot_to_equip)
{
    TRACE_FUNC_BEGIN_VERBOSE;

    scr_to_open_on_new_turn          = Inv_scr::none;
    equip_slot_to_open_on_new_turn   = &slot_to_equip;
    render::draw_map_state();

    auto& inv = map::player->inv();

    inv.sort_backpack();

    filter_player_backpack_equip(slot_to_equip.id);

    Menu_browser browser(backpack_indexes_to_show_.size(), render_inv::inv_h);

    browser.set_y(0);

    audio::play(Sfx_id::backpack);

    render_inv::draw_equip(browser, slot_to_equip.id, backpack_indexes_to_show_);

    while (true)
    {
        const Menu_action action = menu_input::action(browser);

        switch (action)
        {
        case Menu_action::moved:
            render_inv::draw_equip(browser, slot_to_equip.id, backpack_indexes_to_show_);
            break;

        case Menu_action::selected:
        {
            if (!backpack_indexes_to_show_.empty())
            {
                const int       browser_y = browser.y();
                const size_t    idx       = backpack_indexes_to_show_[browser_y];

                render::draw_map_state();

                inv.equip_backpack_item(idx, slot_to_equip.id); //Calls the items equip hook

                game_time::tick();

                TRACE_FUNC_END_VERBOSE;
                return true;
            }
        }
        break;

        case Menu_action::selected_shift:
        {
            if (!backpack_indexes_to_show_.empty())
            {
                const Inv_type  inv_type    = Inv_type::backpack;
                const size_t    idx         = backpack_indexes_to_show_[browser.y()];

                if (run_drop_query(inv_type, idx))
                {
                    TRACE_FUNC_END_VERBOSE;
                    return true;
                }

                render_inv::draw_equip(browser, slot_to_equip.id, backpack_indexes_to_show_);
            }
        }
        break;

        case Menu_action::esc:
        case Menu_action::space:
            return false;
        }
    }

    TRACE_FUNC_END_VERBOSE;
}

} //inv_handling

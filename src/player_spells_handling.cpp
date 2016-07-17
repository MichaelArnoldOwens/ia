#include "player_spells_handling.hpp"

#include <vector>
#include <algorithm>

#include "init.hpp"
#include "item_scroll.hpp"
#include "actor_player.hpp"
#include "msg_log.hpp"
#include "menu_input.hpp"
#include "render.hpp"
#include "inventory.hpp"
#include "item_factory.hpp"
#include "player_bon.hpp"
#include "query.hpp"
#include "map.hpp"
#include "save_handling.hpp"

namespace player_spells_handling
{

namespace
{

struct Spell_opt
{
    Spell_opt() :
        spell       (nullptr),
        src_item    (nullptr) {}

    Spell_opt(Spell* spell_, Item* src_item_) :
        spell       (spell_),
        src_item    (src_item_) {}

    Spell*  spell;
    Item*   src_item;
};

std::vector<Spell*> known_spells_;

void draw(Menu_browser& browser, const std::vector<Spell_opt>& spell_opts)
{
    const int nr_spells = spell_opts.size();

    render::clear_screen();

    render::draw_text_center("Invoke which power?",
                             Panel::screen,
                             P(screen_w / 2, 0),
                             clr_title);

    P p(0, 1);

    std::string key_str = "a) ";

    for (int i = 0; i < nr_spells; ++i)
    {
        const int       cur_idx         = i;
        const bool      is_idx_marked   = browser.is_at_idx(cur_idx);
        Spell_opt       spell_opt       = spell_opts[i];
        Spell* const    spell           = spell_opt.spell;
        std::string     name            = spell->name();
        const int       spi_x           = 25;
        const int       shock_x         = spi_x + 10;

        p.x = 0;

        const Clr clr = is_idx_marked ? clr_menu_highlight : clr_menu_drk;

        render::draw_text(key_str, Panel::screen, p, clr);

        ++key_str[0];

        p.x = key_str.size();

        render::draw_text(name, Panel::screen, p, clr);

        std::string fill_str = "";

        const size_t fill_size = spi_x - p.x - name.size();

        for (size_t ii = 0; ii < fill_size; ii++)
        {
            fill_str.push_back('.');
        }

        Clr fill_clr = clr_gray;

        fill_clr.r /= 3;
        fill_clr.g /= 3;
        fill_clr.b /= 3;

        render::draw_text(fill_str,
                          Panel::screen,
                          P(p.x + name.size(), p.y),
                          fill_clr);

        p.x = spi_x;

        std::string  info_str = "SPI: ";

        const Range spi_cost = spell->spi_cost(false, map::player);
        const std::string lower_str = to_str(spi_cost.min);
        const std::string upper_str = to_str(spi_cost.max);

        info_str += spi_cost.max == 1 ? "1" : (lower_str +  "-" + upper_str);

        render::draw_text(info_str, Panel::screen, p, clr_white);

        p.x = shock_x;

        const Intr_spell_shock shock_type = spell->shock_type_intr_cast();

        switch (shock_type)
        {
        case Intr_spell_shock::mild:
            info_str = "Mild";
            break;

        case Intr_spell_shock::disturbing:
            info_str = "Disturbing";
            break;

        case Intr_spell_shock::severe:
            info_str = "Severe";
            break;
        }

        render::draw_text(info_str, Panel::screen, p, clr_white);

        if (is_idx_marked)
        {
            const auto descr = spell->descr();
            std::vector<Str_and_clr> lines;

            if (!descr.empty())
            {
                for (const auto& line : descr)
                {
                    lines.push_back({line, clr_white_high});
                }
            }

            //If spell source is an item, add info about this
            if (spell_opt.src_item)
            {
                const std::string item_name =
                    spell_opt.src_item->name(Item_ref_type::plain, Item_ref_inf::none);

                lines.push_back({"Spell granted by " + item_name + ".", clr_green});
            }

            if (!lines.empty())
            {
                render::draw_descr_box(lines);
            }
        }

        ++p.y;
    }

    render::update_screen();
}

void spells_avail(std::vector<Spell_opt>& out)
{
    out.clear();

    for (Spell* const spell : known_spells_)
    {
        out.push_back(Spell_opt(spell, nullptr));
    }

    Inventory& inv = map::player->inv();

    for (auto slot : inv.slots_)
    {
        Item* item = slot.item;

        if (item)
        {
            const std::vector<Spell*>& carrier_spells = item->carrier_spells();

            for (Spell* spell : carrier_spells)
            {
                out.push_back(Spell_opt(spell, item));
            }
        }
    }

    for (Item* item : inv.backpack_)
    {
        const std::vector<Spell*>& carrier_spells = item->carrier_spells();

        for (Spell* spell : carrier_spells)
        {
            out.push_back(Spell_opt(spell, item));
        }
    }
}

void try_cast(const Spell_opt& spell_opt)
{
    ASSERT(spell_opt.spell);

    if (
        map::player->prop_handler().allow_cast_spell(Verbosity::verbose) &&
        map::player->prop_handler().allow_speak(Verbosity::verbose))
    {
        msg_log::clear();
        render::draw_map_state();

        Spell* const spell = spell_opt.spell;

        const Range spi_cost_range = spell->spi_cost(false, map::player);

        if (spi_cost_range.max >= map::player->spi())
        {
            msg_log::add("Cast spell and risk depleting your spirit [y/n]?",
                         clr_white_high);

            render::draw_map_state();

            if (query::yes_or_no() == Yes_no_answer::no)
            {
                msg_log::clear();
                render::draw_map_state();
                return;
            }

            msg_log::clear();
        }

        const bool is_blood_sorc = player_bon::traits[size_t(Trait::blood_sorcerer)];
        const bool is_warlock    = player_bon::traits[size_t(Trait::warlock)];

        const int blood_sorc_hp_drained = 2;

        if (is_blood_sorc)
        {
            if (map::player->hp() <= blood_sorc_hp_drained)
            {
                msg_log::add("I do not have enough life force to cast this spell.");
                render::draw_map_state();
                return;
            }
        }

        msg_log::add("I cast " + spell->name() + "!");

        if (is_blood_sorc)
        {
            map::player->hit(blood_sorc_hp_drained, Dmg_type::pure);
        }

        if (map::player->is_alive())
        {
            spell->cast(map::player, true);

            if (is_warlock && rnd::one_in(2))
            {
                auto* const prop = new Prop_warlock_charged(Prop_turns::std);

                map::player->prop_handler().try_add(prop);
            }
        }
    }
}

} //Player_spells_handling

void init()
{
    cleanup();
}

void cleanup()
{
    for (Spell* spell : known_spells_) {delete spell;}

    known_spells_.clear();
}

void save()
{
    save_handling::put_int(known_spells_.size());

    for (Spell* s : known_spells_)
    {
        save_handling::put_int(int(s->id()));
    }
}

void load()
{
    const int nr_spells = save_handling::get_int();

    for (int i = 0; i < nr_spells; ++i)
    {
        const Spell_id id = Spell_id(save_handling::get_int());

        known_spells_.push_back(spell_handling::mk_spell_from_id(id));
    }
}

void player_select_spell_to_cast()
{
    std::vector<Spell_opt> spell_opts;
    spells_avail(spell_opts);

    if (spell_opts.empty())
    {
        msg_log::add("I do not know any spells to invoke.");
    }
    else //Has spells
    {
        Menu_browser browser(spell_opts.size());

        render::draw_map_state();

        draw(browser, spell_opts);

        while (true)
        {
            const Menu_action action = menu_input::action(browser);

            switch (action)
            {
            case Menu_action::moved:
                draw(browser, spell_opts);
                break;

            case Menu_action::esc:
            case Menu_action::space:
                msg_log::clear();
                render::draw_map_state();
                return;

            case Menu_action::selected:
                try_cast(spell_opts[browser.y()]);
                return;

            default:
                break;
            }
        }
    }
}

bool is_spell_learned(const Spell_id id)
{
    for (auto* s : known_spells_)
    {
        if (s->id() == id)
        {
            return true;
        }
    }

    return false;
}

void learn_spell_if_not_known(const Spell_id id)
{
    learn_spell_if_not_known(spell_handling::mk_spell_from_id(id));
}

void learn_spell_if_not_known(Spell* const spell)
{
    bool is_already_learned = false;

    for (Spell* spell_cmpr : known_spells_)
    {
        if (spell_cmpr->id() == spell->id())
        {
            is_already_learned = true;
            break;
        }
    }

    if (is_already_learned)
    {
        delete spell;
    }
    else
    {
        known_spells_.push_back(spell);
    }
}

} //player_spells_handling

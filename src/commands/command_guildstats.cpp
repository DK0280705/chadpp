#include "../database.h"
#include "../utility.h"
#include "commands.h"
#include <dpp/fmt/core.h>
#include <dpp/fmt/format.h>

Command_guildstats::Command_guildstats()
    : Command("guildstats", COMMAND_GUILDSTATS_DESC, {})
{
    category = cat_general;
}

void Command_guildstats::call(const Input& input) const
{
    const pqxx::result res = bot->database.execute_sync("EXECUTE get_guild_active_users(" +
                                                    pqxx::to_string(input->guild_id) + ")");

    std::string active_members;
    for (pqxx::result::size_type i = 0; i < res.size(); i++) {
        const pqxx::row& row = res[i];
        active_members += fmt::vformat(_(input->gl, COMMAND_GUILDSTATS_ACT_MEM_FORMAT),
                                       fmt::make_format_args(i + 1, row[0].c_str(), row[1].c_str(),
                                                             row[2].c_str()));
    }

    dpp::guild* g = dpp::find_guild(input->guild_id);
    dpp::embed e  = dpp::embed()
        .set_title(fmt::format("{} Stats", g->name))
        .set_description(g->description)
        .set_color(c_gray)
        .set_thumbnail(g->get_icon_url())
        .add_field(_(input->gl, INFORMATION),
                   fmt::vformat(_(input->gl, COMMAND_GUILDSTATS_INFO_FORMAT),
                               fmt::make_format_args(
                                    g->owner_id, g->members.size(),
                                    g->channels.size(),
                                    bool_string(g->is_verified()),
                                    bool_string(g->is_discoverable()),
                                    _(input->gl, g->nsfw_level + (int)NSFW_LEVEL_DEFAULT))))
        .add_field(_(input->gl, COMMAND_GUILDSTATS_INFO_TOP10), active_members);
    input.reply(e);
}
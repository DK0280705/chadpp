#include "../command.h"
#include "../database.h"
#include "../input.h"
#include "../variant_cast.h"
#include "events.h"
#include <dpp/fmt/format.h>
#include <exception>

#define SPAM_LIMIT    1
#define MAX_PENALTIES 5

static std::mutex spam_mutex;
static std::unordered_map<dpp::snowflake, uint64_t> member_spam;
static std::unordered_map<dpp::snowflake, uint16_t> spam_penalties;

static void check_spam(Bot* bot, const dpp::message& msg)
{
    // Check spam member.
    // I don't know if it even useful or not. It works like rate limiting.
    // Spam with official discord client is not possible, but, self bot may be possible.
    // Still need more research.
    std::unique_lock lock(spam_mutex);
    const auto gs_it = bot->guild_spam_list.find(msg.guild_id);
    if (gs_it != bot->guild_spam_list.end()) {
        const uint64_t now = dpp::utility::time_f();
        const auto m_it    = member_spam.find(msg.author.id);
        if (m_it != member_spam.end()) {
            if (now - m_it->second <= SPAM_LIMIT) {
                if ((spam_penalties.at(msg.author.id)++) >= MAX_PENALTIES) {
                    lock.unlock();
                    // do something to spam members
                    bot->log(dpp::ll_warning, fmt::format("User id {} is spamming", msg.author.id));
                }
            }
        } else {
            member_spam.emplace(msg.author.id, now);
            spam_penalties.emplace(msg.author.id, 0);
        }
    }
}

static int parse_message(const dpp::message& msg,
                         const std::vector<dpp::command_option>& c_options,
                         Command_options& options,
                         int last_pos)
{
    for (const auto& co : c_options) {
        const int new_pos = msg.content.find(' ', last_pos);
        if (last_pos <= 0 && new_pos <= 0) {
            if (co.required) return PARSE_ERR_ARGUMENTS;
            else if (co.type <= 2) return PARSE_ERR_SUBCOMMAND_EMPTY;
            break;
        }
        dpp::command_parameter cp;
 
        if (co.choices.empty()) {
            switch (co.type) {
            case dpp::co_string:
            {
                if (msg.content[last_pos] == '"') {
                    const int dqpos = msg.content.find('"', last_pos+1);
                    if (dqpos == -1) return PARSE_ERR_STRING_DOUBLE_QUOTES;
                    cp = msg.content.substr(last_pos + 1, dqpos - last_pos - 1);
                    break;
                }
                cp = msg.content.substr(last_pos, new_pos - last_pos);
                break;
            }
            case dpp::co_integer:
            {
                try {
                    cp = std::stoll(msg.content.substr(last_pos, new_pos - last_pos));
                } catch (const std::exception&) {
                    return PARSE_ERR_NUMBER;
                }
                break;
            }
            case dpp::co_number:
            {
                try {
                    cp = std::stod(msg.content.substr(last_pos, new_pos - last_pos));
                } catch (const std::exception&) {
                    return PARSE_ERR_NUMBER;
                }
                break;
            }
            case dpp::co_boolean:
            {
                std::string str = msg.content.substr(last_pos, new_pos - last_pos);
                std::transform(str.begin(), str.end(), str.begin(),
                               [](unsigned char c) { return std::tolower(c); });

                if (str == "true" || str == "1") cp = true;
                else if (str == "false" || str == "0") cp = false;
                else return PARSE_ERR_BOOLEAN;
                break;
            }
            case dpp::co_user:
            {
                const std::string str = msg.content.substr(last_pos, new_pos - last_pos);
                if (str[0] == '<' && str[1] == '@' && str[str.size() - 1] == '>') {
                    try {
                        const uint64_t uid = std::stoull(str.substr(2, str.size() - 1), 0, 10);
                        const dpp::user* u = dpp::find_user(uid);
                        if (u) {
                            dpp::resolved_user m;
                            m.user = *u;

                            const dpp::guild* g = dpp::find_guild(msg.guild_id);
                            if (g->members.find(uid) != g->members.end()) m.member = g->members.at(uid);

                           cp = m;
                            break;
                        }
                    } catch (const std::exception&) {}
                }
                return PARSE_ERR_USER;
            }
            case dpp::co_channel:
            {
                const std::string str = msg.content.substr(last_pos, new_pos - last_pos);
                if (str[0] == '<' && str[1] == '#' && str[str.size() - 1] == '>') {
                    try {
                        const uint64_t cid    = std::stoull(str.substr(2, str.size() - 1), 0, 10);
                        const dpp::channel* c = dpp::find_channel(cid);
                        if (c) {
                            cp = *c;
                            break;
                        }
                    } catch (const std::exception&) {}
                }
                return PARSE_ERR_CHANNEL;
            }
            case dpp::co_role:
            {
                const std::string str = msg.content.substr(last_pos, new_pos - last_pos);
                if (str[0] == '<' && str[1] == '&' && str[str.size() - 1] == '>') {
                    try {
                        const uint64_t rid = std::stoull(str.substr(2, str.size() - 1), 0, 10);
                        const dpp::role* r = dpp::find_role(rid);
                        if (r) {
                            cp = *r;
                            break;
                        }
                    } catch (const std::exception&) {}
                }
                return PARSE_ERR_ROLE;
            }
            default:
            {
                const std::string name = msg.content.substr(last_pos, new_pos - last_pos);

                const auto co_it = std::find_if(c_options.begin(), c_options.end(),
                                                [&](const dpp::command_option& opt) {
                    return opt.name == name;
                });

                if (co_it != c_options.end()) {
                    options.emplace(options.size(), co_it - c_options.begin());
                    parse_message(msg, co_it->options, options, new_pos + 1);
                    return NOTHING;
                } else return PARSE_ERR_SUBCOMMAND;
            }
            }
        } else {
            const std::string name = msg.content.substr(last_pos, new_pos - last_pos);
            auto it = std::find_if(co.choices.begin(), co.choices.end(),
                                   [&](const dpp::command_option_choice& c) {
                return c.name == name;
            });
            
            if (it != co.choices.end()) cp = variant_cast(it->value);
            else return PARSE_ERR_CHOICES;
        }
        options.emplace(options.size(), cp);
        last_pos = new_pos + 1;
    }
    return NOTHING;
}

void event_message_create(const dpp::message_create_t& event)
{
    if (event.msg.author.is_bot()) return;

    check_spam(bot, event.msg);

    // Update or insert active users table.
    // It consumes message content length. Sounds like levelling system but more statistically.
    bot->database->execute(fmt::format("EXECUTE upsert_active_user({}, {}, {})",
                                        event.msg.author.id, event.msg.guild_id,
                                        event.msg.content.length()));

    // Controls whether the user is afk or not.
    bot->database->execute("EXECUTE find_afk_user(" + std::to_string(event.msg.author.id) + ")",
                           [ch_id    = event.msg.channel_id,
                            a_id     = event.msg.author.id, 
                            g_id     = event.msg.guild_id,
                            mentions = event.msg.mentions](const pqxx::result& res) {
        if (!res.empty()) {
            bot->database->execute_sync("DELETE FROM chadpp.afk_users WHERE id = " +
                                        pqxx::to_string(a_id));
            bot->message_create(dpp::message(ch_id, _(bot->guild_lang(g_id), COMMAND_AFK_REMOVED)));
        }
        for (const auto& m : mentions) {
            const pqxx::result res = bot->database->execute_sync("EXECUTE find_afk_user(" +
                                                                 std::to_string(m.first.id) + ")");
            if (!res.empty()) {
                const dpp::embed e = dpp::embed()
                    .set_color(c_gray)
                    .set_description(
                        fmt::vformat(_(bot->guild_lang(g_id), COMMAND_AFK_PINGED),
                                     fmt::make_format_args(m.first.id, res[2][0].c_str(),
                                                           res[1][0].c_str())));
                bot->message_create(dpp::message(ch_id, e));
            }
        }
    });

    if (event.msg.content.rfind(bot->default_prefix, 0) != 0) return;

    int new_pos = event.msg.content.find(' ', 0);
    if (new_pos == bot->default_prefix.size()) return;
    std::string cmd_name = event.msg.content.substr(bot->default_prefix.size(),
                                                    new_pos - bot->default_prefix.size());
    std::transform(cmd_name.begin(), cmd_name.end(), cmd_name.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    std::shared_lock c_lock(bot->cmd_mutex);
    if (Command* c = bot->find_command(cmd_name)) {
        Command_options options;

        if (const int err = parse_message(event.msg, c->options, options, new_pos + 1))
            return event.reply(_(bot->guild_lang(event.msg.guild_id), err));
        
        Input input(bot, event.from, options, event.msg);

        if (const int err = bot->handle_command(c, input))
            return input.reply(_(input->lang_id, err));

        bot->execute_cmd(c, input);
    }
}

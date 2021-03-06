#include "../collector.h"
#include "../utility.h"
#include "commands.h"
#include <algorithm>
#include <dpp/fmt/format.h>

class Bulls_and_cows : public Message_collector 
{
public:
    Bulls_and_cows(Bot* bot, const Input& input, int length, int tries)
        : Collector(bot, 60, false, true)
        , _input(input)
        , _tries(tries)
        , _length(length)
        , _secret(_length, 0)
    {
        generate_numbers();
        _e = dpp::embed()
            .set_title("Bulls and cows")
            .set_description(fmt::format("Guess a {} digit number", _length))
            .set_color(c_gray);
        _input.reply_sync(_e);
        _e.add_field("Attempts", {}, false);
    }

    void generate_numbers()
    {
        bool arr[10] = {false};
        for (int i = 0; i < _length; i++) {
            int num = random(0, 9);

            // Repeat if duplicate numbers
            if (arr[num]) i--;
            else {
                arr[num]   = true;
                _secret[i] = num;
            }
        }
    }

    bool on_collect(const dpp::message& item) override
    {
        if (item.content == "stop") {
            terminating = true;
            return false;
        } else if (item.content.size() != _length || !isdigit(item.content)) return false;
        bot_->message_delete(item.id, _input->channel_id);
        
        _tries--;

        const std::string& ans = item.content;

        int bulls = 0;
        int cows  = 0;

        for (int i = 0; i < _length; i++)
            if ((ans[i] - 48) == _secret[i]) bulls++;
            else if (std::find(_secret.begin(), _secret.end(), (ans[i] - 48)) != _secret.end())
                cows++;

        const bool win = bulls == _secret.size();
        _e.fields[0].value += fmt::format("\n{} - {} Bulls, {} Cows - <@!{}>",
                                          win ? "**" + ans + "**" : "`" + ans + "`", bulls, cows,
                                          item.author.id);

        if (_answers.find(item.author.id) == _answers.end())
            _answers.emplace(item.author.id, 1);
        else _answers.at(item.author.id)++;

        if (win) {
            dpp::embed e = dpp::embed()
                .set_color(c_green)
                .set_description(fmt::format("<@!{}>, {}", item.author.id, _(_input->lang_id, COMMAND_BNC_GAME_WIN)))
                .set_footer({item.author.username, item.author.get_avatar_url(), {}});
            if (_answers.size() > 1) {
                uint64_t tryharder = 0;
                uint16_t most_val  = 0;
                for (const auto& [id, v] : _answers) {
                    if (v > most_val) {
                        most_val  = v;
                        tryharder = id;
                    }
                }
                e.add_field(_(_input->lang_id, COMMAND_BNC_GAME_TRYHARDER),
                            fmt::format("> <@!{}> - {} {}", tryharder, most_val, _(_input->lang_id, COMMAND_BNC_GAME_ATTEMPTS)), false);
            }
            bot_->message_create(dpp::message(_input->channel_id, e));
            terminating = true;
            return false;
        } else if (!_tries) {
            dpp::message m = dpp::message()
                .set_reference(_input->message_id)
                .add_embed(dpp::embed().set_title(_(_input->lang_id, COMMAND_BNC_GAME_MAX_TRIES_REACHED)));
            m.channel_id = _input->channel_id;
            bot_->message_create(m);
            terminating = true;
            return false;
        }
        _input.edit_reply(_e);
        return true;
    }

    void on_end(const std::vector<dpp::message>&) override
    {
        _e.set_description(fmt::format("{} `{}`",
                           _(_input->lang_id, COMMAND_BNC_GAME_ENDED), ivectostr(_secret)));
        if (_e.fields[0].value.empty()) _e.fields.clear();
        _input.edit_reply(_e);
        bot_->remove_message_collector(_input->channel_id);
    }

private:
    Input _input;
    int _tries;
    int _length;

    std::vector<int> _secret;
    std::unordered_map<uint64_t, int> _answers;
    dpp::embed _e;
};

Command_bnc::Command_bnc()
    : Command("bnc",
              COMMAND_BNC_DESC,
              {{dpp::command_option(dpp::co_integer, "length", "")},
               {dpp::command_option(dpp::co_integer, "tries", "")}})
{
    creates_m_coll = true;
    category       = cat_fun;
}

void Command_bnc::call(const Input& input) const
{
    const int length = input.has(0) ? std::get<int64_t>(input[0]) : 4;
    const int tries  = input.has(1) ? std::get<int64_t>(input[1]) : 10;

    int succ = (length < 1 || tries < 1) ? COMMAND_BNC_INVALID_PARAMS
               : (length > 10)           ? COMMAND_BNC_INVALID_PARAM1
               : (tries > 20)            ? COMMAND_BNC_INVALID_PARAM2
                                         : NOTHING;

    if (succ != 0) return input.reply(_(input->lang_id, succ));

    bot->add_message_collector(input->channel_id,
                               new Bulls_and_cows(bot, input, length, tries));
}

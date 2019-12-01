#include "sol.hpp"
#include "AppIO/AppIO.hpp"
#include "AppIO/Config.hpp"
#include "AppIO/Timer.hpp"
#include "JsonConversions.hpp"

std::vector<char*> captureArgumentsFromLua(sol::table args) {
    std::vector<std::string> luaArgs;
    args.for_each([&](sol::object const& key, sol::object const& value) {
        auto argument = value.as<std::string>();
        auto keyAsInt = key.as<int>();
        if (keyAsInt == 0)
            argument.replace(argument.find_first_of(".lua"),
                             argument.find_last_of(".lua"),
                             "");

        if (keyAsInt >= 0) luaArgs.insert(luaArgs.begin(), argument);
    });

    std::vector<char*> cstrings;
    cstrings.reserve(luaArgs.size());
    for(auto& s: luaArgs)
        cstrings.push_back(&s[0]);
    return cstrings;
}


sol::table open_AppIO(sol::this_state L) {
    sol::state_view lua(L);
    sol::table module = lua.create_table();

    auto args = captureArgumentsFromLua(lua["arg"]);

    auto instance = AppIO::AppIO::instance();
    instance->initialize(args.size(), args.data());

    auto config = AppIO::Config::get();
    config->insert({{"_binding", "lua"}});

    auto configDataPtr = sol::reference(jsonToLuaObject(*config->data(), lua));

    module.new_usertype<AppIO::AppIO>("AppIO",
            "new", sol::no_constructor,
            "instance", &AppIO::AppIO::instance,
            "run", &AppIO::AppIO::run);


    module.new_usertype<AppIO::Config>("Config",
            "new", sol::no_constructor,
            "get", &AppIO::Config::get,
            "commit", [config, configDataPtr](AppIO::Config *conf, sol::this_state lua) -> void
            {
                conf->restructure(luaObjectToJson(configDataPtr));
                conf->commit();
            },
            "data", [configDataPtr](AppIO::Config *conf, sol::this_state lua) -> sol::object
            {
                return configDataPtr;
            },
            "insert", [](AppIO::Config *conf, sol::table values, sol::this_state lua) -> void
            {
                conf->insert(luaObjectToJson(values));
            });

    module.new_usertype<AppIO::Timer>("Timer",
            "new", sol::no_constructor,
            "singleShot", [](double secs, AppIO::Timer::timerCallback cb) -> std::shared_ptr<AppIO::Timer>
            {
                return AppIO::Timer::createSingleShot(
                        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(secs)),
                        cb);
            },
            "interval", [](double secs, AppIO::Timer::timerCallback cb) -> std::shared_ptr<AppIO::Timer>
            {
                return AppIO::Timer::createInterval(
                        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(secs)),
                        cb);
            });

    return module;
}

extern "C" int luaopen_libappio_lua (lua_State* L) {
    return sol::stack::call_lua(L, 1, open_AppIO);
}
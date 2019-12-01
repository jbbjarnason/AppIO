#pragma once

#include "sol.hpp"
#include <nlohmann/json.hpp>

sol::object jsonToLuaObject(const nlohmann::json & j, sol::state_view lua) {
    if (j.is_null()) return sol::nil;
    else if (j.is_boolean()) return sol::make_object(lua, j.get<bool>());
    else if (j.is_number()) return sol::make_object(lua, j.get<double>());
    else if (j.is_string()) return sol::make_object(lua, j.get<std::string>());
    else if (j.is_object()) {
        auto obj = lua.create_table();
        for (auto& [key, value] : j.items())
            obj[key] = jsonToLuaObject(value, lua);
        return obj.as<sol::object>();
    } else if (j.is_array()) {
        auto obj = lua.create_table();
        for (auto& [key, value] : j.items())
            obj[atoi(key.c_str()) + 1] = jsonToLuaObject(value, lua);
        return obj;
    }
    return sol::make_object(lua, sol::nil);
}

nlohmann::json luaObjectToJson(const sol::object& obj) {
    nlohmann::json j;
    switch (obj.get_type()) {
        case sol::type::string: return obj.as<std::string>();
        case sol::type::number: return obj.as<double>();
        case sol::type::table: {
            auto table = obj.as<sol::table>();
            for (auto& [key, value] : table)
                j[key.as<std::string>()] = luaObjectToJson(value);
            return j;
        }
        default: {
            std::cout << "This config variable type " << (int)obj.get_type() << " is unhandled by config interface. Please check sol::type enum." << std::endl;
        }
    }
    return j;
}
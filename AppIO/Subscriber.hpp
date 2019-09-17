#pragma once

#include "AppIO.hpp"
#include <rapidjson/document.h>

namespace AppIO {

template<class T>
class Subscriber {
public:
    typedef std::function<T(const std::string &)> toT;

    Subscriber(std::string name) : _subscriber(*AppIO::instance()->getContext()) {
        if (std::is_same<T, bool>::value)
            init(false, "bool", [this](const std::string &json) {
//                std::cout << "to json: " << json << "\n";
                return parse(json)["val"].GetBool();
            });
        else if (std::is_same<T, int>::value)
            init(0, "int", [this](const std::string &json) {
                return parse(json)["val"].GetInt();
            });
        else if (std::is_same<T, double>::value)
            init(0.0, "double", [this](const std::string &json) {
                return parse(json)["val"].GetDouble();
            });
        else if (std::is_same<T, std::string>::value)
            init("", "string", [this](const std::string &json) {
                return parse(json)["val"].GetString();
            });
        else throw "Unknown type in sender declared in default constructor";
        auto myApp = AppIO::instance();
        _address = "/" + _typeName + "/" + myApp->getFullAppName() + "/" + name;

        auto conf = myApp->getConfig();
        if (conf->find("_receivers") == conf->end()) {
            (*conf)["_receivers"] = {};
        }
        if ((*conf)["_receivers"].find(_address) == (*conf)["_receivers"].end()) {
            (*conf)["_receivers"][_address] = "";
        }
        std::string connectedTo = (*conf)["_receivers"][_address];

        if (!connectedTo.empty()) subscribeTo(connectedTo);

        myApp->updateConfigFile();
    }

    Subscriber(T initalState, std::string typeName, const toT &toTemplateVal) : _subscriber(
            *AppIO::instance()->getContext()) {
        init(initalState, typeName, toTemplateVal);
    }

    void setCallback(const std::function<void(T)> &cb) {
        _subscriber.async_receive(asio::buffer(_buf), onCallback(cb));
    }

    T getState() { return _lastState; }

private:
    void init(T initialState, std::string typeName, const toT &toTemplateVal) {
        _lastState = initialState;
        _typeName = typeName;
        _stringToTemplateVal = toTemplateVal;
    }

    inline rapidjson::Document parse(const std::string &json) {
        rapidjson::Document doc;
        doc.Parse(json.c_str());
        return doc;
    }

    void subscribeTo(const std::string &name) {
        _subscriber.connect("ipc://"+name);
        _subscriber.set_option(azmq::socket::subscribe(""));
        std::cout << _address << " is connected to " << name << std::endl;
    }

    inline auto onCallback(const std::function<void(T)> &cb) {
        return [this, cb](auto const &error_code, auto bytes_transferred) {

            if (error_code) {
                std::cout << "Got an error " << error_code << "\n";
                return;
            }
            auto output = std::string(_buf.data(), bytes_transferred);
//            std::cout << "got new output " << output << "\n"; //<< " parsed is "<< _lastState << "\n";
            auto newState = _stringToTemplateVal(output);
            if (newState != _lastState) {
                _lastState = newState;
                cb(newState);
            }
            _subscriber.async_receive(asio::buffer(_buf), onCallback(cb));
        };
    }

    toT _stringToTemplateVal;

    azmq::sub_socket _subscriber;
    std::array<char, 256> _buf{};
    std::string _typeName = "";
    T _lastState;
    std::string _address;
};

}

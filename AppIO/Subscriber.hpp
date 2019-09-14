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
                std::cout << "to json: " << json << "\n";
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
        _address = _typeName + "." + myApp->getAppNameAndInstance() + "." + name;

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
        _subscriber.async_receive(asio::buffer(_buf), [this, cb](auto const &error_code, auto bytes_transferred) {
            if (error_code) {
                std::cout << "Got an error " << error_code << "\n";
                return;
            }
            auto output = std::string(_buf.data(), bytes_transferred);
//            auto output = std::string(_buf.data(), bytes_transferred - 1);
            std::cout << "got new output " << output << "\n"; //<< " parsed is "<< _lastState << "\n";
            _lastState = _stringToTemplateVal(output.substr(_subscribtionStringLen, output.length()));

            cb(_lastState);
        });
    }

    T getState() { return _lastState; }

private:
    void init(T initialState, std::string typeName, const toT &toTemplateVal) {
        _lastState = initialState;
        _typeName = typeName;
        _stringToTemplateVal = toTemplateVal;
        _subscriber.connect("tcp://127.0.0.1:" + std::to_string(PORT));
    }

    inline rapidjson::Document parse(const std::string &json) {
        rapidjson::Document doc;
        doc.Parse(json.c_str());
        return doc;
    }

    void subscribeTo(const std::string &name) {
        _subscriber.set_option(azmq::socket::subscribe(name.c_str()));
        _subscribtionStringLen = name.length();
    }

    toT _stringToTemplateVal;

    azmq::sub_socket _subscriber;
    std::array<char, 256> _buf{};
    std::string _typeName = "";
    uint _subscribtionStringLen;
    T _lastState;
    std::string _address;
};

}

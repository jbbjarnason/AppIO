#pragma once

#include "AppIO.hpp"
#include "MessageCourier.hpp"
#include <rapidjson/document.h>

namespace AppIO {

template<class T>
class Subscriber: public MessageCourier<T> {
public:
    typedef std::function<T(const std::string &)> toT;

    Subscriber(std::string name) {
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
        this->_address = "/" + this->_typeName + "/" + myApp->getFullAppName() + "/" + name;

        auto config = Config::get();
        config->insert({{
                "_subscribers", {
                    {this->_address, ""}
                }
            }
        });
        std::string connectedTo = (*config->data())["_subscribers"][this->_address];
        config->commit();

        if (!connectedTo.empty()) subscribeTo(connectedTo);
    }

    Subscriber(T initalState, std::string typeName, const toT &toTemplateVal) {
        init(initalState, typeName, toTemplateVal);
    }

    void setCallback(const std::function<void(T)> &cb) {
        _subscriber->async_receive(asio::buffer(_buf), onCallback(cb));
    }

private:
    void init(T initialState, std::string typeName, const toT &toTemplateVal) {
        MessageCourier<T>();
        MessageCourier<T>::init(initialState, typeName);
        _stringToTemplateVal = toTemplateVal;
        _subscriber = std::make_unique<azmq::sub_socket>(*this->_app->getContext());
        _subscriberDestructing = false;
        AppIO::instance()->addDestructor([this](){
            _subscriberDestructing = true;
            _subscriber->cancel();
            _subscriber.reset();
        });
    }

    inline rapidjson::Document parse(const std::string &json) {
        rapidjson::Document doc;
        doc.Parse(json.c_str());
        return doc;
    }

    void subscribeTo(const std::string &name) {
        _subscriber->connect("ipc://"+name);
        _subscriber->set_option(azmq::socket::subscribe(""));
        std::cout << this->_address << " is connected to " << name << std::endl;
    }

    inline auto onCallback(const std::function<void(T)> &cb) {
        return [this, cb](auto const &error_code, auto bytes_transferred) {

            if (error_code) {
                if (!_subscriberDestructing) std::cout << "Got an error " << error_code << "\n";
                return;
            }
            auto output = std::string(_buf.data(), bytes_transferred);
//            std::cout << "got new output " << output << "\n"; //<< " parsed is "<< _lastState << "\n";
            auto newState = _stringToTemplateVal(output);
            if (newState != this->_lastState) {
                this->_lastState = newState;
                cb(newState);
            }
            _subscriber->async_receive(asio::buffer(_buf), onCallback(cb));
        };
    }

    toT _stringToTemplateVal;

    std::unique_ptr<azmq::sub_socket> _subscriber;
    bool _subscriberDestructing;
    std::array<char, 256> _buf{};
};

}

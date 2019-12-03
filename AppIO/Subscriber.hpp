#pragma once

#include "MessageCourier.hpp"

namespace AppIO {

template<class T>
class Subscriber: public MessageCourier<T> {
public:
    Subscriber(std::string address, const T& initialValue = T()):
        _subscriber(std::make_unique<azmq::sub_socket>(*this->_app->getContext())),
        _subscriberDestructing(false),
        MessageCourier<T>()
    {
        nlohmann::json j = initialValue;
        MessageCourier<T>::init(initialValue, j.type_name());

        this->_app->addDestructor([this](){
            _subscriberDestructing = true;
            _subscriber->cancel();
            _subscriber.reset();
        });

        this->_address = "/" + this->_typeName + "/" + this->_app->getFullAppName() + "/" + address;

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

    void setCallback(const std::function<void(T)> &cb) {
        _subscriber->async_receive(asio::buffer(_buf), onCallback(cb));
    }

private:
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
            auto newState = nlohmann::json::parse(output).get<T>();
//            auto newState = _stringToTemplateVal(output);
            if (newState != this->_lastState) {
                this->_lastState = newState;
                cb(newState);
            }
            _subscriber->async_receive(asio::buffer(_buf), onCallback(cb));
        };
    }

    std::unique_ptr<azmq::sub_socket> _subscriber;
    bool _subscriberDestructing;
    std::array<char, 256> _buf{};
};

}

#pragma once

#include "AppIO.hpp"
#include "Config.hpp"

namespace AppIO {

template<class T>
class MessageCourier {
public:
    MessageCourier():_app(AppIO::instance()) {}
    virtual ~MessageCourier() {}
    T getState() { return _lastState; }

protected:
    virtual void init(T initialState, std::string typeName) {
        this->_lastState = initialState;
        this->_typeName = typeName;
        _app = AppIO::instance();
    }

    T _lastState;
    std::string _typeName = "";
    std::string _address;
    std::shared_ptr<AppIO> _app;
};

}


#pragma once

#include "AppIO.hpp"

namespace AppIO {

    template<class T>
    class Publisher {
    public:

        Publisher(const std::string &address) : Publisher() {
            setAddress(address);
        }

        void send(const T &val) {
            _lastState = val;
            auto strToSend = _toString(val);
//            std::cout << "publishing " << strToSend << "\n";
            _publisher.send(asio::buffer(strToSend));
        }

        T getState() { return _lastState; }

    private:
        typedef std::function<std::string(T)> TtoString;

        Publisher() : _publisher(*AppIO::instance()->getContext()) {
            if (std::is_same<T, bool>::value)
                init(false, "bool", [this](const T &val) {
                    std::string valAsStr = val ? "true" : "false";
                    return "{\"val\":" + valAsStr + "}";
                });
            else if (std::is_same<T, int>::value)
                init(0, "int", [this](const T &val) {
                    return "{\"val\":" + std::to_string(val) + "}";
                });
            else if (std::is_same<T, double>::value)
                init(0.0, "double", [this](const T &val) {
                    return "{\"val\":" + std::to_string(val) + "}";
                });
            else if (std::is_same<T, std::string>::value)
                init("", "string", [this](const T &val) {
                    return "{\"val\":\"" + std::to_string(val) +
                           "\"}"; // TODO: no sure why i have to to_string a string
                });
            else throw "Unknown type in sender declared in default constructor";
        }

        void setAddress(const std::string &address) {

            auto myApp = AppIO::instance();
            _address = myApp->getFullAppPath() + "ipc/" +_typeName + "/" + address;
//            std::cout << "Binding to address " << _address << " length is " << _address.size();

            std::filesystem::path filePath = _address;
            auto dirPath = filePath.parent_path();
            if (!std::filesystem::exists(dirPath))
                std::filesystem::create_directories(dirPath);


            (*myApp->getConfig())["_senders"].push_back(_address);
            myApp->updateConfigFile();

            _publisher.bind("ipc://"+_address);
        }

        void init(T initialState, std::string typeName, TtoString toString) {
            _lastState = initialState;
            _typeName = typeName;
            _toString = toString;
//            _publisher.bind("tcp://127.0.0.1:" + std::to_string(PORT));
        }

        T _lastState;
        std::string _typeName = "";
        TtoString _toString;
        azmq::pub_socket _publisher;
        std::string _address;
    };

}
#pragma once

#include "AppIO.hpp"
#include "MessageCourier.hpp"

namespace std { // TODO: is this wise ... Used in publisher bool type
    inline std::string to_string(bool val) {
        return val ? "true" : "false";
    }
}

namespace AppIO {

    template<class T>
    class Publisher: public MessageCourier<T> {
    public:

        Publisher(const std::string &address) : Publisher() {
            setAddress(address);
        }

        void send(const T &val) {
            this->_lastState = val;
            auto strToSend = _toString(val);
//            std::cout << "publishing " << strToSend << "\n";
            _publisher.send(asio::buffer(strToSend));
        }

    private:
        typedef std::function<std::string(T)> TtoString;

        Publisher() :
                MessageCourier<T>(),
                _publisher(*this->_app->getContext()) {
            if (std::is_same<T, bool>::value)               init(false, "bool");
            else if (std::is_same<T, int>::value)           init(0, "int");
            else if (std::is_same<T, double>::value)        init(0.0, "double");
            else if (std::is_same<T, std::string>::value)   init("", "string");
            else throw "Unknown type in publisher declared in default constructor";
        }

        std::string toString (const T &val) {
            return "{\"val\":" + std::to_string(val) + "}";
        }

        void setAddress(const std::string &address) {
            this->_address = this->_app->getFullAppPath() + "ipc/" + this->_typeName + "/" + address;
//            std::cout << "Binding to address " << this->_address << " length is " << this->_address.size();

            std::filesystem::path filePath = this->_address;
            auto dirPath = filePath.parent_path();
            if (!std::filesystem::exists(dirPath))
                std::filesystem::create_directories(dirPath);

            auto conf = Config::get();
            (*conf)["_publishers"].push_back(this->_address);
            conf->update();

            _publisher.bind("ipc://"+this->_address);

            std::cout << "Created publisher: " << this->_address << std::endl;
        }

        void init(T initialState, std::string typeName) {
            MessageCourier<T>::init(initialState, typeName);
            boost::system::error_code rc;
//            auto monitorSocket = _publisher.monitor(*this->_app->getContext(), ZMQ_EVENT_ACCEPTED, rc);
//            std::cout << "rc is " << rc << std::endl;
//            monitorSocket.async_receive(asio::buffer(_buf), [this](auto const &error_code, auto bytes_transferred){
//                if (error_code) {
//                    std::cout << "While monitoring publisher got an error " << error_code << std::endl;
//                    return;
//                }
//
//                auto output = std::string(_buf.data(), bytes_transferred);
//
//                std::cout << "Got output " << output << std::endl;
//            });
            _toString = std::bind(&Publisher::toString, this, std::placeholders::_1);
        }

        TtoString _toString;
        azmq::pub_socket _publisher;
        std::array<char, 256> _buf{};

    };

}

#pragma once

#include "AppIO.hpp"
#include "MessageCourier.hpp"

namespace AppIO {

    template<class T>
    class Publisher: public MessageCourier<T> {
    public:
        Publisher(const std::string &address, const T& initialValue) : Publisher(initialValue) {
            setAddress(address);
        }
        void send(const T &val) {
            this->_lastState = val;
            nlohmann::json j = val;
//            std::cout << "publishing " << strToSend << "\n";
            _publisher.send(asio::buffer(j.dump()));
        }
    private:
        explicit Publisher(const T& initialValue) :
                MessageCourier<T>(),
                _publisher(*this->_app->getContext()) {
            nlohmann::json j = initialValue;
            MessageCourier<T>::init(initialValue, j.type_name());
        }

        void setAddress(const std::string &address) {
            this->_address = this->_app->getFullAppPath() + "ipc/" + this->_typeName + "/" + address;
//            std::cout << "Binding to address " << this->_address << " length is " << this->_address.size();

            std::filesystem::path filePath = this->_address;
            auto dirPath = filePath.parent_path();
            if (!std::filesystem::exists(dirPath))
                std::filesystem::create_directories(dirPath);

            auto config = Config::get();
            config->insert({{"_publishers", {this->_address}}});
            config->commit();

            _publisher.bind("ipc://"+this->_address);

            std::cout << "Created publisher: " << this->_address << std::endl;
        }

        azmq::pub_socket _publisher;
        std::array<char, 256> _buf{};
    };
}

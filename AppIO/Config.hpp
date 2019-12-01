#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>
#include "AppIO.hpp"

namespace AppIO {
    namespace asio = boost::asio;

    class Config {
    public:
        static std::shared_ptr<Config> get() {
            static std::weak_ptr<Config> _instance;
            // Todo: support multithread with mutex lock
            if (auto ptr = _instance.lock()) { // .lock() returns a shared_ptr and increments the refcount
                return ptr;
            }
            auto ptr = std::shared_ptr<Config>(new Config());
            ptr->_self = ptr;
            _instance = ptr;
            return ptr;
        }
        Config(const Config &) = delete;
        ~Config() {
            std::cout << "Destructing config" << std::endl;
        }

        void commit() {
            updateConfigFile();
        }

        std::shared_ptr<nlohmann::json> data() {
            return _config;
        }

        void restructure(nlohmann::json completeData) {
            *_config = completeData;
        }

        void insert(const nlohmann::json& mergeIntoConfig) { // Example {{"foo", "bar"}} = {"foo":"bar"}
//            TODO: handle all kinds of objects nlohmann::json::value_t::object
            _config->insert(mergeIntoConfig.begin(), mergeIntoConfig.end());
        }

    private:
        Config() : _app(AppIO::instance()),
                    _pendingWrite(false),
                    _in(std::make_shared<asio::posix::stream_descriptor>(*_app->getContext()))
        {
            initializeConfig();
            _app->addDestructor([this](){
                _self.reset();
            });
        }

        std::shared_ptr<nlohmann::json> _config;

// todo: read this https://stackoverflow.com/questions/36304000/asio-is-there-an-automatically-resizable-buffer-for-receiving-input
        void readConfigFile() {
            auto path = _filePath.string();
            int dev = open(path.c_str(), O_RDONLY);
            if (dev == -1) throw std::runtime_error("failed to open device " + path);
            _in->assign(dev);
            _in->async_read_some(asio::buffer(_buf), [this](auto const &error_code, auto bytes_transferred) {
                std::cout << "\nerror is " << error_code << "\n";
                auto output = std::string(_buf.data(), bytes_transferred);
                std::cout << "got new output " << output << "\n"; //<< " parsed is "<< _lastState << "\n";
            });
        }

        void updateConfigFile() {
            if (_out) {
                _pendingWrite = true;
                return;
            }

            auto path = _filePath.string();
            int dev = open(path.c_str(), O_WRONLY);
            if (dev == -1) throw std::runtime_error("failed to open device " + path);
            _out = std::make_shared<asio::posix::stream_descriptor>(*_app->getContext());
            _out->assign(dev);

            std::string configAsString = _config->dump() + "\n";
            auto buf = asio::buffer(configAsString.c_str(), configAsString.length());
            _out->async_write_some(buf, [this](auto const &error_code, auto bytes_transferred) {
                if (error_code) {
                    std::cout << "Got an error " << error_code << " when writing to config file\n";
                    return;
                }
//                std::cout << "successfully wrote to config file\n";
                _out.reset();
                if (_pendingWrite) {
                    _pendingWrite = false;
                    updateConfigFile();
                }
            });
        }

        nlohmann::json openConfig(const std::filesystem::path &cfgFile) {
            nlohmann::json configAsJson;
            std::ifstream inCfg(cfgFile.string());
            inCfg >> configAsJson;
            inCfg.close();
            return configAsJson;
        }

        void initializeConfig() {
            _filePath = _app->getFullAppPath() + "config.json";
            if (!std::filesystem::exists(_filePath)) {
                writeDefaultConfig(_filePath);
            }

            _config = std::make_shared<nlohmann::json>(openConfig(_filePath));
        }

        void writeDefaultConfig(const std::filesystem::path &cfgFile) {
            auto dirPath = cfgFile.parent_path();
            if (!std::filesystem::exists(dirPath)) {
                std::filesystem::create_directories(dirPath.string());
            }

            std::ofstream outCfg(cfgFile.string());
            nlohmann::json j;
            std::time_t result = std::time(nullptr);
            j["_created"] = std::asctime(std::localtime(&result));
            outCfg << j << std::endl;
            outCfg.close();
        }


        std::shared_ptr<AppIO> _app;
        std::filesystem::path _filePath;
        bool _pendingWrite;
        std::shared_ptr<asio::posix::stream_descriptor> _out;
        std::shared_ptr<asio::posix::stream_descriptor> _in;
        std::array<char, 256> _buf{};
        std::shared_ptr<Config> _self;
    };

}

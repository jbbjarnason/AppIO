#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>
#include <mutex>
#include <memory>
#include <boost/asio.hpp>

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
            _instance = ptr;
            return ptr;
        }

        Config(const Config &) = delete;

        ~Config() {}

        nlohmann::json& operator[] (const std::string& key) {
            return (*_config)[key];
        }

        auto find (const std::string& key) {
            return _config->find(key);
        }

        auto end () {
            return _config->end();
        }

        void update() {
            updateConfigFile();
        }

        template<class T>
        void set(const std::string& key, T value) { // Overrides and or adds the value

        }

        template<class T>
        bool add(const std::string& key, T value) { // Adds value if it is not existing, returns true if it got inserted.
            return true;
        }

    private:

        Config() : _app(AppIO::instance()),
                    _in(std::make_shared<asio::posix::stream_descriptor>(*_app->getContext()))
        {
            initializeConfig();
        }

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
            _configLockGuard.lock();
            if (_out) { // If pending write, cancel it and create a new one with more new config
                _out->cancel();
                _out.reset();
            }
            _configLockGuard.unlock();

            auto path = _filePath.string();
            int dev = open(path.c_str(), O_WRONLY);
            if (dev == -1) throw std::runtime_error("failed to open device " + path);
            _out = std::make_shared<asio::posix::stream_descriptor>(*_app->getContext());
            _out->assign(dev);

            std::string configAsString = _config->dump();
            auto buf = asio::buffer(configAsString.c_str(), configAsString.length());
            _out->async_write_some(buf, [this](auto const &error_code, auto bytes_transferred) {
                if (error_code) {
                    std::cout << "Got an error " << error_code << " when writing to config file\n";
                    return;
                }
//                std::cout << "successfully wrote to config file\n";
                _out.reset();
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
            (*_config)["_senders"] = {};
        }

        void writeDefaultConfig(const std::filesystem::path &cfgFile) {
            auto dirPath = cfgFile.parent_path();
            if (!std::filesystem::exists(dirPath)) {
                std::filesystem::create_directories(dirPath);
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
        std::shared_ptr<nlohmann::json> _config;
        std::mutex _configLockGuard;
        std::shared_ptr<asio::posix::stream_descriptor> _out;
        std::shared_ptr<asio::posix::stream_descriptor> _in;
        std::array<char, 256> _buf{};
    };

}

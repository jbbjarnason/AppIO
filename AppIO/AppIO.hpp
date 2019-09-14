#pragma once

#include <fstream>
#include <azmq/socket.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <nlohmann/json.hpp>
#include <array>
#include <iostream>
#include <type_traits>
#include <memory>
#include <filesystem>
#include <mutex>

namespace AppIO {

    namespace asio = boost::asio;
    namespace options = boost::program_options;

// https://dakerfp.github.io/post/weak_ptr_singleton/
    class AppIO {
    private:
        AppIO() : _ioContext(std::make_shared<asio::io_context>(1)),
                  _in(*_ioContext),
                  _signals(*_ioContext, SIGINT, SIGTERM) {
        }

        std::shared_ptr<asio::io_context> _ioContext;
        std::string _appName;
        std::string _appInstanceName;
        std::shared_ptr<nlohmann::json> _config;
        std::filesystem::path _cfgFile;
        asio::posix::stream_descriptor _in;
        std::shared_ptr<asio::posix::stream_descriptor> _out;
        std::array<char, 256> _buf{};
        std::mutex _configLockGuard;
        boost::asio::signal_set _signals;


        AppIO(const AppIO &) = delete;

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

        nlohmann::json openConfig(const std::filesystem::path &cfgFile) {
            nlohmann::json configAsJson;
            std::ifstream inCfg(cfgFile.string());
            inCfg >> configAsJson;
            inCfg.close();
            return configAsJson;
        }

        void initializeConfig() {
            std::string home = getenv("HOME");
            _cfgFile = home + "/.industry/config/" + getAppNameAndInstance() + ".json";
            if (!std::filesystem::exists(_cfgFile)) {
                writeDefaultConfig(_cfgFile);
            }

            _config = std::make_shared<nlohmann::json>(openConfig(_cfgFile));
            (*_config)["_senders"] = {};
        }

    public:
        ~AppIO() {}

        static std::shared_ptr<AppIO> instance() {
            static std::weak_ptr<AppIO> _instance;
            // Todo: support multithread with mutex lock
            if (auto ptr = _instance.lock()) { // .lock() returns a shared_ptr and increments the refcount
                return ptr;
            }
            auto ptr = std::shared_ptr<AppIO>(new AppIO());
            _instance = ptr;
            return ptr;
        }

        std::shared_ptr<asio::io_context> getContext() {
            return _ioContext;
        }

        const std::string &getAppName() {
            return _appName;
        }

        const std::string &getInstanceAppName() {
            return _appInstanceName;
        }

        const std::string getAppNameAndInstance() {
            return _appName + "/" + _appInstanceName;
        }

        std::shared_ptr<nlohmann::json> getConfig() {
            return _config;
        }

        void
        readConfigFile() { // todo: read this https://stackoverflow.com/questions/36304000/asio-is-there-an-automatically-resizable-buffer-for-receiving-input
            auto path = _cfgFile.string();
            int dev = open(path.c_str(), O_RDONLY);
            if (dev == -1) throw std::runtime_error("failed to open device " + path);
            _in.assign(dev);
            _in.async_read_some(asio::buffer(_buf), [this](auto const &error_code, auto bytes_transferred) {
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

            auto path = _cfgFile.string();
            int dev = open(path.c_str(), O_WRONLY);
            if (dev == -1) throw std::runtime_error("failed to open device " + path);
            _out = std::make_shared<asio::posix::stream_descriptor>(*_ioContext);
            _out->assign(dev);

            std::string configAsString = _config->dump();
            auto buf = asio::buffer(configAsString.c_str(), configAsString.length());
            _out->async_write_some(buf, [this](auto const &error_code, auto bytes_transferred) {
                if (error_code) {
                    std::cout << "Got an error " << error_code << " when writing to config file\n";
                    return;
                }
                std::cout << "successfully wrote to config file\n";
                _out.reset();
            });
        }

        void initialize(int argc, char **argv) {
            _appName = std::filesystem::path(argv[0]).filename();

            options::options_description desc{"Options"};
            desc.add_options()
                    ("help,h", "Help screen")
                    ("name,n", options::value<std::string>()->default_value("default"),
                     "Application named used for configuration and data distribution topics");

            options::variables_map vm;
            options::store(parse_command_line(argc, argv, desc), vm);
            options::notify(vm);


            if (vm.count("help")) {
                std::cout << desc << '\n';
                exit(0);
            }
            _appInstanceName = vm["name"].as<std::string>();
            std::cout << "Starting app: " << _appName << "." << _appInstanceName << '\n';

            initializeConfig();

            _signals.async_wait([this](auto, auto) {
                _ioContext->stop();
                std::cout << " deinitializing\n";
            });
        }

        void run() {
            _ioContext->run();
        }
    };

}



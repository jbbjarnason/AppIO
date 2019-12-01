#pragma once

#include <fstream>
#include <azmq/socket.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <memory>
#include "filesystem.hpp"

namespace AppIO {
#define PORT 7721

    namespace asio = boost::asio;
    namespace options = boost::program_options;

// https://dakerfp.github.io/post/weak_ptr_singleton/
    class AppIO {
    private:
        AppIO() : _ioContext(std::make_shared<asio::io_context>(1)),
                  _signals(*_ioContext, SIGINT, SIGTERM, SIGQUIT) {
        }

        std::shared_ptr<asio::io_context> _ioContext;
        std::string _processName;
        std::string _appName;
        boost::asio::signal_set _signals;
        std::vector<std::function<void()>> _destructFunctions;

        AppIO(const AppIO &) = delete;

        void destruct() {
            std::cout << std::endl << "Destructing app " << getAppName() << std::endl;
            for (const auto& externalDestructFunc : _destructFunctions) {
                externalDestructFunc();
            }
        }

    public:
        ~AppIO() {
            std::cout << "Exiting now" << std::endl;
        }

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

        const std::string &getProcessName() {
            return _processName;
        }

        const std::string &getAppName() {
            return _appName;
        }

        const std::string getFullAppName() {
            return getProcessName() + "/" + getAppName();
        }

        const std::string getGlobalPath() {
            std::string home = getenv("HOME");
            return home + "/.appio/";
        }

        const std::string getProcessPath() {
            return getGlobalPath()+getProcessName()+"/";
        }

        const std::string getFullAppPath() {
            return getGlobalPath()+getFullAppName()+"/";
        }

        void initialize(int argc, char **argv) {
            _processName = std::filesystem::path(std::string(argv[0])).filename();

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
            _appName = vm["name"].as<std::string>();
            std::cout << "Starting app: " << getFullAppName() << std::endl;

            _signals.async_wait([this](auto, auto) {
                stop();
            });
        }

        void run() {
            _ioContext->run();
        }

        void stop() {
            _ioContext->stop();
            destruct();
        }

        void addDestructor(std::function<void()> destructFunc) {
            _destructFunctions.push_back(destructFunc);
        }
    };

}



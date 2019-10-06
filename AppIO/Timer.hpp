#pragma once

#include "AppIO.hpp"

namespace AppIO {

    class Timer {
    public:
        using timerCallback = std::function<void()>;
        using nanosec = const std::chrono::nanoseconds &;

        static std::shared_ptr<Timer> createSingleShot(nanosec nanosecs, timerCallback ownerOnTimeout) {
            auto instance = std::make_shared<Timer>(nanosecs, ownerOnTimeout);
            instance->_self = instance;
            return instance;
        }

        static std::shared_ptr<Timer> createInterval(nanosec nanosecs, timerCallback ownerOnTimeout) {
            auto instance = std::make_shared<Timer>(nanosecs, ownerOnTimeout, false);
            instance->_self = instance;
            return instance;
        }

        void cancel() {
            _timer->cancel();
            _timer.reset();
            _self.reset();
        }

        Timer(nanosec nanosecs, timerCallback ownerOnTimeout, bool singleShot = true) :
                _timer(std::make_unique<asyncTimer>(*AppIO::instance()->getContext(), nanosecs)),
                _ownerCallback(ownerOnTimeout){
            if (singleShot) _timer->async_wait(singleShotTimeout());
            else _timer->async_wait(recurringTimeout(nanosecs));
        }

    private:
        using asyncTimer = boost::asio::high_resolution_timer;
        using internalTimerCallback = std::function<bool(boost::system::error_code const &error_code)>;

        inline internalTimerCallback onTimeout() {
            return [this](boost::system::error_code const &error_code) {
                if (error_code) {
                    std::cout << "Timer got boost error " << error_code.message() << std::endl;
                    return false;
                }
                _ownerCallback();
                return true;
            };
        }

        inline internalTimerCallback singleShotTimeout() {
            return [this](boost::system::error_code const &error_code) {
                auto status = onTimeout()(error_code);
                _self.reset();
                return status;
            };
        }

        inline internalTimerCallback recurringTimeout(nanosec nanosecs) {
            // never let nanosecs pass to lambda as reference somehow it gets lost when timer goes out of owner's scope
            return [this, nanosecs](boost::system::error_code const &error_code) {
                if (!onTimeout()(error_code)) return false;
                _timer->expires_at(_timer->expiry() + nanosecs);
                _timer->async_wait(recurringTimeout(nanosecs));
                return true;
            };
        }
        std::unique_ptr<asyncTimer> _timer;
        std::shared_ptr<Timer> _self;
        timerCallback _ownerCallback;
    };

}
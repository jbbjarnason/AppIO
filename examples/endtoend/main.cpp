
#include <AppIO/AppIO.hpp>
#include <AppIO/Publisher.hpp>
#include <AppIO/Subscriber.hpp>
#include <AppIO/Timer.hpp>


int main(int argc, char **argv) {
    auto app = AppIO::AppIO::instance();
    app->initialize(argc, argv);

    AppIO::Publisher<bool> output("NASDAQ");

    AppIO::Subscriber<bool> input("whazza");
    input.setCallback([](bool state) {
        auto at = std::chrono::steady_clock::now().time_since_epoch().count();
        std::cout << "\n\n\n state is " << state << " at " << at << "\n\n\n";
    });

    bool lastState = false;

    auto timer = AppIO::Timer::createInterval(std::chrono::milliseconds(10), [&output, &lastState]() {
        lastState = !lastState;
        auto at = std::chrono::steady_clock::now().time_since_epoch().count();
        std::cout << "got new time interval publishing " << lastState << " at " << at << std::endl;
        output.send(lastState);
    });


    app->run();
}

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
        std::cout << "\n\n\n state is " << state << "\n\n\n";
    });

    bool lastState = false;

    auto timer = AppIO::Timer::createInterval(std::chrono::milliseconds(1000), [&output, &lastState]() {
        lastState = !lastState;
        std::cout << "got new time interval publishing " << lastState << std::endl;
        output.send(lastState);
    });


    app->run();

    app->destruct();
}
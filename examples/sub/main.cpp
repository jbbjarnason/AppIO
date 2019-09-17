
#include <AppIO/AppIO.hpp>
#include <AppIO/Subscriber.hpp>

int main(int argc, char **argv) {
    auto app = AppIO::AppIO::instance();
    app->initialize(argc, argv);

    AppIO::Subscriber<bool> input("whazza");
    input.setCallback([](bool state) {
        std::cout << "\n\n\n state is " << state << "\n\n\n";
    });

    app->run();
}
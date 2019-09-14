
#include <AppIO/AppIO.hpp>
#include <AppIO/Publisher.hpp>
#include <AppIO/Subscriber.hpp>


int main(int argc, char **argv) {
    auto app = AppIO::AppIO::instance();
    app->initialize(argc, argv);

    AppIO::Subscriber<bool> input("whazza");
    input.setCallback([](bool state) {
        std::cout << "\n\n\n state is " << state << "\n\n\n";
    });

    AppIO::Publisher<bool> output("NASDAQ");

    boost::asio::steady_timer t(*app->getContext(), std::chrono::seconds(1));

    auto onTimeout = [&output, app](boost::system::error_code const &ec) {
        std::cout << "got new time interval publishing data\n";
        output.send(true);
    };

    t.async_wait(onTimeout);

    app->run();
}
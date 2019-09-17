
#include <AppIO/AppIO.hpp>
#include <AppIO/Publisher.hpp>

int main(int argc, char **argv) {
    auto app = AppIO::AppIO::instance();
    app->initialize(argc, argv);

    AppIO::Publisher<bool> output("NASDAQ");

    boost::asio::steady_timer t(*app->getContext(), std::chrono::seconds(1));

    auto onTimeout = [&output, app](boost::system::error_code const &ec) {
        std::cout << "got new time interval publishing data\n";
        output.send(true);
    };

    t.async_wait(onTimeout);

    boost::asio::steady_timer t2(*app->getContext(), std::chrono::seconds(2));

    auto onTimeout2 = [&output, app](boost::system::error_code const &ec) {
        std::cout << "got new time interval publishing data\n";
        output.send(false);
    };

    t2.async_wait(onTimeout2);

    app->run();
}

#include "../../AppIO/Publisher.hpp"
#include "AppIO/AppIO.hpp"

int main(int argc, char **argv) {
    Publisher p;
    auto app = AppIO::AppIO::instance();
    app->initialize(argc, argv);

    app->run();
}
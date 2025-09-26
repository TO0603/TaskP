#include "Client.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    kvs::qt::Application app( argc, argv );
    Client client( app );;
    return app.run();
}

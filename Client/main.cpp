#include "Client.h"

int main(int argc, char *argv[])
{
    kvs::qt::Application app( argc, argv );
    Client client( app );
    app.run();
}

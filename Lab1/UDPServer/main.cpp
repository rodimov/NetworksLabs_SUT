#include <QtCore>

#include "server.h"
#include "func.h"

int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);
	int port = 0;

	consoleOut() << "Enter port: ";
	consoleOut().flush();
	consoleIn() >> port;

	Server* server = new Server(port, &app);

	return app.exec();
}

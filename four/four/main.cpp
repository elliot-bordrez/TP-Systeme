#include <QtCore/QCoreApplication>
#include <QSettings>
#include "four.h"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	QSettings *settings = new QSettings("config.ini", QSettings::IniFormat);
	QStringList l = settings->allKeys();
	if (!settings->contains("WS/port")) {
		settings->beginGroup("WS");
		settings->setValue("port", "3005");
		settings->endGroup();
		settings->sync();
	}

	int port = settings->value("WS/port", "config").toInt();

	new four(port);

	return a.exec();
}

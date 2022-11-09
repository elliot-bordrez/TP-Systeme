//**********************************************************************************************//
//* Programme	:	four.cpp																	//
//*																	Date : 08/11/2022			//
//*																	Last update : 09/11/2022	//
//*---------------------------------------------------------------------------------------------//
//* Developper	:	Elliot Bordrez / Valentin Foure												//
//*---------------------------------------------------------------------------------------------//
//*																								//
//* Goal		:	Receive values of the Four and send them to database						//
//*																								//
//**********************************************************************************************//

#include "four.h"

/* Constructeur */
four::four(quint16 port)
{
	card = Register_Card(PCI_9111DG, 0);

	this->webSocketServer = new QWebSocketServer(QStringLiteral("Server WebSocket"), QWebSocketServer::NonSecureMode);

	if (this->webSocketServer->listen(QHostAddress::AnyIPv4, port))
	{
		qDebug() << "Server WebSocket: Nouvelle connexion sur le port " << port << "\n";

		QObject::connect(webSocketServer, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
	}
	else
	{
		qDebug() << "Server WebSocket: Erreur d'ecoute sur le port " << port << "\n";
	}
}

/* Destructeur */
four::~four()
{
	four::issue(0, 0);

	webSocketServer->close();
}

/* New WebSocket Client Connection */
void four::onNewConnection()
{
	socket = this->webSocketServer->nextPendingConnection();

	QObject::connect(socket, SIGNAL(textMessageReceived(const QString&)), this, SLOT(processTextMessage(const QString&)));

	QObject::connect(socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));

	this->wsclients.push_back(socket);
}

/* WebSocket Client Disconnection */
void four::socketDisconnected()
{
	issue(0, 0);

	timer->stop();

	qDebug() << "Server WebSocket: Deconnexion\n";
}

/* Receive message of WebSocket Client */
void four::processTextMessage(const QString & message)
{
	if (message.section(";", 0, 0) == "connection")
	{
		QString hostName = message.section(";", 1, 1);
		QString userName = message.section(";", 2, 2);
		QString password = message.section(";", 3, 3);
		QString dbName = message.section(";", 4, 4);

		dbInit(hostName, userName, password, dbName);
	}
	else if (message.section(";", 0, 0) == "value")
	{
		QString value = message.section(";", 1, 1);

		issue(0, value.toInt());
	}
	else
	{
		qDebug() << "Server WebSocket: Protocol message error\n";
	}

}

/* Initialisation database */
void four::dbInit(QString hostName, QString userName, QString password, QString dbName)
{
	db = QSqlDatabase::addDatabase("QMYSQL");
	db.setHostName(hostName);
	db.setUserName(userName);
	db.setPassword(password);
	db.setDatabaseName(dbName);

	if (db.open()) {
		qDebug() << "Database: New Connection\n";
		selectValue();
		timerOn();
	}
	else {
		qDebug() << db.lastError();
	}

}

/* Start timer of the sampling */
void four::timerOn()
{
	timer = new QTimer(this);
	QObject::connect(timer, SIGNAL(timeout()), this, SLOT(insertValue()));

	timer->start(10000);
}

/* Send value at the card */
void four::issue(int value1, int value2)
{
	if (value2 > 0)
	{
		value2 /= 10;
	}

	AO_VWriteChannel(card, value1, value2);
}

/* Recuperation of four values and send them to database */
void four::insertValue()
{
	float bruteValue = AI_VReadChannel(card, 0, AD_B_10_V, &voltage);
	float voltageValue = voltage;
	float tempValue = (voltageValue * 51) - 73;

	QSqlQuery query;
	query.prepare("INSERT INTO `four`(`bruteValue`, `tension`, `temperature`) VALUES(:bruteValue, :voltageValue, :tempValue)");
	query.bindValue(":bruteValue", bruteValue);
	query.bindValue(":voltageValue", voltageValue);
	query.bindValue(":tempValue", tempValue);
	query.exec();

	query.exec("SELECT `bruteValue`, `tension`, `temperature`, DATE_FORMAT(`date`, '%H:%i:%s %d/%m/%Y') FROM `four` ORDER BY `date` DESC");

	if (query.next())
	{
		QString bruteValue = query.value(0).toString();
		QString voltageValue = query.value(1).toString();
		QString tempValue = query.value(2).toString();
		QString date = query.value(3).toString();

		QString sendText = bruteValue + ";" + voltageValue + ";" + tempValue + ";" + date;

		for (QList<QWebSocket*>::iterator it = wsclients.begin(); it != wsclients.end(); it++) {
			(*it)->sendTextMessage(sendText);
		}
	}
}

/* Send	values at WebSocket Client */
void four::selectValue()
{
	QSqlQuery query;
	query.exec("SELECT `bruteValue`, `tension`, `temperature`, DATE_FORMAT(`date`, '%H:%i:%s %d/%m/%Y') FROM `four` ORDER BY `date`");

	while (query.next())
	{
		QString bruteValue = query.value(0).toString();
		QString voltageValue = query.value(1).toString();
		QString tempValue = query.value(2).toString();
		QString date = query.value(3).toString();

		QString sendText = bruteValue + ";" + voltageValue + ";" + tempValue + ";" + date;

		socket->sendTextMessage(sendText);
	}

}
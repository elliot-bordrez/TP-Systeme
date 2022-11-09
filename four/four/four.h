#pragma once

#include <QObject>
#include <qdebug.h>

#include <QSqlDatabase>
#include <QtSql/QtSql>
#include <QSqlQuery>
#include <QtSql>

#include "Dask.h"

#include <QWebSocket>
#include <QWebSocketServer>

#include <QFile>
#include <QTextStream>

class four : public QObject
{
	Q_OBJECT

	QWebSocketServer *webSocketServer;
	QWebSocket * socket;
	QList<QWebSocket*> wsclients;

public:

	/* Constructeur */
	four(quint16 port);

	/* Destructeur */
	~four();

	/* Initialisation database */
	void dbInit(QString hostName, QString userName, QString password, QString dbName);

private slots:

	/* New WebSocket Client Connection */
	void onNewConnection();

	/* WebSocket Client Disconnection */
	void socketDisconnected();

	/* Receive message of WebSocket Client */
	void processTextMessage(const QString& message);

	/* Start timer of the sampling */
	void timerOn();

	/* Send value at the card */
	void issue(int value1, int value2);

	/* Recuperation of four values and send them to database */
	void insertValue();

	/* Send	values at WebSocket Client */
	void selectValue();

private:

	QSqlDatabase db;

	I16 card;

	QTimer *timer;

	double voltage;
};



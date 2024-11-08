#include "statisticsdb.h"
#include <QSqlDatabase>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QTableView>
#include <QtSql>


StatisticsDB::StatisticsDB()
{

    createDbConnection();
}


bool StatisticsDB::createDbConnection()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("db.db");
    if(!db.open()){
        qDebug()<<"Error while opening SQLITE"<<db.lastError().text();
        return false;
    }
    else
        qDebug()<<"SQLITE success opened"<<db.lastError().text();

    return true;
}

StatisticsDB::~StatisticsDB()
{
    QSqlDatabase::removeDatabase("QSQLITE");
}

/*
              kueued - create xml data for kueue's qmon 
              (C) 2012 Stefan Bogner <sbogner@suse.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the

    Free Software Foundation, Inc.
    59 Temple Place - Suite 330
    Boston, MA  02111-1307, USA

    Have a lot of fun :-)

*/

#include "database.h"
#include "debug.h"
#include "settings.h"

#include <QDateTime>

Database::Database()
{   
    QSqlDatabase mysqlDB = QSqlDatabase::addDatabase("QMYSQL", "mysqlDB" );
    
    mysqlDB.setHostName( Settings::mysqlHost() );
    mysqlDB.setDatabaseName( Settings::mysqlDatabase() );
    mysqlDB.setUserName( Settings::mysqlUser() );
    mysqlDB.setPassword( Settings::mysqlPassword() );
    
    if ( !mysqlDB.open() )
    {
        Debug::print( "database", "Failed to open the database " + mysqlDB.lastError().text() );
    }

    QSqlQuery query( mysqlDB );
       
    if ( !query.exec( "CREATE TABLE IF NOT EXISTS qmon_siebel( ID VARCHAR(20) PRIMARY KEY UNIQUE, QUEUE TEXT, "
                      "HOURS TEXT, GEO TEXT, ODATE TEXT, ADATE TEXT, QDATE TEXT, STATUS TEXT, SEVERITY TEXT, "
                      "CONTRACT TEXT, SLA TEXT, CRSR TEXT, BDESC TEXT, CUSTOMER TEXT, CONTACTVIA TEXT )" ) )
    {
        Debug::print( "database", "Error: " + query.lastError().text() );
    }
    
    if ( !query.exec( "CREATE TABLE IF NOT EXISTS qmon_chat( ID VARCHAR(40) PRIMARY KEY UNIQUE, SR VARCHAR(15), "
                      "NAME TEXT, DATE TEXT )" ) ) 
    {
        Debug::print( "database", "Error: " + query.lastError().text() );
    }
}

Database::~Database()
{   
    QSqlDatabase::removeDatabase( "mysqlDB" );
}

QStringList Database::getQmonSiebelList()
{
    QSqlQuery query( QSqlDatabase::database( "mysqlDB" ) );
    QStringList l;
    
    query.prepare( "SELECT ID FROM qmon_siebel" );
    
    query.exec();
    
    while( query.next() )
    {
        l.append( query.value( 0 ).toString() );
    }
    
    return l;    
}

bool Database::siebelExistsInDB( const QString& id )
{
    QSqlQuery query( QSqlDatabase::database( "mysqlDB" ) );
    
    query.prepare( "SELECT ID FROM qmon_siebel WHERE ( ID = :id )" );
    query.bindValue( ":id", id );
    
    query.exec();
    
    if ( query.next() )
    {
        return true;
    }
    else 
    {    
        return false;
    }
}

bool Database::isChat( const QString& id )
{
    QSqlQuery query( QSqlDatabase::database( "mysqlDB" ) );
    
    query.prepare( "SELECT ID FROM qmon_chat WHERE ( SR = :id )" );
    query.bindValue( ":id", id );
    
    query.exec();
    
    if ( query.next() )
    {
        return true;
    }
    else 
    {    
        return false;
    }
}

QList< SiebelItem* > Database::getSrsForQueue( const QString& queue )
{
    QSqlQuery query( QSqlDatabase::database( "mysqlDB" ) );
    QList< SiebelItem* > list;
    
    if (  queue == "NONE" )
    {
        query.prepare( "SELECT ID, QUEUE, HOURS, GEO, ODATE, ADATE, QDATE, STATUS, SEVERITY, "
                       "CONTRACT, SLA, CRSR, BDESC, CUSTOMER, CONTACTVIA, HIGHVALUE, CRITSIT FROM qmon_siebel" );
    }
    else
    {
        query.prepare( "SELECT ID, QUEUE, HOURS, GEO, ODATE, ADATE, QDATE, STATUS, SEVERITY, "
                       "CONTRACT, SLA, CRSR, BDESC, CUSTOMER, CONTACTVIA, HIGHVALUE, CRITSIT FROM qmon_siebel WHERE ( QUEUE = :queue )" );
        
        query.bindValue( ":queue", queue );
    }
    
    query.exec();
    
    while ( query.next() ) 
    {
        SiebelItem* si = new SiebelItem;
        
        si->id = query.value( 0 ).toString();
        si->queue = query.value( 1 ).toString();
        si->hours = query.value( 2 ).toString();
        si->geo = query.value( 3 ).toString();
        si->odate = query.value( 4 ).toString();
        si->adate = query.value( 5 ).toString();
        si->qdate = query.value( 6 ).toString();
        si->status = query.value( 7 ).toString();
        si->severity = query.value( 8 ).toString();
        si->contract = query.value( 9 ).toString();
        si->sla = query.value( 10 ).toString();
        si->crSr = query.value( 11 ).toString();
        si->bdesc = query.value( 12 ).toString();
        si->customer = query.value( 13 ).toString();
        si->contactvia = query.value( 14 ).toString();
        si->highValue = query.value( 15 ).toBool();
        si->critSit = query.value( 16 ).toBool();
        
        if ( getBomgarQueue( query.value( 0 ).toString() ) == "NOCHAT" )
        {
            si->isChat = false;
        }
        else
        {
            si->isChat = true;
            si->bomgarQ = getBomgarQueue( query.value( 0 ).toString() );
        }
     
        if ( query.value( 11 ).toString().isEmpty() )
        {
            si->isCr = false;
        }
        else
        {
            si->isCr = true;
        }
        
        list.append( si );
    }
        
    return list;
}

QStringList Database::getCurrentBomgars()
{
    QStringList list;
    QSqlQuery query( QSqlDatabase::database( "mysqlDB" ) );

    query.prepare( "SELECT SR, NAME FROM qmon_chat" );
    
    query.exec();
    
    while( query.next() )
    {
        QString tmp = query.value( 0 ).toString() + "|||" + query.value( 1 ).toString();
        list.append( tmp );
    }

    return list;
}

QString Database::getBomgarQueue( const QString& id )
{
    QSqlQuery query( QSqlDatabase::database( "mysqlDB" ) );
    
    query.prepare( "SELECT NAME FROM qmon_chat WHERE ( SR = :id )" );
    query.bindValue( ":id", id );
    
    query.exec();
    
    if( query.next() ) 
    {
        return query.value( 0 ).toString();
    }
    else
    {
        return "NOCHAT";
    }
}

QString Database::convertTime( const QString& dt )
{
    QDateTime d = QDateTime::fromString( dt, "M/d/yyyy h:mm:ss AP" );
    return ( d.toString("yyyy-MM-dd hh:mm:ss") );
}

QStringList Database::getSrNumsForQueue( const QString& queue, const QString& geo )
{
    QSqlQuery query( QSqlDatabase::database( "mysqlDB" ) );
    QStringList list;

    query.prepare( "SELECT ID FROM qmon_siebel WHERE ( QUEUE = :queue ) AND ( GEO = :geo )" );
    query.bindValue( ":queue", queue );
    query.bindValue( ":geo", geo );

    query.exec();

    while( query.next() )
    {  
        list.append( query.value( 0 ).toString() );
    }

    return list;
}

/*QList< PseudoQueueItem* > Database::getPseudoQueues()
{
    QSqlQuery query( QSqlDatabase::database( "qmonDB" ) );
    QList< PseudoQueueItem* > list;
    
    query.prepare( "SELECT DisplayName, PseudoQueue from _NovQueuePseudoQueue" );
    query.exec();

    while ( query.next() )
    {
        PseudoQueueItem* i = new PseudoQueueItem;
        i->display = query.value( 0 ).toString();
        i->name = query.value( 1 ).toString();
        list.append( i );
    }
    
    return list;
}

QString Database::critSitFlagForSr( const QString& sr )
{
    QSqlQuery query( QSqlDatabase::database( "oracleDB" ) );
    query.prepare("SELECT CRITSIT_FLAG from NTSDM.OLAP_SR_DX2 WHERE SR_NUM = :srnum");
    query.bindValue( ":srnum", sr );
    query.exec();

    if ( query.next() )
    {
        return query.value( 0 ).toString();
    }
    else
    {
        return "ERROR";
    }
}

QString Database::highValueFlagForSr( const QString& sr )
{
    QSqlQuery query( QSqlDatabase::database( "oracleDB" ) );
    query.prepare("SELECT HIGHVALUE_FLAG from NTSDM.OLAP_SR_DX2 WHERE SR_NUM = :srnum");
    query.bindValue( ":srnum", sr );
    query.exec();

    if ( query.next() )
    {
        return query.value( 0 ).toString();
    }
    else
    {
        return "ERROR";
    }
}

QString Database::highValueCritSitFlagForSr( const QString& sr )
{
    QSqlQuery query( "SELECT HIGHVALUE_FLAG, CRITSIT_FLAG from NTSDM.OLAP_SR_DX2 WHERE SR_NUM = :srnum", QSqlDatabase::database( "oracleDB" ) );
    query.bindValue( ":srnum", sr );
    query.exec();

    if ( query.next() )
    {
        return query.value( 0 ).toString() + query.value( 1 ).toString();
    }
    else
    {
        return "ERROR";
    }
}

QStringList Database::getSrsForUser( const QString& user )
{
    QStringList list;
    
    QSqlQuery query( QSqlDatabase::database( "siebelDB" ) );
    query.prepare( "SELECT SR_NUM FROM SIEBEL.S_SRV_REQ WHERE OWNER_EMP_ID = ( SELECT PAR_ROW_ID FROM SIEBEL.S_USER WHERE LOGIN = :login ) AND SR_STAT_ID = 'Open'" );
    //query.prepare( "SELECT * FROM ALL_TABLES" );
    query.bindValue( ":login", user.toUpper() );
    query.exec();
    
    qDebug() << query.lastError().text();

    while ( query.next() )
    {
        list.append( query.value( 0 ).toString() );
    }
    
    return list;
}

QString Database::getBugForSr( const QString& sr )
{
    QSqlQuery query( QSqlDatabase::database( "oracleDB" ) );
    query.prepare( "SELECT DEFECT_ID from NTSDM.OLAP_SR_DX2 WHERE SR_NUM = :srnum" );
    query.bindValue( ":srnum", sr );
    query.exec();

    if ( query.next() )
    {
        return query.value( 0 ).toString();
    }
    else
    {
        return "NOBUG";
    }
}

QStringList Database::getOracleSrList()
{
    QSqlQuery query( QSqlDatabase::database( "oracleDB" ) );
    QStringList l;
    query.prepare( "SELECT SR_NUM FROM NTSDM.NTS_OPEN_SR WHERE OWNER='SBOGNER'" );
    query.exec();

    while( query.next() ) 
    {
            l.append( query.value( 0 ).toString() );
            qDebug() << "append";
    }

    return l;
}*/

#include "database.moc"

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

#include "xml.h"
#include "settings.h"
#include "debug.h"
#include "serverthread.h"
#include "database.h"
#include "network.h"

#include <iostream>
#include <QtSql>
#include <QSslSocket>
#include <QTcpSocket>
#include <QHostAddress>
#include <QtNetwork>
#include <QTime>

extern "C" {
#include <lber.h>
#include <ldap.h>
#include <sasl/sasl.h>
}

ServerThread::ServerThread( int sd, QObject* parent, bool ssl ) : QRunnable()
{

    mSocket = sd;
    mSsl = ssl;
}

ServerThread::~ServerThread()
{
    if ( QSqlDatabase::database( mMysqlDB ).isOpen() ) {
        QSqlDatabase::database( mMysqlDB ).close();
        QSqlDatabase::removeDatabase( mMysqlDB );
    }
    if ( QSqlDatabase::database( mQmonDB ).isOpen() ) {
        QSqlDatabase::database( mQmonDB ).close();
        QSqlDatabase::removeDatabase( mQmonDB );
    }
    if ( QSqlDatabase::database( mSiebelDB ).isOpen() ) {
        QSqlDatabase::database( mSiebelDB ).close();
        QSqlDatabase::removeDatabase( mSiebelDB );
    }
    if ( QSqlDatabase::database( mReportDB ).isOpen() ) {
        QSqlDatabase::database( mReportDB ).close();
        QSqlDatabase::removeDatabase( mReportDB );
    }
}

void ServerThread::run()
{
    mThreadID = QString::number( QThread::currentThreadId() );

    mMysqlDB = "mysqlDB-" + mThreadID;
    mSiebelDB = "siebelDB-" + mThreadID;
    mQmonDB = "qmonDB-" + mThreadID;
    mReportDB = "reportDB-" + mThreadID;

    QString cmd;
    QString out;

    int rc;
    int protocol = LDAP_VERSION3;
    unsigned sasl_flags = LDAP_SASL_QUIET;
    QByteArray user;
    QByteArray password;
    char *binddn = NULL;
    char *ldapuri = ber_strdup( Settings::ldapuri().toUtf8() );
    char hostname[ 1024 ];
    LDAP* ld;
    bool authenticated = false;

    gethostname( hostname, sizeof( hostname ) );
    mHostname = hostname;

    socket = new QSslSocket;

    if ( mSocket != 0 ) {
        socket->setSocketDescriptor( mSocket );
        socket->setLocalCertificate( Settings::sslcert().toUtf8(),QSsl::Pem);
        socket->setPrivateKey( Settings::sslkey().toUtf8(),QSsl::Rsa);
        if (mSsl) {
            socket->startServerEncryption();
            socket->waitForEncrypted();
        } 
/*        out = "Welcome to Kueued 5.5.6 (do the needfull)\r\n" ;
        socket->write(out.toUtf8());
        socket->flush();
        socket->waitForBytesWritten();*/
        socket->waitForReadyRead();
    } else {
        delete socket;
        return;
    }

    Debug::print( "serverthread", "Socket " + QString::number( mSocket ) + " connected using " + ((mSsl) ? "SSL" : "plain") );
    QString dm;

    dm += socket->peerAddress().toString();

    if ( socket->canReadLine() ) {
        /* read the client request (GET....) */
        QString r = socket->readLine();
        /* if request does not start with GET either the client uses
         * SSL and we in a non SSL thread or client send a invalid command */
        if (! r.startsWith("GET")) {
            Debug::print( "serverthread", "Socket " + QString::number( mSocket ) + " invalid command from client");
            socket->disconnectFromHost();
            delete socket;
            return;
        }
        while ( socket->canReadLine() ) {
            QString tmp = socket->readLine();
            if ( tmp.startsWith( "User-Agent" ) ) {
                dm += " - " + tmp.remove( "User-Agent: " ).trimmed() + " - ";
            }
            /* client sent authentication data */
            if (tmp.startsWith( "Authorization: Basic ") ) {
                QString auth;
                auth = tmp.remove( "Authorization: Basic ").trimmed();
                Debug::print( "serverthread", "Socket " + QString::number( mSocket ) + " client credentials: " + auth);
                QByteArray credentials;
                credentials.append(auth);
                auth = credentials.fromBase64(credentials);
                QStringList tokens = auth.split(":");
                user.append(tokens[0]);
                password.append(tokens[1]);
                user.append("\0");
                password.append("\0");
            }
        }
        /* if we have authentication data try to verify */
        if (! user.isEmpty() && ! password.isEmpty()) {
            rc = ldap_initialize(&ld, ldapuri);
            if ( rc != LDAP_SUCCESS ) {
                return;
            }
            if(ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &protocol) != LDAP_OPT_SUCCESS) {
                rc=ldap_destroy(ld);
                return;
            }
            QByteArray dn;
            dn.append("cn=");
            dn.append(user);
            dn.append(",o=Novell");
            binddn = dn.data();
            rc = ldap_simple_bind_s(ld,binddn,password);
            if(rc != LDAP_SUCCESS) {
               rc=ldap_destroy(ld);
               /* invalid crendetials */
                Debug::print( "serverthread", "Socket " + QString::number( mSocket ) + " authentication failure.");
                send_nok(403,"Forbidden");
                return;
            }
            /* correct credentials */
            rc = ldap_unbind(ld);
            authenticated = true;
        }
        /* kueued host runs a update job, no authentication
         * is needed for his own local connection */
        if ( socket->peerAddress() == socket->localAddress() ) {
            Debug::print( "serverthread", "Socket " + QString::number( mSocket ) + " local connection");
            authenticated = true;
        }
        if ( dm.isEmpty() ) {
            Debug::log( "serverthread", " - " + r.trimmed() + " using " + ((mSsl) ? "SSL" : "plain") );
        } else {
            Debug::log( "serverthread", dm + r.trimmed() + " using " + ((mSsl) ? "SSL" : "plain") );
        }
        Debug::print( "serverthread", "Socket " + QString::number( mSocket ) + " " + dm + r.trimmed() );
        QStringList tokens = r.split( QRegExp( "[ \r\n][ \r\n]*" ) );
        QString req = tokens[ 0 ];
        QString cmd = tokens[ 1 ];
        if ( req == "GET" ) {
/* qmon_date */
            if ( cmd.startsWith( "/qmon_date" ) ) {
                send_ok();
                Database::openMysqlDB( mMysqlDB );
                QString x = XML::qmonDate( Database::getSrsForQueue( "NONE", mMysqlDB ) );
                out=xml();
                out.append(x);
                socket->write(out.toUtf8());
/* ltsscustomers */
            } else if ( cmd.startsWith( "/ltsscustomers" ) ) {
                send_ok();
                Database::openReportDB( mReportDB );
                QString x = XML::ltssCust( Database::getLTSScustomersExt( mReportDB ) );
                out=xml();
                out.append(x);
                socket->write(out.toUtf8());
/* qmon */
            } else if ( cmd.startsWith( "/qmon" ) ) {
                send_ok();
                Database::openMysqlDB( mMysqlDB );
                QString x = XML::qmon( Database::getSrsForQueue( "NONE", mMysqlDB ) );
                out=xml();
                out.append(x);
                socket->write(out.toUtf8());
            }
/* srnrs */
            else if ( cmd.startsWith( "/srnrs" ) ) {
                send_ok();
                Database::openMysqlDB( mMysqlDB );
                QString q = cmd.remove( "/srnrs" );
                out=text();
                if ( q.remove( "/" ).isEmpty() ) {
                    out.append("Please specify queue");
                } else if ( !q.contains( "|" ) ) {
                    out.append("Please specify geo");
                } else {
                    QStringList l = Database::getSrnumsForQueue( q.remove( "/" ).split( "|" ).at( 0 ), q.remove( "/" ).split( "|" ).at( 1 ), mMysqlDB );
                    for ( int i = 0; i < l.size(); ++i ) {
                        out.append(l.at( i ) + "\n");
                    }
                }
                out.append("\r\n");
                socket->write(out.toUtf8());
/* srforcr */
            } else if ( cmd.startsWith( "/srforcr" ) ) {
                send_ok();
                Database::openMysqlDB( mMysqlDB );
                QRegExp srnr( "^[0-9]{11,12}$" );
                QString q = cmd.remove( "/srforcr" );
                out=text();
                if ((q.remove("/").isEmpty()) || (!srnr.exactMatch(q.remove("/")))) {
                    out.append("No SR number");
                } else {
                    out.append(Database::getSrForCr(q, mMysqlDB, mReportDB));
                }
                socket->write(out.toUtf8());
/* srinfo */
            } else if (cmd.startsWith( "/srinfo")) {
                send_ok();
                Database::openSiebelDB( mSiebelDB );
                Database::openMysqlDB( mMysqlDB );
                QRegExp srnr( "^[0-9]{11,12}$" );
                QString q = cmd.remove( "/srinfo" );
                if ( ( q.remove("/").isEmpty()) || (!srnr.exactMatch(q.remove("/")))) {
                    out=text();
                    out.append("No SR number\r\n");
                } else {
                    out=xml();
                    out.append(XML::sr( Database::getSrInfo( q.remove( "/" ), mSiebelDB, mMysqlDB, mReportDB ) ));
                }
                socket->write(out.toUtf8());
/* srstatus */            
             } else if ( cmd.startsWith( "/srstatus" ) ) {
                send_ok();
                Database::openSiebelDB( mSiebelDB );
                QRegExp srnr( "^[0-9]{11,12}$" );
                QString q = cmd.remove( "/srstatus" );
                out=text();
                if ((q.remove("/").isEmpty()) || (!srnr.exactMatch(q.remove("/")))) {
                    out.append("No SR number\r\n");
                } else {
                    out.append(Database::getSrStatus( q.remove( "/" ), mSiebelDB ));
                }
                socket->write(out.toUtf8());
/* latestkueue */
            } else if ( cmd.startsWith( "/latestkueue" ) ) {
                send_ok();
                out=text();
                out.append(Settings::latestVersion());
                socket->write(out.toUtf8());
/* detailed */
            } else if ( cmd.startsWith( "/detailed" ) ) {
                send_ok();
                QString q = cmd.remove( "/detailed" );
                out=text();
                if ( q.remove( "/" ).isEmpty() ) {
                    out.append("Please specify sr number\r\n");
                    socket->write(out.toUtf8());
                } else {
                    Database::openSiebelDB( mSiebelDB );
                    out.append(Database::getDetDesc( q, mSiebelDB ));
                    socket->write(out.toUtf8());
                }
/* ltssupdate */
            } else if ( cmd.startsWith( "/ltssupdate" ) ) {
                if (authenticated) {
                    send_ok();
                } else {
                    Debug::print( "serverthread", "Socket " + QString::number( mSocket ) + " denied ltssupdate without auth.");
                    send_nok(401,"Unauthorized\r\nWWW-Authenticate: Basic basic-credentials");
                    return;
                }
                Database::openMysqlDB( mMysqlDB );
                Database::openReportDB( mReportDB );
                int btime;
                QTime timer;
                timer.start();
                Debug::print( "serverthread", "Starting LTSS update..." );
                Database::updateLTSScustomers( mReportDB, mMysqlDB );
                btime = timer.elapsed() / 1000;
                Debug::print( "serverthread", "LTSS update finished, took " + QString::number( btime ) + " sec" );
                out=text();
                out.append("LTSS update took " +  QString::number( btime ) + " sec\n");
                out.append("LTSS Customer List updated.\n");
                socket->write(out.toUtf8());
/* updateDB */
            } else if ( cmd.startsWith( "/updateDB" ) ) {
                if (authenticated) {
                    send_ok();
                } else {
                    Debug::print( "serverthread", "Socket " + QString::number( mSocket ) + " denied updateDB without auth.");
                    send_nok(401,"Unauthorized\r\nWWW-Authenticate: Basic basic-credentials");
                    return;
                }
                out=text();
                bool full = false;
                QString q = cmd.remove( "/updateDB" );
                if ( q.remove( "/" ).isEmpty() ) {
                    full = false;
                } else if ( q.remove( "/" ) == "full" ) {
                    full = true;
                } else {
                    full = false;
                }
                Database::openMysqlDB( mMysqlDB );
                Database::openSiebelDB( mSiebelDB );
                Database::openQmonDB( mQmonDB );
                Database::openReportDB( mReportDB );
                int btime;
                QTime timer;
                timer.start();
                Debug::print( "serverthread", "Starting DB update..." );
                if ( full ) {
                    Debug::print( "serverthread", "Starting PseudoQ update..." );
                    Database::updatePseudoQueues( mQmonDB, mMysqlDB );
                    btime = timer.elapsed() / 1000;
                    timer.restart();
                    Debug::print( "serverthread", "PseudoQ update finished, took " + QString::number( btime ) + " sec" );
                    out.append("PseudoQ update took " +  QString::number( btime ) + " sec\n");
                }
                Debug::print( "serverthread", "Starting Bomgar update..." );
                QList< BomgarItem > list = Database::getChats( mQmonDB );
                QStringList l;
                for ( int i = 0; i < list.size(); ++i ) {
                    l.append( list.at( i ).id );
                    if ( !Database::bomgarExistsInDB( list.at( i ).id, mMysqlDB ) ) {
                        Database::updateBomgarItemInDB( list.at( i ), mMysqlDB );
                    } else if ( ( Database::getBomgarQueueById( list.at( i ).id, mMysqlDB ) != list.at( i ).name ) ) {
                        Database::updateBomgarQueue( list.at( i ), mMysqlDB );
                    }
                }
                QStringList existList = Database::getQmonBomgarList( mMysqlDB );
                for ( int i = 0; i < existList.size(); ++i ) {
                    if ( !l.contains( existList.at( i ) ) ) {
                        Database::deleteBomgarItemFromDB( existList.at( i ), mMysqlDB );
                    }
                }
                btime = timer.elapsed() / 1000;
                timer.restart();
                Debug::print( "serverthread", "Bomgar update finished, took " + QString::number( btime ) + " sec" );
                Debug::print( "serverthread", "Starting Unity update..." );
                out.append("Bomgar update took " +  QString::number( btime ) + " sec\n");
                QList<SiebelItem> ql = Database::getQmonSrs( mSiebelDB, mMysqlDB, mReportDB );
                QStringList newList;
                for ( int i = 0; i < ql.size(); ++i ) {
                    newList.append( ql.at( i ).id );
                    if ( !Database::siebelExistsInDB( ql.at( i ).id, mMysqlDB ) ) {
                        Database::insertSiebelItemIntoDB( ql.at( i ), mMysqlDB );
                    } else {
                        if ( Database::siebelQueueChanged( ql.at( i ), mMysqlDB ) ) {
                            Database::updateSiebelQueue( ql.at( i ), mMysqlDB );
                        }
                        Database::updateSiebelItem( ql.at( i ), mMysqlDB, mSiebelDB );
                    }
                }
                QStringList qexistList = Database::getQmonSiebelList( mMysqlDB );
                for ( int i = 0; i < qexistList.size(); ++i ) {
                    if ( !newList.contains( qexistList.at( i ) ) ) {
                        Database::deleteSiebelItemFromDB( qexistList.at( i ), mMysqlDB );
                    }
                }
                out.append("Unity update took " + QString::number( timer.elapsed() / 1000 ) + " sec\n\n");
                out.append("UPDATE FINISHED\n");
                socket->write(out.toUtf8());
                Debug::print( "serverthread", "Unity update finished, took " + QString::number( timer.elapsed() / 1000 ) + " sec" );
                Debug::print( "serverthread", "DB Update finished" );
/* chat */
            } else if ( cmd.startsWith( "/chat" ) ) {
                send_ok();
                Database::openMysqlDB( mMysqlDB );
                QStringList l = Database::getCurrentBomgars();
                out=xml();
                out.append("<chat>\n");
                for ( int i = 0; i < l.size(); ++i ) {
                    out.append("  <bomgar>\n");
                    out.append("    <bomgarQ>" + l.at(i).split("|||").at(1) + "</bomgarQ>\n");
                    out.append("    <sr>" + l.at(i).split("|||").at(0) + "</sr>\n");
                    out.append("  </bomgar>\n");
                }
                out.append("</chat>");
                socket->write(out.toUtf8());
/* pseudoQ */
            } else if ( cmd.startsWith( "/pseudoQ" ) ) {
                send_ok();
                Database::openMysqlDB( mMysqlDB );
                QStringList pl = Database::getPseudoQueues( mMysqlDB );
                out=text();
                for ( int i = 0; i < pl.size(); ++i ) {
                    out.append(pl.at(i) + "\n");
                }
                socket->write(out.toUtf8());
/* unityURL */
            } else if ( cmd.startsWith( "/unityURL" ) ) {
                send_ok();
                out=text();
                out.append(Settings::unityURL() + "\n");
                socket->write(out.toUtf8());
/* assign */
            } else if ( cmd.startsWith( "/assign" ) ) {
                if (authenticated) {
                    send_ok();
                } else { 
                    Debug::print( "serverthread", "Socket " + QString::number( mSocket ) + " denied assign without auth.");
                    send_nok(401,"Unauthorized\r\nWWW-Authenticate: Basic basic-credentials");
                    return;
                }
                QString q = cmd.remove( "/assign" );
                out=text();
                if ( q.contains( "%7C" ) ) {
                    q.replace( "%7C", "|" );
                }
                if ( q.remove( "/" ).isEmpty() ) {
                    out.append("Please specify sr number and engineer delimited by |\r\n");
                } else if ( !q.contains( "|" ) ) {
                    out.append("Please specify engineer\r\n");
                } else {
                    Network* net = new Network();
                    QEventLoop loop;
                    QString o;
                    QNetworkReply* ass;
                    ass = net->get( QUrl( "http://proetus.provo.novell.com/qmon/assign2.asp?sr=" + q.remove( "/" ).split( "|" ).at( 0 ) +
                                          "&owner=" + q.remove( "/" ).split( "|" ).at( 1 ) + "&force=1" ) );
                    QObject::connect( ass, SIGNAL( finished() ), &loop, SLOT( quit() ) );
                    loop.exec();
                    o = ass->readAll();
                    out.append(o);
                    delete net;
                }
                socket->write(out.toUtf8());
/* userquque */
            } else if ( cmd.startsWith( "/userqueue" ) ) {
                if (authenticated || !Settings::enforceauth()) {
                    send_ok();
                } else {
                    Debug::print( "serverthread", "Socket " + QString::number( mSocket ) + " denied userqueue without auth.");
                    send_nok(401,"Unauthorized\r\nWWW-Authenticate: Basic basic-credentials");
                    return;
                }
                Database::openSiebelDB( mSiebelDB );
                Database::openMysqlDB( mMysqlDB );
                QString q = cmd.remove( "/userqueue" );
                if ( q.startsWith( "/full/" ) ) {
                    QString eng;
                    if (Settings::enforceauth()) {
                      eng = QString::fromUtf8(user.data()).toUpper();
                    } else {
                      eng = q.remove( "/full/" ).remove( "/" ).toUpper();
                    }
                    out=xml();
                    out.append(XML::queue( Database::getUserQueue( eng, mSiebelDB, mMysqlDB, mReportDB, true ) ));
                } else {
                    out=xml();
                    out.append(XML::queue( Database::getUserQueue( QString::fromUtf8(user.data()).toUpper(), mSiebelDB, mMysqlDB, mReportDB ) ));
                }
                socket->write(out.toUtf8());
/* stats */
            } else if ( cmd.startsWith( "/stats" ) ) {
                if (authenticated || !Settings::enforceauth()) {
                    send_ok();
                } else {
                    Debug::print( "serverthread", "Socket " + QString::number( mSocket ) + " denied stats without auth.");
                    send_nok(401,"Unauthorized\r\nWWW-Authenticate: Basic basic-credentials");
                    return;
                }
                out=text();
                Network* net = new Network();
                QString eng;
                if (Settings::enforceauth()) {
                  eng = QString::fromUtf8(user.data()).toUpper();
                } else {
                  eng = cmd.remove( "/stats" );
                  if ( eng.remove( "/" ).isEmpty() ) {
                    out.append("Please specify engineer.\r\n");
                    socket->write(out.toUtf8());
                    socket->flush();
                    socket->waitForBytesWritten();
                    socket->disconnectFromHost();
                    Debug::print( "serverthread", "Socket " + QString::number( mSocket ) + " request failed.");
                    delete socket;
                    return;
                  }
                }
                QString wf = getWF( eng, net );
                if ( wf == "00000" ) {
                    out.append("Invalid engineer\r\n");
                } else {
                    Database::openSiebelDB( mSiebelDB );
                    Statistics statz;
                    QString numbers;
                    QNetworkReply* r = net->get( QUrl( "http://proetus.provo.novell.com/qmon/closed2.asp?tse=" + QString::fromUtf8(user.data()).toUpper() ) );
                    QEventLoop loop;
                    QObject::connect( r, SIGNAL( finished() ), &loop, SLOT( quit() ) );
                    loop.exec();
                    if ( !r->error() ) {
                        numbers = r->readAll();
                    } else {
                        numbers = "ERROR";
                    }
                    QString csat;
                    QNetworkReply* csr = net->get( QUrl( "http://proetus.provo.novell.com/qmon/custsat.asp?wf=" + wf ) );
                    QObject::connect( csr, SIGNAL( finished() ), &loop, SLOT( quit() ) );
                    loop.exec();
                    QStringList csatList;
                    if ( !csr->error() ) {
                        csat = csr->readAll();
                        if ( csat.contains( "<br>" ) ) {
                            csatList = csat.split( "<br>" );
                        }
                    } else {
                        csat = "ERROR";
                    }
                    QString tts;
                    QNetworkReply* ttr = net->get( QUrl( "http://proetus.provo.novell.com/qmon/timetosolutiontse.asp?tse=" + QString::fromUtf8(user.data()).toUpper() ) );
                    QObject::connect( ttr, SIGNAL( finished() ), &loop, SLOT( quit() ) );
                    loop.exec();
                    QStringList ttsList;
                    if ( !ttr->error() ) {
                        tts = ttr->readAll();
                        if ( tts.contains( "<br>" ) ) {
                            ttsList = tts.split( "<br>" );
                        }
                    } else {
                        tts = "ERROR";
                    }
                    if ( tts != "ERROR" && csat != "ERROR" && numbers != "ERROR" ) {
                        if ( numbers.contains( "<br>" ) ) {
                            QString o = numbers.split("<br>").at(0);
                            o.remove( QRegExp( "<(?:div|span|tr|td|br|body|html|tt|a|strong|p)[^>]*>", Qt::CaseInsensitive ) );

                            statz.closedSr = o.split("|").at(0).trimmed();
                            statz.closedCr = o.split("|").at(1).trimmed();
                        }
                        QList<ClosedItem> closedList;
                        for ( int i = 0; i < ttsList.size() - 1; ++i ) {
                            ClosedItem ci;
                            QString tmp = ttsList.at(i);
                            tmp.remove( QRegExp( "<(?:div|span|tr|td|br|body|html|tt|a|strong|p)[^>]*>", Qt::CaseInsensitive ) );
                            ci.sr = tmp.split( "|" ).at( 1 );
                            ci.tts = tmp.split( "|" ).at( 2 ).toInt();
                            QStringList info = Database::srInfo( ci.sr, mSiebelDB );
                            ci.customer = info.at( 3 ) + " (" + info.at(1) + " " + info.at(2) + ")";
                            ci.bdesc = info.at( 0 );
                            closedList.append( ci );
                        }
                        QList<CsatItem> csatItemList;
                        for ( int i = 0; i < csatList.size() - 1; ++i ) {
                            CsatItem csi;
                            QString tmp = csatList.at(i);
                            tmp.remove( QRegExp( "<(?:div|span|tr|td|br|body|html|tt|a|strong|p)[^>]*>", Qt::CaseInsensitive ) );
                            csi.sr = tmp.split( "|" ).at( 1 ).trimmed();
                            if ( tmp.split( "|" ).at( 2 ).isEmpty() ) {
                                csi.srsat = 88;
                            } else {
                                csi.srsat = tmp.split( "|" ).at( 2 ).trimmed().toInt();
                            }
                            if ( tmp.split( "|" ).at( 3 ).isEmpty() ) {
                                csi.engsat = 88;
                            } else {
                                csi.engsat = tmp.split( "|" ).at( 3 ).trimmed().toInt();
                            }
                            csi.rts = tmp.split( "|" ).at( 4 ).trimmed().toInt();
                            QStringList info = Database::srInfo( csi.sr, mSiebelDB );
                            csi.customer = info.at( 3 ) + " (" + info.at(1) + " " + info.at(2) + ")";
                            csi.bdesc = info.at(0);
                            csatItemList.append( csi );
                        }
                        statz.closedList = closedList;
                        statz.csatList = csatItemList;
                        out=xml();
                        out.append(XML::stats( statz ));
                    } else {
                        out.append("ERROR\r\n");
                    }
                }
                delete net;
                socket->write(out.toUtf8());
/* productmenu */
            } else if ( cmd.startsWith( "/productmenu" ) ) {
                send_ok();
                out=xml();
                out.append(XML::SendMenu());
                socket->write(out.toUtf8());
/* default */
            } else {
                send_ok();
                out=text();
                out.append("Welcome to kueued\n\n");
                out.append("Usage:\n\n");
                out.append("  * http://kueue.hwlab.suse.de:8080/qmon\n    Get a list of all SRs in all pseudo queues\n\n");
                out.append("  * http://kueue.hwlab.suse.de:8080/qmon/$QUEUE-NAME\n    Get a list of all SRs in $QUEUE-NAME\n\n");
                out.append("    This is the qmon xml output.\n");
                out.append("\n");
                out.append("    Please note that not all fields are available for each SR, if they are not, they aren't shown at all.\n");
                out.append("\n");
                out.append("    <sr>\n");
                out.append("      <id>SR number</id>\n");
                out.append("      <queue>Queue</queue>\n");
                out.append("      <bomgarQ>Bomgar Queue (if in chat)</bomgarQ>\n");
                out.append("      <srtype>sr/cr</srtype>\n");
                out.append("      <creator>Only for CRs: who created it?</creator>\n");
                out.append("      <customer>\n");
                out.append("        <account>Company name</account>\n");
                out.append("        <firstname>Customer's first name</firstname>\n");
                out.append("        <lastname>Customer's last name</lastname>\n");
                out.append("        <title>Customer's title (ie. job)</title>\n");
                out.append("        <email>Customer's email</email>\n");
                out.append("        <phone>Customer's phone number</phone>\n");
                out.append("        <onsitephone>Customer's onsite phone number</onsitephone>\n");
                out.append("        <lang>Customer's preferred language</lang>\n");
                out.append("      </customer>\n");
                out.append("      <severity>SR severity</severity>\n");
                out.append("      <status>Current status</status>\n");
                out.append("      <bdesc>Brief description</bdesc>\n");
                out.append("      <ddesc>Detailed description</ddesc>\n");
                out.append("      <geo>Geo</geo>\n");
                out.append("      <hours>Support hours</hours>\n");
                out.append("      <source>How was the SR opened (web, phone..)</source>\n");
                out.append("      <support_program>Customer's contract</support_program>\n");
                out.append("      <support_program_long>A slightly more detailed version of the above</support_program_long>\n");
                out.append("      <routing_product>Routing product</routing_product>\n");
                out.append("      <support_group_routing>What group it is routed to internally</support_group_routing>\n");
                out.append("      <int_type>Internal type</int_type>\n");
                out.append("      <subtype>Subtype</subtype>\n");
                out.append("      <servce_level>Some number indicating something</service_level>\n");
                out.append("      <category>SR category (Adv/Knowledge)</category>\n");
                out.append("      <respond_via>How the customer would like to be contacted</respond_via>\n");
                out.append("      <age>SR age in seconds</age>\n");
                out.append("      <lastupdate>Time to last update in seconds</lastupdate>\n");
                out.append("      <timeinQ>Time in the current queue in seconds</timeinQ>\n");
                out.append("      <sla>SLA left in seconds</sla>\n");
                out.append("      <highvalue>Is the customer considered high value?</highvalue>\n");
                out.append("      <critsit>Is there a critsit going on with the customer?</critsit>\n");
                out.append("    </sr>   \n\n");
                out.append("  * http://kueue.hwlab.suse.de:8080/bug/$SRNR\n    Get the bugreport for $SRNR (if any)\n\n");
                out.append("Stay tuned for more features!\n");
                socket->write(out.toUtf8());
            }
            if ( socket->waitForBytesWritten() ) {
                socket->disconnectFromHost();
                socket->waitForDisconnected();
            }
            Debug::print( "serverthread", "Socket " + QString::number( mSocket ) + " request finisehd success.");
            delete socket;
        }
    }
}

void ServerThread::send_ok()
{
    QString out;
    out.append("HTTP/1.1 200 OK\r\n");
    out.append("Server: kueued @ " + mHostname + " (Linux)\r\n");
    socket->write(out.toUtf8());
    socket->flush();
}

void ServerThread::send_nok(int error, QString reason)
{
    QString out;
    out.append("HTTP/1.1 " + QString::number(error) + " " + reason +"\r\n");
    out.append("Server: kueued @ " + mHostname + " (Linux)\r\n");
    socket->write(out.toUtf8());
    socket->flush();
    socket->waitForBytesWritten();
    socket->disconnectFromHost();
    Debug::print( "serverthread", "Socket " + QString::number( mSocket ) + " request failed.");
    delete socket;
}

QString ServerThread::text()
{
    return( "Content-Type: text/plain; charset=\"utf-8\"\r\n"
            "\r\n" );
}

QString ServerThread::xml()
{
    return( "Content-Type: text/xml; charset=\"utf-8\"\r\n"
            "\r\n"
            "<?xml version='1.0'?>\n\n" );
}

QString ServerThread::getWF( const QString& engineer, Network* net )
{
    QEventLoop loop;
    QString wfid;

    QNetworkReply *reply = net->get( QUrl( Settings::dBServer() + "/workforce.asp?tse=" + engineer ) );

    loop.connect( reply, SIGNAL( readyRead() ),
                  &loop, SLOT( quit() ) );

    loop.exec();

    wfid = reply->readAll();
    wfid.remove( QRegExp( "<(?:div|span|tr|td|br|body|html|tt|a|strong|p)[^>]*>", Qt::CaseInsensitive ) );

    return wfid.trimmed();
}

#include "serverthread.moc"

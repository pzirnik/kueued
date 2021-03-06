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

#ifndef MAX_SIEBEL_CONNECTIONS
#define MAX_SIEBEL_CONNECTIONS 15
#endif

#ifndef SERVERTHREAD_H
#define SERVERTHREAD_H

#include <QThread>
#include <QTime>
#include <QRunnable>
#include <QSslSocket>

extern "C" {
#include <lber.h>
#include <ldap.h>
#include <sasl/sasl.h>
}

#include "network.h"

class Network;

class ServerThread : public QObject, public QRunnable
{
    Q_OBJECT
    public:
        ServerThread( int sd = 0, QObject *parent = 0L, bool ssl = false );
        ~ServerThread();
        
        void run();
    

    private:
        int mSocket;
	bool mSsl;
        QString mMysqlDB;
        QString mSiebelDB;
        QString mReportDB;
        QString mThreadID;
        QString mQmonDB;
        QString mEngineer;
        QString mHostname;
        QString getWF( const QString&, Network*  );

        QTime mTime;
        void send_ok();
	void send_nok(int error, QString reason);
        QString text();
        QString xml();
	QSslSocket* socket;
};


#endif

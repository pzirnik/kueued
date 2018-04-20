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

#include "kueued.h"
#include <QDebug>
#include <QtSingleCoreApplication>
#include <iostream>
#include "simplecrypt/simplecrypt.h"

int main(int argc, char *argv[])
{
    
    QtSingleCoreApplication app( argc, argv );

    QStringList arglst = app.arguments();
    if ( arglst.size() == 3 ) {
      if ( arglst[1] == "-c" ) {
	SimpleCrypt crypto( Q_UINT64_C( 0424632454124622 ) );
	QString pwd;
	pwd = crypto.encryptToString(arglst[2]);
	std::cout << pwd.toStdString() << std::endl;
	return 0;
      } 
    }
    
    if ( app.isRunning() )
    {
        std::cout <<  "kueued is already running, exiting";
    	return 0;
    }
        
    QCoreApplication::setOrganizationName( "nts" );
    QCoreApplication::setApplicationName( "kueued" );
    QCoreApplication::setApplicationVersion( "5.6.0" );

    Kueued k;
    return app.exec();
}

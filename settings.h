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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>

class Settings
{
    public:
       
        /* General Group */
        
        static QString dBServer();
        static int refreshSeconds();
        static QString mysqlHost();
        static QString mysqlUser();
        static QString mysqlPassword();
        static QString mysqlDatabase();
        static QString reportHost();
        static QString reportUser();
        static QString reportPassword();
        static QString reportDatabase();
        static QString latestVersion();
        static QString siebelHost();
        static QString siebelUser();
        static QString siebelPassword();
        static QString siebelDatabase();
        static QString qmonDbDatabase();
        static QString qmonDbUser();
        static QString qmonDbPassword();
        static QString unityURL();
        static QString bugzillaUser();
        static QString bugzillaPassword();
        static QString l3Server();
        static QString l3User();
        static QString l3ApiKey();
        static int reportPort();
        static int timezoneCorrection();
        static bool logQueries();
        static bool debugLog();
	static QString ldapuri();
	static QString sslcert();
	static QString sslkey();
        static bool enforceauth();
	
};    

#endif

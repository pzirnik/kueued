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

QString XML::sr( SiebelItem* si )
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime odate = QDateTime::fromString( si->odate, "yyyy-MM-dd hh:mm:ss" );
    QDateTime adate = QDateTime::fromString( si->adate, "yyyy-MM-dd hh:mm:ss" );
    QDateTime sladate = QDateTime::fromString( si->sla, "yyyy-MM-dd hh:mm:ss" );
    QDateTime qdate = QDateTime::fromString( si->qdate, "yyyy-MM-dd hh:mm:ss" );
    
    QString xml;
    
    qint64 age = ( odate.secsTo( now ) - ( Settings::timezoneCorrection() * 3600 ) );
    qint64 lu = ( adate.secsTo( now ) - ( Settings::timezoneCorrection() * 3600 ));
    qint64 qt = ( qdate.secsTo( now ) );
    qint64 sla = ( now.secsTo( sladate ) );
    
    /*id
queue
hours
source
contactvia
odate
adate
status
category
severity
supportprogram
supportgroup
sla
srtype
srsubtype
servicelevel
cid
pq_email
bdesc
last_activity
account
pq_phone
onsitephone
detdesc
routing
geo
bomgarQ*/
    xml += "  <sr>\n";
    xml += "    <id><![CDATA[" + si->id + "]]></id>\n";
    xml += "    <queue><![CDATA[" + si->queue + "]]></queue>\n";
    xml += "    <hours><![CDATA[" + si->hours + "]]></hours>\n";
    xml += "    <geo><![CDATA[" + si->geo + "]]></geo>\n";
    xml += "    <contactvia><![CDATA[" + si->contactvia + "]]></contactvia>\n";
    xml += "    <status><![CDATA[" + si->status + "]]></status>\n";
    xml += "    <severity><![CDATA[" + si->severity + "]]></severity>\n";
    xml += "    <contract><![CDATA[" + si->contract + "]]></contract>\n";
    
    if ( si->isCr )
    {
        xml += "    <srtype>cr</srtype>\n";
        
    }
    else
    {
        xml += "    <srtype>sr</srtype>\n";
    }
    
    xml += "    <crsr>" + si->crSr + "</crsr>\n";
    xml += "    <bdesc><![CDATA[" + si->bdesc + "]]></bdesc>\n";
    xml += "    <customer><![CDATA[" + si->customer + "]]></customer>\n";
    
    if ( si->bomgarQ != "NOCHAT" )
    {
        xml += "    <bomgarQ>" + si->bomgarQ + "</bomgarQ>\n";
    }
    
    xml += "    <age>" + QString::number( age ) + "</age>\n";
    xml += "    <lastupdate>" + QString::number( lu ) + "</lastupdate>\n";
    xml += "    <timeinQ>" + QString::number( qt ) + "</timeinQ>\n";

    if ( sla > 0 )      
    {
         xml += "    <sla>" + QString::number( sla ) + "</sla>\n";
    }
    else
    {
         xml += "    <sla>" + QString::number( 0 ) + "</sla>\n";
    }
    
    if ( si->highValue )
    {
        xml += "    <highvalue>yes</highvalue>\n";
    }
    else
    {
        xml += "    <highvalue>no</highvalue>\n";
    }
    
    if ( si->critSit )
    {
        xml += "    <critsit>yes</critsit>\n";
    }
    else
    {
        xml += "    <critsit>no</critsit>\n";
    }
    
    xml += "  </sr>\n";
  
    return xml;
}
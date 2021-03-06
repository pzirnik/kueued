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
#include "network.h"

#include <QtXml>
#include <QDateTime>

Database::Database()
{   
    Debug::print( "database", "Constructing" );
}

Database::~Database()
{   
    Debug::print( "database", "Destroying" );
}

void Database::insertSiebelItemIntoDB( SiebelItem item, const QString& dbname )
{
    Debug::print( "database", "Inserting SiebelItem for " + item.id + " " + item.queue );

    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname ); 
    db.transaction();
    
    QSqlQuery query( db );
    
    query.prepare( "INSERT INTO QMON_SIEBEL( ID, QUEUE, GEO, HOURS, STATUS, SEVERITY, SOURCE, RESPOND_VIA, "
                   "                         CREATED, LAST_UPDATE, INQUEUE, SLA, SUPPORT_PROGRAM, SUPPORT_PROGRAM_LONG, "
                   "                         ROUTING_PRODUCT, SUPPORT_GROUP_ROUTING, INT_TYPE, SUBTYPE, SERVICE_LEVEL, "
                   "                         BRIEF_DESC, CRITSIT, HIGH_VALUE, DETAILED_DESC, CATEGORY, CREATOR, ROW_ID, SUBOWNER, RATING ) "
                   " VALUES "
                   "( :id, :queue, :geo, :hours, :status, :severity, :source, :respond_via, :created, :last_update, :inqueue, "
                   "  :sla, :support_program, :support_program_long, :routing_product, :support_group_routing, :int_type, :subtype, "
                   "  :service_level, :brief_desc, :critsit, :high_value,  :detailed_desc, :category, :creator, :row_id, :subowner, :rating )" );

    query.bindValue( ":id", item.id );
    query.bindValue( ":queue", item.queue );
    query.bindValue( ":geo", item.geo );
    query.bindValue( ":hours", item.hours );
    query.bindValue( ":status", item.status );
    query.bindValue( ":severity", item.severity );
    query.bindValue( ":source", item.source );
    query.bindValue( ":respond_via", item.respond_via );
    query.bindValue( ":created", convertTime( item.created ) );
    query.bindValue( ":last_update", convertTime( item.last_update ) );
    query.bindValue( ":inqueue", QDateTime::currentDateTime().toString( "yyyy-MM-dd hh:mm:ss" ) );
    query.bindValue( ":sla", convertTime( item.sla ) );
    query.bindValue( ":support_program", item.support_program );
    query.bindValue( ":support_program_long", item.support_program_long );
    query.bindValue( ":routing_product", item.routing_product );
    query.bindValue( ":support_group_routing", item.support_group_routing );
    query.bindValue( ":int_type", item.int_type );
    query.bindValue( ":subtype", item.subtype );
    query.bindValue( ":service_level", item.service_level );
    query.bindValue( ":brief_desc", item.brief_desc );   
    query.bindValue( ":critsit", item.critsit );
    query.bindValue( ":high_value", item.high_value );    
    query.bindValue( ":detailed_desc", item.detailed_desc );
    query.bindValue( ":category", item.category );
    query.bindValue( ":creator", item.creator );
    query.bindValue( ":row_id", item.row_id );
    query.bindValue( ":subowner", item.subowner );
    query.bindValue( ":rating", item.rating );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    Debug::logQuery( query, db.connectionName() );
    
    QSqlQuery cquery( db );
     
    cquery.prepare( "INSERT INTO CUSTOMER( ID, CUSTOMER, CONTACT_PHONE, CONTACT_FIRSTNAME, CONTACT_LASTNAME, "
                   "                           CONTACT_EMAIL, CONTACT_TITLE, CONTACT_LANG, ONSITE_PHONE, ORACLE_ID ) "
                   "VALUES"
                   "( :id, :customer, :contact_phone, :contact_firstname, :contact_lastname, :contact_email, "
                   "  :contact_title, :contact_lang, :onsite_phone, :oracle_id )" ); 
    
    cquery.bindValue( ":id", item.id );
    cquery.bindValue( ":customer", item.customer );
    cquery.bindValue( ":contact_phone", item.contact_phone );
    cquery.bindValue( ":contact_firstname", item.contact_firstname );
    cquery.bindValue( ":contact_lastname", item.contact_lastname );
    cquery.bindValue( ":contact_email", item.contact_email );
    cquery.bindValue( ":contact_title", item.contact_title );
    cquery.bindValue( ":contact_lang", item.contact_lang );
    cquery.bindValue( ":onsite_phone", item.onsite_phone );
    cquery.bindValue( ":oracle_id", item.cstNum );
    
    if ( !cquery.exec() ) qDebug() << cquery.lastError().text();                  
    db.commit();
    Debug::logQuery( cquery, db.connectionName() );
}

bool Database::checkLTSSbyId( const QString& id, const QString& dbname )
{
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    
    query.prepare( "SELECT ACCOUNT_NAME FROM LTSSCUSTOMERS WHERE ( ORACLE_CUSTOMER_NR = :id )" );
    query.bindValue( ":id", id );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    if ( query.next() )
    {
        qDebug() << "LTSS!!!";
        return true;
    }
    else
    {
        qDebug() << "NO LTSS";        
        return false;
    }
}


QString Database::getSrForCr( const QString& cr, const QString& mysqlname, const QString& reportname )
{
    QString sr = getSrForCrMysql( cr, mysqlname );
    
    if ( sr == QString::Null() )
    {
        QSqlDatabase r = QSqlDatabase::database( reportname );
        
        if ( !r.isOpen() ) 
        {
            openReportDB( reportname );
        }
        
        sr = getSrForCrReport( cr, mysqlname, reportname );
    }
        
    return sr;
}

QString Database::getSrForCrMysql( const QString& cr, const QString& dbname )
{
    QSqlDatabase db;
    QString sr;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    
    query.prepare( "SELECT SR FROM CRSR WHERE ( CR = :cr )" );
    query.bindValue( ":cr", cr );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    if ( query.next() )
    {
        return query.value( 0 ).toString();
    }
    else
    {
        return QString::Null();
    }
}


QString Database::getSrForCrReport( const QString& cr, const QString& dbname1, const QString& dbname )
{
    QSqlDatabase db;
    QSqlDatabase db1;
    QString sr;
    
    db = QSqlDatabase::database( dbname );
    db1 = QSqlDatabase::database( dbname1 );
    db.transaction();
    
    QSqlQuery query( db );
    
    query.prepare( "SELECT dx.SR_NUM FROM NTSDM.SBL_SR_REL re, NTSDM.OLAP_SR_DX2 dx "
                   "WHERE re.REL_SR_ID = (SELECT ROW_ID FROM NTSDM.OLAP_SR_DX2 WHERE SR_NUM = :cr) "
                   "AND re.SR_ID = dx.ROW_ID" );
        
    query.bindValue( ":cr", cr );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    db.commit();
    
    if ( query.next() )
    {
        sr = query.value( 0 ).toString();
        
        QSqlQuery query1( db1 );
        
        query1.prepare( "INSERT INTO CRSR( CR, SR ) VALUES ( :cr, :sr )" );
        query1.bindValue( ":cr", cr );
        query1.bindValue( ":sr", sr );
        
        if ( !query1.exec() ) qDebug() << query1.lastError().text();
        
        return sr;
    }
    else
    {
        sr = "ERROR";
    }
    
    return sr;
}

QList< LTSScustomer > Database::getLTSScustomersExt( const QString& dbname )
{
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    QDate today = QDate::currentDate();
    QString todayDate = today.toString( "dd-MM-yyyy" );
        
    query.prepare(  "SELECT      SBL_ENTITLEMENT.ENTITLEMENT_NAME, "
                    "            SBL_AGREE.AGREEMENT_NUM, "
                    "            SBL_ENTITLEMENT.ENTITLEMENT_ID, "
                    "            SBL_ENTITLEMENT.ENTITLEMENT_END_DATE, "
                    "            SBL_AGREE.AGREEMENT_STATUS, "
                    "            SBL_ACCOUNT.NAME, "
                    "            SBL_ENTITLEMENT.ENTITLEMENT_START_DATE, "
                    "            SBL_AGREE.SUPPORT_PROGRAM, "
                    "            SBL_ACCOUNT.C_GEO1, "
                    "            SBL_ACCOUNT.ORACLE_CUSTOMER_NUM "
                    "FROM       (NTSDM.SBL_AGREE SBL_AGREE "
                    "INNER JOIN  NTSDM.SBL_ACCOUNT SBL_ACCOUNT ON SBL_AGREE.ACCOUNT_ID = SBL_ACCOUNT.SBL_ACCOUNT_ID) "
                    "INNER JOIN  NTSDM.SBL_ENTITLEMENT SBL_ENTITLEMENT ON SBL_AGREE.AGREEMENT_NUM = SBL_ENTITLEMENT.AGREEMENT_NUMBER "
                    "WHERE       SBL_AGREE.AGREEMENT_STATUS = 'Active' "
                    "AND         SBL_ENTITLEMENT.ENTITLEMENT_END_DATE >= TO_DATE('" + todayDate + "00:00:00', 'DD-MM-YYYY HH24:MI:SS') "
                    "AND        (SBL_ENTITLEMENT.ENTITLEMENT_NAME LIKE '%Long Term Service Pack%'"
                    "OR          SBL_ENTITLEMENT.ENTITLEMENT_NAME LIKE '%LTSS%')" );
        
      
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    QList< LTSScustomer > list;
    
    while ( query.next() ) 
    {
        LTSScustomer lc;
        
        lc.entitlement_name = query.value( 0 ).toString();
        lc.agreement_nr = query.value( 1 ).toString();
        lc.entitlement_id = query.value( 2 ).toString();
        lc.entitlement_end_date = query.value( 3 ).toString();
        lc.agreement_status = query.value( 4 ).toString();
        lc.account_name = query.value( 5 ).toString();
        lc.entitlement_start_date = query.value( 6 ).toString();
        lc.support_program = query.value( 7 ).toString();
        lc.geo = query.value( 8 ).toString();
        lc.oracle_customer_nr = query.value( 9 ).toString();
        
        list.append( lc );        
    }
    
    db.commit();
    return list;
}

QList< LTSScustomer > Database::getLTSScustomers( const QString& dbname )
{
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    
    query.prepare( "" );
    
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    QList< LTSScustomer > list;
    
    while ( query.next() ) 
    {
        LTSScustomer lc;
        
        lc.entitlement_name = query.value( 0 ).toString();
        lc.agreement_nr = query.value( 1 ).toString();
        lc.entitlement_id = query.value( 2 ).toString();
        lc.entitlement_end_date = query.value( 3 ).toString();
        lc.agreement_status = query.value( 4 ).toString();
        lc.account_name = query.value( 5 ).toString();
        lc.entitlement_start_date = query.value( 6 ).toString();
        lc.support_program = query.value( 7 ).toString();
        lc.geo = query.value( 8 ).toString();
        lc.oracle_customer_nr = query.value( 9 ).toString();
        
        list.append( lc );        
    }
    
    db.commit();
    return list;
}

void Database::updateSiebelItem( SiebelItem item, const QString& dbname, const QString& dbname1 )
{
    QSqlDatabase db;
    QSqlDatabase db1;
    
    db = QSqlDatabase::database( dbname );
    db1 = QSqlDatabase::database( dbname1 );
    db.transaction();
    db1.transaction();
    
    QSqlQuery query( db );
    
    query.prepare( "UPDATE QMON_SIEBEL SET GEO = :geo, HOURS = :hours, STATUS = :status, SEVERITY = :severity, "
                   "                       SOURCE = :source, RESPOND_VIA = :respond_via, CREATED = :created, "
                   "                       LAST_UPDATE = :last_update, SLA = :sla, SUPPORT_PROGRAM = :support_program,"
                   "                       SUPPORT_PROGRAM_LONG = :support_program_long, ROUTING_PRODUCT = :routing_product,"
                   "                       SUPPORT_GROUP_ROUTING = :support_group_routing, INT_TYPE = :int_type,"
                   "                       SUBTYPE = :subtype, SERVICE_LEVEL = :service_level, BRIEF_DESC = :brief_desc,"
                   "                       CRITSIT = :critsit, HIGH_VALUE = :high_value, DETAILED_DESC = :detailed_desc, "
                   "                       CATEGORY = :category, CREATOR = :creator, ROW_ID = :row_id, SUBOWNER = :subowner, RATING = :rating WHERE ID = :id" );
    
    query.bindValue( ":geo", item.geo );
    query.bindValue( ":hours", item.hours );
    query.bindValue( ":status", item.status );
    query.bindValue( ":severity", item.severity );
    query.bindValue( ":source", item.source );
    query.bindValue( ":respond_via", item.respond_via );
    query.bindValue( ":created", convertTime( item.created ) );
    query.bindValue( ":last_update", convertTime( item.last_update ) );
    query.bindValue( ":sla", convertTime( item.sla ) );
    query.bindValue( ":support_program", item.support_program );
    query.bindValue( ":support_program_long", item.support_program_long );
    query.bindValue( ":routing_product", item.routing_product );
    query.bindValue( ":support_group_routing", item.support_group_routing );
    query.bindValue( ":int_type", item.int_type );
    query.bindValue( ":subtype", item.subtype );
    query.bindValue( ":service_level", item.service_level );
    query.bindValue( ":brief_desc", item.brief_desc );
    query.bindValue( ":critsit", item.critsit );
    query.bindValue( ":high_value", item.high_value );
    query.bindValue( ":detailed_desc", item.detailed_desc );
    query.bindValue( ":category", item.category );
    
    if ( item.subtype == "Collaboration" )
    {
        query.bindValue( ":creator", getCreator( item.id, dbname1 ) );
    }
    
    query.bindValue( ":row_id", item.row_id );
    query.bindValue( ":subowner", item.subowner );
    query.bindValue( ":rating", item.rating );
    query.bindValue( ":id", item.id );

    if ( !query.exec() ) qDebug() << query.lastError().text();

    Debug::logQuery( query, db.connectionName() );
    
    QSqlQuery cquery( db );
    
    cquery.prepare( "UPDATE CUSTOMER SET CUSTOMER = :customer, CONTACT_PHONE = :contact_phone, CONTACT_FIRSTNAME = :contact_firstname, "
                   "                          CONTACT_LASTNAME = :contact_lastname, CONTACT_EMAIL = :contact_email, "
                   "                          CONTACT_TITLE = :contact_title, CONTACT_LANG = :contact_lang, ONSITE_PHONE = :onsite_phone, ORACLE_ID = :oracle_id "
                   "                          WHERE ID = :id" );
    
    cquery.bindValue( ":customer", item.customer );
    cquery.bindValue( ":contact_phone", item.contact_phone );
    cquery.bindValue( ":contact_firstname", item.contact_firstname );
    cquery.bindValue( ":contact_lastname", item.contact_lastname );
    cquery.bindValue( ":contact_email", item.contact_email );
    cquery.bindValue( ":contact_title", item.contact_title );
    cquery.bindValue( ":contact_lang", item.contact_lang );
    cquery.bindValue( ":onsite_phone", item.onsite_phone );
    cquery.bindValue( ":oracle_id", item.cstNum );
    cquery.bindValue( ":id", item.id );

    if ( !cquery.exec() ) qDebug() << cquery.lastError().text();
    db.commit();
    Debug::logQuery( cquery, db.connectionName() );
}

QString Database::getDetDesc( const QString& sr, const QString& dbname )
{
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );

    query.prepare( "SELECT DESC_TEXT FROM SIEBEL.S_SRV_REQ WHERE SR_NUM = :sr" );
    
    query.bindValue( ":sr", sr );
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    db.commit();
    
    if ( query.next() )
    {
        return query.value(0).toString();
    }
    else
    {
        return "ERROR";
    }
}

void Database::updateSiebelQueue( SiebelItem si, const QString& dbname )
{
    Debug::print( "database", "Updating Siebel queue for " + si.id + " to " + si.queue );
    
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    
    query.prepare( "UPDATE QMON_SIEBEL SET QUEUE = :queue, INQUEUE = :inqueue WHERE id = :id" );
                
    query.bindValue( ":queue", si.queue );
    query.bindValue( ":inqueue", QDateTime::currentDateTime().toString( "yyyy-MM-dd hh:mm:ss" ) );
    query.bindValue( ":id", si.id );
                
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    db.commit();
    
    Debug::logQuery( query, db.connectionName() );
}

QString Database::getCreator(const QString& sr, const QString& dbname )
{
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    
    query.prepare( "SELECT LOGIN FROM SIEBEL.S_USER WHERE PAR_ROW_ID = ( SELECT CREATED_BY FROM SIEBEL.S_SRV_REQ WHERE SR_NUM = :sr )" );

    query.bindValue( ":sr", sr );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();

    db.commit();
    
    Debug::logQuery( query, db.connectionName() );
    
    if ( query.next() )
    {
        return query.value( 0 ).toString();
    }
    else
    {
        return "ERROR";
    }
}

QList< QueueItem > Database::getUserQueue( const QString& engineer, const QString& dbname, const QString& mysqlname, const QString& reportname, bool subowner )
{
    QSqlDatabase db;
    QSqlDatabase mysqldb;
    
    db = QSqlDatabase::database( dbname );
    mysqldb = QSqlDatabase::database( mysqlname );
    db.transaction();
    mysqldb.transaction();
    
    QSqlQuery query( db );
    QList< QueueItem > list;
    QString q( "select "
                    "  sr.sr_num as ID, "
                    "  case "
                    "    when sr.BU_ID = '0-R9NH' then 'Default Organization' "
                    "    when sr.BU_ID = '1-AHT' then 'EMEA' "
                    "    when sr.BU_ID = '1-AHV' then 'ASIAPAC' "
                    "    when sr.BU_ID = '1-AHX' then 'USA' "
                    "    when sr.BU_ID = '1-AHZ' then 'LATIN AMERICA' "
                    "    when sr.BU_ID = '1-AI1' then 'CANADA' "
                    "  else 'Undefined' end as GEO,  "
                    "  cal.NAME as HOURS, "
                    "  sr.SR_SUB_STAT_ID as STATUS, "
                    "  sr.SR_SEV_CD as SEVERITY, "
                    "  sr.CREATED, "
                    "  sr.CREATED_BY, "
                    "  sr.ACTL_RESP_TS as LAST_UPDATE,  "
                    "  e.NAME as SUPPORT_PROGRAM,"
                    "  sr.X_SR_SUB_TYPE as SUBTYPE, "
                    "  e.ENTL_PRIORITY_NUM as SERVICE_LEVEL, "
                    "  SR_TITLE as BRIEF_DESC, "
                    "  flag.ATTRIB_11 as CRITSIT, "
                    "  flag.ATTRIB_56 as HIGH_VALUE, "
                    "  ext.NAME as CUSTOMER, "
                    "  REGEXP_REPLACE( g.WORK_PH_NUM, '(.*)' || CHR(10) || '(.*)', '\\1') AS CONTACT_PHONE,"
                    "  REGEXP_REPLACE( g.WORK_PH_NUM, '(.*)' || CHR(10) || '(.*)', '\\2') AS FORMAT_STRING,"
                    "  g.FST_NAME as CONTACT_FIRSTNAME, "
                    "  g.LAST_NAME as CONTACT_LASTNAME, "
                    "  g.EMAIL_ADDR as CONTACT_EMAIL, "
                    "  g.JOB_TITLE as CONTACT_TITLE, "
                    "  g.PREF_LANG_ID as CONTACT_LANG,"
                    "  REGEXP_REPLACE( sr.X_ONSITE_PH_NUM, '(.*)' || CHR(10) || '(.*)', '\\1') AS ONSITE_PHONE,"
                    "  REGEXP_REPLACE( sr.X_ONSITE_PH_NUM, '(.*)' || CHR(10) || '(.*)', '\\2') AS ONSITE_FORMAT_STRING,"
                    "  sr.DESC_TEXT as DETAILED_DESC, "
                    "  sr.X_ALT_CONTACT, "
                    "  sr.X_DEFECT_NUM, "
                    "  ext.X_ORACLE_CUSTOMER_ID, "
                    "  owner.login as OWNER, "
                    "  subowner.login as SUBOWNER "
                    "from "
                    "  siebel.s_srv_req sr"
                    "  LEFT OUTER JOIN siebel.s_user owner ON sr.owner_emp_id = owner.row_id,"
                    "  siebel.s_srv_req_x srx"
                    "  LEFT OUTER JOIN siebel.s_user subowner ON srx.attrib_07 = subowner.row_id,"
                    "  siebel.s_user u, "
                    "  siebel.s_contact c, "
                    "  siebel.s_contact g, "
                    "  siebel.s_entlmnt e, "
                    "  siebel.s_sched_cal cal, "
                    "  siebel.s_org_ext ext, "
                    "  siebel.s_prod_int prd, "
                    "  siebel.s_org_ext_x flag "
                    "where " );

    if ( subowner )
    {
        q += ("  ( sr.owner_emp_id = u.row_id"
              "  or srx.attrib_07 = u.row_id )" );
    }
    else
    {
        q += (          "  sr.owner_emp_id = u.row_id" );
    }


    q+=(            "  and srx.row_id = sr.row_id"
                    "  and u.row_id = c.row_id "
                    "  and g.row_id = sr.CST_CON_ID  "
                    "  and u.login = :engineer "
                    "  and sr.sr_stat_id = 'Open' "
                    "  and sr.agree_id = e.row_id "
                    "  and e.svc_calendar_id = cal.row_id "
                    "  and sr.cst_ou_id = ext.row_id "
                    "  and sr.X_PROD_FEATURE_ID = prd.row_id "
                    "  and ext.row_id = flag.row_id" );

    query.prepare( q );
        
    query.bindValue( ":engineer", engineer );
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    Debug::logQuery( query, db.connectionName() );
    
    while ( query.next() )
    {
        QueueItem i;
        
        i.id = query.value( 0 ).toString();
        i.geo = query.value( 1 ).toString();
        i.hours = query.value( 2 ).toString();
        i.status = query.value( 3 ).toString();
        i.severity = query.value( 4 ).toString();
        i.created = convertTime( query.value( 5 ).toString() );
        i.last_update = convertTime( query.value( 7 ).toString() );
        i.support_program = query.value( 8 ).toString();
        i.subtype = query.value( 9 ).toString();
        i.rating = "E";
        
        if ( query.value( 9 ).toString() == "Collaboration" )
        {
            i.isCr = true;
            i.creator = getCreator( i.id, dbname );
            QString sr = getSrForCr( i.id, mysqlname, reportname );
            
            if (!sr.isEmpty() && sr != "ERROR") {
                i.crsr = sr;
            
                QStringList infosr;
                QString cus_nam;
            
                infosr = srInfo( i.crsr, dbname);
            
                if (infosr.size() > 3) {
                    cus_nam = infosr.at( 3 );
            
                    if (!cus_nam.isEmpty()) 
                    {
                        i.rating = getRating( cus_nam, mysqlname );
                    }
                }
            }
        }
        else
        {
            i.isCr = false;
            i.customer = query.value( 14 ).toString();
            
            QString p = query.value( 15 ).toString();
            QString f = query.value( 16 ).toString();
            
            if ( p != f && !f.isEmpty() )
            {
                i.contact_phone = formatPhone( p, f );
            }
            else
            {
                i.contact_phone = p;
            }
                
            i.contact_firstname = query.value( 17 ).toString();
            i.contact_lastname = query.value( 18 ).toString();
            i.contact_email = query.value( 19 ).toString();
            i.contact_title = query.value( 20 ).toString();
            i.contact_lang = query.value( 21 ).toString();
         
            QString op = query.value( 22 ).toString();
            QString of = query.value( 23 ).toString();
        
            if ( op != of && !of.isEmpty() )
            {
                i.onsite_phone = formatPhone( op, of );
            }
            else
            {
                i.onsite_phone = op;
            }
            i.rating = getRating(i.customer, mysqlname );
        }
            
        i.service_level = query.value( 10 ).toInt();
        i.brief_desc = query.value( 11 ).toString().replace( "]]>", "]]&gt;" );
        
        if ( query.value( 12 ).toString() == "Y"  && !i.isCr )
        {
            i.critsit = true;
        }
        else
        {
            i.critsit = false;
        }
        
        if ( query.value( 13 ).toString() == "Y" )
        {
            i.high_value = true;
        }
        else
        {
            i.high_value = false;
        }
        
        i.detailed_desc =  query.value( 24 ).toString();
        i.alt_contact = query.value( 25 ).toString();
        i.bugId = query.value( 26 ).toString();
        i.cstNum = query.value( 27 ).toString();
        i.owner = query.value( 28 ).toString();
        i.subOwner = query.value( 29 ).toString();
        
	if ( !i.bugId.isEmpty() )
        {
            i.bugDesc = getBugDesc( i.bugId, mysqlname );
        }
        
        //Database::checkLTSSbyId( i.cstNum, mysqlname );
        list.append( i );
    }
        
    db.commit();
    mysqldb.commit();
    
    return list;
}

QueueItem Database::getSrInfo( const QString& sr, const QString& dbname, const QString& mysqlname, const QString& reportname )
{
    QSqlDatabase db;
    QSqlDatabase mysqldb;
     
    db = QSqlDatabase::database( dbname );
    mysqldb = QSqlDatabase::database( mysqlname );
    db.transaction();
    mysqldb.transaction();
    
    QSqlQuery query( db );
    QueueItem i;

    query.prepare( "select "
                    "  sr.sr_num as ID, "
                    "  case "
                    "    when sr.BU_ID = '0-R9NH' then 'Default Organization' "
                    "    when sr.BU_ID = '1-AHT' then 'EMEA' "
                    "    when sr.BU_ID = '1-AHV' then 'ASIAPAC' "
                    "    when sr.BU_ID = '1-AHX' then 'USA' "
                    "    when sr.BU_ID = '1-AHZ' then 'LATIN AMERICA' "
                    "    when sr.BU_ID = '1-AI1' then 'CANADA' "
                    "  else 'Undefined' end as GEO,  "
                    "  cal.NAME as HOURS, "
                    "  sr.SR_SUB_STAT_ID as STATUS, "
                    "  sr.SR_SEV_CD as SEVERITY, "
                    "  sr.CREATED, "
                    "  sr.CREATED_BY, "
                    "  sr.LAST_UPD as LAST_UPDATE,  "
                    "  e.NAME as SUPPORT_PROGRAM,"
                    "  sr.X_SR_SUB_TYPE as SUBTYPE, "
                    "  e.ENTL_PRIORITY_NUM as SERVICE_LEVEL, "
                    "  SR_TITLE as BRIEF_DESC, "
                    "  flag.ATTRIB_11 as CRITSIT, "
                    "  flag.ATTRIB_56 as HIGH_VALUE, "
                    "  ext.NAME as CUSTOMER, "
                    "  REGEXP_REPLACE( g.WORK_PH_NUM, '(.*)' || CHR(10) || '(.*)', '\\1') AS CONTACT_PHONE,"
                    "  REGEXP_REPLACE( g.WORK_PH_NUM, '(.*)' || CHR(10) || '(.*)', '\\2') AS FORMAT_STRING,"
                    "  g.FST_NAME as CONTACT_FIRSTNAME, "
                    "  g.LAST_NAME as CONTACT_LASTNAME, "
                    "  g.EMAIL_ADDR as CONTACT_EMAIL, "
                    "  g.JOB_TITLE as CONTACT_TITLE, "
                    "  g.PREF_LANG_ID as CONTACT_LANG,"
                    "  REGEXP_REPLACE( sr.X_ONSITE_PH_NUM, '(.*)' || CHR(10) || '(.*)', '\\1') AS ONSITE_PHONE,"
                    "  REGEXP_REPLACE( sr.X_ONSITE_PH_NUM, '(.*)' || CHR(10) || '(.*)', '\\2') AS ONSITE_FORMAT_STRING,"
                    "  sr.DESC_TEXT as DETAILED_DESC, "
                    "  sr.X_ALT_CONTACT, "
                    "  sr.X_DEFECT_NUM, "
                    "  ext.X_ORACLE_CUSTOMER_ID "
                    "from "
                    "  siebel.s_srv_req sr, "
                    "  siebel.s_user u, "
                    "  siebel.s_contact c, "
                    "  siebel.s_contact g, "
                    "  siebel.s_entlmnt e, "
                    "  siebel.s_sched_cal cal, "
                    "  siebel.s_org_ext ext, "
                    "  siebel.s_prod_int prd, "
                    "  siebel.s_org_ext_x flag "
                    "where "
                    "  sr.owner_emp_id = u.row_id "
                    "  and u.row_id = c.row_id "
                    "  and g.row_id = sr.CST_CON_ID  "
                    "  and sr.sr_num = :sr "
                    "  and sr.agree_id = e.row_id "
                    "  and e.svc_calendar_id = cal.row_id "
                    "  and sr.cst_ou_id = ext.row_id "
                    "  and sr.X_PROD_FEATURE_ID = prd.row_id "
                    "  and ext.row_id = flag.row_id" );
        
    query.bindValue( ":sr", sr );
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    Debug::logQuery( query, db.connectionName() );
    
    if ( query.next() )
    {
        i.id = query.value( 0 ).toString();
        i.geo = query.value( 1 ).toString();
        i.hours = query.value( 2 ).toString();
        i.status = query.value( 3 ).toString();
        i.severity = query.value( 4 ).toString();
        i.created = convertTime( query.value( 5 ).toString() );
        i.last_update = convertTime( query.value( 7 ).toString() );
        i.support_program = query.value( 8 ).toString();
        i.subtype = query.value( 9 ).toString();
        i.rating = "E";
        
        if ( query.value( 9 ).toString() == "Collaboration" )
        {
            i.isCr = true;
            i.creator = getCreator( i.id, dbname );
            QString sr = getSrForCr( i.id, mysqlname, reportname );
            
            if (!sr.isEmpty() && sr != "ERROR") {
                i.crsr = sr;
            
                QStringList infosr;
                QString cus_nam;
            
                infosr = srInfo( i.crsr, dbname);
            
                if (infosr.size() > 3) {
                    cus_nam = infosr.at( 3 );
            
                    if (!cus_nam.isEmpty()) 
                    {
                        i.rating = getRating( cus_nam, mysqlname );
                    }
                }
                if (!cus_nam.isEmpty()) 
                {
                    i.rating = getRating( cus_nam, mysqlname );
                }
            }
        }
        else
        {
            i.isCr = false;
            i.customer = query.value( 14 ).toString();
            
            QString p = query.value( 15 ).toString();
            QString f = query.value( 16 ).toString();
            
            if ( p != f && !f.isEmpty() )
            {
                i.contact_phone = formatPhone( p, f );
            }
            else
            {
                i.contact_phone = p;
            }
                
            i.contact_firstname = query.value( 17 ).toString();
            i.contact_lastname = query.value( 18 ).toString();
            i.contact_email = query.value( 19 ).toString();
            i.contact_title = query.value( 20 ).toString();
            i.contact_lang = query.value( 21 ).toString();
         
            QString op = query.value( 22 ).toString();
            QString of = query.value( 23 ).toString();
        
            if ( op != of && !of.isEmpty() )
            {
                i.onsite_phone = formatPhone( op, of );
            }
            else
            {
                i.onsite_phone = op;
            }
            i.rating = getRating( i.customer, mysqlname );
        }
            
        i.service_level = query.value( 10 ).toInt();
        i.brief_desc = query.value( 11 ).toString().replace( "]]>", "]]&gt;" );;
        
        if ( query.value( 12 ).toString() == "Y" )
        {
            i.critsit = true;
        }
        else
        {
            i.critsit = false;
        }
        
        if ( query.value( 13 ).toString() == "Y" )
        {
            i.high_value = true;
        }
        else
        {
            i.high_value = false;
        }
        
        i.detailed_desc = escapeString( query.value( 24 ).toString().replace( "]]>", "]]&gt;" ) );
        i.alt_contact = query.value( 25 ).toString();
        
	    QString b = query.value(26).toString();

        if ( isBugID( b ) ) i.bugId = query.value( 26 ).toString();

        i.cstNum = query.value( 27 ).toString();
        
        if ( !i.bugId.isEmpty() && isBugID( i.bugId ) )
        {
            i.bugDesc = getBugDesc( i.bugId, mysqlname );
        }
    }
    
    db.commit();
    mysqldb.commit();
    
    return i;
}

bool Database::isBugID( const QString& bug )
{
    QRegExp reg( "^[0-9]{6}$" );

    if ( reg.exactMatch( bug.trimmed() ) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

QString Database::getSrStatus( const QString& sr, const QString& dbname )
{
    QSqlDatabase db;
        
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    QueueItem i;

    query.prepare( "select SR_STAT_ID from siebel.s_srv_req where sr_num = :sr" );
        
    query.bindValue( ":sr", sr );
    
    if ( !query.exec() ) 
    {
        qDebug() << query.lastError().text();
    }
    
    db.commit();
    Debug::logQuery( query, db.connectionName() );
    
    if ( query.next() )
    {
        return query.value( 0 ).toString();
    }
    else
    {
        return "NOT FOUND";
    }
}

void Database::updatePseudoQueues( const QString& qDb, const QString& mDb )
{
    QSqlDatabase qdb;
    QSqlDatabase mdb;
    
    qdb = QSqlDatabase::database( qDb );
    mdb = QSqlDatabase::database( mDb );
    qdb.transaction();
    mdb.transaction();
    
    QSqlQuery query( qdb );
    QSqlQuery inQuery( mdb );
    
    query.prepare( "SELECT PseudoQueue FROM _NovQueuePseudoQueue" );
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    while ( query.next() )
    {
        qDebug() << query.value(0).toString();
          inQuery.prepare( "INSERT INTO PSEUDOQ( QUEUENAME ) VALUES ( :queuename )" );
          inQuery.bindValue( ":queuename", query.value(0).toString() );
          inQuery.exec();
    }
    
    qdb.commit();
    mdb.commit();
}

void Database::updateLTSScustomers( const QString& rDb, const QString& mDb )
{
    QSqlDatabase mdb;
    QList< LTSScustomer > list;
    
    list = getLTSScustomersExt( rDb );
    mdb = QSqlDatabase::database( mDb );
    mdb.transaction();
    
    QSqlQuery delquery( mdb );
    
    delquery.prepare( "DELETE FROM LTSSCUSTOMERS" );
    delquery.exec();
    
    for ( int i = 0; i < list.size(); ++i )
    {  
        LTSScustomer c = list.at( i );
        QSqlQuery query( mdb );
        
        query.prepare( "INSERT INTO LTSSCUSTOMERS( ACCOUNT_NAME, ORACLE_CUSTOMER_NR, GEO, "
                       "                           SUPPORT_PROGRAM, AGREEMENT_NR, AGREEMENT_STATUS, ENTITLEMENT_ID, "
                       "                           ENTITLEMENT_NAME, ENTITLEMENT_START_DATE, ENTITLEMENT_END_DATE )"
                       "VALUES (                   :account_name, :oracle_customer_nr, :geo, :support_program, "
                       "                           :agreement_nr, :agreement_status, :entitlement_id, :entitlement_name, "
                       "                           :entitlement_start_date, :entitlement_end_date )" );
                       
        query.bindValue( ":account_name", c.account_name );
        query.bindValue( ":oracle_customer_nr", c.oracle_customer_nr );
        query.bindValue( ":geo", c.geo );
        query.bindValue( ":support_program", c.support_program );
        query.bindValue( ":agreement_nr", c.agreement_nr );
        query.bindValue( ":agreement_status", c.agreement_status );
        query.bindValue( ":entitlement_id", c.entitlement_id );
        query.bindValue( ":entitlement_name", c.entitlement_name );
        query.bindValue( ":entitlement_start_date", c.entitlement_start_date );
        query.bindValue( ":entitlement_end_date", c.entitlement_end_date );
        
        if ( !query.exec() ) qDebug() << query.lastError().text();
    }
    
    mdb.commit();
}

QStringList Database::getPseudoQueues( const QString& dbname )
{
    QStringList list;
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    query.exec( "SELECT * FROM PSEUDOQ ORDER BY QUEUENAME ASC" );
    
    while ( query.next() )
    {
        list.append( query.value( 0 ).toString() );
    }
    
    db.commit();
    
    return list;
}


void Database::deleteSiebelItemFromDB( const QString& id, const QString& dbname )
{
    Debug::print( "database", "Deleting SiebelItem " + id );
    
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    QSqlQuery query( db );
    
    query.prepare( "DELETE FROM QMON_SIEBEL WHERE ID = :id" );
    query.bindValue( ":id", id );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    Debug::logQuery( query, db.connectionName() );

    QSqlQuery cquery( db );
    
    cquery.prepare( "DELETE FROM CUSTOMER WHERE ID = :id" );
    cquery.bindValue( ":id", id );
    
    if ( !cquery.exec() ) qDebug() << cquery.lastError().text();
    
    db.commit();
    Debug::logQuery( cquery, db.connectionName() );
}

QStringList Database::getQmonSiebelList( const QString& dbname)
{
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
   
    QStringList l;
    
    query.prepare( "SELECT ID FROM QMON_SIEBEL" );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    db.commit();
    Debug::logQuery( query, db.connectionName() );

    while( query.next() )
    {
        l.append( query.value( 0 ).toString() );
    }
    
    return l;    
}

QStringList Database::getSrnumsForQueue( const QString& queue, const QString& geo, const QString& dbname  )
{
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    QSqlQuery query( db );
   
    QStringList l;
    
    query.prepare( "SELECT ID FROM QMON_SIEBEL WHERE ( GEO = :geo ) AND ( QUEUE = :queue )" );
    query.bindValue( ":geo", geo );
    query.bindValue( ":queue", queue );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    Debug::logQuery( query, db.connectionName() );

    while( query.next() )
    {
        l.append( query.value( 0 ).toString() );
    }
    
    db.commit();
    return l;    
}

bool Database::siebelExistsInDB( const QString& id, const QString& dbname )
{
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    QSqlQuery query( db );
    
    query.prepare( "SELECT ID FROM QMON_SIEBEL WHERE ( ID = :id )" );
    query.bindValue( ":id", id );
       
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    Debug::logQuery( query, db.connectionName() );
    db.commit();
    
    if ( query.next() )
    {
        return true;
    }
    else 
    {    
        return false;
    }
}

bool Database::siebelQueueChanged( SiebelItem si, const QString& dbname )
{
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    QSqlQuery query( db );
    
    query.prepare( "SELECT QUEUE FROM QMON_SIEBEL WHERE ( ID = :id )" );
    query.bindValue( ":id", si.id );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();

    db.commit();
    Debug::logQuery( query, db.connectionName() );
    
    if ( query.next() )
    {
        if ( query.value( 0 ).toString() == si.queue )
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return true;
    }
}

bool Database::siebelSeverityChanged( SiebelItem si, const QString& dbname )
{
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    
    query.prepare( "SELECT SEVERITY FROM QMON_SIEBEL WHERE ( ID = :id )" );
    query.bindValue( ":id", si.id );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();

    db.commit();
    Debug::logQuery( query, db.connectionName() );
    
    if ( query.next() )
    {
        if ( query.value( 0 ).toString() == si.severity )
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return true;
    }
}

bool Database::isChat( const QString& id, const QString& dbname )
{
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    
    query.prepare( "SELECT ID FROM QMON_CHAT WHERE ( SR = :id )" );
    query.bindValue( ":id", id );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    Debug::logQuery( query, db.connectionName() );
    db.commit();
    
    if ( query.next() )
    {
        return true;
    }
    else 
    {    
        return false;
    }
}

QString Database::getQmonBdesc( const QString& id, const QString& dbname )
{
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    
    query.prepare( "SELECT BDESC FROM QMON_SIEBEL WHERE ( ID = :id )" );
    query.bindValue( ":id", id );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    db.commit();
    
    Debug::logQuery( query, db.connectionName() );
    
    if ( query.next() )
    {
        return query.value( 0 ).toString();
    }
    else
    {
        return "ERROR";
    }
}

void Database::updateBomgarItemInDB( BomgarItem bi, const QString& dbname )
{
    Debug::print( "database", "Inserting BomgarItem " + bi.id + " " + bi.sr );
        
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    
    query.prepare( "INSERT INTO QMON_CHAT( ID, SR, NAME, DATE ) VALUES ( :id, :sr, :name, :date )" );
                     
    query.bindValue( ":id", bi.id );
    query.bindValue( ":sr", bi.sr );
    query.bindValue( ":name", bi.name );
    query.bindValue( ":date", bi.date );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    db.commit();
    Debug::logQuery( query, db.connectionName() );   
}

void Database::deleteBomgarItemFromDB( const QString& id, const QString& dbname )
{
    Debug::print( "database", "Deleting BomgarItem " + id );
    
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    QSqlQuery query( db );
    
    query.prepare( "DELETE FROM QMON_CHAT WHERE ID = :id" );
    query.bindValue( ":id", id );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    db.commit();
    Debug::logQuery( query, db.connectionName() );    
}


QList< SiebelItem > Database::getSrsForQueue( const QString& queue, const QString& dbname )
{
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    
    QList< SiebelItem > list;
    
    if (  queue == "NONE" )
    {    
        query.prepare( "SELECT ID, QUEUE, GEO, HOURS, STATUS, SEVERITY, SOURCE, RESPOND_VIA, CREATED, LAST_UPDATE, "
                       "INQUEUE, SLA, SUPPORT_PROGRAM, SUPPORT_PROGRAM_LONG, ROUTING_PRODUCT, SUPPORT_GROUP_ROUTING, "
                       "INT_TYPE, SUBTYPE, SERVICE_LEVEL, BRIEF_DESC, CRITSIT, HIGH_VALUE, DETAILED_DESC, CATEGORY, "
                       "CREATOR, ROW_ID, SUBOWNER, RATING from QMON_SIEBEL ORDER BY CREATED ASC" );
    }
    else
    {
        query.prepare( "SELECT ID, QUEUE, GEO, HOURS, STATUS, SEVERITY, SOURCE, RESPOND_VIA, CREATED, LAST_UPDATE, "
                       "INQUEUE, SLA, SUPPORT_PROGRAM, SUPPORT_PROGRAM_LONG, ROUTING_PRODUCT, SUPPORT_GROUP_ROUTING, "
                       "INT_TYPE, SUBTYPE, SERVICE_LEVEL, BRIEF_DESC, CRITSIT, HIGH_VALUE, DETAILED_DESC, CATEGORY, "
                       "CREATOR, ROW_ID, SUBOWNER, RATING from QMON_SIEBEL WHERE ( QUEUE = :queue ) ORDER BY CREATED ASC" );
        
        query.bindValue( ":queue", queue );
    }
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    Debug::logQuery( query, db.connectionName() );
    
    while ( query.next() ) 
    {
        SiebelItem si;
        
        si.id = query.value( 0 ).toString();
        si.queue = query.value( 1 ).toString();
        si.geo = query.value( 2 ).toString();
        si.hours = query.value( 3 ).toString();
        si.status = query.value( 4 ).toString();
        si.severity = query.value( 5 ).toString();
        si.source = query.value( 6 ).toString();
        si.respond_via = query.value( 7 ).toString();
        si.created = query.value( 8 ).toString();
        si.last_update = query.value( 9 ).toString();
        si.inqueue = query.value( 10 ).toString();
        si.sla = query.value( 11 ).toString();
        si.support_program = query.value( 12 ).toString();
        si.support_program_long = query.value( 13 ).toString();
        si.routing_product = query.value( 14 ).toString();
        si.support_group_routing = query.value( 15 ).toString();
        si.int_type = query.value( 16 ).toString();
        si.subtype = query.value( 17 ).toString();
        si.service_level = query.value( 18 ).toString();
        si.brief_desc = query.value( 19 ).toString().replace( "]]>", "]]&gt;" );
        si.critsit = query.value( 20 ).toBool();
        si.high_value = query.value( 21 ).toBool();
        si.detailed_desc = query.value( 22 ).toString().replace( "]]>", "]]&gt;" );
        si.category = query.value( 23 ).toString();
        si.row_id = query.value( 25 ).toString();
	si.subowner = query.value( 26 ).toString();
        si.rating = query.value( 27 ).toString();
        
        if ( getBomgarQueue( query.value( 0 ).toString(), dbname ) == "NOCHAT" )
        {
            si.isChat = false;
        }
        else
        {
            si.isChat = true;
            si.bomgarQ = getBomgarQueue( query.value( 0 ).toString(), dbname );
        }
        
        if ( si.subtype == "Collaboration" )
        {
            si.isCr = true;
            si.creator = query.value( 24 ).toString();
            si.crsr = getSrForCrMysql( si.id, dbname );
        }
        else
        {
            si.isCr = false;
        
            QSqlQuery cquery( db );
        
            cquery.prepare( "SELECT CUSTOMER, CONTACT_PHONE, CONTACT_FIRSTNAME, CONTACT_LASTNAME, CONTACT_EMAIL, "
                            "       CONTACT_TITLE, CONTACT_LANG, ONSITE_PHONE, ORACLE_ID FROM CUSTOMER WHERE ID = :id" );
        
            cquery.bindValue( ":id", si.id );
        
            if ( !cquery.exec() ) qDebug() << cquery.lastError().text();
        
            if ( cquery.next() ) 
            {
                si.customer = cquery.value( 0 ).toString();
                si.contact_phone = cquery.value( 1 ).toString();
                si.contact_firstname = cquery.value( 2 ).toString();
                si.contact_lastname = cquery.value( 3 ).toString();
                si.contact_email = cquery.value( 4 ).toString();
                si.contact_title = cquery.value( 5 ).toString();
                si.contact_lang = cquery.value( 6 ).toString();
                si.onsite_phone = cquery.value( 7 ).toString(); 
                si.cstNum = cquery.value( 8 ).toString();
            }
        }
       
        list.append( si );        
    }
        
    db.commit();
    return list;
}

QStringList Database::getCurrentBomgars( const QString& dbname )
{
    QStringList list;
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );

    query.prepare( "SELECT SR, NAME FROM QMON_CHAT" );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    Debug::logQuery( query, db.connectionName() );
        
    while( query.next() )
    {
        QString tmp = query.value( 0 ).toString() + "|||" + query.value( 1 ).toString();
        list.append( tmp );
    }

    db.commit();
    return list;
}

QStringList Database::getQmonBomgarList( const QString& dbname )
{
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    QStringList l;
    
    query.prepare( "SELECT ID FROM QMON_CHAT" );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    Debug::logQuery( query, db.connectionName() );
    
    while( query.next() ) 
    {
        l.append( query.value( 0 ).toString() );
    }
    
    db.commit();
    return l;    
}

bool Database::bomgarExistsInDB( const QString& id, const QString& dbname )
{
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    
    query.prepare( "SELECT ID FROM QMON_CHAT WHERE ( ID = :id )" );
    query.bindValue( ":id", id );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    db.commit();
    Debug::logQuery( query, db.connectionName() );
    
    if ( query.next() )
    {
        return true;
    }
    else 
    {
        return false;
    }
}

void Database::updateBomgarQueue( BomgarItem bi, const QString& dbname )
{
    Debug::print( "database", "Updating BomgarQueue for " + bi.id + " " + bi.sr + " to " + bi.name );
    
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    
    query.prepare( "UPDATE QMON_CHAT SET NAME = :name WHERE ID = :id" );
    query.bindValue( ":name", bi.name );
    query.bindValue( ":id", bi.id );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    db.commit();
    Debug::logQuery( query, db.connectionName() );   
}

QString Database::getBomgarQueue( const QString& id, const QString& dbname )
{
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    
    query.prepare( "SELECT NAME FROM QMON_CHAT WHERE ( SR = :id )" );
    query.bindValue( ":id", id );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    db.commit();
    Debug::logQuery( query, db.connectionName() );
    
    if( query.next() ) 
    {
        return query.value( 0 ).toString();
    }
    else
    {
        return "NOCHAT";
    }
}

QString Database::getBomgarQueueById( const QString& id, const QString& dbname )
{
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    
    query.prepare( "SELECT NAME FROM QMON_CHAT WHERE ( ID = :id )" );
    query.bindValue( ":id", id );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    Debug::logQuery( query, db.connectionName() );
    
    if( query.next() ) 
    {
        db.commit();
        return query.value( 0 ).toString();
    }
    else
    {
        db.commit();
        return "NOCHAT";
    }
}

QString Database::convertTime( const QString& dt )
{
    QDateTime d = QDateTime::fromString( dt, "M/d/yyyy hh:mm:ss AP" );
    
    if ( !d.isValid() ) d = QDateTime::fromString( dt, "yyyy-MM-ddThh:mm:ss" );

    return ( d.toString("yyyy-MM-dd hh:mm:ss") );
}

QList< SiebelItem > Database::getQmonSrs( const QString& dbname, const QString& mysqlname, const QString& reportname )
{
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    
    QList< SiebelItem > list;
    
    query.prepare( " SELECT T10.SR_NUM  ID, "
                   "        T11.LOGIN                                                        QUEUE, "
                   " CASE "
                   "         WHEN T10.BU_ID = '0-R9NH' THEN 'Default Organization'"
                   "         WHEN T10.BU_ID = '1-AHT'  THEN 'EMEA'"
                   "         WHEN T10.BU_ID = '1-AHV'  THEN 'ASIAPAC'"
                   "         WHEN T10.BU_ID = '1-AHX'  THEN 'USA'"
                   "         WHEN T10.BU_ID = '1-AHZ'  THEN 'LATIN AMERICA'"
                   "         WHEN T10.BU_ID = '1-AI1'  THEN 'CANADA'"
                   "         ELSE 'Undefined' "
                   "       END                                                               GEO,"
                   "       T15.NAME                                                          HOURS,"
                   "       T10.SR_SUB_STAT_ID                                                STATUS,"
                   "       T10.SR_SEV_CD                                                     SEVERITY,"
                   "       T10.SR_SUBTYPE_CD                                                 SOURCE,"
                   "       T10.X_RESPOND_VIA                                                 RESPOND_VIA,"
                   "       T10.CREATED,"
                   "       T10.LAST_UPD                                                      LAST_UPDATE,"
                   "       NVL(T10.EXP_CLOSE_DT,TO_DATE('01-01-1970','MM-DD-YYYY'))          SLA,"
                   "       T10.X_SUPPORT_PROG                                                SUPPORT_PROGRAM,"
                   "       T14.NAME                                                          SUPPORT_PROGRAM_LONG,"
                   "       T17.NAME                                                          ROUTING_PRODUCT,"
                   "       T10.X_SUPP_GRP_ROUTING                                            SUPPORT_GROUP_ROUTING,"
                   "       T10.SR_TYPE_CD                                                    INT_TYPE,"
                   "       T10.X_SR_SUB_TYPE                                                 SUBTYPE,"
                   "       T14.ENTL_PRIORITY_NUM                                             SERVICE_LEVEL,"
                   "       SR_TITLE                                                          BRIEF_DESC,"
                   "       T18.ATTRIB_11                                                     CRITSIT,"
                   "       T18.ATTRIB_56                                                     HIGH_VALUE,"
                   "       T16.NAME                                                          CUSTOMER,"
                   "       REGEXP_REPLACE(T13.WORK_PH_NUM,'(.*)'||CHR(10)||'(.*)','\\1')     CONTACT_PHONE,"
                   "       REGEXP_REPLACE(T13.WORK_PH_NUM,'(.*)'||CHR(10)||'(.*)','\\2')     FORMAT_STRING,"
                   "       T13.FST_NAME                                                      CONTACT_FIRSTNAME,"
                   "       T13.LAST_NAME                                                     CONTACT_LASTNAME,"
                   "       T13.EMAIL_ADDR                                                    CONTACT_EMAIL,"
                   "       T13.JOB_TITLE                                                     CONTACT_TITLE,"
                   "       T13.PREF_LANG_ID                                                  CONTACT_LANG,"
                   "       REGEXP_REPLACE(T10.X_ONSITE_PH_NUM,'(.*)'||CHR(10)||'(.*)','\\1') ONSITE_PHONE,"
                   "       REGEXP_REPLACE(T10.X_ONSITE_PH_NUM,'(.*)'||CHR(10)||'(.*)','\\2') ONSITE_FORMAT_STRING,"
                   "       T10.DESC_TEXT                                                     DETAILED_DESC,"
                   "       T10.SR_CATEGORY_CD                                                CATEGORY,"
                   "       T10.ROW_ID,"
                   "       T10.X_ALT_CONTACT,"
                   "       T10.X_DEFECT_NUM,"
                   "       T16.X_ORACLE_CUSTOMER_ID,"
                   "       T20.LOGIN                                                         SUBOWNER"
                   "  FROM SIEBEL.S_SRV_REQ   T10,"
                   "       SIEBEL.S_USER      T11,"
                   "       SIEBEL.S_CONTACT   T12,"
                   "       SIEBEL.S_CONTACT   T13,"
                   "       SIEBEL.S_ENTLMNT   T14,"
                   "       SIEBEL.S_SCHED_CAL T15,"
                   "       SIEBEL.S_ORG_EXT   T16,"
                   "       SIEBEL.S_PROD_INT  T17,"
                   "       SIEBEL.S_ORG_EXT_X T18,"
                   "       SIEBEL.S_SRV_REQ_X T19,"
                   "       SIEBEL.S_USER      T20"
                   " WHERE T10.SR_STAT_ID = 'Open'"
                   "   AND T11.ROW_ID     = T10.OWNER_EMP_ID"
                   "   AND T12.ROW_ID     = T10.OWNER_EMP_ID"
                   "   AND T12.JOB_TITLE  = 'Pseudo User'"
                   "   AND T13.ROW_ID     = T10.CST_CON_ID"
                   "   AND T14.ROW_ID     = T10.AGREE_ID"
                   "   AND T15.ROW_ID     = T14.SVC_CALENDAR_ID"
                   "   AND T16.ROW_ID     = T10.CST_OU_ID"
                   "   AND T17.ROW_ID     = T10.X_PROD_FEATURE_ID"
                   "   AND T18.ROW_ID     = T10.CST_OU_ID "
                   "   AND T19.ROW_ID     = T10.ROW_ID"
                   "   AND T20.ROW_ID (+) = T19.ATTRIB_07"
                   " UNION ALL "
                   "SELECT T10.SR_NUM                                                        ID,"
                   "       T11.LOGIN                                                         QUEUE,"
                   "       CASE"
                   "         WHEN T10.BU_ID = '0-R9NH' THEN 'Default Organization'"
                   "         WHEN T10.BU_ID = '1-AHT'  THEN 'EMEA'"
                   "         WHEN T10.BU_ID = '1-AHV'  THEN 'ASIAPAC'"
                   "         WHEN T10.BU_ID = '1-AHX'  THEN 'USA'"
                   "         WHEN T10.BU_ID = '1-AHZ'  THEN 'LATIN AMERICA'"
                   "         WHEN T10.BU_ID = '1-AI1'  THEN 'CANADA'"
                   "         ELSE 'Undefined'"
                   "       END                                                               GEO,"
                   "       T15.NAME                                                          HOURS,"
                   "       T10.SR_SUB_STAT_ID                                                STATUS,"
                   "       T10.SR_SEV_CD                                                     SEVERITY,"
                   "       T10.SR_SUBTYPE_CD                                                 SOURCE,"
                   "       T10.X_RESPOND_VIA                                                 RESPOND_VIA,"
                   "       T10.CREATED,"
                   "       T10.LAST_UPD                                                      LAST_UPDATE,"
                   "       NVL(T10.EXP_CLOSE_DT,TO_DATE('01-01-1970','MM-DD-YYYY'))          SLA,"
                   "       T10.X_SUPPORT_PROG                                                SUPPORT_PROGRAM,"
                   "       T14.NAME                                                          SUPPORT_PROGRAM_LONG,"
                   "       T17.NAME                                                          ROUTING_PRODUCT,"
                   "       T10.X_SUPP_GRP_ROUTING                                            SUPPORT_GROUP_ROUTING,"
                   "       T10.SR_TYPE_CD                                                    INT_TYPE,"
                   "       T10.X_SR_SUB_TYPE                                                 SUBTYPE,"
                   "       T14.ENTL_PRIORITY_NUM                                             SERVICE_LEVEL,"
                   "       SR_TITLE                                                          BRIEF_DESC,"
                   "       T18.ATTRIB_11                                                     CRITSIT,"
                   "       T18.ATTRIB_56                                                     HIGH_VALUE,"
                   "       T16.NAME                                                          CUSTOMER,"
                   "       REGEXP_REPLACE(T13.WORK_PH_NUM,'(.*)'||CHR(10)||'(.*)','\\1')     CONTACT_PHONE,"
                   "       REGEXP_REPLACE(T13.WORK_PH_NUM,'(.*)'||CHR(10)||'(.*)','\\2')     FORMAT_STRING,"
                   "       T13.FST_NAME                                                      CONTACT_FIRSTNAME,"
                   "       T13.LAST_NAME                                                     CONTACT_LASTNAME,"
                   "       T13.EMAIL_ADDR                                                    CONTACT_EMAIL,"
                   "       T13.JOB_TITLE                                                     CONTACT_TITLE,"
                   "       T13.PREF_LANG_ID                                                  CONTACT_LANG,"
                   "       REGEXP_REPLACE(T10.X_ONSITE_PH_NUM,'(.*)'||CHR(10)||'(.*)','\\1') ONSITE_PHONE,"
                   "       REGEXP_REPLACE(T10.X_ONSITE_PH_NUM,'(.*)'||CHR(10)||'(.*)','\\2') ONSITE_FORMAT_STRING,"
                   "       T10.DESC_TEXT                                                     DETAILED_DESC,"
                   "       T10.SR_CATEGORY_CD                                                CATEGORY,"
                   "       T10.ROW_ID,"
                   "       T10.X_ALT_CONTACT,"
                   "       T10.X_DEFECT_NUM,"
                   "       T16.X_ORACLE_CUSTOMER_ID,"
                   "       T20.LOGIN                                                         SUBOWNER"
                   "  FROM SIEBEL.S_SRV_REQ   T10,"
                   "       SIEBEL.S_USER      T11,"
                   "       SIEBEL.S_CONTACT   T13,"
                   "       SIEBEL.S_ENTLMNT   T14,"
                   "       SIEBEL.S_SCHED_CAL T15,"
                   "       SIEBEL.S_ORG_EXT   T16,"
                   "       SIEBEL.S_PROD_INT  T17,"
                   "       SIEBEL.S_ORG_EXT_X T18,"
                   "       SIEBEL.S_SRV_REQ_X T19,"
                   "       SIEBEL.S_USER      T20,"
                   "       SIEBEL.S_CONTACT   T21"
                   " WHERE T10.SR_STAT_ID = 'Open'"
                   "   AND T13.ROW_ID     = T10.CST_CON_ID"
                   "   AND T14.ROW_ID     = T10.AGREE_ID"
                   "   AND T15.ROW_ID     = T14.SVC_CALENDAR_ID"
                   "   AND T16.ROW_ID     = T10.CST_OU_ID"
                   "   AND T17.ROW_ID     = T10.X_PROD_FEATURE_ID"
                   "   AND T18.ROW_ID     = T10.CST_OU_ID "
                   "   AND T19.ROW_ID     = T10.ROW_ID"
                   "   AND T21.ROW_ID (+) = T19.ATTRIB_07"
                   "   AND T20.ROW_ID (+) = T19.ATTRIB_07"
                   "   AND T11.ROW_ID (+) = T10.OWNER_EMP_ID"
                   "   AND T21.JOB_TITLE  = 'Pseudo User'"
                   " UNION ALL "
                   " SELECT T20.SR_NUM                                               ID, "
                   "       NVL(T21.LOGIN,'null')                                    QUEUE, "
                   "       CASE" 
                   "         WHEN T20.BU_ID = '0-R9NH' THEN 'Default Organization'"
                   "         WHEN T20.BU_ID = '1-AHT'  THEN 'EMEA'"
                   "         WHEN T20.BU_ID = '1-AHV'  THEN 'ASIAPAC'"
                   "         WHEN T20.BU_ID = '1-AHX'  THEN 'USA'"
                   "         WHEN T20.BU_ID = '1-AHZ'  THEN 'LATIN AMERICA'"
                   "         WHEN T20.BU_ID = '1-AI1'  THEN 'CANADA'"
                   "         ELSE 'Undefined'"
                   "       END                                                      GEO,"
                   "       T23.NAME                                                 HOURS,"
                   "       T20.SR_SUB_STAT_ID                                       STATUS,"
                   "       T20.SR_SEV_CD                                            SEVERITY,"
                   "       T20.SR_SUBTYPE_CD                                        SOURCE,"
                   "       T20.X_RESPOND_VIA                                        RESPOND_VIA,"
                   "       T20.CREATED,"
                   "       T20.LAST_UPD                                             LAST_UPDATE,"
                   "       NVL(T20.EXP_CLOSE_DT,TO_DATE('01-01-1970','MM-DD-YYYY')) SLA,"
                   "       T20.X_SUPPORT_PROG                                       SUPPORT_PROGRAM,"
                   "       T22.NAME                                                 SUPPORT_PROGRAM_LONG,"
                   "       T25.NAME                                                 ROUTING_PRODUCT,"
                   "       T20.X_SUPP_GRP_ROUTING                                   SUPPORT_GROUP_ROUTING,"
                   "       T20.SR_TYPE_CD                                           INT_TYPE,"
                   "       T20.X_SR_SUB_TYPE                                        SUBTYPE,"
                   "       T22.ENTL_PRIORITY_NUM                                    SERVICE_LEVEL,"
                   "       SR_TITLE                                                 BRIEF_DESC,"
                   "       T26.ATTRIB_11                                            CRITSIT,"
                   "       T26.ATTRIB_56                                            HIGH_VALUE,"
                   "       T24.NAME                                                 CUSTOMER,"
                   "       DECODE(T20.OWNER_EMP_ID,'0-1','1',NULL,'0')              CONTACT_PHONE,"
                   "       DECODE(T20.OWNER_EMP_ID,'0-1','1',NULL,'0')              FORMAT_STRING,"
                   "       DECODE(T20.OWNER_EMP_ID,'0-1','1',NULL,'0')              CONTACT_FIRSTNAME,"
                   "       DECODE(T20.OWNER_EMP_ID,'0-1','1',NULL,'0')              CONTACT_LASTNAME,"
                   "       DECODE(T20.OWNER_EMP_ID,'0-1','1',NULL,'0')              CONTACT_EMAIL,"
                   "       DECODE(T20.OWNER_EMP_ID,'0-1','1',NULL,'0')              CONTACT_TITLE,"
                   "       DECODE(T20.OWNER_EMP_ID,'0-1','1',NULL,'0')              CONTACT_LANG,"
                   "       DECODE(T20.OWNER_EMP_ID,'0-1','1',NULL,'0')              ONSITE_PHONE,"
                   "       DECODE(T20.OWNER_EMP_ID,'0-1','1',NULL,'0')              ONSITE_FORMAT_STRING,"
                   "       DECODE(T20.OWNER_EMP_ID,'0-1','1',NULL,'0')              DETAILED_DESC,"
                   "       T20.SR_CATEGORY_CD                                       SR_CATEGORY,"
                   "       T20.ROW_ID,"
                   "       T20.X_ALT_CONTACT,"
                   "       T20.X_DEFECT_NUM,"
                   "       T24.X_ORACLE_CUSTOMER_ID,"
                   "       T28.LOGIN                                                SUBOWNER "
                   "  FROM SIEBEL.S_SRV_REQ   T20,"
                   "       SIEBEL.S_USER      T21,"
                   "       SIEBEL.S_ENTLMNT   T22,"
                   "       SIEBEL.S_SCHED_CAL T23,"
                   "       SIEBEL.S_ORG_EXT   T24,"
                   "       SIEBEL.S_PROD_INT  T25,"
                   "       SIEBEL.S_ORG_EXT_X T26,"
                   "       SIEBEL.S_SRV_REQ_X T27,"
                   "       SIEBEL.S_USER      T28"
                   " WHERE (T20.OWNER_EMP_ID = '0-1' OR T20.OWNER_EMP_ID IS NULL)"
                   "   AND T21.ROW_ID (+)    = T20.OWNER_EMP_ID "
                   "   AND T20.SR_STAT_ID    = 'Open'"
                   "   AND T22.ROW_ID        = T20.AGREE_ID"
                   "   AND T23.ROW_ID        = T22.SVC_CALENDAR_ID"
                   "   AND T24.ROW_ID        = T20.CST_OU_ID"
                   "   AND T25.ROW_ID        = T20.X_PROD_FEATURE_ID"
                   "   AND T26.ROW_ID        = T24.ROW_ID"
                   "   AND T28.ROW_ID (+)    = T27.ATTRIB_07"
                   "   AND T28.ROW_ID        = T20.ROW_ID" );

    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    Debug::logQuery( query, db.connectionName() );
    
    while ( query.next() ) 
    {
        SiebelItem si;
                
        si.id = query.value( 0 ).toString();
        si.queue = query.value( 1 ).toString();
        si.geo = query.value( 2 ).toString();
        si.hours = query.value( 3 ).toString();
        si.status = query.value( 4 ).toString();
        si.severity = query.value( 5 ).toString();
        si.source = query.value( 6 ).toString();
        si.respond_via = query.value( 7 ).toString();
        si.created = query.value( 8 ).toString();
        si.last_update = query.value( 9 ).toString();
        si.sla = query.value( 10 ).toString();
        si.support_program = query.value( 11 ).toString();
        si.support_program_long = query.value( 12 ).toString();
        si.routing_product = query.value( 13 ).toString();
        si.support_group_routing = query.value( 14 ).toString();
        si.int_type = query.value( 15 ).toString();
        si.subtype = query.value( 16 ).toString();
        si.service_level = query.value( 17 ).toString();
        si.brief_desc = query.value( 18 ).toString();
        si.customer = query.value( 21 ).toString();
        si.rating = "E";

        if ( query.value( 16 ).toString() == "Collaboration" )
        {
            si.isCr = true;
            si.creator = getCreator( si.id, dbname );
            
            QString sr = getSrForCr( si.id, mysqlname, reportname );
            
            if (!sr.isEmpty() && sr != "ERROR") {
                si.crsr = sr;
                
                QStringList infosr;
                QString cus_nam;
            
                infosr = srInfo( si.crsr, dbname);
                
                if (infosr.size() > 3) {
                    cus_nam = infosr.at( 3 );
                           
                    if (!cus_nam.isEmpty()) 
                    {
                        si.rating = getRating( cus_nam, mysqlname );
                    }
                }
            }
        }
        else
        {
            si.isCr = false;
            si.rating = getRating( si.customer, mysqlname );
        }
        
        if ( query.value( 19 ).toString() == "Y" )
        {
            si.critsit = true;
        }
        else
        {
            si.critsit = false;
        }
        
        if ( query.value( 20 ).toString() == "Y" )
        {
            si.high_value = true;
        }
        else
        {
            si.high_value = false;
        }
                
        QString p = query.value( 22 ).toString();
        QString f = query.value( 23 ).toString();
        
        if ( p != f && !f.isEmpty() )
        {
            si.contact_phone = formatPhone( p, f );
        }
        else
        {
            si.contact_phone = p;
        }
        
        //si.contact_phone = query.value( 22 ).toString();
        si.contact_firstname = query.value( 24 ).toString();
        si.contact_lastname = query.value( 25 ).toString();
        si.contact_email = query.value( 26 ).toString();
        si.contact_title = query.value( 27 ).toString();
        si.contact_lang = query.value( 28 ).toString();
        
        QString op = query.value( 29 ).toString();
        QString of = query.value( 30 ).toString();
        
        if ( op != of && !of.isEmpty() )
        {
            si.onsite_phone = formatPhone( op, of );
        }
        else
        {
            si.onsite_phone = op;
        }
        
        si.detailed_desc = query.value( 31 ).toString();
        si.category = query.value( 32 ).toString();
        si.row_id = query.value( 33 ).toString();
        si.alt_contact = query.value( 34 ).toString();
        si.bugId = query.value( 35 ).toString();
        si.cstNum = query.value( 36 ).toString();
	si.subowner = query.value( 37 ).toString();
        
        list.append( si );
    }

    db.commit();
    return list;
}

QStringList Database::srInfo( const QString& sr, const QString& dbname )
{
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    
    query.prepare( "SELECT SR_TITLE, g.FST_NAME, g.LAST_NAME, ext.NAME "
                   "from siebel.s_srv_req sr, siebel.s_contact g, siebel.s_org_ext ext "
                   "where sr.sr_num = :sr and g.row_id = sr.CST_CON_ID and ext.row_id = sr.CST_OU_ID" );
    
    query.bindValue( ":sr", sr );
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    QStringList info;
    
    if ( query.next() )
    {
        info.append( query.value( 0 ).toString() );
        info.append( query.value( 1 ).toString() );
        info.append( query.value( 2 ).toString() );
        info.append( query.value( 3 ).toString() );
    }

    db.commit();
    return info;
}

QList< BomgarItem > Database::getChats( const QString& dbname )
{
    QSqlDatabase db;
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    
    QList< BomgarItem > list;
    
    query.prepare( "SELECT LSID, EXTERNAL_KEY, CONFERENCE_NAME, TIMESTAMP FROM Collector_ODBC_NTSCHAT" );
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    Debug::logQuery( query, db.connectionName() );
    
    while ( query.next() ) 
    {
        BomgarItem bi;
        
        bi.id = query.value( 0 ).toString();
        bi.sr = query.value( 1 ).toString();
        bi.name = query.value( 2 ).toString();
        bi.date = query.value( 3 ).toString();
     
        list.append( bi );
    }
     
    db.commit();
    return list;
}

QString Database::formatPhone( QString p, const QString& f )
{
    int off;
    QChar zero( '0' );
    
    if ( p.startsWith( "+" ) )
    {
        p.insert( 3, " " );
        off = 4;
    }
        
    for ( int i = 0; i < f.size() - 1; ++i ) 
    {
        if ( f.at( i ) != zero )
        {
            p.insert( ( off + i ), f.at( i ) );
            off++;
        }
    }
        
    return p;
}

QString Database::getRating( QString customer, const QString& dbname )
{
    QSqlDatabase db;
    customer = customer.trimmed();
    
    if ( customer.isEmpty() ) {
        return "E";
    }
    
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    
    query.prepare( "SELECT RATING FROM TOPACCOUNTS WHERE ( ACCOUNT LIKE :customer )");
    
    query.bindValue( ":customer", customer);
    
    if ( !query.exec() ) qDebug() << query.lastError().text();
    
    db.commit();
    
    if ( query.size() == 0 ) {
         return "E";
    } else if ( query.next() ) {
         return query.value( 0 ).toString();
    } else {
         return "E";   
    }
}

QString Database::getBugDesc( QString bug, const QString& dbname )
{
    QSqlDatabase db;
    QString desc;
    bug = bug.trimmed();   

    if ( !isBugID( bug ) )
    {
        return QString::Null();
    }
 
    db = QSqlDatabase::database( dbname );
    db.transaction();
    
    QSqlQuery query( db );
    
    query.prepare( "SELECT TITLE FROM BUG WHERE ( ID = :bug )" );
                
    query.bindValue( ":bug", bug );
                
    if ( !query.exec() ) qDebug() << query.lastError().text();
        
    db.commit();
    
    if ( query.size() == 0 )
    {
        Debug::print( "database", "Could not find bug " + bug + " in the db, fetching info..." );
        Network* net = new Network;
        QEventLoop loop;
        QString xml;
        
        QUrl url( "https://apibugzilla.novell.com/show_bug.cgi?id=" + bug + "&ctype=xml" );
        url.setUserName( Settings::bugzillaUser() );
        url.setPassword( Settings::bugzillaPassword() );
        
        QNetworkReply *reply = net->getExt( url );
        
        loop.connect( reply, SIGNAL( readyRead() ),
                    &loop, SLOT( quit() ) );
            
        loop.exec();
        
        xml = reply->readAll();
        
        QDomDocument xmldoc;
        xmldoc.setContent(xml);
        QDomNodeList list = xmldoc.elementsByTagName( "short_desc" );
        desc = list.item(0).toElement().text();
        
        query.prepare( "INSERT INTO BUG( ID, TITLE ) VALUES ( :bug, :title )" );
                
        query.bindValue( ":bug", bug );
        query.bindValue( ":title", desc.trimmed() );
        
        if ( !query.exec() ) qDebug() << query.lastError().text();
        delete net;
        return desc.trimmed();
    }
    else if ( query.next() )
    { 
        return query.value( 0 ).toString();
    }
    else
    {
        return "No description - something went wrong :-(";
    }
}

QString Database::escapeString(QString str)
{
    QRegExp rx("(\\x001b|\\x0011)");
    rx.setMinimal(true);
    int s = -1;
    
    while( ( s = rx.indexIn( str, s+1 ) ) >= 0 )
    {
        str.replace(s, rx.cap(0).length(), "");
        s+= rx.cap( 1 ).length();
    }
    
    return str;
}

bool Database::openMysqlDB( const QString& name )
{
    print_open_dbs();
    if ( !QSqlDatabase::database( name ).isOpen() )
    {
        QSqlDatabase mysqlDB = QSqlDatabase::addDatabase( "QMYSQL", name );
        
        mysqlDB.setHostName( Settings::mysqlHost() );
        mysqlDB.setDatabaseName( Settings::mysqlDatabase() );
        mysqlDB.setUserName( Settings::mysqlUser() );
        mysqlDB.setPassword( Settings::mysqlPassword() );
        
        if ( !mysqlDB.open() )
        {
            Debug::print( "database", "Failed to open the database " + mysqlDB.lastError().text() );
            return false;
        }
        else
        {
            Debug::print( "database", "Opened DB " + mysqlDB.connectionName() );
            return true;
        }
    }
    else
    {
        Debug::print( "database", "MySQL DB already open in this thread " + name );
        return true;
    }
}

bool Database::openQmonDB( const QString& name )
{
    print_open_dbs();
    if ( !QSqlDatabase::database( name ).isOpen() )
    {
        QSqlDatabase qmonDB = QSqlDatabase::addDatabase( "QODBC", name );
        
        qmonDB.setDatabaseName( Settings::qmonDbDatabase() );
        qmonDB.setUserName( Settings::qmonDbUser() );
        qmonDB.setPassword( Settings::qmonDbPassword() );
        
        if ( !qmonDB.open() )
        {
            Debug::print( "database", "Failed to open the Qmon DB " + qmonDB.lastError().text() );
            return false;
        }
        else
        {
            Debug::print( "database", "Opened DB " + qmonDB.connectionName() );
            return true;
        }
    }
    else
    {
        Debug::print( "database", "DB already open in this thread " + name );
        return true;
    }
}

bool Database::openSiebelDB( const QString& name )
{
    print_open_dbs();
    if ( !QSqlDatabase::database( name ).isOpen() )
    {
        QSqlDatabase siebelDB = QSqlDatabase::addDatabase( "QOCI", name );
        
        siebelDB.setDatabaseName( Settings::siebelDatabase() );
        siebelDB.setHostName( Settings::siebelHost() );
        siebelDB.setPort( 1521 );
        siebelDB.setUserName( Settings::siebelUser() );
        siebelDB.setPassword( Settings::siebelPassword() );
        
        if ( !siebelDB.open() )
        {
            Debug::print( "database", "Failed to open the Siebel DB " + siebelDB.lastError().text() );
            return false;
        }
        else
        {
            Debug::print( "database", "Opened DB " + siebelDB.connectionName() );
            return true;
        }
    }
    else
    {
        Debug::print( "database", "DB already open in this thread " + name );
        return true;
    }
}

bool Database::openReportDB( const QString& name )
{
    print_open_dbs();
    if ( !QSqlDatabase::database( name ).isOpen() )
    {
        QSqlDatabase reportDB = QSqlDatabase::addDatabase( "QOCI", name );

        reportDB.setDatabaseName( Settings::reportDatabase() );
        reportDB.setHostName( Settings::reportHost() );
        reportDB.setPort( 1521 );
        reportDB.setUserName( Settings::reportUser() );
        reportDB.setPassword( Settings::reportPassword() );
        
        if ( !reportDB.open() )
        {
            Debug::print( "database", "Failed to open the report DB " + reportDB.lastError().text() );
            return false;
        }
        else
        {
            Debug::print( "database", "Opened DB " + reportDB.connectionName() );
            return true;
        }
    }
    else
    {
        Debug::print( "database", "DB already open in this thread " + name );
        return true;
    }
}

void Database::print_open_dbs() {
    QStringList dbs = QSqlDatabase::connectionNames();
    
    foreach(const QString &str, dbs) {
        Debug::print( "database", "existing connections: " + str);
    }
}

#include "database.moc"

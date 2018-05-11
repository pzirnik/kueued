-- MySQL dump 10.11
--
-- Host: localhost    Database: kueued
-- ------------------------------------------------------
-- Server version	5.0.96

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

CREATE DATABASE /*!32312 IF NOT EXISTS*/ `kueued` /*!40100 DEFAULT CHARACTER SET latin1 */;

USE `kueued`;


--
-- Table structure for table `BUG`
--

DROP TABLE IF EXISTS `BUG`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `BUG` (
  `ID` varchar(10) NOT NULL,
  `TITLE` text,
  PRIMARY KEY  (`ID`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `CRSR`
--

DROP TABLE IF EXISTS `CRSR`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `CRSR` (
  `cr` varchar(12) NOT NULL default '',
  `sr` varchar(12) default NULL,
  PRIMARY KEY  (`cr`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `CUSTOMER`
--

DROP TABLE IF EXISTS `CUSTOMER`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `CUSTOMER` (
  `ID` varchar(20) character set utf8 collate utf8_unicode_ci NOT NULL,
  `CUSTOMER` text character set utf8 collate utf8_unicode_ci,
  `CONTACT_FIRSTNAME` text character set utf8 collate utf8_unicode_ci,
  `CONTACT_LASTNAME` text character set utf8 collate utf8_unicode_ci,
  `CONTACT_EMAIL` text character set utf8 collate utf8_unicode_ci,
  `CONTACT_TITLE` text character set utf8 collate utf8_unicode_ci,
  `CONTACT_LANG` text character set utf8 collate utf8_unicode_ci,
  `CONTACT_PHONE` text character set utf8 collate utf8_unicode_ci,
  `ONSITE_PHONE` text character set utf8 collate utf8_unicode_ci,
  `ORACLE_ID` text character set utf8 collate utf8_unicode_ci,
  `LTSS` tinyint(1) default NULL,
  PRIMARY KEY  (`ID`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `LTSSCUSTOMERS`
--

DROP TABLE IF EXISTS `LTSSCUSTOMERS`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `LTSSCUSTOMERS` (
  `ACCOUNT_NAME` text,
  `ORACLE_CUSTOMER_NR` varchar(10) default NULL,
  `GEO` text,
  `SUPPORT_PROGRAM` text,
  `AGREEMENT_NR` text,
  `AGREEMENT_STATUS` text,
  `ENTITLEMENT_ID` text,
  `ENTITLEMENT_NAME` text,
  `ENTITLEMENT_START_DATE` text,
  `ENTITLEMENT_END_DATE` text
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `PSEUDOQ`
--

DROP TABLE IF EXISTS `PSEUDOQ`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `PSEUDOQ` (
  `QUEUENAME` varchar(99) NOT NULL,
  PRIMARY KEY  (`QUEUENAME`),
  UNIQUE KEY `QUEUENAME` (`QUEUENAME`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `QMON_CHAT`
--

DROP TABLE IF EXISTS `QMON_CHAT`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `QMON_CHAT` (
  `ID` varchar(40) character set utf8 collate utf8_unicode_ci NOT NULL,
  `SR` varchar(15) character set utf8 collate utf8_unicode_ci default NULL,
  `NAME` text character set utf8 collate utf8_unicode_ci,
  `DATE` text character set utf8 collate utf8_unicode_ci,
  PRIMARY KEY  (`ID`),
  UNIQUE KEY `ID` (`ID`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `QMON_SIEBEL`
--

DROP TABLE IF EXISTS `QMON_SIEBEL`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `QMON_SIEBEL` (
  `ID` varchar(20) character set utf8 collate utf8_unicode_ci NOT NULL,
  `QUEUE` text character set utf8 collate utf8_unicode_ci,
  `GEO` text character set utf8 collate utf8_unicode_ci,
  `HOURS` text character set utf8 collate utf8_unicode_ci,
  `STATUS` text character set utf8 collate utf8_unicode_ci,
  `SEVERITY` text character set utf8 collate utf8_unicode_ci,
  `SOURCE` text character set utf8 collate utf8_unicode_ci,
  `RESPOND_VIA` text character set utf8 collate utf8_unicode_ci,
  `CREATED` text character set utf8 collate utf8_unicode_ci,
  `LAST_UPDATE` text character set utf8 collate utf8_unicode_ci,
  `INQUEUE` text character set utf8 collate utf8_unicode_ci,
  `SLA` text character set utf8 collate utf8_unicode_ci,
  `SUPPORT_PROGRAM` text character set utf8 collate utf8_unicode_ci,
  `SUPPORT_PROGRAM_LONG` text character set utf8 collate utf8_unicode_ci,
  `ROUTING_PRODUCT` text character set utf8 collate utf8_unicode_ci,
  `SUPPORT_GROUP_ROUTING` text character set utf8 collate utf8_unicode_ci,
  `INT_TYPE` text character set utf8 collate utf8_unicode_ci,
  `SUBTYPE` text character set utf8 collate utf8_unicode_ci,
  `SERVICE_LEVEL` text character set utf8 collate utf8_unicode_ci,
  `BRIEF_DESC` text character set utf8 collate utf8_unicode_ci,
  `CRITSIT` tinyint(4) default NULL,
  `HIGH_VALUE` tinyint(4) default NULL,
  `DETAILED_DESC` text character set utf8 collate utf8_unicode_ci,
  `CATEGORY` text character set utf8 collate utf8_unicode_ci,
  `CREATOR` text character set utf8 collate utf8_unicode_ci,
  `ROW_ID` text character set utf8 collate utf8_unicode_ci,
  `SUBOWNER` text,
  `RATING` varchar(4) character set utf8 collate utf8_unicode_ci NOT NULL DEFAULT "None",
  PRIMARY KEY  (`ID`),
  UNIQUE KEY `ID` (`ID`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

DROP TABLE IF EXISTS `TOPACCOUNTS`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `TOPACCOUNTS` (
  `ACCOUNT` varchar(250) character set utf8,
  `RATING` varchar(1) character set utf8 collate utf8_unicode_ci NOT NULL DEFAULT "E",
  PRIMARY KEY  (`ACCOUNT`),
  UNIQUE KEY `ACCOUNT` (`ACCOUNT`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2016-10-19 14:14:41

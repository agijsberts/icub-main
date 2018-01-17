// -*- Mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

/*
 * Copyright (C) 2012 iCub Facility, Istituto Italiano di Tecnologia
 * Authors: Alberto Cardellino
 * CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
 *
 */

#include "ethResource.h"
#include <ethManager.h>
#include <yarp/os/Time.h>
#include <yarp/os/Network.h>
#include <yarp/os/NetType.h>

// embobj
#include "EOropframe_hid.h"
#include "EOarray.h"
#include "EoProtocol.h"
#include "EoManagement.h"
#include "EoProtocolMN.h"
#include "can_string_eth.h"

using namespace yarp::dev;
using namespace yarp::os;
using namespace yarp::os::impl;

#include <theNVmanager.h>
#include "ethParser.h"
using namespace eth;





// - class EthResource

EthResource::EthResource()
{
    ipv4addr = 0;
    ipv4addrstring = "0.0.0.0";
    ethboardtype = detectedBoardType = eobrd_ethtype_unknown;
    boardTypeString = "unknown";

    ethManager                  = NULL;
    isInRunningMode             = false;
    objLock                     = new Semaphore(1);

    verifiedBoardPresence       = false;
    askedBoardVersion           = false;
    verifiedBoardTransceiver    = false;
    txrateISset                 = false;
    cleanedBoardBehaviour       = false;

    boardVersion.major = boardVersion.minor = 0;
    boardMNprotocolversion.major = boardMNprotocolversion.minor;
    boardDate.year = 1999;
    boardDate.month = 9;
    boardDate.day = 9;
    boardDate.hour = 12;
    boardDate.min = 12;

    memset(verifiedEPprotocol, 0, sizeof(verifiedEPprotocol));

    usedNumberOfRegularROPs = 0;

    for(int i = 0; i<16; i++)
    {
        c_string_handler[i] = NULL;
    }

    ConstString tmp = NetworkBase::getEnvironment("ETH_VERBOSEWHENOK");
    if (tmp != "")
    {
        verbosewhenok = (bool)NetType::toInt(tmp);
    }
    else
    {
        verbosewhenok = false;
    }

    verbosewhenok = true;

    regularsAreSet = false;
}


EthResource::~EthResource()
{
    ethManager = NULL;
    delete objLock;


    // Delete every initialized can_string_eth object
    for(int i=0; i<16; i++)
    {
        if (c_string_handler[i] != NULL)
        {
            delete c_string_handler[i];
            c_string_handler[i] = NULL;
        }
    }
}

bool EthResource::lock(bool on)
{
    if(true == on)
        objLock->wait();
    else
        objLock->post();

    return true;
}

#if 1
bool EthResource::open2(eOipv4addr_t remIP, yarp::os::Searchable &cfgtotal)
{
    ethManager = eth::TheEthManager::instance();

    eth::parser::pc104Data pc104data;
    eth::parser::read(cfgtotal, pc104data);
//    eth::parser::print(pc104data);


    eth::parser::boardData brddata;
    eth::parser::read(cfgtotal, brddata);
//    eth::parser::print(brddata);



    // i fill remote address
    ipv4addr = remIP;
    ipv4addrstring = brddata.properties.ipv4string;
    ipv4addressing = brddata.properties.ipv4addressing;

    ethboardtype = brddata.properties.type;
    boardTypeString = brddata.properties.typestring;

    boardName = brddata.settings.name;


    eth::EthMonitorPresence::Config mpConfig;

    // default values ...
    mpConfig.enabled = brddata.actions.monitorpresence_enabled;
    mpConfig.timeout = brddata.actions.monitorpresence_timeout;
    mpConfig.periodmissingreport = brddata.actions.monitorpresence_periodofmissingreport;
    mpConfig.name = ipv4addrstring + " (" + boardName + ")";



    // now i init objects

    lock(true);

    // 1. init transceiver

    eOipv4addressing_t localIPv4 = ethManager->getLocalIPV4addressing();


    if(false == transceiver.init2(cfgtotal, localIPv4, remIP))
    {
        yError() << "EthResource::open2() cannot init transceiver w/ HostTransceiver::init2() for BOARD" << boardName << "IP" << ipv4addrstring;
        lock(false);
        return false;
    }

    // 2. init monitor presence

    monitorpresence.config(mpConfig);
    monitorpresence.tick();


    lock(false);

    return true;
}

#else
bool EthResource::open2(eOipv4addr_t remIP, yarp::os::Searchable &cfgtotal)
{
    ethManager = eth::TheEthManager::instance();

//    eth::parser::pc104Data pc104data;
//    eth::parser::read(cfgtotal, pc104data);
//    eth::parser::print(pc104data);


//    eth::parser::boardData brddata;
//    eth::parser::read(cfgtotal, brddata);
//    eth::parser::print(brddata);


    Bottle groupEthBoard  = Bottle(cfgtotal.findGroup("ETH_BOARD"));
    if(groupEthBoard.isNull())
    {
        yError() << "EthResource::open2() cannot find ETH_BOARD group in config files";
        return NULL;
    }
    Bottle groupEthBoardProps = Bottle(groupEthBoard.findGroup("ETH_BOARD_PROPERTIES"));
    if(groupEthBoardProps.isNull())
    {
        yError() << "EthResource::open2() cannot find ETH_BOARD_PROPERTIES group in config files";
        return NULL;
    }
    Bottle groupEthBoardSettings = Bottle(groupEthBoard.findGroup("ETH_BOARD_SETTINGS"));
    if(groupEthBoardSettings.isNull())
    {
        yError() << "EthResource::open2() cannot find ETH_BOARD_PROPERTIES group in config files";
        return NULL;
    }

    // i fill remote address
    ipv4addr = remIP;
    eo_common_ipv4addr_to_string(ipv4addr, ipv4addrstring, sizeof(ipv4addrstring));
    ipv4addressing.addr = remIP;
    ipv4addressing.port = 12345;


    // -> ETH_BOARD/ETH_BOARD_PROPERTIES

    // IpAddress:
    // it is already inside remIP

    // IpPort:
    if(true == groupEthBoardProps.check("IpPort"))
    {
        ipv4addressing.port = groupEthBoardProps.find("IpPort").asInt();;
    }

    // Type:
    Bottle b_ETH_BOARD_PROPERTIES_Type = groupEthBoardProps.findGroup("Type");
    ConstString Type = b_ETH_BOARD_PROPERTIES_Type.get(1).asString();
    const char *strType = Type.c_str();
    // 1. compare with the exceptions which may be in some old xml files ("EMS4", "MC4PLUS", "MC2PLUS"), and then then call proper functions
    if(0 == strcmp(strType, "EMS4"))
    {
        ethboardtype = eobrd_ethtype_ems4;
    }
    else if(0 == strcmp(strType, "MC4PLUS"))
    {
        ethboardtype = eobrd_ethtype_mc4plus;
    }
    else if(0 == strcmp(strType, "MC2PLUS"))
    {
        ethboardtype = eobrd_ethtype_mc2plus;
    }
    else
    {
        eObrd_type_t brd = eobrd_unknown;
        if(eobrd_unknown == (brd = eoboards_string2type2(strType, eobool_true)))
        {
            brd = eoboards_string2type2(strType, eobool_false);
        }

        // if not found in compact or extended string format, we accept that the board is unknown

        ethboardtype = eoboards_type2ethtype(brd);
    }
    snprintf(boardTypeString, sizeof(boardTypeString), "%s", eoboards_type2string2(eoboards_ethtype2type(ethboardtype), eobool_true));

    // maxSizeRXpacket:
    // maxSizeROP:
    // dont do it in here ...

    // <- ETH_BOARD/ETH_BOARD_PROPERTIES


    // -> ETH_BOARD/ETH_BOARD_SETTINGS

    Bottle paramNameBoard(groupEthBoardSettings.find("Name").asString());
    char xmlboardname[64] = {0};
    snprintf(xmlboardname, sizeof(xmlboardname), "%s", paramNameBoard.toString().c_str());


    if(0 != strlen(xmlboardname))
    {
        snprintf(boardName, sizeof(boardName), "%s", xmlboardname);
    }
    else
    {
        snprintf(boardName, sizeof(boardName), "NOT-NAMED");
    }


    // -> ETH_BOARD/ETH_BOARD_SETTINGS/RUNNINGMODE
    // we dont do it in here
    // <- ETH_BOARD/ETH_BOARD_SETTINGS/RUNNINGMODE


    // <- ETH_BOARD/ETH_BOARD_SETTINGS


    // -> ETH_BOARD/ETH_BOARD_ACTIONS
    // -> ETH_BOARD/ETH_BOARD_ACTIONS/MONITOR_ITS_PRESENCE
    eth::EthMonitorPresence::Config mpConfig;

    // default values ...
    mpConfig.enabled = true;
    mpConfig.timeout = 0.020;
    mpConfig.periodmissingreport = 60.0;
    mpConfig.name = std::string(ipv4addrstring) + " (" + std::string(boardName) + ")";

    // do we have a proper section ETH_BOARD_ACTIONS/MONITOR_ITS_PRESENCE? if so we change its config

    Bottle groupEthBoardActions = Bottle(groupEthBoard.findGroup("ETH_BOARD_ACTIONS"));
    if(!groupEthBoardActions.isNull())
    {

        Bottle groupEthBoardActions_Monitor = Bottle(groupEthBoardActions.findGroup("MONITOR_ITS_PRESENCE"));
        if(!groupEthBoardActions_Monitor.isNull())
        {

            Bottle groupEthBoardActions_Monitor_enabled = groupEthBoardActions_Monitor.findGroup("enabled");
            ConstString Ena = groupEthBoardActions_Monitor_enabled.get(1).asString();
            const char *strEna = Ena.c_str();

            if(0 == strcmp(strEna, "true"))
            {
                mpConfig.enabled = true;
            }

            if(true == groupEthBoardActions_Monitor.check("timeout"))
            {
                double presenceTimeout = groupEthBoardActions_Monitor.find("timeout").asDouble();

                if(presenceTimeout <= 0)
                {
                    presenceTimeout = 0;
                    mpConfig.enabled = false;
                }

                if(presenceTimeout > 0.100)
                {
                    presenceTimeout = 0.100;
                }

                mpConfig.timeout = presenceTimeout;

            }


            if(true == groupEthBoardActions_Monitor.check("periodOfMissingReport"))
            {
                double reportMissingPeriod = groupEthBoardActions_Monitor.find("periodOfMissingReport").asDouble();

                if(reportMissingPeriod <= 0)
                {
                    reportMissingPeriod = 0.0;
                }

                if(reportMissingPeriod > 600)
                {
                    reportMissingPeriod = 600;
                }

                mpConfig.periodmissingreport = reportMissingPeriod;

            }
        }
    }

    // <- ETH_BOARD/ETH_BOARD_ACTIONS/MONITOR_ITS_PRESENCE
    // <- ETH_BOARD/ETH_BOARD_ACTIONS



    // now i init objects

    lock(true);

    // 1. init transceiver

    eOipv4addressing_t localIPv4 = ethManager->getLocalIPV4addressing();

    bool ret;
    uint8_t num = 0;
    eo_common_ipv4addr_to_decimal(remIP, NULL, NULL, NULL, &num);
    if(false == transceiver.init2(groupEthBoard, localIPv4, remIP))
    {
        ret = false;
        char ipinfo[20] = {0};
        eo_common_ipv4addr_to_string(remIP, ipinfo, sizeof(ipinfo));
        yError() << "EthResource::open2() cannot init transceiver w/ HostTransceiver::init2() for BOARD" << xmlboardname << "IP" << ipinfo;
    }
    else
    {
        ret = true;
    }

    // 2. init monitor presence

    monitorpresence.config(mpConfig);
    monitorpresence.tick();


    lock(false);


    return ret;
}
#endif


bool EthResource::close()
{
    yTrace();
    return false;
}


bool EthResource::getTXpacket(uint8_t **packet, uint16_t *size, uint16_t *numofrops)
{
    return transceiver.getTransmit(packet, size, numofrops);
}


bool EthResource::Tick()
{
    monitorpresence.tick();
    return true;
}


bool EthResource::Check()
{
    if(false == regularsAreSet)
    {   // we dont comply if the regulars are not set because ... poor board: it does not regularly transmit
        return true;
    }

    return monitorpresence.check();
}



bool EthResource::canProcessRXpacket(uint64_t *data, uint16_t size)
{
    if(NULL == data)
    {
        return false;
    }

    if(size > transceiver.getCapacityOfRXpacket())
    {
        return false;
    }

    return true;
}


void EthResource::processRXpacket(uint64_t *data, uint16_t size)
{
    transceiver.onMsgReception(data, size);
}


//ACE_INET_Addr EthResource::getRemoteAddress()
//{
//    return remote_dev;
//}

eOipv4addr_t EthResource::getIPv4remoteAddress(void)
{
    return ipv4addr;
}

bool EthResource::getIPv4remoteAddressing(eOipv4addressing_t &addressing)
{
    addressing = ipv4addressing;
    return true;
}

const string& EthResource::getName(void)
{
    return boardName;
}

const string & EthResource::getIPv4string(void)
{
    return ipv4addrstring;
}

eObrd_ethtype_t EthResource::getBoardType(void)
{
    return ethboardtype;
}

const string & EthResource::getBoardTypeString(void)
{
    return boardTypeString;
}

void EthResource::getBoardInfo(eOdate_t &date, eOversion_t &version)
{
    date = boardDate;
    version = boardVersion;
}



bool EthResource::isID32supported(eOprotID32_t id32)
{
    return transceiver.isID32supported(id32);
}



bool EthResource::isRunning(void)
{
    return(isInRunningMode);
}



bool EthResource::verifyBoardTransceiver()
{
    // the transceiver is verified if we have the same mn protocol version inside eOmn_comm_status_t::managementprotocolversion

    if(verifiedBoardTransceiver)
    {
        return(true);
    }

    // we dont ask anything to the board ...
#define DONT_ASK_COMM_STATUS

#if defined(DONT_ASK_COMM_STATUS)

    const eoprot_version_t * pc104versionMN = eoprot_version_of_endpoint_get(eoprot_endpoint_management);
    const eoprot_version_t * brdversionMN = &boardMNprotocolversion;

#else

    theNVmanager& nvman = theNVmanager::getInstance();


    // step 1: we ask the remote board the eoprot_tag_mn_comm_status variable and then we verify vs mn protocol version

    const eoprot_version_t * pc104versionMN = eoprot_version_of_endpoint_get(eoprot_endpoint_management);
    const double timeout = 0.100;   // now the timeout can be reduced because the board is already connected.

    eOprotID32_t id32 = eoprot_ID_get(eoprot_endpoint_management, eoprot_entity_mn_comm, 0, eoprot_tag_mn_comm_status);
    eOmn_comm_status_t brdstatus = {0};
    uint16_t size = 0;

    bool rr = nvman.ask(ipv4addr, id32, &brdstatus, timeout);

    if(false == rr)
    {
        yError() << "EthResource::verifyBoardTransceiver() cannot read brdstatus w/ theNVmanager for BOARD" << getName() << "with IP" << getIPv4string() << ": cannot proceed any further";
        return(false);
    }

    eoprot_version_t * brdversionMN = (eoprot_version_t*)&brdstatus.managementprotocolversion;

#endif

    if(pc104versionMN->major != brdversionMN->major)
    {
        yError() << "EthResource::verifyBoardTransceiver() detected different mn protocol major versions: local =" << pc104versionMN->major << ", remote =" << brdversionMN->major << ": cannot proceed any further";
        yError() << "ACTION REQUIRED: BOARD" << getName() << "with IP" << getIPv4string() << "needs a FW update.";
        return(false);
    }

    if(pc104versionMN->minor != brdversionMN->minor)
    {
        yError() << "EthResource::verifyBoardTransceiver() detected different mn protocol minor versions: local =" << pc104versionMN->minor << ", remote =" << brdversionMN->minor << ": cannot proceed any further.";
        yError() << "ACTION REQUIRED: BOARD" << getName() << "with IP" << getIPv4string() << "needs a FW update.";
        return(false);
    }


    if(verbosewhenok)
    {
        yDebug() << "EthResource::verifyBoardTransceiver() has validated the transceiver of BOARD" << getName() << "with IP" << getIPv4string();
    }

    verifiedBoardTransceiver = true;

    return(true);
}



bool EthResource::setTimingOfRunningCycle()
{

#if defined(ETHRES_DEBUG_DONTREADBACK)
    yWarning() << "EthResource::setTimingOfRunningCycle() is in ETHRES_DEBUG_DONTREADBACK mode";
    txrateISset = true;
    return true;
#endif

    if(txrateISset)
    {
        return(true);
    }

    // step 1: we send the remote board a message of type eoprot_tag_mn_appl_config with the value read from the proper section
    //         if does find the section we use default values

    // call a set until verified

    eOprotID32_t id32 = eoprot_ID_get(eoprot_endpoint_management, eoprot_entity_mn_appl, 0, eoprot_tag_mn_appl_config);

    eOmn_appl_config_t config = {0};
    transceiver.get(config);


    theNVmanager& nvman = theNVmanager::getInstance();

    if(false == nvman.setcheck(ipv4addr, id32, &config, 5, 0.010, 2.0))
    {
        yWarning() << "EthResource::setTimingOfRunningCycle() for BOARD" << getName() << "with IP" << getIPv4string() << "could not configure: cycletime =" << config.cycletime << "usec, RX DO TX = (" << config.maxtimeRX << config.maxtimeDO << config.maxtimeTX << ") usec and TX rate =" << config.txratedivider << " every cycle";
        return false;
    }
    else
    {
        if(verbosewhenok)
        {
            yDebug() << "EthResource::setTimingOfRunningCycle() for BOARD" << getName() << "with IP" << getIPv4string() << "has succesfully set: cycletime =" << config.cycletime << "usec, RX DO TX = (" << config.maxtimeRX << config.maxtimeDO << config.maxtimeTX << ") usec and TX rate =" << config.txratedivider << " every cycle";
        }
    }

    txrateISset = true;


    return(true);
}


bool EthResource::cleanBoardBehaviour(void)
{
    if(cleanedBoardBehaviour)
    {
        return(true);
    }

    // send a ...
    if(false == serviceStop(eomn_serv_category_all))
    {
        yError() << "EthResource::cleanBoardBehaviour() cannot stop services for BOARD" << getName() << "with IP" << getIPv4string() << ": cannot proceed any further";
        return(false);
    }

    regularsAreSet = false;


    if(verbosewhenok)
    {
        yDebug() << "EthResource::cleanBoardBehaviour() has cleaned the application in BOARD" << getName() << "with IP" << getIPv4string() << ": config mode + cleared all its regulars";
    }

    cleanedBoardBehaviour = true;

    return(true);

}


bool EthResource::verifyEPprotocol(eOprot_endpoint_t ep)
{
    if((uint8_t)ep >= eoprot_endpoints_numberof)
    {
        yError() << "EthResource::verifyEPprotocol() called with wrong ep = " << ep << ": cannot proceed any further";
        return(false);
    }

    if(true == verifiedEPprotocol[ep])
    {
        return(true);
    }

    if(false == verifyBoard())
    {
        yError() << "EthResource::verifyEPprotocol() cannot verify BOARD" << getName() << "with IP" << getIPv4string() << ": cannot proceed any further";
        return(false);
    }

    if(false == askBoardVersion())
    {
        yError() << "EthResource::verifyEPprotocol() cannot ask the version to BOARD" << getName() << "with IP" << getIPv4string() << ": cannot proceed any further";
        return(false);
    }

#if defined(ETHRES_DEBUG_DONTREADBACK)
    verifiedEPprotocol[ep] =  true;
    yWarning() << "EthResource::verifyEPprotocol() is in ETHRES_DEBUG_DONTREADBACK mode";
    return true;
#endif

    // 1. send a set<eoprot_tag_mn_comm_cmmnds_command_queryarray> and wait for the arrival of a sig<eoprot_tag_mn_comm_cmmnds_command_replyarray>
    //    the opc to send is eomn_opc_query_array_EPdes which will trigger a opc in reception eomn_opc_reply_array_EPdes
    // 2. the resulting array will contains a eoprot_endpoint_descriptor_t item for the specifeid ep with the protocol version of the ems.



    const double timeout = 0.100;

    eOprotID32_t id2send = eo_prot_ID32dummy;
    eOprotID32_t id2wait = eo_prot_ID32dummy;
    eOmn_command_t command = {0};


    // step 1: ask all the EP descriptors. from them we can extract protocol version of MN and of the target ep
    id2send = eoprot_ID_get(eoprot_endpoint_management, eoprot_entity_mn_comm, 0, eoprot_tag_mn_comm_cmmnds_command_queryarray);
    memset(&command, 0, sizeof(command));
    command.cmd.opc                             = eomn_opc_query_array_EPdes;
    command.cmd.queryarray.opcpar.opc           = eomn_opc_query_array_EPdes;
    command.cmd.queryarray.opcpar.endpoint      = eoprot_endpoint_all;
    command.cmd.queryarray.opcpar.setnumber     = 0;
    command.cmd.queryarray.opcpar.setsize       = 0;

    // the semaphore must be retrieved using the id of the variable which is waited. in this case, it is the array of descriptors
    id2wait = eoprot_ID_get(eoprot_endpoint_management, eoprot_entity_mn_comm, 0, eoprot_tag_mn_comm_cmmnds_command_replyarray);

    theNVmanager& nvman = theNVmanager::getInstance();

    if(false == nvman.command(ipv4addr, id2send, &command, id2wait, &command, timeout))
    {
        yError() << "EthResource::verifyEPprotocol() retrieve the endpoint descriptors from BOARD" << getName() << "with IP" << getIPv4string() << ": cannot proceed any further";
        return(false);
    }

    // the array is ...
    eOmn_cmd_replyarray_t* cmdreplyarray = (eOmn_cmd_replyarray_t*)&command.cmd.replyarray;
    EOarray* array = (EOarray*)cmdreplyarray->array;


    uint8_t sizeofarray = eo_array_Size(array);


    for(int i=0; i<sizeofarray; i++)
    {
        eoprot_endpoint_descriptor_t *epd = (eoprot_endpoint_descriptor_t*)eo_array_At(array, i);

        if(epd->endpoint == eoprot_endpoint_management)
        {
            const eoprot_version_t * pc104versionMN = eoprot_version_of_endpoint_get(eoprot_endpoint_management);
            if(pc104versionMN->major != epd->version.major)
            {
                yError() << "EthResource::verifyEPprotocol() for ep =" << eoprot_EP2string(epd->endpoint) << "detected: pc104.version.major =" << pc104versionMN->major << "and board.version.major =" << epd->version.major;
                yError() << "EthResource::verifyEPprotocol() detected mismatching protocol version.major in BOARD" << getName() << "with IP" << getIPv4string() << "for eoprot_endpoint_management: cannot proceed any further.";
                yError() << "ACTION REQUIRED: BOARD" << getName() << "with IP" << getIPv4string() << "needs a FW update.";
                return(false);
            }
            if(pc104versionMN->minor != epd->version.minor)
            {
                yError() << "EthResource::verifyEPprotocol() for ep =" << eoprot_EP2string(epd->endpoint) << "detected: pc104.version.minor =" << pc104versionMN->minor << "and board.version.minor =" << epd->version.minor;
                yError() << "EthResource::verifyEPprotocol() detected mismatching protocol version.minor BOARD" << getName() << "with IP" << getIPv4string() << "for eoprot_endpoint_management: cannot proceed any further.";
                yError() << "ACTION REQUIRED: BOARD" << getName() << "with IP" << getIPv4string() << "needs a FW update.";
                return false;
            }
        }
        if(epd->endpoint == ep)
        {
            const eoprot_version_t * pc104versionEP = eoprot_version_of_endpoint_get(ep);
            if(pc104versionEP->major != epd->version.major)
            {
                yError() << "EthResource::verifyEPprotocol() for ep =" << eoprot_EP2string(epd->endpoint) << "detected: pc104.version.major =" << pc104versionEP->major << "and board.version.major =" << epd->version.major;
                yError() << "EthResource::verifyEPprotocol() detected mismatching protocol version.major in BOARD" << getName() << "with IP" << getIPv4string() << " for" << eoprot_EP2string(ep) << ": cannot proceed any further.";
                yError() << "ACTION REQUIRED: BOARD" << getName() << "with IP" << getIPv4string() << "needs a FW update to offer services for" << eoprot_EP2string(ep);
                return(false);
            }
            if(pc104versionEP->minor != epd->version.minor)
            {
                yError() << "EthResource::verifyEPprotocol() for ep =" << eoprot_EP2string(epd->endpoint) << "detected: pc104.version.minor =" << pc104versionEP->minor << "and board.version.minor =" << epd->version.minor;
                yError() << "EthResource::verifyEPprotocol() detected mismatching protocol version.minor in BOARD" << getName() << "with IP" << getIPv4string() << " for" << eoprot_EP2string(ep) << ": annot proceed any further";
                yError() << "ACTION REQUIRED: BOARD" << getName() << "with IP" << getIPv4string() << "needs a FW update to offer services for" << eoprot_EP2string(ep);
                return(false);
            }
        }
    }

    verifiedEPprotocol[ep] = true;

    return(true);

}



bool EthResource::verifyBoard(void)
{
    if((true == verifyBoardPresence())      &&
       (true == verifyBoardTransceiver())   &&
       (true == cleanBoardBehaviour())      &&
       (true == setTimingOfRunningCycle()) )
    {
        return(true);
    }

    return(false);
}


bool EthResource::verifyBoardPresence(void)
{
    if(verifiedBoardPresence)
    {
        return(true);
    }

    const double timeout = 1.00;    // 1 sec is more than enough if board is present. if link is not on it is a good time to wait
    const int retries = 20;         // the number of retries depends on the above timeout and on link-up time of the EMS.

    double start_time = yarp::os::Time::now();

    theNVmanager& nvman = theNVmanager::getInstance();
    verifiedBoardPresence = nvman.ping(ipv4addr, boardMNprotocolversion, timeout, retries);

    double end_time = yarp::os::Time::now();

    if(true == verifiedBoardPresence)
    {
        verifiedBoardPresence = true;
        if(verbosewhenok)
        {
            yDebug() << "EthResource::verifyBoardPresence() found BOARD" << getName() << "with IP" << getIPv4string() << "after" << end_time-start_time << "seconds";
        }
    }
    else
    {
        yError() << "EthResource::verifyBoardPresence() DID NOT have replies from BOARD" << getName() << "with IP" << getIPv4string() << "after" << end_time-start_time << "seconds: CANNOT PROCEED ANY FURTHER";
    }

    return(verifiedBoardPresence);
}


bool EthResource::askBoardVersion(void)
{

#if defined(ETHRES_DEBUG_DONTREADBACK)
    yWarning() << "EthResource::askBoardVersion() is in ETHRES_DEBUG_DONTREADBACK mode";
    askedBoardVersion =  true;
    return true;
#endif

    if(askedBoardVersion)
    {
        return(true);
    }

    const double timeout = 0.500;   // 500 ms is more than enough if board is present. if link is not on it is a good time to wait

    eOprotID32_t id32 = eoprot_ID_get(eoprot_endpoint_management, eoprot_entity_mn_appl, 0, eoprot_tag_mn_appl_status);
    eOmn_appl_status_t applstatus = {0};

    theNVmanager& nvman = theNVmanager::getInstance();

    askedBoardVersion = nvman.ask(ipv4addr, id32, &applstatus, timeout);

    if(false == askedBoardVersion)
    {
        yError() << "EthResource::askBoardVersion() cannot reach BOARD" << getName() << "with IP" << getIPv4string() << "w/ timeout of" << timeout << "seconds";
        return false;
    }


    // now i store the ....
    boardVersion.major = applstatus.version.major;
    boardVersion.minor = applstatus.version.minor;

    boardDate.year = applstatus.buildate.year;
    boardDate.month = applstatus.buildate.month;
    boardDate.day = applstatus.buildate.day;
    boardDate.hour = applstatus.buildate.hour;
    boardDate.min = applstatus.buildate.min;

    if(eobool_true == eoboards_is_eth((eObrd_type_t)applstatus.boardtype))
    {
        detectedBoardType =  (eObrd_ethtype_t) applstatus.boardtype;
    }
    else
    {
        detectedBoardType = eobrd_ethtype_unknown;
    }


    char datestr[32] = {0};
    eo_common_date_to_string(boardDate, datestr, sizeof(datestr));

    yInfo() << "EthResource::askBoardVersion() found BOARD" << getName() << "@ IP" << getIPv4string() << "of type" << eoboards_type2string2(eoboards_ethtype2type(detectedBoardType), eobool_true) << "with FW version = ("<< boardVersion.major << "," << boardVersion.minor << ") and build date" << datestr;


    return(askedBoardVersion);
}


bool EthResource::getRemoteValue(const eOprotID32_t id32, void *value, const double timeout, const unsigned int retries)
{
#if defined(ETHRES_DEBUG_DONTREADBACK)
    yWarning() << "EthResource::getRemoteValue() is in ETHRES_DEBUG_DONTREADBACK mode, thus it does not verify";
    return true;
#endif

    bool replied = false;

    double start_time = yarp::os::Time::now();

    theNVmanager& nvman = theNVmanager::getInstance();

    for(unsigned int numOfattempts=0; numOfattempts<(retries+1); numOfattempts++)
    {
        if(true == nvman.ask(this, id32, value, timeout))
        {
            replied = true;
            // stop attempts
            break;
        }

        if(!replied)
        {
            yWarning() << "EthResource::getRemoteValue() cannot have a reply from BOARD" << getName() << "with IP" << getIPv4string() << "at attempt #" << numOfattempts+1 << "w/ timeout of" << timeout << "seconds";
        }

    }

    double end_time = yarp::os::Time::now();

    if(false == replied)
    {
        yError() << "  FATAL: EthResource::getRemoteValue() DID NOT have replies from BOARD" << getName() << "with IP" << getIPv4string() << " even after" << end_time-start_time << "seconds: CANNOT PROCEED ANY FURTHER";
    }

    return replied;
}


bool EthResource::setRemoteValue(const eOprotID32_t id32, void *value)
{
    theNVmanager& nvman = theNVmanager::getInstance();
    return nvman.set(ipv4addr, id32, value);
}

bool EthResource::setcheckRemoteValue(const eOprotID32_t id32, void *value, const unsigned int retries, const double waitbeforecheck, const double timeout)
{
    theNVmanager& nvman = theNVmanager::getInstance();
    return nvman.setcheck(ipv4addr, id32, value, retries, waitbeforecheck, timeout);
}

bool EthResource::CANPrintHandler(eOmn_info_basic_t *infobasic)
{
    char str[256];
    char canfullmessage[64];

    static const char * sourcestrings[] =
    {
        "LOCAL",
        "CAN1",
        "CAN2",
        "UNKNOWN"
    };
    int source                      = EOMN_INFO_PROPERTIES_FLAGS_get_source(infobasic->properties.flags);
    const char * str_source         = (source > eomn_info_source_can2) ? (sourcestrings[3]) : (sourcestrings[source]);
    uint16_t address                = EOMN_INFO_PROPERTIES_FLAGS_get_address(infobasic->properties.flags);
    uint8_t *p64 = (uint8_t*)&(infobasic->properties.par64);

    int msg_id = (p64[1]&0xF0) >> 4;

    uint32_t sec = infobasic->timestamp / 1000000;
    uint32_t msec = (infobasic->timestamp % 1000000) / 1000;
    uint32_t usec = infobasic->timestamp % 1000;

    const char *boardstr = boardName.c_str();

    // Validity check
    if(address > 15)
    {
        snprintf(canfullmessage,sizeof(canfullmessage),"Error while parsing the message: CAN address detected is out of allowed range");
        snprintf(str,sizeof(str), "from BOARD %s (%s), src %s, adr %d, time %ds %dm %du: CAN PRINT MESSAGE[id %d] -> %s",
                                    ipv4addrstring.c_str(),
                                    boardstr,
                                    str_source,
                                    address,
                                    sec,
                                    msec,
                                    usec,
                                    msg_id,
                                    canfullmessage
                                    );
        feat_PrintError(str);
    }
    else
    {
        // Initialization needed?
        if (c_string_handler[address] == NULL)
            c_string_handler[address] = new can_string_eth();

        CanFrame can_msg;
        can_msg.setCanData(infobasic->properties.par64);
        can_msg.setId(msg_id);
        can_msg.setSize(infobasic->properties.par16);
        int ret = c_string_handler[address]->add_string(&can_msg);

        // String finished?
        if (ret != -1)
        {
            char* themsg = c_string_handler[address]->get_string(ret);
            memcpy(canfullmessage, themsg, sizeof(canfullmessage));
            canfullmessage[63] = 0;
            c_string_handler[address]->clear_string(ret);

            snprintf(str,sizeof(str), "from BOARD %s (%s), src %s, adr %d, time %ds %dm %du: CAN PRINT MESSAGE[id %d] -> %s",
                                        ipv4addrstring.c_str(),
                                        boardstr,
                                        str_source,
                                        address,
                                        sec,
                                        msec,
                                        usec,
                                        msg_id,
                                        canfullmessage
                                        );
            feat_PrintInfo(str);
        }
    }
    return true;
}


bool EthResource::serviceCommand(eOmn_serv_operation_t operation, eOmn_serv_category_t category, const eOmn_serv_parameter_t* param, double timeout, int times)
{
#if defined(ETHRES_DEBUG_DONTREADBACK)
    yWarning() << "EthResource::serviceCommand() is in ETHRES_DEBUG_DONTREADBACK mode, thus it does not send the command";
    return true;
#endif


    eOprotID32_t id2send = eoprot_ID_get(eoprot_endpoint_management, eoprot_entity_mn_service, 0, eoprot_tag_mn_service_cmmnds_command);
    eOprotID32_t id2wait = eoprot_ID_get(eoprot_endpoint_management, eoprot_entity_mn_service, 0, eoprot_tag_mn_service_status_commandresult);;

    eOmn_service_cmmnds_command_t command = {0};
    eOmn_service_command_result_t result = {0};

    command.operation = operation;
    command.category = category;
    if(NULL != param)
    {
        memcpy(&command.parameter, param, sizeof(eOmn_serv_parameter_t));
    }
    else
    {
       memset(&command.parameter, 0, sizeof(eOmn_serv_parameter_t));
       if((eomn_serv_operation_regsig_load == operation) || ((eomn_serv_operation_regsig_clear == operation)))
       {    // we send an empty array
            eo_array_New(eOmn_serv_capacity_arrayof_id32, 4, &command.parameter.arrayofid32);
       }
       else
       {
            command.parameter.configuration.type = eomn_serv_NONE;
       }
    }

    theNVmanager& nvman = theNVmanager::getInstance();


    bool replied = false;
    for(int i=0; i<times; i++)
    {
        if(true == nvman.command(ipv4addr, id2send, &command, id2wait, &result, timeout))
        {
            replied = true;
            break;
        }
    }

    if(false == replied)
    {
        yError() << "EthResource::serviceCommand() failed an acked activation request to BOARD" << getName() << "with IP" << getIPv4string() << "after" << times << "attempts" << "each with waiting timeout of" << timeout << "seconds";
        return false;
    }

    yDebug() << "result is:" << result.latestcommandisok;

    return(result.latestcommandisok);
}


bool EthResource::serviceVerifyActivate(eOmn_serv_category_t category, const eOmn_serv_parameter_t* param, double timeout)
{
    return(serviceCommand(eomn_serv_operation_verifyactivate, category, param, timeout, 3));
}


bool EthResource::serviceSetRegulars(eOmn_serv_category_t category, vector<eOprotID32_t> &id32vector, double timeout)
{
    eOmn_serv_parameter_t param = {0};
    EOarray *array = eo_array_New(eOmn_serv_capacity_arrayof_id32, 4, &param.arrayofid32);
    for(int i=0; i<id32vector.size(); i++)
    {
        eOprotID32_t id32 = id32vector.at(i);
        eo_array_PushBack(array, &id32);
    }

    regularsAreSet = serviceCommand(eomn_serv_operation_regsig_load, category, &param, timeout, 3);

    return regularsAreSet;
}



bool EthResource::serviceStart(eOmn_serv_category_t category, double timeout)
{
    bool ret = serviceCommand(eomn_serv_operation_start, category, NULL, timeout, 3);

    if(ret)
    {
        isInRunningMode = true;
    }

    return ret;
}


bool EthResource::serviceStop(eOmn_serv_category_t category, double timeout)
{
    bool ret = serviceCommand(eomn_serv_operation_stop, category, NULL, timeout, 3);

    if(ret && (category == eomn_serv_category_all))
    {
        regularsAreSet = false;
    }

    //#warning TODO: the result for command stop shall also tell if the the board is in running mode or not.
    return ret;
}


// new methods from host transceiver

bool EthResource::readBufferedValue(eOprotID32_t id32,  uint8_t *data, uint16_t* size)
{
    return transceiver.readBufferedValue(id32, data, size);
}

bool EthResource::addSetMessage(eOprotID32_t id32, uint8_t* data)
{
    return transceiver.addSetROP(id32, data);
}

bool EthResource::addSetMessageAndCacheLocally(eOprotID32_t id32, uint8_t* data)
{
    return transceiver.addSetROPandCacheLocally(id32, data);
}

bool EthResource::addGetMessage(eOprotID32_t id32)
{
    return transceiver.addGetROP(id32);
}

bool EthResource::addGetMessage(eOprotID32_t id32, std::uint32_t signature)
{
    return transceiver.addGetROPwithSignature(id32, signature);
}

EOnv* EthResource::getNVhandler(eOprotID32_t id32, EOnv* nv)
{
    return transceiver.getnvhandler(id32, nv);
}

bool EthResource::readSentValue(eOprotID32_t id32, uint8_t *data, uint16_t* size)
{
    return transceiver.readSentValue(id32, data, size);
}

bool EthResource::isFake()
{
    return false;
}

// eof








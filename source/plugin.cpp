// autor: Leonardo Seiji Oyama
// contato: leonardooyama@gmail.com

#define PI 3.1415926535897932384626433832795

//XPLM
#include "XPLMCamera.h"
#include "XPLMDataAccess.h"
#include "XPLMDefs.h"
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMMenus.h"
#include "XPLMNavigation.h"
#include "XPLMPlanes.h"
#include "XPLMPlugin.h"
#include "XPLMProcessing.h"
#include "XPLMScenery.h"
#include "XPLMUtilities.h"

//Widgets
#include "XPStandardWidgets.h"
#include "XPUIGraphics.h"
#include "XPWidgetDefs.h"
#include "XPWidgets.h"
#include "XPWidgetUtils.h"

//Wrappers
#include "XPCBroadcaster.h"
#include "XPCDisplay.h"
#include "XPCListener.h"
#include "XPCProcessing.h"
#include "XPCWidget.h"
#include "XPCWidgetAttachments.h"

#if IBM
#include <windows.h>
#endif

#include <QUdpSocket>
#include <QDateTime>
#include <QNetworkDatagram>
#include <QString>


XPLMDataRef DataRefFlightModelLat;
XPLMDataRef DataRefFlightModelLon;
XPLMDataRef DataRefFlightModelElev;

double flightModelLat, flightModelLon, flightModelElev;

float FlightLoopSendUDPDatagram(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon);
float FlightLoopCheckDatagramReceived(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon);

XPLMWindowID	gWindow = NULL;

std::string pluginName = "CustomIO_UDP";
std::string pluginSignature = "coter.CustomIO_UDP";
std::string pluginDescription = "Plugin to add custom network communication to X-Plane, performing input and ouput of data.";

QUdpSocket *UDPSocketMainSender;
QUdpSocket *UDPSocketSecondarySender;
QUdpSocket *UDPSocketReceiver;

void DebugToXPlaneLog(QString debugString);

void MyDrawWindowCallback(
        XPLMWindowID         inWindowID,
        void *               inRefcon);

void MyHandleKeyCallback(
        XPLMWindowID         inWindowID,
        char                 inKey,
        XPLMKeyFlags         inFlags,
        char                 inVirtualKey,
        void *               inRefcon,
        int                  losingFocus);

int MyHandleMouseClickCallback(
        XPLMWindowID         inWindowID,
        int                  x,
        int                  y,
        XPLMMouseStatus      inMouse,
        void *               inRefcon);

PLUGIN_API int XPluginStart(
        char *	outName,
        char *	outSig,
        char *	outDesc)
{
    strcpy_s(outName, pluginName.size() + 1, pluginName.c_str());
    strcpy_s(outSig, pluginSignature.size() + 1, pluginSignature.c_str());
    strcpy_s(outDesc, pluginDescription.size() + 1, pluginDescription.c_str());

    XPLMRegisterFlightLoopCallback(FlightLoopSendUDPDatagram, xplm_FlightLoop_Phase_AfterFlightModel, NULL);
    XPLMRegisterFlightLoopCallback(FlightLoopCheckDatagramReceived, xplm_FlightLoop_Phase_AfterFlightModel, NULL);

    DataRefFlightModelLat = XPLMFindDataRef("sim/flightmodel/position/latitude");
    DataRefFlightModelLon = XPLMFindDataRef("sim/flightmodel/position/longitude");
    DataRefFlightModelElev = XPLMFindDataRef("sim/flightmodel/position/elevation");
    UDPSocketMainSender = new QUdpSocket();
    UDPSocketSecondarySender = new QUdpSocket();
    UDPSocketReceiver = new QUdpSocket();
    bool bindSuccess = UDPSocketReceiver->bind(QHostAddress::AnyIPv4, 15000, QAbstractSocket::ReuseAddressHint);
    QString debugString;
    if (bindSuccess)
    {
        debugString = "Success binding UDP port!";
        DebugToXPlaneLog(debugString);
    }
    else
    {
        debugString = "Failed binding UDP port!";
        DebugToXPlaneLog(debugString);
    }
    return 1;
}


PLUGIN_API void	XPluginStop(void)
{

}

PLUGIN_API void XPluginDisable(void)
{
}

PLUGIN_API int XPluginEnable(void)
{
    return 1;
}

PLUGIN_API void XPluginReceiveMessage(
        XPLMPluginID	inFromWho,
        long			inMessage,
        void *			inParam)
{
}

float FlightLoopSendUDPDatagram(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon)
{
    // get updated flight model data
    flightModelLat = XPLMGetDatad(DataRefFlightModelLat);
    flightModelLon = XPLMGetDatad(DataRefFlightModelLon);
    flightModelElev = XPLMGetDatad(DataRefFlightModelElev);

    QByteArray dataToSend;
    dataToSend.clear();
    QString separator = "; ";
    dataToSend.append(QString::number(flightModelLat, 'f', 6).toUtf8());
    dataToSend.append(separator.toUtf8());
    dataToSend.append(QString::number(flightModelLon, 'f', 6).toUtf8());
    dataToSend.append(separator.toUtf8());
    dataToSend.append(QString::number(flightModelElev, 'f', 6).toUtf8());
    dataToSend.append(separator.toUtf8());
    QNetworkDatagram datagram(dataToSend,QHostAddress::Broadcast,10000);
    UDPSocketMainSender->writeDatagram(datagram);
    return 5.0;
}

float FlightLoopCheckDatagramReceived(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon)
{
    if (UDPSocketReceiver->hasPendingDatagrams())
    {
        DebugToXPlaneLog("Got some data from UDPSocketReceiver!");
        QNetworkDatagram receivedDatagram = UDPSocketReceiver->receiveDatagram();
        QNetworkDatagram datagramToSend;
        datagramToSend.setDestination(QHostAddress::Broadcast, 12000);
        datagramToSend.setData(receivedDatagram.data());
        UDPSocketSecondarySender->writeDatagram(datagramToSend);
    }
    return -1;
}

void DebugToXPlaneLog(QString debugString)
{
    QString dbg;
    dbg = QDateTime::currentDateTime().toString("dd-MM-yyyy, hh'h' mm'min' ss's': ");
    dbg+= "[" + QString::fromStdString(pluginSignature) + "]: ";
    dbg+= debugString + "\n";
    XPLMDebugString(dbg.toStdString().c_str());
}

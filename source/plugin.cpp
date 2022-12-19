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
XPLMDataRef DataRefFlightModelTrueTheta;
XPLMDataRef DataRefFlightModelTruePhi;
XPLMDataRef DataRefFlightModelTruePsi;
XPLMDataRef DataRefFlightModelMagPsi;
XPLMDataRef DataRefFlightModelLocalVx;
XPLMDataRef DataRefFlightModelLocalVy;
XPLMDataRef DataRefFlightModelLocalVz;
XPLMDataRef DataRefFlightModelLocalAx;
XPLMDataRef DataRefFlightModelLocalAy;
XPLMDataRef DataRefFlightModelLocalAz;

double flightModelLat, flightModelLon, flightModelElev;
float flightModelTrueTheta, flightModelTruePhi, flightModelTruePsi, flightModelMagPsi;
float flightModelLocalVx, flightModelLocalVy, flightModelLocalVz;
float flightModelLocalAx, flightModelLocalAy, flightModelLocalAz;

float FlightLoopSendUDPDatagram(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon);
float FlightLoopCheckDatagramReceived(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon);

XPLMWindowID	gWindow = NULL;

QUdpSocket *UDPSocketMainSender;
QUdpSocket *UDPSocketSecondarySender;
QUdpSocket *UDPSocketReceiver;

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
    strcpy(outName, "CustomIO_UDP");
    strcpy(outSig, "coter.CustomIO_UDP");
    strcpy(outDesc, "Plugin to add custom network communication to X-Plane, performing input and ouput of data.");

    XPLMRegisterFlightLoopCallback(FlightLoopSendUDPDatagram, xplm_FlightLoop_Phase_AfterFlightModel, NULL);
    XPLMRegisterFlightLoopCallback(FlightLoopCheckDatagramReceived, xplm_FlightLoop_Phase_AfterFlightModel, NULL);

    DataRefFlightModelLat = XPLMFindDataRef("sim/flightmodel/position/latitude");
    DataRefFlightModelLon = XPLMFindDataRef("sim/flightmodel/position/longitude");
    DataRefFlightModelElev = XPLMFindDataRef("sim/flightmodel/position/elevation");
    DataRefFlightModelTrueTheta = XPLMFindDataRef("sim/flightmodel/position/true_theta");
    DataRefFlightModelTruePhi = XPLMFindDataRef("sim/flightmodel/position/true_phi");
    DataRefFlightModelTruePsi = XPLMFindDataRef("sim/flightmodel/position/true_psi");
    DataRefFlightModelMagPsi = XPLMFindDataRef("sim/flightmodel/position/mag_psi");
    DataRefFlightModelLocalVx = XPLMFindDataRef("sim/flightmodel/position/local_vx");
    DataRefFlightModelLocalVy = XPLMFindDataRef("sim/flightmodel/position/local_vy");
    DataRefFlightModelLocalVz = XPLMFindDataRef("sim/flightmodel/position/local_vz");
    DataRefFlightModelLocalAx = XPLMFindDataRef("sim/flightmodel/position/local_ax");
    DataRefFlightModelLocalAy = XPLMFindDataRef("sim/flightmodel/position/local_ay");
    DataRefFlightModelLocalAz = XPLMFindDataRef("sim/flightmodel/position/local_az");
    UDPSocketMainSender = new QUdpSocket();
    UDPSocketSecondarySender = new QUdpSocket();
    UDPSocketReceiver = new QUdpSocket();
    bool bindSuccess = UDPSocketReceiver->bind(QHostAddress::AnyIPv4, 15000, QAbstractSocket::ReuseAddressHint);
    if (bindSuccess)
    {
        XPLMDebugString("Success binding UDP port!\n");
    }
    else
    {
        XPLMDebugString("Failed binding UDP port!\n");
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

    flightModelTrueTheta = XPLMGetDataf(DataRefFlightModelTrueTheta);
    flightModelTruePhi = XPLMGetDataf(DataRefFlightModelTruePhi);
    flightModelTruePsi = XPLMGetDataf(DataRefFlightModelTruePsi);
    flightModelMagPsi = XPLMGetDataf(DataRefFlightModelMagPsi);

    flightModelLocalVx = XPLMGetDataf(DataRefFlightModelLocalVx);
    flightModelLocalVy = XPLMGetDataf(DataRefFlightModelLocalVy);
    flightModelLocalVz = XPLMGetDataf(DataRefFlightModelLocalVz);

    flightModelLocalAx = XPLMGetDataf(DataRefFlightModelLocalAx);
    flightModelLocalAy = XPLMGetDataf(DataRefFlightModelLocalAy);
    flightModelLocalAz = XPLMGetDataf(DataRefFlightModelLocalAz);

    QByteArray dataToSend;
    dataToSend.clear();
    //    dataToSend.append((const char*)&flightModelLat);
    //    dataToSend.append((const char*)&flightModelLon);
    //    dataToSend.append((const char*)&flightModelElev);
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
        XPLMDebugString("Got some data from UDPSocketReceiver!\n");
        QNetworkDatagram receivedDatagram = UDPSocketReceiver->receiveDatagram();
        QNetworkDatagram datagramToSend;
        datagramToSend.setDestination(QHostAddress::Broadcast, 12000);
        datagramToSend.setData(receivedDatagram.data());
        UDPSocketSecondarySender->writeDatagram(datagramToSend);
    }
    return -1;
}

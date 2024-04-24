/*
 * osd.h: Frontend Status Monitor plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __FEMON_OSD_H
#define __FEMON_OSD_H

#include <sys/time.h>
#include <vdr/osd.h>
#include <vdr/thread.h>
#include <vdr/status.h>
#include <vdr/plugin.h>
#include <vdr/channels.h>
#include <vdr/transfer.h>
#include <vdr/tools.h>

#include "receiver.h"
#include "svdrpservice.h"

#define MAX_BM_NUMBER 8

class cFemonOsd : public cOsdObject, public cThread, public cStatus {
private:
  enum eDeviceSourceType {
    DEVICESOURCE_DVBAPI = 0,
    DEVICESOURCE_IPTV,
    DEVICESOURCE_PVRINPUT,
    DEVICESOURCE_COUNT
    };

  static cFemonOsd *pInstanceS;

  cOsd             *osdM;
  cFemonReceiver   *receiverM;
  int               svdrpFrontendM;
  double            svdrpVideoBitRateM;
  double            svdrpAudioBitRateM;
  SvdrpConnection_v1_0 svdrpConnectionM;
  cPlugin          *svdrpPluginM;
  int               numberM;
  int               oldNumberM;
  int               qualityM;
  bool              qualityValidM;
  int               strengthM;
  bool              strengthValidM;
  double            cnrM;
  bool              cnrValidM;
  double            signalM;
  bool              signalValidM;
  double            berM;
  bool              berValidM;
  double            perM;
  bool              perValidM;
  cString           frontendNameM;
  cString           frontendTypeM;
  int               frontendStatusM;
  bool              frontendStatusValidM;
  dvb_frontend_info frontendInfoM;
  eDeviceSourceType deviceSourceM;
  int               displayModeM;
  int               osdWidthM;
  int               osdHeightM;
  int               osdLeftM;
  int               osdTopM;
  cFont            *fontM;
  cTimeMs           inputTimeM;
  cCondWait         sleepM;
  cMutex            mutexM;

  bool AttachFrontend(void);
  void DrawStatusWindow(void);
  void DrawInfoWindow(void);
  bool SvdrpConnect(void);
  bool SvdrpTune(void);

protected:
  cFemonOsd();
  cFemonOsd(const cFemonOsd&);
  cFemonOsd& operator= (const cFemonOsd&);
  virtual void Action(void);
  virtual void ChannelSwitch(const cDevice *deviceP, int channelNumberP, bool liveViewP);
  virtual void SetAudioTrack(int indexP, const char * const *tracksP);

public:
  static cFemonOsd *Instance(bool createP = false);
  ~cFemonOsd();

  virtual void Show(void);
  virtual eOSState ProcessKey(eKeys keyP);

  bool    DeviceSwitch(int directionP);
  double  GetVideoBitrate(void);
  double  GetAudioBitrate(void);
  double  GetDolbyBitrate(void);
};

#endif //__FEMON_OSD_H


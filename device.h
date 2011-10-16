/*
 * device.h: The basic device interface
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: device.h 2.28 2011/10/16 13:27:23 kls Exp $
 */

#ifndef __DEVICE_H
#define __DEVICE_H

#include "channels.h"
#include "ci.h"
#include "dvbsubtitle.h"
#include "eit.h"
#include "filter.h"
#include "nit.h"
#include "pat.h"
#include "remux.h"
#include "ringbuffer.h"
#include "sdt.h"
#include "sections.h"
#include "spu.h"
#include "thread.h"
#include "tools.h"

#define MAXDEVICES         16 // the maximum number of devices in the system
#define MAXPIDHANDLES      64 // the maximum number of different PIDs per device
#define MAXRECEIVERS       16 // the maximum number of receivers per device
#define MAXVOLUME         255
#define VOLUMEDELTA         5 // used to increase/decrease the volume
#define MAXOCCUPIEDTIMEOUT 99 // max. time (in seconds) a device may be occupied

enum eSetChannelResult { scrOk, scrNotAvailable, scrNoTransfer, scrFailed };

enum ePlayMode { pmNone,           // audio/video from decoder
                 pmAudioVideo,     // audio/video from player
                 pmAudioOnly,      // audio only from player, video from decoder
                 pmAudioOnlyBlack, // audio only from player, no video (black screen)
                 pmVideoOnly,      // video only from player, audio from decoder
                 pmExtern_THIS_SHOULD_BE_AVOIDED
                 // external player (e.g. MPlayer), release the device
                 // WARNING: USE THIS MODE ONLY AS A LAST RESORT, IF YOU
                 // ABSOLUTELY, POSITIVELY CAN'T IMPLEMENT YOUR PLAYER
                 // THE WAY IT IS SUPPOSED TO WORK. FORCING THE DEVICE
                 // TO RELEASE ITS FILES HANDLES (OR WHATEVER RESOURCES
                 // IT MAY USE) TO ALLOW AN EXTERNAL PLAYER TO ACCESS
                 // THEM MEANS THAT SUCH A PLAYER WILL NEED TO HAVE
                 // DETAILED KNOWLEDGE ABOUT THE INTERNALS OF THE DEVICE
                 // IN USE. AS A CONSEQUENCE, YOUR PLAYER MAY NOT WORK
                 // IF A PARTICULAR VDR INSTALLATION USES A DEVICE NOT
                 // KNOWN TO YOUR PLAYER.
               };

enum eVideoSystem { vsPAL,
                    vsNTSC
                  };

enum eVideoDisplayFormat { vdfPanAndScan,
                           vdfLetterBox,
                           vdfCenterCutOut
                         };

enum eTrackType { ttNone,
                  ttAudio,
                  ttAudioFirst = ttAudio,
                  ttAudioLast  = ttAudioFirst + 31, // MAXAPIDS - 1
                  ttDolby,
                  ttDolbyFirst = ttDolby,
                  ttDolbyLast  = ttDolbyFirst + 15, // MAXDPIDS - 1
                  ttSubtitle,
                  ttSubtitleFirst = ttSubtitle,
                  ttSubtitleLast  = ttSubtitleFirst + 31, // MAXSPIDS - 1
                  ttMaxTrackTypes
                };

#define IS_AUDIO_TRACK(t) (ttAudioFirst <= (t) && (t) <= ttAudioLast)
#define IS_DOLBY_TRACK(t) (ttDolbyFirst <= (t) && (t) <= ttDolbyLast)
#define IS_SUBTITLE_TRACK(t) (ttSubtitleFirst <= (t) && (t) <= ttSubtitleLast)

struct tTrackId {
  uint16_t id;                  // The PES packet id or the PID.
  char language[MAXLANGCODE2];  // something like either "eng" or "deu+eng"
  char description[32];         // something like "Dolby Digital 5.1"
  };

class cPlayer;
class cReceiver;
class cLiveSubtitle;

class cDeviceHook : public cListObject {
public:
  cDeviceHook(void);
          ///< Creates a new device hook object.
          ///< Do not delete this object - it will be automatically deleted when the
          ///< program ends.
  virtual bool DeviceProvidesTransponder(const cDevice *Device, const cChannel *Channel) const;
          ///< Returns true if the given Device can provide the given Channel's transponder.
  };

/// The cDevice class is the base from which actual devices can be derived.

class cDevice : public cThread {
  friend class cLiveSubtitle;
  friend class cDeviceHook;
private:
  static int numDevices;
  static int useDevice;
  static cDevice *device[MAXDEVICES];
  static cDevice *primaryDevice;
  static cDevice *avoidDevice;
public:
  static int NumDevices(void) { return numDevices; }
         ///< Returns the total number of devices.
  static bool WaitForAllDevicesReady(int Timeout = 0);
         ///< Waits until all devices have become ready, or the given Timeout
         ///< (seconds) has expired. While waiting, the Ready() function of each
         ///< device is called in turn, until they all return true.
         ///< \return True if all devices have become ready within the given
         ///< timeout.
  static void SetUseDevice(int n);
         ///< Sets the 'useDevice' flag of the given device.
         ///< If this function is not called before initializing, all devices
         ///< will be used.
  static bool UseDevice(int n) { return useDevice == 0 || (useDevice & (1 << n)) != 0; }
         ///< Tells whether the device with the given card index shall be used in
         ///< this instance of VDR.
  static bool SetPrimaryDevice(int n);
         ///< Sets the primary device to 'n'.
         ///< \param n must be in the range 1...numDevices.
         ///< \return true if this was possible.
  static cDevice *PrimaryDevice(void) { return primaryDevice; }
         ///< Returns the primary device.
  static cDevice *ActualDevice(void);
         ///< Returns the actual receiving device in case of Transfer Mode, or the
         ///< primary device otherwise.
  static cDevice *GetDevice(int Index);
         ///< Gets the device with the given Index.
         ///< \param Index must be in the range 0..numDevices-1.
         ///< \return A pointer to the device, or NULL if the Index was invalid.
  static cDevice *GetDevice(const cChannel *Channel, int Priority, bool LiveView);
         ///< Returns a device that is able to receive the given Channel at the
         ///< given Priority, with the least impact on active recordings and
         ///< live viewing. The LiveView parameter tells whether the device will
         ///< be used for live viewing or a recording.
         ///< If the Channel is encrypted, a CAM slot that claims to be able to
         ///< decrypt the channel is automatically selected and assigned to the
         ///< returned device. Whether or not this combination of device and CAM
         ///< slot is actually able to decrypt the channel can only be determined
         ///< by checking the "scrambling control" bits of the received TS packets.
         ///< The Action() function automatically does this and takes care that
         ///< after detaching any receivers because the channel can't be decrypted,
         ///< this device/CAM combination will be skipped in the next call to
         ///< GetDevice().
         ///< See also ProvidesChannel().
  static void SetAvoidDevice(cDevice *Device) { avoidDevice = Device; }
         ///< Sets the given Device to be temporarily avoided in the next call to
         ///< GetDevice(const cChannel, int, bool).
  static void Shutdown(void);
         ///< Closes down all devices.
         ///< Must be called at the end of the program.
private:
  static int nextCardIndex;
  int cardIndex;
protected:
  cDevice(void);
  virtual ~cDevice();
  virtual bool Ready(void);
         ///< Returns true if this device is ready. Devices with conditional
         ///< access hardware may need some time until they are up and running.
         ///< This function is called in a loop at startup until all devices
         ///< are ready (see WaitForAllDevicesReady()).
  static int NextCardIndex(int n = 0);
         ///< Calculates the next card index.
         ///< Each device in a given machine must have a unique card index, which
         ///< will be used to identify the device for assigning Ca parameters and
         ///< deciding whether to actually use that device in this particular
         ///< instance of VDR. Every time a new cDevice is created, it will be
         ///< given the current nextCardIndex, and then nextCardIndex will be
         ///< automatically incremented by 1. A derived class can determine whether
         ///< a given device shall be used by checking UseDevice(NextCardIndex()).
         ///< If a device is skipped, or if there are possible device indexes left
         ///< after a derived class has set up all its devices, NextCardIndex(n)
         ///< must be called, where n is the number of card indexes to skip.
  virtual void MakePrimaryDevice(bool On);
         ///< Informs a device that it will be the primary device. If there is
         ///< anything the device needs to set up when it becomes the primary
         ///< device (On = true) or to shut down when it no longer is the primary
         ///< device (On = false), it should do so in this function.
         ///< A derived class must call the MakePrimaryDevice() function of its
         ///< base class.
public:
  bool IsPrimaryDevice(void) const { return this == primaryDevice; }
  int CardIndex(void) const { return cardIndex; }
         ///< Returns the card index of this device (0 ... MAXDEVICES - 1).
  int DeviceNumber(void) const;
         ///< Returns the number of this device (0 ... numDevices).
  virtual bool HasDecoder(void) const;
         ///< Tells whether this device has an MPEG decoder.
  virtual bool AvoidRecording(void) const { return false; }
         ///< Returns true if this device should only be used for recording
         ///< if no other device is available.

// Device hooks

private:
  static cList<cDeviceHook> deviceHooks;
protected:
  bool DeviceHooksProvidesTransponder(const cChannel *Channel) const;

// SPU facilities

private:
  cLiveSubtitle *liveSubtitle;
  cDvbSubtitleConverter *dvbSubtitleConverter;
public:
  virtual cSpuDecoder *GetSpuDecoder(void);
         ///< Returns a pointer to the device's SPU decoder (or NULL, if this
         ///< device doesn't have an SPU decoder).

// Channel facilities

private:
  time_t occupiedTimeout;
protected:
  static int currentChannel;
public:
  virtual bool ProvidesSource(int Source) const;
         ///< Returns true if this device can provide the given source.
  virtual bool ProvidesTransponder(const cChannel *Channel) const;
         ///< Returns true if this device can provide the transponder of the
         ///< given Channel (which implies that it can provide the Channel's
         ///< source).
  virtual bool ProvidesTransponderExclusively(const cChannel *Channel) const;
         ///< Returns true if this is the only device that is able to provide
         ///< the given channel's transponder.
  virtual bool ProvidesChannel(const cChannel *Channel, int Priority = -1, bool *NeedsDetachReceivers = NULL) const;
         ///< Returns true if this device can provide the given channel.
         ///< In case the device has cReceivers attached to it or it is the primary
         ///< device, Priority is used to decide whether the caller's request can
         ///< be honored.
         ///< The special Priority value -1 will tell the caller whether this device
         ///< is principally able to provide the given Channel, regardless of any
         ///< attached cReceivers.
         ///< If NeedsDetachReceivers is given, the resulting value in it will tell the
         ///< caller whether or not it will have to detach any currently attached
         ///< receivers from this device before calling SwitchChannel. Note
         ///< that the return value in NeedsDetachReceivers is only meaningful if the
         ///< function itself actually returns true.
         ///< The default implementation always returns false, so a derived cDevice
         ///< class that can provide channels must implement this function.
  virtual bool ProvidesEIT(void) const;
         ///< Returns true if this device provides EIT data and thus wants to be tuned
         ///< to the channels it can receive regularly to update the data.
         ///< The default implementation returns false.
  virtual int NumProvidedSystems(void) const;
         ///< Returns the number of individual "delivery systems" this device provides.
         ///< The default implementation returns 0, so any derived class that can
         ///< actually provide channels must implement this function.
         ///< The result of this function is used when selecting a device, in order
         ///< to avoid devices that provide more than one system.
  virtual int SignalStrength(void) const;
         ///< Returns the "strength" of the currently received signal.
         ///< This is a value in the range 0 (no signal at all) through
         ///< 100 (best possible signal). A value of -1 indicates that this
         ///< device has no concept of a "signal strength".
  virtual int SignalQuality(void) const;
         ///< Returns the "quality" of the currently received signal.
         ///< This is a value in the range 0 (worst quality) through
         ///< 100 (best possible quality). A value of -1 indicates that this
         ///< device has no concept of a "signal quality".
  virtual const cChannel *GetCurrentlyTunedTransponder(void) const;
         ///< Returns a pointer to the currently tuned transponder.
         ///< This is not one of the channels in the global cChannels list, but rather
         ///< a local copy. The result may be NULL if the device is not tuned to any
         ///< transponder.
  virtual bool IsTunedToTransponder(const cChannel *Channel);
         ///< Returns true if this device is currently tuned to the given Channel's
         ///< transponder.
  virtual bool MaySwitchTransponder(void);
         ///< Returns true if it is ok to switch the transponder on this device,
         ///< without disturbing any other activities.
  bool SwitchChannel(const cChannel *Channel, bool LiveView);
         ///< Switches the device to the given Channel, initiating transfer mode
         ///< if necessary.
  static bool SwitchChannel(int Direction);
         ///< Switches the primary device to the next available channel in the given
         ///< Direction (only the sign of Direction is evaluated, positive values
         ///< switch to higher channel numbers).
private:
  eSetChannelResult SetChannel(const cChannel *Channel, bool LiveView);
         ///< Sets the device to the given channel (general setup).
protected:
  virtual bool SetChannelDevice(const cChannel *Channel, bool LiveView);
         ///< Sets the device to the given channel (actual physical setup).
public:
  static int CurrentChannel(void) { return primaryDevice ? currentChannel : 0; }
         ///< Returns the number of the current channel on the primary device.
  static void SetCurrentChannel(const cChannel *Channel) { currentChannel = Channel ? Channel->Number() : 0; }
         ///< Sets the number of the current channel on the primary device, without
         ///< actually switching to it. This can be used to correct the current
         ///< channel number while replaying.
  void ForceTransferMode(void);
         ///< Forces the device into transfermode for the current channel.
  int Occupied(void) const;
         ///< Returns the number of seconds this device is still occupied for.
  void SetOccupied(int Seconds);
         ///< Sets the occupied timeout for this device to the given number of
         ///< Seconds, This can be used to tune a device to a particular transponder
         ///< and make sure it will stay there for a certain amount of time, for
         ///< instance to collect EPG data. This function shall only be called
         ///< after the device has been successfully tuned to the requested transponder.
         ///< Seconds will be silently limited to MAXOCCUPIEDTIMEOUT. Values less than
         ///< 0 will be silently ignored.
  virtual bool HasLock(int TimeoutMs = 0);
         ///< Returns true if the device has a lock on the requested transponder.
         ///< Default is true, a specific device implementation may return false
         ///< to indicate that it is not ready yet.
         ///< If TimeoutMs is not zero, waits for the given number of milliseconds
         ///< before returning false.
  virtual bool HasProgramme(void);
         ///< Returns true if the device is currently showing any programme to
         ///< the user, either through replaying or live.

// PID handle facilities

private:
  virtual void Action(void);
protected:
  enum ePidType { ptAudio, ptVideo, ptPcr, ptTeletext, ptDolby, ptOther };
  class cPidHandle {
  public:
    int pid;
    int streamType;
    int handle;
    int used;
    cPidHandle(void) { pid = streamType = used = 0; handle = -1; }
    };
  cPidHandle pidHandles[MAXPIDHANDLES];
  bool HasPid(int Pid) const;
         ///< Returns true if this device is currently receiving the given PID.
  bool AddPid(int Pid, ePidType PidType = ptOther, int StreamType = 0);
         ///< Adds a PID to the set of PIDs this device shall receive.
  void DelPid(int Pid, ePidType PidType = ptOther);
         ///< Deletes a PID from the set of PIDs this device shall receive.
  virtual bool SetPid(cPidHandle *Handle, int Type, bool On);
         ///< Does the actual PID setting on this device.
         ///< On indicates whether the PID shall be added or deleted.
         ///< Handle->handle can be used by the device to store information it
         ///< needs to receive this PID (for instance a file handle).
         ///< Handle->used indicates how many receivers are using this PID.
         ///< Type indicates some special types of PIDs, which the device may
         ///< need to set in a specific way.

// Section filter facilities

private:
  cSectionHandler *sectionHandler;
  cEitFilter *eitFilter;
  cPatFilter *patFilter;
  cSdtFilter *sdtFilter;
  cNitFilter *nitFilter;
protected:
  void StartSectionHandler(void);
       ///< A derived device that provides section data must call
       ///< this function (typically in its constructor) to actually set
       ///< up the section handler.
  void StopSectionHandler(void);
       ///< A device that has called StartSectionHandler() must call this
       ///< function (typically in its destructor) to stop the section
       ///< handler.
public:
  virtual int OpenFilter(u_short Pid, u_char Tid, u_char Mask);
       ///< Opens a file handle for the given filter data.
       ///< A derived device that provides section data must
       ///< implement this function.
  virtual void CloseFilter(int Handle);
       ///< Closes a file handle that has previously been opened
       ///< by OpenFilter(). If this is as simple as calling close(Handle),
       ///< a derived class need not implement this function, because this
       ///< is done by the default implementation.
  void AttachFilter(cFilter *Filter);
       ///< Attaches the given filter to this device.
  void Detach(cFilter *Filter);
       ///< Detaches the given filter from this device.

// Common Interface facilities:

private:
  time_t startScrambleDetection;
  cCamSlot *camSlot;
public:
  virtual bool HasCi(void);
         ///< Returns true if this device has a Common Interface.
  void SetCamSlot(cCamSlot *CamSlot);
         ///< Sets the given CamSlot to be used with this device.
  cCamSlot *CamSlot(void) const { return camSlot; }
         ///< Returns the CAM slot that is currently used with this device,
         ///< or NULL if no CAM slot is in use.

// Image Grab facilities

public:
  virtual uchar *GrabImage(int &Size, bool Jpeg = true, int Quality = -1, int SizeX = -1, int SizeY = -1);
         ///< Grabs the currently visible screen image.
         ///< \param Size The size of the returned data block.
         ///< \param Jpeg If true will write a JPEG file. Otherwise a PNM file will be written.
         ///< \param Quality The compression factor for JPEG. 1 will create a very blocky
         ///<        and small image, 70..80 will yield reasonable quality images while keeping the
         ///<        image file size around 50 KB for a full frame. The default will create a big
         ///<        but very high quality image.
         ///< \param SizeX The number of horizontal pixels in the frame (default is the current screen width).
         ///< \param SizeY The number of vertical pixels in the frame (default is the current screen height).
         ///< \return A pointer to the grabbed image data, or NULL in case of an error.
         ///< The caller takes ownership of the returned memory and must free() it once it isn't needed any more.
  bool GrabImageFile(const char *FileName, bool Jpeg = true, int Quality = -1, int SizeX = -1, int SizeY = -1);
         ///< Calls GrabImage() and stores the resulting image in a file with the given name.
         ///< \return True if all went well.
         ///< The caller is responsible for making sure that the given file name
         ///< doesn't lead to overwriting any important other file.

// Video format facilities

public:
  virtual void SetVideoDisplayFormat(eVideoDisplayFormat VideoDisplayFormat);
         ///< Sets the video display format to the given one (only useful
         ///< if this device has an MPEG decoder).
         ///< A derived class must first call the base class function!
  virtual void SetVideoFormat(bool VideoFormat16_9);
         ///< Sets the output video format to either 16:9 or 4:3 (only useful
         ///< if this device has an MPEG decoder).
  virtual eVideoSystem GetVideoSystem(void);
         ///< Returns the video system of the currently displayed material
         ///< (default is PAL).
  virtual void GetVideoSize(int &Width, int &Height, double &VideoAspect);
         ///< Returns the With, Height and VideoAspect ratio of the currently
         ///< displayed video material. Width and Height are given in pixel
         ///< (e.g. 720x576) and VideoAspect is e.g. 1.33333 for a 4:3 broadcast,
         ///< or 1.77778 for 16:9.
         ///< The default implementation returns 0 for Width and Height
         ///< and 1.0 for VideoAspect.
  virtual void GetOsdSize(int &Width, int &Height, double &PixelAspect);
         ///< Returns the With, Height and PixelAspect ratio the OSD should use
         ///< to best fit the resolution of the output device. If PixelAspect
         ///< is not 1.0, the OSD may take this as a hint to scale its
         ///< graphics in a way that, e.g., a circle will actually
         ///< show up as a circle on the screen, and not as an ellipse.
         ///< Values greater than 1.0 mean to stretch the graphics in the
         ///< vertical direction (or shrink it in the horizontal direction,
         ///< depending on which dimension shall be fixed). Values less than
         ///< 1.0 work the other way round. Note that the OSD is not guaranteed
         ///< to actually use this hint.

// Track facilities

private:
  tTrackId availableTracks[ttMaxTrackTypes];
  eTrackType currentAudioTrack;
  eTrackType currentSubtitleTrack;
  cMutex mutexCurrentAudioTrack;
  cMutex mutexCurrentSubtitleTrack;
  int currentAudioTrackMissingCount;
  bool autoSelectPreferredSubtitleLanguage;
  int pre_1_3_19_PrivateStream;
protected:
  virtual void SetAudioTrackDevice(eTrackType Type);
       ///< Sets the current audio track to the given value.
  virtual void SetSubtitleTrackDevice(eTrackType Type);
       ///< Sets the current subtitle track to the given value.
public:
  void ClrAvailableTracks(bool DescriptionsOnly = false, bool IdsOnly = false);
       ///< Clears the list of currently availabe tracks. If DescriptionsOnly
       ///< is true, only the track descriptions will be cleared. With IdsOnly
       ///< set to true only the ids will be cleared. IdsOnly is only taken
       ///< into account if DescriptionsOnly is false.
  bool SetAvailableTrack(eTrackType Type, int Index, uint16_t Id, const char *Language = NULL, const char *Description = NULL);
       ///< Sets the track of the given Type and Index to the given values.
       ///< Type must be one of the basic eTrackType values, like ttAudio or ttDolby.
       ///< Index tells which track of the given basic type is meant.
       ///< If Id is 0 any existing id will be left untouched and only the
       ///< given Language and Description will be set.
       ///< \return Returns true if the track was set correctly, false otherwise.
  const tTrackId *GetTrack(eTrackType Type);
       ///< Returns a pointer to the given track id, or NULL if Type is not
       ///< less than ttMaxTrackTypes.
  int NumTracks(eTrackType FirstTrack, eTrackType LastTrack) const;
       ///< Returns the number of tracks in the given range that are currently
       ///< available.
  int NumAudioTracks(void) const;
       ///< Returns the number of audio tracks that are currently available.
       ///< This is just for information, to quickly find out whether there
       ///< is more than one audio track.
  int NumSubtitleTracks(void) const;
       ///< Returns the number of subtitle tracks that are currently available.
  eTrackType GetCurrentAudioTrack(void) { return currentAudioTrack; }
  bool SetCurrentAudioTrack(eTrackType Type);
       ///< Sets the current audio track to the given Type.
       ///< \return Returns true if Type is a valid audio track, false otherwise.
  eTrackType GetCurrentSubtitleTrack(void) { return currentSubtitleTrack; }
  bool SetCurrentSubtitleTrack(eTrackType Type, bool Manual = false);
       ///< Sets the current subtitle track to the given Type.
       ///< IF Manual is true, no automatic preferred subtitle language selection
       ///< will be done for the rest of the current replay session, or until
       ///< the channel is changed.
       ///< \return Returns true if Type is a valid subtitle track, false otherwise.
  void EnsureAudioTrack(bool Force = false);
       ///< Makes sure an audio track is selected that is actually available.
       ///< If Force is true, the language and Dolby Digital settings will
       ///< be verified even if the current audio track is available.
  void EnsureSubtitleTrack(void);
       ///< Makes sure one of the preferred language subtitle tracks is selected.
       ///< Only has an effect if Setup.DisplaySubtitles is on.

// Audio facilities

private:
  bool mute;
  int volume;
protected:
  virtual int GetAudioChannelDevice(void);
       ///< Gets the current audio channel, which is stereo (0), mono left (1) or
       ///< mono right (2).
  virtual void SetAudioChannelDevice(int AudioChannel);
       ///< Sets the audio channel to stereo (0), mono left (1) or mono right (2).
  virtual void SetVolumeDevice(int Volume);
       ///< Sets the audio volume on this device (Volume = 0...255).
  virtual void SetDigitalAudioDevice(bool On);
       ///< Tells the actual device that digital audio output shall be switched
       ///< on or off.
public:
  bool IsMute(void) const { return mute; }
  bool ToggleMute(void);
       ///< Turns the volume off or on and returns the new mute state.
  int GetAudioChannel(void);
       ///< Gets the current audio channel, which is stereo (0), mono left (1) or
       ///< mono right (2).
  void SetAudioChannel(int AudioChannel);
       ///< Sets the audio channel to stereo (0), mono left (1) or mono right (2).
       ///< Any other values will be silently ignored.
  void SetVolume(int Volume, bool Absolute = false);
       ///< Sets the volume to the given value, either absolutely or relative to
       ///< the current volume.
  static int CurrentVolume(void) { return primaryDevice ? primaryDevice->volume : 0; }//XXX???

// Player facilities

private:
  cPlayer *player;
  cPatPmtParser patPmtParser;
  cTsToPes tsToPesVideo;
  cTsToPes tsToPesAudio;
  cTsToPes tsToPesSubtitle;
  bool isPlayingVideo;
protected:
  const cPatPmtParser *PatPmtParser(void) const { return &patPmtParser; }
       ///< Returns a pointer to the patPmtParser, so that a derived device
       ///< can use the stream information from it.
  virtual bool CanReplay(void) const;
       ///< Returns true if this device can currently start a replay session.
  virtual bool SetPlayMode(ePlayMode PlayMode);
       ///< Sets the device into the given play mode.
       ///< \return true if the operation was successful.
  virtual int PlayVideo(const uchar *Data, int Length);
       ///< Plays the given data block as video.
       ///< Data points to exactly one complete PES packet of the given Length.
       ///< PlayVideo() shall process the packet either as a whole (returning
       ///< Length) or not at all (returning 0 or -1 and setting 'errno' accordingly).
       ///< \return Returns the number of bytes actually taken from Data, or -1
       ///< in case of an error.
  virtual int PlayAudio(const uchar *Data, int Length, uchar Id);
       ///< Plays the given data block as audio.
       ///< Data points to exactly one complete PES packet of the given Length.
       ///< Id indicates the type of audio data this packet holds.
       ///< PlayAudio() shall process the packet either as a whole (returning
       ///< Length) or not at all (returning 0 or -1 and setting 'errno' accordingly).
       ///< \return Returns the number of bytes actually taken from Data, or -1
       ///< in case of an error.
  virtual int PlaySubtitle(const uchar *Data, int Length);
       ///< Plays the given data block as a subtitle.
       ///< Data points to exactly one complete PES packet of the given Length.
       ///< PlaySubtitle() shall process the packet either as a whole (returning
       ///< Length) or not at all (returning 0 or -1 and setting 'errno' accordingly).
       ///< \return Returns the number of bytes actually taken from Data, or -1
       ///< in case of an error.
  virtual int PlayPesPacket(const uchar *Data, int Length, bool VideoOnly = false);
       ///< Plays the single PES packet in Data with the given Length.
       ///< If VideoOnly is true, only the video will be displayed,
       ///< which is necessary for trick modes like 'fast forward'.
       ///< Data must point to one single, complete PES packet.
  virtual int PlayTsVideo(const uchar *Data, int Length);
       ///< Plays the given data block as video.
       ///< Data points to exactly one complete TS packet of the given Length
       ///< (which is always TS_SIZE).
       ///< PlayTsVideo() shall process the packet either as a whole (returning
       ///< Length) or not at all (returning 0 or -1 and setting 'errno' accordingly).
       ///< The default implementation collects all incoming TS payload belonging
       ///< to one PES packet and calls PlayVideo() with the resulting packet.
  virtual int PlayTsAudio(const uchar *Data, int Length);
       ///< Plays the given data block as audio.
       ///< Data points to exactly one complete TS packet of the given Length
       ///< (which is always TS_SIZE).
       ///< PlayTsAudio() shall process the packet either as a whole (returning
       ///< Length) or not at all (returning 0 or -1 and setting 'errno' accordingly).
       ///< The default implementation collects all incoming TS payload belonging
       ///< to one PES packet and calls PlayAudio() with the resulting packet.
  virtual int PlayTsSubtitle(const uchar *Data, int Length);
       ///< Plays the given data block as a subtitle.
       ///< Data points to exactly one complete TS packet of the given Length
       ///< (which is always TS_SIZE).
       ///< PlayTsSubtitle() shall process the packet either as a whole (returning
       ///< Length) or not at all (returning 0 or -1 and setting 'errno' accordingly).
       ///< The default implementation collects all incoming TS payload belonging
       ///< to one PES packet and displays the resulting subtitle via the OSD.
public:
  virtual int64_t GetSTC(void);
       ///< Gets the current System Time Counter, which can be used to
       ///< synchronize audio, video and subtitles. If this device is able to
       ///< replay, it must provide an STC.
       ///< The value returned doesn't need to be an actual "clock" value,
       ///< it is sufficient if it holds the PTS (Presentation Time Stamp) of
       ///< the most recently presented frame. A proper value must be returned
       ///< in normal replay mode as well as in any trick modes (like slow motion,
       ///< fast forward/rewind).
       ///< Only the lower 32 bit of this value are actually used, since some
       ///< devices can't handle the msb correctly.
  virtual bool IsPlayingVideo(void) const { return isPlayingVideo; }
       ///< \return Returns true if the currently attached player has delivered
       ///< any video packets.
  virtual bool HasIBPTrickSpeed(void) { return false; }
       ///< Returns true if this device can handle all frames in 'fast forward'
       ///< trick speeds.
  virtual void TrickSpeed(int Speed);
       ///< Sets the device into a mode where replay is done slower.
       ///< Every single frame shall then be displayed the given number of
       ///< times.
       ///< The cDvbPlayer uses the following values for the various speeds:
       ///<                   1x   2x   3x
       ///< Fast Forward       6    3    1
       ///< Fast Reverse       6    3    1
       ///< Slow Forward       8    4    2
       ///< Slow Reverse      63   48   24
  virtual void Clear(void);
       ///< Clears all video and audio data from the device.
       ///< A derived class must call the base class function to make sure
       ///< all registered cAudio objects are notified.
  virtual void Play(void);
       ///< Sets the device into play mode (after a previous trick
       ///< mode).
  virtual void Freeze(void);
       ///< Puts the device into "freeze frame" mode.
  virtual void Mute(void);
       ///< Turns off audio while replaying.
       ///< A derived class must call the base class function to make sure
       ///< all registered cAudio objects are notified.
  virtual void StillPicture(const uchar *Data, int Length);
       ///< Displays the given I-frame as a still picture.
       ///< Data points either to TS (first byte is 0x47) or PES (first byte
       ///< is 0x00) data of the given Length. The default implementation
       ///< converts TS to PES and calls itself again, allowing a derived class
       ///< to display PES if it can't handle TS directly.
  virtual bool Poll(cPoller &Poller, int TimeoutMs = 0);
       ///< Returns true if the device itself or any of the file handles in
       ///< Poller is ready for further action.
       ///< If TimeoutMs is not zero, the device will wait up to the given number
       ///< of milleseconds before returning in case it can't accept any data.
  virtual bool Flush(int TimeoutMs = 0);
       ///< Returns true if the device's output buffers are empty, i. e. any
       ///< data which was bufferd so far has been processed.
       ///< If TimeoutMs is not zero, the device will wait up to the given
       ///< number of milliseconds before returning in case there is still
       ///< data in the buffers.
  virtual int PlayPes(const uchar *Data, int Length, bool VideoOnly = false);
       ///< Plays all valid PES packets in Data with the given Length.
       ///< If Data is NULL any leftover data from a previous call will be
       ///< discarded. If VideoOnly is true, only the video will be displayed,
       ///< which is necessary for trick modes like 'fast forward'.
       ///< Data should point to a sequence of complete PES packets. If the
       ///< last packet in Data is not complete, it will be copied and combined
       ///< to a complete packet with data from the next call to PlayPes().
       ///< That way any functions called from within PlayPes() will be
       ///< guaranteed to always receive complete PES packets.
  virtual int PlayTs(const uchar *Data, int Length, bool VideoOnly = false);
       ///< Plays the given TS packet.
       ///< If VideoOnly is true, only the video will be displayed,
       ///< which is necessary for trick modes like 'fast forward'.
       ///< Data points to a single TS packet, Length is always TS_SIZE (the total
       ///< size of a single TS packet).
       ///< If Data is NULL any leftover data from a previous call will be
       ///< discarded.
       ///< A derived device can reimplement this function to handle the
       ///< TS packets itself. Any packets the derived function can't handle
       ///< must be sent to the base class function. This applies especially
       ///< to the PAT/PMT packets.
       ///< Returns -1 in case of error, otherwise the number of actually
       ///< processed bytes is returned.
       ///< PlayTs() shall process the TS packets either as a whole (returning
       ///< n*TS_SIZE) or not at all, returning 0 or -1 and setting 'errno' accordingly).
  bool Replaying(void) const;
       ///< Returns true if we are currently replaying.
  bool Transferring(void) const;
       ///< Returns true if we are currently in Transfer Mode.
  void StopReplay(void);
       ///< Stops the current replay session (if any).
  bool AttachPlayer(cPlayer *Player);
       ///< Attaches the given player to this device.
  void Detach(cPlayer *Player);
       ///< Detaches the given player from this device.

// Receiver facilities

private:
  cMutex mutexReceiver;
  cReceiver *receiver[MAXRECEIVERS];
public:
  int Priority(void) const;
      ///< Returns the priority of the current receiving session (0..MAXPRIORITY),
      ///< or -1 if no receiver is currently active. The primary device will
      ///< always return at least Setup.PrimaryLimit-1.
protected:
  virtual bool OpenDvr(void);
      ///< Opens the DVR of this device and prepares it to deliver a Transport
      ///< Stream for use in a cReceiver.
  virtual void CloseDvr(void);
      ///< Shuts down the DVR.
  virtual bool GetTSPacket(uchar *&Data);
      ///< Gets exactly one TS packet from the DVR of this device and returns
      ///< a pointer to it in Data. Only the first 188 bytes (TS_SIZE) Data
      ///< points to are valid and may be accessed. If there is currently no
      ///< new data available, Data will be set to NULL. The function returns
      ///< false in case of a non recoverable error, otherwise it returns true,
      ///< even if Data is NULL.
public:
  bool Receiving(bool CheckAny = false) const;
       ///< Returns true if we are currently receiving.
  bool AttachReceiver(cReceiver *Receiver);
       ///< Attaches the given receiver to this device.
  void Detach(cReceiver *Receiver);
       ///< Detaches the given receiver from this device.
  void DetachAll(int Pid);
       ///< Detaches all receivers from this device for this pid.
  void DetachAllReceivers(void);
       ///< Detaches all receivers from this device.
  };

/// Derived cDevice classes that can receive channels will have to provide
/// Transport Stream (TS) packets one at a time. cTSBuffer implements a
/// simple buffer that allows the device to read a larger amount of data
/// from the driver with each call to Read(), thus avoiding the overhead
/// of getting each TS packet separately from the driver. It also makes
/// sure the returned data points to a TS packet and automatically
/// re-synchronizes after broken packets.

class cTSBuffer : public cThread {
private:
  int f;
  int cardIndex;
  bool delivered;
  cRingBufferLinear *ringBuffer;
  virtual void Action(void);
public:
  cTSBuffer(int File, int Size, int CardIndex);
  ~cTSBuffer();
  uchar *Get(void);
  };

#endif //__DEVICE_H

#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "FileItem.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "interfaces/IAnnouncer.h"
#include "threads/Event.h"
#include "threads/Thread.h"
#include "utils/EventStream.h"
#include "utils/JobManager.h"
#include "utils/Observer.h"

#include "pvr/PVREvent.h"
#include "pvr/PVRTypes.h"
#include "pvr/recordings/PVRRecording.h"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

class CGUIDialogExtendedProgressBar;
class CGUIDialogProgressBarHandle;
class CStopWatch;
class CAction;
class CFileItemList;
class CVariant;

namespace EPG
{
  class CEpgContainer;
}

namespace PVR
{
  class CPVRClient;
  class CPVRClients;
  class CPVRChannelGroupsContainer;
  class CPVRChannelGroup;
  class CPVRRecordings;
  class CPVRTimers;
  class CPVRGUIInfo;
  class CPVRDatabase;
  class CGUIWindowPVRCommon;

  enum ContinueLastChannelOnStartup
  {
    CONTINUE_LAST_CHANNEL_OFF  = 0,
    CONTINUE_LAST_CHANNEL_IN_BACKGROUND,
    CONTINUE_LAST_CHANNEL_IN_FOREGROUND
  };

  class CPVRManagerJobQueue
  {
  public:
    CPVRManagerJobQueue();

    void Start();
    void Stop();
    void Clear();

    void AppendJob(CJob * job);
    void ExecutePendingJobs();
    bool WaitForJobs(unsigned int milliSeconds);

  private:
    CCriticalSection m_critSection;
    CEvent m_triggerEvent;
    std::vector<CJob *> m_pendingUpdates;
    bool m_bStopped;
  };

  class CPVRManager : private CThread, public Observable, public ANNOUNCEMENT::IAnnouncer, public ISettingCallback
  {
    friend class CPVRClients;

  public:
    /*!
     * @brief Create a new CPVRManager instance, which handles all PVR related operations in XBMC.
     */
    CPVRManager(void);

  private:
    /*!
     * @brief Updates the last watched timestamps of the channel and group which are currently playing.
     * @param channel The channel which is updated
     */
    void UpdateLastWatched(const CPVRChannelPtr &channel);

    /*!
     * @brief Set the playing group to the first group the channel is in if the given channel is not part of the current playing group
     * @param channel The channel
     */
    void SetPlayingGroup(const CPVRChannelPtr &channel);

  public:
    /*!
     * @brief Stop the PVRManager and destroy all objects it created.
     */
    virtual ~CPVRManager(void);

    virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data) override;

    // ISettingCallback implementation
    void OnSettingChanged(const CSetting *setting) override;

    /*!
     * @brief Get the channel groups container.
     * @return The groups container.
     */
    CPVRChannelGroupsContainerPtr ChannelGroups(void) const;

    /*!
     * @brief Get the recordings container.
     * @return The recordings container.
     */
    CPVRRecordingsPtr Recordings(void) const;

    /*!
     * @brief Get the timers container.
     * @return The timers container.
     */
    CPVRTimersPtr Timers(void) const;

    /*!
     * @brief Get the timers container.
     * @return The timers container.
     */
    CPVRClientsPtr Clients(void) const;

    /*!
     * @brief Init PVRManager.
     */
    void Init(void);

    /*!
     * @brief Reinit PVRManager.
     */
    void Reinit(void);

    /*!
     * @brief Start the PVRManager, which loads all PVR data and starts some threads to update the PVR data.
     */
    void Start();

    /*!
     * @brief Stop PVRManager.
     */
    void Stop(void);

    /*!
     * @brief Stop PVRManager, unload data.
     */
    void Unload();

    /*!
     * @brief Deinit PVRManager, unload data, unload addons.
     */
    void Deinit();

    /*!
     * @brief Propagate event on system sleep
     */
    void OnSleep();

    /*!
     * @brief Propagate event on system wake
     */
    void OnWake();

    /*!
     * @brief Get the TV database.
     * @return The TV database.
     */
    CPVRDatabasePtr GetTVDatabase(void) const;

    /*!
     * @brief Get a GUIInfoManager character string.
     * @param dwInfo The string to get.
     * @return The requested string or an empty one if it wasn't found.
     */
    bool TranslateCharInfo(DWORD dwInfo, std::string &strValue) const;

    /*!
     * @brief Get a GUIInfoManager integer.
     * @param dwInfo The integer to get.
     * @return The requested integer or 0 if it wasn't found.
     */
    int TranslateIntInfo(DWORD dwInfo) const;

    /*!
     * @brief Get a GUIInfoManager boolean.
     * @param dwInfo The boolean to get.
     * @return The requested boolean or false if it wasn't found.
     */
    bool TranslateBoolInfo(DWORD dwInfo) const;

    /*!
     * @brief Show the player info.
     * @param iTimeout Hide the player info after iTimeout seconds.
     * @todo not really the right place for this :-)
     */
    void ShowPlayerInfo(int iTimeout);

    /*!
     * @brief Check if a TV channel, radio channel or recording is playing.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlaying(void) const;

    /*!
     * @brief Check if the given channel is playing.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlayingChannel(const CPVRChannelPtr &channel) const;

    /*!
     * @brief Check if the given recording is playing.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlayingRecording(const CPVRRecordingPtr &recording) const;

    /*!
     * @return True while the PVRManager is initialising.
     */
    inline bool IsInitialising(void) const
    {
      return GetState() == ManagerStateStarting;
    }

    /*!
     * @brief Check whether the PVRManager has fully started.
     * @return True if started, false otherwise.
     */
    inline bool IsStarted(void) const
    {
      return GetState() == ManagerStateStarted;
    }

    /*!
     * @brief Check whether the PVRManager is stopping
     * @return True while the PVRManager is stopping.
     */
    inline bool IsStopping(void) const
    {
      return GetState() == ManagerStateStopping;
    }

    /*!
     * @brief Check whether the PVRManager has been stopped.
     * @return True if stopped, false otherwise.
     */
    inline bool IsStopped(void) const
    {
      return GetState() == ManagerStateStopped;
    }

    /*!
     * @brief Return the channel that is currently playing.
     * @return The channel or NULL if none is playing.
     */
    CPVRChannelPtr GetCurrentChannel(void) const;

    /*!
     * @brief Return the recording that is currently playing.
     * @return The recording or NULL if none is playing.
     */
    CPVRRecordingPtr GetCurrentRecording(void) const;

    /*!
     * @brief Update the channel displayed in guiinfomanager and application to match the currently playing channel.
     */
    void UpdateCurrentChannel(void);

    /*!
     * @brief Return the EPG for the channel that is currently playing.
     * @param channel The EPG or NULL if no channel is playing.
     * @return The amount of results that was added or -1 if none.
     */
    int GetCurrentEpg(CFileItemList &results) const;

    /*!
     * @brief Check whether EPG tags for channels have been created.
     * @return True if EPG tags have been created, false otherwise.
     */
    bool EpgsCreated(void) const;

    /*!
     * @brief Reset the playing EPG tag.
     */
    void ResetPlayingTag(void);

    /*!
     * @brief Switch to the given channel.
     * @param channel The channel to switch to.
     * @param bPreview True to show a preview, false otherwise.
     * @return True if the switch was successful, false otherwise.
     */
    bool PerformChannelSwitch(const CPVRChannelPtr &channel, bool bPreview);

    /*!
     * @brief Close an open PVR stream.
     */
    void CloseStream(void);

    /*!
     * @brief Open a stream from the given channel.
     * @param fileItem The file item with the channel to open.
     * @return True if the stream was opened, false otherwise.
     */
    bool OpenLiveStream(const CFileItem &fileItem);

    /*!
     * @brief Open a stream from the given recording.
     * @param tag The recording to open.
     * @return True if the stream was opened, false otherwise.
     */
    bool OpenRecordedStream(const CPVRRecordingPtr &tag);

    /*!
     * @brief Start or stop recording on the channel that is currently being played.
     * @param bOnOff True to start recording, false to stop.
     */
    void StartRecordingOnPlayingChannel(bool bOnOff);

    /*!
     * @brief Check whether there are active recordings.
     * @return True if there are active recordings, false otherwise.
     */
    bool IsRecording(void) const;

    /*!
     * @brief Check whether the system Kodi is running on can be powered down
     *        (shutdown/reboot/suspend/hibernate) without stopping any active
     *        recordings and/or without preventing the start of recordings
     *        scheduled for now + pvrpowermanagement.backendidletime.
     * @param bAskUser True to informs user in case of potential
     *        data loss. User can decide to allow powerdown anyway. False to
     *        not to ask user and to not confirm power down.
     * @return True if system can be safely powered down, false otherwise.
     */
    bool CanSystemPowerdown(bool bAskUser = true) const;

    /*!
     * @brief Set the current playing group, used to load the right channel.
     * @param group The new group.
     */
    void SetPlayingGroup(const CPVRChannelGroupPtr &group);

    /*!
     * @brief Get the current playing group, used to load the right channel.
     * @param bRadio True to get the current radio group, false to get the current TV group.
     * @return The current group or the group containing all channels if it's not set.
     */
    CPVRChannelGroupPtr GetPlayingGroup(bool bRadio = false);

    /*!
     * @brief Let the background thread create epg tags for all channels.
     */
    void TriggerEpgsCreate(void);

    /*!
     * @brief Let the background thread update the recordings list.
     */
    void TriggerRecordingsUpdate(void);

    /*!
     * @brief Let the background thread update the timer list.
     */
    void TriggerTimersUpdate(void);

    /*!
     * @brief Let the background thread update the channel list.
     */
    void TriggerChannelsUpdate(void);

    /*!
     * @brief Let the background thread update the channel groups list.
     */
    void TriggerChannelGroupsUpdate(void);

    /*!
     * @brief Let the background thread search for missing channel icons.
     */
    void TriggerSearchMissingChannelIcons(void);

    /*!
     * @brief Update the channel that is currently active.
     * @param item The new channel.
     * @return True if it was updated correctly, false otherwise.
     */
    bool UpdateItem(CFileItem& item);

    /*!
     * @brief Switch to a channel given it's channel id.
     * @param iChannelId The channel id to switch to.
     * @return True if the channel was switched, false otherwise.
     */
    bool ChannelSwitchById(unsigned int iChannelId);

    /*!
     * @brief Switch to the next channel in this group.
     * @param iNewChannelNumber The new channel number after the switch.
     * @param bPreview If true, don't do the actual switch but just update channel pointers.
     *                Used to display event info while doing "fast channel switching"
     * @return True if the channel was switched, false otherwise.
     */
    bool ChannelUp(unsigned int *iNewChannelNumber, bool bPreview = false) { return ChannelUpDown(iNewChannelNumber, bPreview, true); }

    /*!
     * @brief Switch to the previous channel in this group.
     * @param iNewChannelNumber The new channel number after the switch.
     * @param bPreview If true, don't do the actual switch but just update channel pointers.
     *                Used to display event info while doing "fast channel switching"
     * @return True if the channel was switched, false otherwise.
     */
    bool ChannelDown(unsigned int *iNewChannelNumber, bool bPreview = false) { return ChannelUpDown(iNewChannelNumber, bPreview, false); }

    /*!
     * @brief Get the total duration of the currently playing LiveTV item.
     * @return The total duration in milliseconds or NULL if no channel is playing.
     */
    int GetTotalTime(void) const;

    /*!
     * @brief Get the current position in milliseconds since the start of a LiveTV item.
     * @return The position in milliseconds or NULL if no channel is playing.
     */
    int GetStartTime(void) const;

    /*!
     * @brief Update the current playing file in the guiinfomanager and application.
     */
    void UpdateCurrentFile(void);

    /*!
     * @brief Check whether names are still correct after the language settings changed.
     */
    void LocalizationChanged(void);

    /*!
     * @brief Check if a TV channel is playing.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlayingTV(void) const;

    /*!
     * @brief Check if a radio channel is playing.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlayingRadio(void) const;

    /*!
     * @brief Check if a recording is playing.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlayingRecording(void) const;

    /*!
     * @brief Try to find missing channel icons automatically
     */
    void SearchMissingChannelIcons(void);

    /*!
     * @brief Check if parental lock is overridden at the given moment.
     * @param channel The channel to open.
     * @return True if parental lock is overridden, false otherwise.
     */
    bool IsParentalLocked(const CPVRChannelPtr &channel);

    /*!
     * @brief Restart the parental timer.
     */
    void RestartParentalTimer();

    /*!
     * @brief Create EPG tags for all channels in internal channel groups
     * @return True if EPG tags where created successfully, false otherwise
     */
    bool CreateChannelEpgs(void);

    /*!
    * @brief get the name of the channel group of the current playing channel
    * @return name of channel if tv channel is playing
    */
    std::string GetPlayingTVGroupName();

    /*!
     * @brief Signal a connection change of a client
     */
    void ConnectionStateChange(CPVRClient *client, std::string connectString, PVR_CONNECTION_STATE state, std::string message);

    /*!
     * @brief Explicitly set the state of channel preview. This is when channel is displayed on OSD without actually switching
     */
    void SetChannelPreview(bool preview);

    /*!
     * @brief Query the state of channel preview
     */
    bool IsChannelPreview() const;

    /*!
     * @brief Query the events available for CEventStream
     */
    CEventStream<PVREvent>& Events() { return m_events; }

    /*!
     * @brief Publish an event
     * @param state the event
     */
    void PublishEvent(PVREvent state);

    /*!
     * @brief Show an extended progress bar dialog.
     * @param strTitle the title for the dialog.
     * @return the handle that can be used to control the progress dialog.
     */
    CGUIDialogProgressBarHandle* ShowProgressDialog(const std::string &strTitle) const;

  protected:
    /*!
     * @brief PVR update and control thread.
     */
    virtual void Process(void) override;

  private:
    /*!
     * @brief Save the currently playing channel as last played channel
     */
    void SaveLastPlayedChannel() const;

    /*!
     * @brief Executes "pvrpowermanagement.setwakeupcmd"
     */
    bool SetWakeupCommand(void);

    /*!
     * @brief Show or update the progress dialog.
     * @param strText The current status.
     * @param iProgress The current progress in %.
     */
    void ShowProgressDialog(const std::string &strText, int iProgress);

    /*!
     * @brief Hide the progress dialog if it's visible.
     */
    void HideProgressDialog(void);

    /*!
     * @brief Load at least one client and load all other PVR data after loading the client.
     * If some clients failed to load here, the pvrmanager will retry to load them every second.
     * @param bShowProgress True, to show a progress dialog for the different load stages.
     * @return If at least one client and all pvr data was loaded, false otherwise.
     */
    bool Load(bool bShowProgress);

    /*!
     * @brief Reset all properties.
     */
    void ResetProperties(void);

    /*!
     * @brief Destroy PVRManager's objects.
     */
    void Clear(void);

    /*!
     * @brief Called by ChannelUp() and ChannelDown() to perform a channel switch.
     * @param iNewChannelNumber The new channel number after the switch.
     * @param bPreview Preview window if true.
     * @param bUp Go one channel up if true, one channel down if false.
     * @return True if the switch was successful, false otherwise.
     */
    bool ChannelUpDown(unsigned int *iNewChannelNumber, bool bPreview, bool bUp);

    /*!
     * @brief Continue playback on the last played channel.
     */
    void TriggerContinueLastChannel(void);

    enum ManagerState
    {
      ManagerStateError = 0,
      ManagerStateStopped,
      ManagerStateStarting,
      ManagerStateStopping,
      ManagerStateInterrupted,
      ManagerStateStarted
    };

    ManagerState GetState(void) const;

    void SetState(ManagerState state);

    bool AllLocalBackendsIdle(CPVRTimerInfoTagPtr& causingEvent) const;
    bool EventOccursOnLocalBackend(const CFileItemPtr& item) const;
    bool IsNextEventWithinBackendIdleTime(void) const;

    /** @name containers */
    //@{
    CPVRChannelGroupsContainerPtr  m_channelGroups;               /*!< pointer to the channel groups container */
    CPVRRecordingsPtr              m_recordings;                  /*!< pointer to the recordings container */
    CPVRTimersPtr                  m_timers;                      /*!< pointer to the timers container */
    CPVRClientsPtr                 m_addons;                      /*!< pointer to the pvr addon container */
    std::unique_ptr<CPVRGUIInfo>   m_guiInfo;                     /*!< pointer to the guiinfo data */
    //@}

    CPVRManagerJobQueue             m_pendingUpdates;              /*!< vector of pending pvr updates */

    CFileItemPtr                    m_currentFile;                 /*!< the PVR file that is currently playing */
    CPVRDatabasePtr                 m_database;                    /*!< the database for all PVR related data */
    CCriticalSection                m_critSection;                 /*!< critical section for all changes to this class, except for changes to triggers */
    bool                            m_bFirstStart;                 /*!< true when the PVR manager was started first, false otherwise */
    bool                            m_bIsSwitchingChannels;        /*!< true while switching channels */
    bool                            m_bEpgsCreated;                /*!< true if epg data for channels has been created */
    CGUIDialogExtendedProgressBar * m_progressBar;                 /*!< extended progress dialog instance pointer */
    CGUIDialogProgressBarHandle *   m_progressHandle;              /*!< progress dialog that is displayed while the pvrmanager is loading */

    CCriticalSection                m_managerStateMutex;
    ManagerState                    m_managerState;
    std::unique_ptr<CStopWatch>     m_parentalTimer;

    CCriticalSection                m_startStopMutex; // mutex for protecting pvr manager's start/restart/stop sequence */

    std::atomic_bool m_isChannelPreview;
    CEventSource<PVREvent> m_events;

    // settings cache
    bool m_bSettingPowerManagementEnabled; // SETTING_PVRPOWERMANAGEMENT_ENABLED
    std::string m_strSettingWakeupCommand; // SETTING_PVRPOWERMANAGEMENT_SETWAKEUPCMD
  };
}

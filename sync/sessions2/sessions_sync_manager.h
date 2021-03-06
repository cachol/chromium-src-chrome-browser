// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_SESSIONS2_SESSIONS_SYNC_MANAGER_H_
#define CHROME_BROWSER_SYNC_SESSIONS2_SESSIONS_SYNC_MANAGER_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/gtest_prod_util.h"
#include "base/memory/scoped_vector.h"
#include "base/time/time.h"
#include "chrome/browser/sessions/session_id.h"
#include "chrome/browser/sessions/session_types.h"
#include "chrome/browser/sync/glue/device_info.h"
#include "chrome/browser/sync/glue/favicon_cache.h"
#include "chrome/browser/sync/glue/synced_session.h"
#include "chrome/browser/sync/glue/synced_session_tracker.h"
#include "chrome/browser/sync/sessions2/tab_node_pool2.h"
#include "chrome/browser/sync/sync_prefs.h"
#include "sync/api/syncable_service.h"

class Profile;

namespace syncer {
class SyncErrorFactory;
}

namespace sync_pb {
class SessionHeader;
class SessionSpecifics;
class SessionTab;
class SessionWindow;
class TabNavigation;
}  // namespace sync_pb

namespace browser_sync {

class DataTypeErrorHandler;
class SyncedTabDelegate;

// Contains all logic for associating the Chrome sessions model and
// the sync sessions model.
class SessionsSyncManager : public syncer::SyncableService {
 public:
  // Isolates SessionsSyncManager from having to depend on sync internals.
  class SyncInternalApiDelegate {
   public:
    // Returns sync's representation of the local device info.
    // Return value is an empty scoped_ptr if the device info is unavailable.
    virtual scoped_ptr<DeviceInfo> GetLocalDeviceInfo() const = 0;

    // Used for creation of the machine tag for this local session.
    virtual std::string GetCacheGuid() const = 0;
  };

  SessionsSyncManager(Profile* profile,
                      scoped_ptr<SyncPrefs> sync_prefs,
                      SyncInternalApiDelegate* delegate);
  virtual ~SessionsSyncManager();

  // A local navigation event took place that affects the synced session
  // for this instance of Chrome.
  void OnLocalTabModified(const SyncedTabDelegate& modified_tab,
                          syncer::SyncError* error);

  // When a Browser window is opened, we want to know so we can make sure our
  // bookkeeping of open windows / sessions on this device is up-to-date.
  void OnBrowserOpened();

  // A local navigation occurred that triggered updates to favicon data for
  // each URL in |updated_page_urls|.  This is routed through Sessions Sync so
  // that we can filter (exclude) favicon updates for pages that aren't
  // currently part of the set of local open tabs, and pass relevant updates
  // on to FaviconCache for out-of-band favicon syncing.
  void ForwardRelevantFaviconUpdatesToFaviconCache(
      const std::set<GURL>& updated_favicon_page_urls);

  // Returns the tag used to uniquely identify this machine's session in the
  // sync model.
  const std::string& current_machine_tag() const {
    DCHECK(!current_machine_tag_.empty());
    return current_machine_tag_;
  }

  // Builds a list of all foreign sessions. Caller does NOT own SyncedSession
  // objects.
  // Returns true if foreign sessions were found, false otherwise.
  bool GetAllForeignSessions(std::vector<const SyncedSession*>* sessions);

  // If a valid favicon for the page at |url| is found, fills |favicon_png| with
  // the png-encoded image and returns true. Else, returns false.
  bool GetSyncedFaviconForPageURL(
      const std::string& page_url,
      scoped_refptr<base::RefCountedMemory>* favicon_png) const;

  // syncer::SyncableService implementation.
  virtual syncer::SyncMergeResult MergeDataAndStartSyncing(
      syncer::ModelType type,
      const syncer::SyncDataList& initial_sync_data,
      scoped_ptr<syncer::SyncChangeProcessor> sync_processor,
      scoped_ptr<syncer::SyncErrorFactory> error_handler) OVERRIDE;
  virtual void StopSyncing(syncer::ModelType type) OVERRIDE;
  virtual syncer::SyncDataList GetAllSyncData(
      syncer::ModelType type) const OVERRIDE;
  virtual syncer::SyncError ProcessSyncChanges(
      const tracked_objects::Location& from_here,
      const syncer::SyncChangeList& change_list) OVERRIDE;

 private:
  FRIEND_TEST_ALL_PREFIXES(SessionsSyncManagerTest, DeleteForeignSession);
  FRIEND_TEST_ALL_PREFIXES(SessionsSyncManagerTest,
                           WriteForeignSessionToNodeTabsFirst);
  FRIEND_TEST_ALL_PREFIXES(SessionsSyncManagerTest,
                           WriteForeignSessionToNodeMissingTabs);
  FRIEND_TEST_ALL_PREFIXES(SessionsSyncManagerTest, Favicons);
  FRIEND_TEST_ALL_PREFIXES(SessionsSyncManagerTest,
                           SaveUnassociatedNodesForReassociation);
  FRIEND_TEST_ALL_PREFIXES(SessionsSyncManagerTest, MergeDeletesCorruptNode);

  void InitializeCurrentMachineTag();

  // Load and add window or tab data for a foreign session to our internal
  // tracking.
  void UpdateTrackerWithForeignSession(
      const sync_pb::SessionSpecifics& specifics,
      const base::Time& modification_time);

  // Returns true if |sync_data| contained a header node for the current
  // machine, false otherwise.
  bool InitFromSyncModel(const syncer::SyncDataList& sync_data,
                         syncer::SyncChangeList* new_changes);

  // Helper to construct a deletion SyncChange for a *tab node*.
  // Caller should check IsValid() on the returned change, as it's possible
  // this node could not be deleted.
  syncer::SyncChange TombstoneTab(const sync_pb::SessionSpecifics& tab);

  // Helper method to load the favicon data from the tab specifics. If the
  // favicon is valid, stores the favicon data into the favicon cache.
  void RefreshFaviconVisitTimesFromForeignTab(
      const sync_pb::SessionTab& tab, const base::Time& modification_time);

  // Removes a foreign session from our internal bookkeeping.
  // Returns true if the session was found and deleted, false if no data was
  // found for that session.  This will *NOT* trigger sync deletions. See
  // DeleteForeignSession below.
  bool DisassociateForeignSession(const std::string& foreign_session_tag);

  // Delete a foreign session and all its sync data.
  // |change_output| *must* be provided as a link to the SyncChange pipeline
  // that exists in the caller's context. This function will append necessary
  // changes for processing later.
  void DeleteForeignSession(const std::string& tag,
                            syncer::SyncChangeList* change_output);

  // Used to populate a session header from the session specifics header
  // provided.
  void PopulateSessionHeaderFromSpecifics(
      const sync_pb::SessionHeader& header_specifics,
      base::Time mtime,
      SyncedSession* session_header) const;

  // Builds |session_window| from the session specifics window
  // provided and updates the SessionTracker with foreign session data created.
  void BuildSyncedSessionFromSpecifics(
      const std::string& session_tag,
      const sync_pb::SessionWindow& specifics,
      base::Time mtime,
      SessionWindow* session_window);

  // Resync local window information. Updates the local sessions header node
  // with the status of open windows and the order of tabs they contain. Should
  // only be called for changes that affect a window, not a change within a
  // single tab.
  //
  // RELOAD_TABS will additionally cause a resync of all tabs (same as calling
  // AssociateTabs with a vector of all tabs).
  //
  // Returns: false if the local session's sync nodes were deleted and
  // reassociation is necessary, true otherwise.
  //
  // |change_output| *must* be provided as a link to the SyncChange pipeline
  // that exists in the caller's context. This function will append necessary
  // changes for processing later.
  enum ReloadTabsOption {
    RELOAD_TABS,
    DONT_RELOAD_TABS
  };
  void AssociateWindows(ReloadTabsOption option,
                        syncer::SyncChangeList* change_output);

  // Loads and reassociates the local tabs referenced in |tabs|.
  // |change_output| *must* be provided as a link to the SyncChange pipeline
  // that exists in the caller's context. This function will append necessary
  // changes for processing later.
  void AssociateTab(SyncedTabDelegate* const tab,
                    syncer::SyncChangeList* change_output);

  // Control which local tabs we're interested in syncing.
  // Ensures the profile matches sync's profile and that the tab has valid
  // entries.
  bool ShouldSyncTab(const SyncedTabDelegate& tab) const;

  SyncedSessionTracker session_tracker_;
  FaviconCache favicon_cache_;

  // Pool of used/available sync nodes associated with local tabs.
  TabNodePool2 local_tab_pool_;

  scoped_ptr<SyncPrefs> sync_prefs_;

  scoped_ptr<syncer::SyncErrorFactory> error_handler_;
  scoped_ptr<syncer::SyncChangeProcessor> sync_processor_;

  const SyncInternalApiDelegate* const delegate_;

  // Unique client tag.
  std::string current_machine_tag_;

  // User-visible machine name.
  std::string current_session_name_;

  // SyncID for the sync node containing all the window information for this
  // client.
  int local_session_header_node_id_;

  DISALLOW_COPY_AND_ASSIGN(SessionsSyncManager);
};

}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_SESSIONS2_SESSIONS_SYNC_MANAGER_H_

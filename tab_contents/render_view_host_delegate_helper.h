// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TAB_CONTENTS_RENDER_VIEW_HOST_DELEGATE_HELPER_H_
#define CHROME_BROWSER_TAB_CONTENTS_RENDER_VIEW_HOST_DELEGATE_HELPER_H_
#pragma once

#include <map>
#include <string>

#include "base/basictypes.h"
#include "content/browser/webui/web_ui.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/common/window_container_type.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebPopupType.h"
#include "webkit/glue/webpreferences.h"
#include "webkit/glue/window_open_disposition.h"

class BackgroundContents;
class Profile;
class RenderViewHost;
class RenderViewHostDelegate;
class RenderWidgetHost;
class RenderWidgetHostView;
class SiteInstance;
class TabContents;
struct ViewHostMsg_CreateWindow_Params;

namespace content {
class BrowserContext;
class RenderProcessHost;
class WebContents;
}

namespace gfx {
class Rect;
}

// Provides helper methods that provide common implementations of some
// TabContentsView methods.
class RenderViewHostDelegateViewHelper : public content::NotificationObserver {
 public:
  RenderViewHostDelegateViewHelper();
  virtual ~RenderViewHostDelegateViewHelper();

  // Creates a new window; call |ShowCreatedWindow| below to show it.
  TabContents* CreateNewWindow(content::WebContents* web_contents,
                               int route_id,
                               const ViewHostMsg_CreateWindow_Params& params);

  // Creates a new popup or fullscreen widget; call |ShowCreatedWidget| below to
  // show it. If |is_fullscreen| is true it is a fullscreen widget, if not then
  // a pop-up. |popup_type| is only meaningful for a pop-up.
  RenderWidgetHostView* CreateNewWidget(content::WebContents* web_contents,
                                        int route_id,
                                        bool is_fullscreen,
                                        WebKit::WebPopupType popup_type);

  // Shows a window created with |CreateNewWindow| above.
  TabContents* ShowCreatedWindow(content::WebContents* web_contents,
                                 int route_id,
                                 WindowOpenDisposition disposition,
                                 const gfx::Rect& initial_pos,
                                 bool user_gesture);

  // Shows a widget created with |CreateNewWidget| above. |initial_pos| is only
  // meaningful for non-fullscreen widgets.
  RenderWidgetHostView* ShowCreatedWidget(content::WebContents* web_contents,
                                          int route_id,
                                          bool is_fullscreen,
                                          const gfx::Rect& initial_pos);

 private:
  // content::NotificationObserver implementation
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  // Creates a new renderer for window.open. This will either be a
  // BackgroundContents (if the window_container_type ==
  // WINDOW_CONTAINER_TYPE_BACKGROUND and permissions allow) or a TabContents.
  // If a TabContents is created, it is returned. Otherwise NULL is returned.
  TabContents* CreateNewWindowImpl(
      int route_id,
      Profile* profile,
      SiteInstance* site,
      WebUI::TypeID webui_type,
      RenderViewHostDelegate* opener,
      WindowContainerType window_container_type,
      const string16& frame_name);

  BackgroundContents* MaybeCreateBackgroundContents(
      int route_id,
      Profile* profile,
      SiteInstance* site,
      const GURL& opener_url,
      const string16& frame_name);

  // Finds the new RenderWidgetHost and returns it. Note that this can only be
  // called once as this call also removes it from the internal map.
  RenderWidgetHostView* GetCreatedWidget(int route_id);

  // Finds the new RenderViewHost/Delegate by route_id, initializes it for
  // for renderer-initiated creation, and returns the TabContents that needs
  // to be shown, if there is one (i.e. not a BackgroundContents). Note that
  // this can only be called once as this call also removes it from the internal
  // map.
  TabContents* GetCreatedWindow(int route_id);

  // Tracks created RenderViewHost objects that have not been shown yet.
  // They are identified by the route ID passed to CreateNewWindow.
  typedef std::map<int, RenderViewHost*> PendingContents;
  PendingContents pending_contents_;

  // These maps hold on to the widgets that we created on behalf of the
  // renderer that haven't shown yet.
  typedef std::map<int, RenderWidgetHostView*> PendingWidgetViews;
  PendingWidgetViews pending_widget_views_;

  // Registers and unregisters us for notifications.
  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(RenderViewHostDelegateViewHelper);
};

#endif  // CHROME_BROWSER_TAB_CONTENTS_RENDER_VIEW_HOST_DELEGATE_HELPER_H_

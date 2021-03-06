// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_CHROMIUM_APPLICATION_H_
#define CHROME_BROWSER_ANDROID_CHROMIUM_APPLICATION_H_

#include <jni.h>

#include "base/basictypes.h"

namespace chrome {
namespace android {

// Represents Android Chromium Application. This is a singleton and
// provides functions to request browser side actions, such as opening a
// settings page.
class ChromiumApplication {
 public:
  static bool RegisterBindings(JNIEnv* env);

  // Opens a protected content settings page, if available.
  static void OpenProtectedContentSettings();

 private:
  ChromiumApplication() {}
  ~ChromiumApplication() {}

  DISALLOW_COPY_AND_ASSIGN(ChromiumApplication);
};

}  // namespace android
}  // namespace chrome

#endif  // CHROME_BROWSER_ANDROID_CHROMIUM_APPLICATION_H_

// Copyright 2024 Normal Browser Authors. All rights reserved.
// ghost_profile_host.h — Browser-side Mojo host for GhostProfileProvider.
//
// Lives in the browser process. Serves profile data to renderer processes.
// File location in Chromium: content/browser/ghost_profile_host.h

#ifndef NORMAL_BROWSER_CHROMIUM_PATCHES_CONTENT_GHOST_PROFILE_HOST_H_
#define NORMAL_BROWSER_CHROMIUM_PATCHES_CONTENT_GHOST_PROFILE_HOST_H_

#include "content/common/ghost_profile.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace normal_browser {

class GhostProfileHost : public content::mojom::GhostProfileProvider {
 public:
  GhostProfileHost();
  ~GhostProfileHost() override;

  void BindReceiver(
      mojo::PendingReceiver<content::mojom::GhostProfileProvider> receiver);

  // content::mojom::GhostProfileProvider implementation:
  void GetProfile(GetProfileCallback callback) override;
  void RendererReady() override;

 private:
  // Converts the C++ DeviceProfile to the Mojo struct.
  content::mojom::GhostDeviceProfilePtr BuildMojoProfile();

  mojo::ReceiverSet<content::mojom::GhostProfileProvider> receivers_;
};

}  // namespace normal_browser

#endif  // NORMAL_BROWSER_CHROMIUM_PATCHES_CONTENT_GHOST_PROFILE_HOST_H_

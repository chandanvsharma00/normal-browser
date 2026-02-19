// Copyright 2024 Normal Browser Authors. All rights reserved.
// ghost_profile_client.cc — Renderer-side profile cache implementation.

#include "third_party/blink/renderer/core/ghost_profile_client.h"

#include "base/no_destructor.h"

namespace normal_browser {

GhostProfileClient::GhostProfileClient() = default;
GhostProfileClient::~GhostProfileClient() = default;

// static
GhostProfileClient* GhostProfileClient::Get() {
  // This is a process-wide singleton (NoDestructor). Every frame in this
  // renderer process — including cross-origin iframes via site isolation —
  // reads from the same RendererProfile instance.
  //
  // FPJS Pro's iframe isolation test creates a fresh <iframe> and compares
  // navigator.userAgent, screen.width, etc. between main frame and iframe.
  // Because both frames use this same singleton, they always return identical
  // values. Cross-origin iframes in a DIFFERENT renderer process get a
  // separate GhostProfileClient, but it's initialized from the same
  // browser-side DeviceProfileStore via Mojo, so values still match.
  static base::NoDestructor<GhostProfileClient> instance;
  return instance.get();
}

void GhostProfileClient::Initialize() {
  // In the real build, this would:
  // 1. Get a mojo::Remote<content::mojom::GhostProfileProvider>
  // 2. Call GetProfile() on it
  // 3. Copy all fields from the Mojo struct to profile_
  // 4. Set ready_ = true
  //
  // The mojo remote is obtained from the RenderFrame's BrowserInterfaceBroker:
  //
  //   mojo::Remote<content::mojom::GhostProfileProvider> provider;
  //   render_frame->GetBrowserInterfaceBroker().GetInterface(
  //       provider.BindNewPipeAndPassReceiver());
  //   provider->GetProfile(
  //       base::BindOnce(&GhostProfileClient::OnProfileReceived,
  //                      base::Unretained(this)));
  //
  // For now, initialization is handled by SetProfile() called from
  // the content layer during RenderFrame creation.

  ready_ = true;
}

int GhostProfileClient::GetCanvasMinMs() const {
  // Performance timing by SoC tier
  static const int kMin[] = {5, 12, 25, 50};
  int tier = profile_.soc_tier;
  if (tier < 0 || tier > 3) tier = 2;
  return kMin[tier];
}

int GhostProfileClient::GetCanvasMaxMs() const {
  static const int kMax[] = {12, 25, 50, 100};
  int tier = profile_.soc_tier;
  if (tier < 0 || tier > 3) tier = 2;
  return kMax[tier];
}

int GhostProfileClient::GetWebGLMinMs() const {
  static const int kMin[] = {3, 8, 15, 30};
  int tier = profile_.soc_tier;
  if (tier < 0 || tier > 3) tier = 2;
  return kMin[tier];
}

int GhostProfileClient::GetWebGLMaxMs() const {
  static const int kMax[] = {8, 15, 30, 60};
  int tier = profile_.soc_tier;
  if (tier < 0 || tier > 3) tier = 2;
  return kMax[tier];
}

}  // namespace normal_browser

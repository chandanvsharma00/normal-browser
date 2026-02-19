// Copyright 2024 Normal Browser Authors. All rights reserved.
// profile_store.cc — Thread-safe DeviceProfile singleton implementation.

#include "device_profile_generator/profile_store.h"

#include "base/no_destructor.h"
#include "base/synchronization/lock.h"
#include "device_profile_generator/profile_generator.h"

namespace normal_browser {

DeviceProfileStore::DeviceProfileStore() = default;
DeviceProfileStore::~DeviceProfileStore() = default;

// static
DeviceProfileStore* DeviceProfileStore::Get() {
  static base::NoDestructor<DeviceProfileStore> instance;
  return instance.get();
}

void DeviceProfileStore::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void DeviceProfileStore::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void DeviceProfileStore::NotifyProfileChanged(const DeviceProfile& profile) {
  for (auto& observer : observers_) {
    observer.OnProfileChanged(profile);
  }
}

const DeviceProfile& DeviceProfileStore::GetProfile() const {
  base::AutoLock lock(lock_);
  return profile_;
}

void DeviceProfileStore::GenerateNewProfile() {
  DeviceProfile new_profile = GenerateDeviceProfile(/* master_seed= */ 0);
  {
    base::AutoLock lock(lock_);
    profile_ = std::move(new_profile);
    has_profile_ = true;
  }
  NotifyProfileChanged(profile_);
}

void DeviceProfileStore::GenerateNewProfileWithSeed(uint64_t seed) {
  DeviceProfile new_profile = GenerateDeviceProfile(seed);
  {
    base::AutoLock lock(lock_);
    profile_ = std::move(new_profile);
    has_profile_ = true;
  }
  NotifyProfileChanged(profile_);
}

void DeviceProfileStore::SetProfile(DeviceProfile profile) {
  {
    base::AutoLock lock(lock_);
    profile_ = std::move(profile);
    has_profile_ = true;
  }
  NotifyProfileChanged(profile_);
}

void DeviceProfileStore::RotateSession() {
  {
    base::AutoLock lock(lock_);
    if (has_profile_) {
      normal_browser::RotateSessionSeeds(profile_);
    } else {
      return;
    }
  }
  NotifyProfileChanged(profile_);
}

bool DeviceProfileStore::HasProfile() const {
  base::AutoLock lock(lock_);
  return has_profile_;
}

bool DeviceProfileStore::IsGhostModeActive() const {
  base::AutoLock lock(lock_);
  return ghost_mode_active_;
}

void DeviceProfileStore::SetGhostModeActive(bool active) {
  bool needs_generate = false;
  {
    base::AutoLock lock(lock_);
    ghost_mode_active_ = active;
    if (active && !has_profile_) {
      needs_generate = true;
    }
  }
  // Notify observers of Ghost Mode change.
  for (auto& observer : observers_) {
    observer.OnGhostModeChanged(active);
  }
  if (needs_generate) {
    GenerateNewProfile();
  }
}

}  // namespace normal_browser

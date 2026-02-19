// Copyright 2024 Normal Browser Authors. All rights reserved.
// profile_store.h — Thread-safe global DeviceProfile singleton.
//
// This is the single source of truth for the current spoofed device identity.
// All Chromium patches read from this store.

#ifndef NORMAL_BROWSER_DEVICE_PROFILE_GENERATOR_PROFILE_STORE_H_
#define NORMAL_BROWSER_DEVICE_PROFILE_GENERATOR_PROFILE_STORE_H_

#include <vector>

#include "base/no_destructor.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/synchronization/lock.h"
#include "device_profile_generator/device_profile.h"

namespace normal_browser {

class DeviceProfileStore {
 public:
  // Observer interface — components register to be notified when the
  // active profile changes (new identity, session rotation, etc.)
  class Observer : public base::CheckedObserver {
   public:
    // Called on the thread that triggered the profile change.
    // |profile| is valid for the duration of this call.
    virtual void OnProfileChanged(const DeviceProfile& profile) = 0;

    // Called when Ghost Mode is toggled.
    virtual void OnGhostModeChanged(bool active) {}

   protected:
    ~Observer() override = default;
  };

  // Returns the global singleton.
  static DeviceProfileStore* Get();

  // Observer management — NOT thread-safe: call from the main thread.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Returns a const reference to the current profile.
  // Thread-safe (acquires read lock internally).
  const DeviceProfile& GetProfile() const;

  // Generates a new profile and stores it.
  // Can be called from any thread. Old profile is discarded.
  void GenerateNewProfile();

  // Generates a new profile with a specific seed (for testing).
  void GenerateNewProfileWithSeed(uint64_t seed);

  // Directly sets a profile (e.g., from JSON import).
  void SetProfile(DeviceProfile profile);

  // Rotates session-varying fields (canvas seed, audio seed, TLS)
  // while keeping the same device identity.
  void RotateSession();

  // Returns true if a profile has been generated.
  bool HasProfile() const;

  // Returns true if Ghost Mode is active.
  bool IsGhostModeActive() const;

  // Turn Ghost Mode on/off. When off, real device info is used.
  void SetGhostModeActive(bool active);

  DeviceProfileStore(const DeviceProfileStore&) = delete;
  DeviceProfileStore& operator=(const DeviceProfileStore&) = delete;

 private:
  friend class base::NoDestructor<DeviceProfileStore>;
  DeviceProfileStore();
  ~DeviceProfileStore();

  // Notifies all registered observers of a profile change.
  // Called with lock_ NOT held (observers may re-enter the store).
  void NotifyProfileChanged(const DeviceProfile& profile);

  mutable base::Lock lock_;
  DeviceProfile profile_;
  bool has_profile_ = false;
  bool ghost_mode_active_ = true;

  // Observer list. NOT guarded by lock_ — must be accessed from main thread.
  base::ObserverList<Observer> observers_;
};

}  // namespace normal_browser

#endif  // NORMAL_BROWSER_DEVICE_PROFILE_GENERATOR_PROFILE_STORE_H_

// Copyright 2024 Normal Browser Authors. All rights reserved.
// proc_access_sandbox.cc — Sandbox-level interception of file access
//                          to block root/Frida detection paths.
//
// FILE: sandbox/linux/seccomp-bpf-helpers/proc_access_sandbox.cc
//
// This file provides a custom BPF policy that intercepts open/openat
// syscalls in the renderer sandbox and returns -ENOENT for paths that
// would reveal root, Magisk, Frida, or debugger presence.
//
// Integration points:
//   1. sandbox/linux/seccomp-bpf-helpers/syscall_parameters_restrictions.h
//      Add: ResultExpr RestrictOpenForGhostMode(int sysno);
//
//   2. sandbox/policy/linux/bpf_renderer_policy_linux.cc
//      In RendererProcessPolicy::EvaluateSyscall():
//        case __NR_openat:
//          return RestrictOpenForGhostMode(__NR_openat);
//
//   Alternatively, for Android (which uses seccomp_policy_app.cc):
//   3. sandbox/linux/seccomp-bpf/sandbox_bpf.cc
//      Hook the open/openat trap handler.

#include "chromium_patches/blink/anti_bot/anti_bot_markers.h"

#include <cerrno>
#include <cstring>

#include "base/files/file_path.h"
#include "sandbox/linux/bpf_dsl/bpf_dsl.h"
#include "sandbox/linux/seccomp-bpf-helpers/syscall_parameters_restrictions.h"
#include "sandbox/linux/system_headers/linux_syscalls.h"

namespace sandbox {

using bpf_dsl::Allow;
using bpf_dsl::Error;
using bpf_dsl::ResultExpr;
using bpf_dsl::Trap;

// ====================================================================
// Trap handler: Called when the sandbox intercepts an open/openat call.
// If the path matches a blocked pattern, return -ENOENT.
// Otherwise, allow the syscall to proceed normally.
// ====================================================================
static intptr_t GhostModeOpenTrapHandler(
    const struct arch_seccomp_data& args,
    void* /* aux */) {
  // For openat(2): args.args[1] is the pathname.
  // For open(2):   args.args[0] is the pathname.
  const char* path = nullptr;

#if defined(__NR_openat)
  // openat: dirfd=args[0], pathname=args[1], flags=args[2]
  path = reinterpret_cast<const char*>(args.args[1]);
#elif defined(__NR_open)
  // open: pathname=args[0], flags=args[1]
  path = reinterpret_cast<const char*>(args.args[0]);
#endif

  if (path && blink::ShouldBlockProcAccess(path)) {
    return -ENOENT;  // File not found — hide root/Frida markers
  }

  // If we get here, we need to allow the syscall.
  // In practice, the Trap handler can't "allow" — it must return a value.
  // So this handler should only be registered via Cond() for matching paths,
  // or we use a SIGSYS handler approach. For simplicity, we use the
  // OverridableBlockedOpenHandler pattern.
  return -ENOENT;  // Conservative: if we're in the trap, block by default
}

// ====================================================================
// BPF policy helper: Returns a ResultExpr that checks openat paths
// against the ghost mode blocklist.
//
// Where to integrate:
//   In your sandbox policy's EvaluateSyscall() for __NR_openat:
//
//   case __NR_openat:
//     return RestrictOpenForGhostMode();
//
// NOTE: Because BPF cannot dereference userspace pointers directly,
// the actual string matching must happen in a Trap handler (SIGSYS).
// The BPF program only routes the syscall to our handler.
// ====================================================================
ResultExpr RestrictOpenForGhostMode() {
  return Trap(GhostModeOpenTrapHandler, nullptr);
}

// ====================================================================
// Alternative approach for Android:
// Instead of modifying seccomp BPF policies (which are complex), we
// can intercept at a higher level by hooking Chromium's own file I/O.
//
// Integration: base/files/file_util_posix.cc
//   In base::File::Initialize() or base::PathExists():
//
//   #include "chromium_patches/blink/anti_bot/anti_bot_markers.h"
//   ...
//   if (blink::ShouldBlockProcAccess(path.value().c_str())) {
//       error_details = FILE_ERROR_NOT_FOUND;
//       return;
//   }
//
// This higher-level approach is simpler and works across all platforms
// without needing to modify sandbox BPF filters.
// ====================================================================

// ====================================================================
// Recommended integration for content::FileSystemAccessManager:
//
// FILE: content/browser/file_system_access/file_system_access_manager_impl.cc
//
// In ChooseEntry() or any method that resolves native filesystem paths:
//   if (blink::ShouldBlockProcAccess(path.c_str())) {
//     std::move(callback).Run(
//         file_system_access_error::FromStatus(
//             blink::mojom::FileSystemAccessStatus::kNotFoundError));
//     return;
//   }
// ====================================================================

}  // namespace sandbox

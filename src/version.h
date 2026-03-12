#ifndef PUNCH_VERSION_H_
#define PUNCH_VERSION_H_

#include <cstdint>
#include <format>
#include <string>

#include "version.inc"

uint32_t GetVersionInt(int major = PUNCH_VERSION_MAJOR,
                       int minor = PUNCH_VERSION_MINOR,
                       int patch = PUNCH_VERSION_PATCH) {
  return major * 1000000 + minor * 1000 + patch;
}

std::string GetVersionString(int major = PUNCH_VERSION_MAJOR,
                             int minor = PUNCH_VERSION_MINOR,
                             int patch = PUNCH_VERSION_PATCH,
                             std::string_view postfix = PUNCH_VERSION_POSTFIX) {
  if (postfix.empty()) {
    return std::format("{}.{}.{}", major, minor, patch);
  }
  return std::format("{}.{}.{}-{}", major, minor, patch, postfix);
}

#endif  // PUNCH_VERSION_H_

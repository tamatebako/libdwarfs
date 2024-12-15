/**
 *
 * Copyright (c) 2024 [Ribose Inc](https://www.ribose.com).
 * All rights reserved.
 * This file is a part of tebako
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "tebako-pch-pp.h"
#include "tebako-package-descriptor.h"

namespace tebako {

const char* package_descriptor::signature = "TAMATEBAKO";

// Constructor for deserialization
package_descriptor::package_descriptor(const std::vector<char>& buffer)
{
  size_t offset = 0;

  auto read_from_buffer = [&buffer, &offset](void* data, size_t size) {
    if (offset + size > buffer.size()) {
      throw std::out_of_range("Buffer too short for deserialization");
    }
    std::memcpy(data, buffer.data() + offset, size);
    offset += size;
  };

  size_t signature_length = std::strlen(signature);
  if (offset + signature_length > buffer.size() ||
      std::memcmp(buffer.data() + offset, signature, signature_length) != 0) {
    throw std::invalid_argument("Invalid or missing signature");
  }
  offset += signature_length;

  // Read fixed-size fields
  read_from_buffer(&ruby_version_major, sizeof(ruby_version_major));
  read_from_buffer(&ruby_version_minor, sizeof(ruby_version_minor));
  read_from_buffer(&ruby_version_patch, sizeof(ruby_version_patch));
  read_from_buffer(&tebako_version_major, sizeof(tebako_version_major));
  read_from_buffer(&tebako_version_minor, sizeof(tebako_version_minor));
  read_from_buffer(&tebako_version_patch, sizeof(tebako_version_patch));

  // Read mount_point size and content
  uint16_t mount_point_size;
  read_from_buffer(&mount_point_size, sizeof(mount_point_size));
  if (offset + mount_point_size > buffer.size()) {
    throw std::out_of_range("Buffer too short for mount_point");
  }
  mount_point.resize(mount_point_size);
  read_from_buffer(mount_point.data(), mount_point_size);

  // Read entry_point size and content
  uint16_t entry_point_size;
  read_from_buffer(&entry_point_size, sizeof(entry_point_size));
  if (offset + entry_point_size > buffer.size()) {
    throw std::out_of_range("Buffer too short for entry_point");
  }
  entry_point.resize(entry_point_size);
  read_from_buffer(entry_point.data(), entry_point_size);

  // Read cwd presence flag and content if present
  char cwd_present;
  read_from_buffer(&cwd_present, sizeof(cwd_present));
  if (cwd_present) {
    uint16_t cwd_size;
    read_from_buffer(&cwd_size, sizeof(cwd_size));
    if (offset + cwd_size > buffer.size()) {
      throw std::out_of_range("Buffer too short for cwd");
    }
    std::string cwd_value(cwd_size, '\0');
    read_from_buffer(cwd_value.data(), cwd_size);
    cwd = cwd_value;
  }
  else {
    cwd.reset();
  }
}

// Constructor from version strings and other parameters
package_descriptor::package_descriptor(const std::string& ruby_version,
                                       const std::string& tebako_version,
                                       const std::string& mount_point,
                                       const std::string& entry_point,
                                       const std::optional<std::string>& cwd)
    : mount_point(mount_point), entry_point(entry_point), cwd(cwd)
{
  auto parse_version = [](const std::string& version, uint16_t& major, uint16_t& minor, uint16_t& patch) {
    std::stringstream ss(version);
    char dot;
    if (!(ss >> major >> dot >> minor >> dot >> patch) || dot != '.') {
      throw std::invalid_argument("Invalid version format: " + version);
    }
  };

  parse_version(ruby_version, ruby_version_major, ruby_version_minor, ruby_version_patch);
  parse_version(tebako_version, tebako_version_major, tebako_version_minor, tebako_version_patch);
}

// Serialize the object to a binary format
std::vector<char> package_descriptor::serialize() const
{
  std::vector<char> buffer;
  auto append_to_buffer = [&buffer](const void* data, size_t size) {
    const char* bytes = static_cast<const char*>(data);
    buffer.insert(buffer.end(), bytes, bytes + size);
  };

  append_to_buffer(signature, std::strlen(signature));

  // Append fixed-size fields
  append_to_buffer(&ruby_version_major, sizeof(ruby_version_major));
  append_to_buffer(&ruby_version_minor, sizeof(ruby_version_minor));
  append_to_buffer(&ruby_version_patch, sizeof(ruby_version_patch));
  append_to_buffer(&tebako_version_major, sizeof(tebako_version_major));
  append_to_buffer(&tebako_version_minor, sizeof(tebako_version_minor));
  append_to_buffer(&tebako_version_patch, sizeof(tebako_version_patch));

  // Append mount_point size and content
  uint16_t mount_point_size = static_cast<uint16_t>(mount_point.size());
  append_to_buffer(&mount_point_size, sizeof(mount_point_size));
  append_to_buffer(mount_point.data(), mount_point_size);

  // Append entry_point size and content
  uint16_t entry_point_size = static_cast<uint16_t>(entry_point.size());
  append_to_buffer(&entry_point_size, sizeof(entry_point_size));
  append_to_buffer(entry_point.data(), entry_point_size);

  // Append cwd presence flag and content if present
  char cwd_present = cwd.has_value() ? 1 : 0;
  append_to_buffer(&cwd_present, sizeof(cwd_present));
  if (cwd) {
    uint16_t cwd_size = static_cast<uint16_t>(cwd->size());
    append_to_buffer(&cwd_size, sizeof(cwd_size));
    append_to_buffer(cwd->data(), cwd_size);
  }

  return buffer;
}

}  // namespace tebako

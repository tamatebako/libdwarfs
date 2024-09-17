/**
 *
 * Copyright (c) 2024, [Ribose Inc](https://www.ribose.com).
 * All rights reserved.
 * This file is a part of tebako (libdwarfs-wr)
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
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "tests.h"
#include <tebako-mnt.h>

namespace tebako {

class TebakoMountTableTests : public ::testing::Test {
 protected:
  sync_tebako_mount_table& mount_table =
      sync_tebako_mount_table::get_tebako_mount_table();

  void SetUp() override { mount_table.clear(); }

  void TearDown() override { mount_table.clear(); }
};

TEST_F(TebakoMountTableTests, CheckPathExists)
{
  std::string path = "/path1";
  std::string mount = "mount1";
  mount_table.insert(path, mount);

  EXPECT_TRUE(mount_table.check(path));
}

TEST_F(TebakoMountTableTests, CheckPathDoesNotExist)
{
  std::string path = "/path2";

  EXPECT_FALSE(mount_table.check(path));
}

TEST_F(TebakoMountTableTests, ErasePath)
{
  std::string path = "/path3";
  std::string mount = "mount3";
  mount_table.insert(path, mount);
  mount_table.erase(path);

  EXPECT_FALSE(mount_table.check(path));
}

TEST_F(TebakoMountTableTests, GetExistingPath)
{
  std::string path = "/path4";
  std::string mount = "mount4";
  mount_table.insert(path, mount);

  std::optional<std::string> result = mount_table.get(path);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), mount);
}

TEST_F(TebakoMountTableTests, GetNonExistingPath)
{
  std::string path = "/path5";

  std::optional<std::string> result = mount_table.get(path);
  EXPECT_FALSE(result.has_value());
}

TEST_F(TebakoMountTableTests, InsertPath)
{
  std::string path = "/path6";
  std::string mount = "mount6";

  EXPECT_TRUE(mount_table.insert(path, mount));
  EXPECT_TRUE(mount_table.check(path));
}

TEST_F(TebakoMountTableTests, InsertDuplicatePath)
{
  std::string path = "/path7";
  std::string mount1 = "mount7";
  std::string mount2 = "mount8";

  EXPECT_TRUE(mount_table.insert(path, mount1));
  EXPECT_FALSE(mount_table.insert(
      path, mount2));  // Insertion should fail for duplicate path
  std::optional<std::string> result = mount_table.get(path);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), mount1);  // Original mount should remain
}

TEST_F(TebakoMountTableTests, ConcurrentInsertAndCheck)
{
  const int num_threads = 10;
  const int num_operations = 100;
  std::vector<std::thread> threads;

  auto insert_task = [this](int id) {
    for (int i = 0; i < num_operations; ++i) {
      std::string path = "/path" + std::to_string(id) + "_" + std::to_string(i);
      std::string mount =
          "mount" + std::to_string(id) + "_" + std::to_string(i);
      mount_table.insert(path, mount);
      mount_table.check(path);
    }
  };

  // Start insert threads
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(insert_task, i);
  }

  // Join all threads
  for (auto& thread : threads) {
    thread.join();
  }

  // Verify that all paths were inserted
  for (int i = 0; i < num_threads; ++i) {
    for (int j = 0; j < num_operations; ++j) {
      std::string path = "/path" + std::to_string(i) + "_" + std::to_string(j);
      EXPECT_TRUE(mount_table.check(path));
    }
  }
}

TEST_F(TebakoMountTableTests, ConcurrentInsertAndErase)
{
  const int num_threads = 10;
  const int num_operations = 100;
  std::vector<std::thread> threads;

  auto insert_task = [this](int id) {
    for (int i = 0; i < num_operations; ++i) {
      std::string path = "/path" + std::to_string(id) + "_" + std::to_string(i);
      std::string mount =
          "mount" + std::to_string(id) + "_" + std::to_string(i);
      mount_table.insert(path, mount);
      mount_table.erase(path);
    }
  };

  // Start insert threads
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(insert_task, i);
  }

  // Join all threads
  for (auto& thread : threads) {
    thread.join();
  }

  // Verify that all paths were erased
  for (int i = 0; i < num_threads; ++i) {
    for (int j = 0; j < num_operations; ++j) {
      std::string path = "/path" + std::to_string(i) + "_" + std::to_string(j);
      EXPECT_FALSE(mount_table.check(path));
    }
  }
}

}  // namespace tebako

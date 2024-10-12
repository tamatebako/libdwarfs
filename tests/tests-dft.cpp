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

struct tebako_dirent;

#include <tebako-dfs.h>
#include <tebako-dft.h>

namespace tebako {

class SyncTebakoMemfsTableTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up code if needed
    }

    void TearDown() override {
        // Clean up code if needed
        sync_tebako_memfs_table::get_tebako_memfs_table().clear();
    }
};

TEST_F(SyncTebakoMemfsTableTest, get_tebako_memfs_table) {
    auto& table1 = sync_tebako_memfs_table::get_tebako_memfs_table();
    auto& table2 = sync_tebako_memfs_table::get_tebako_memfs_table();
    EXPECT_EQ(&table1, &table2); // Ensure that the same instance is returned
}

TEST_F(SyncTebakoMemfsTableTest, check_existing_index) {
    auto& table = sync_tebako_memfs_table::get_tebako_memfs_table();
    table.insert(1, new memfs("AAA", 3));

    EXPECT_TRUE(table.check(1)); // Check if the entry exists
}

TEST_F(SyncTebakoMemfsTableTest, check_non_existing_index) {
    auto& table = sync_tebako_memfs_table::get_tebako_memfs_table();
    EXPECT_FALSE(table.check(999)); // Check if a non-existing entry does not exist
}

TEST_F(SyncTebakoMemfsTableTest, clear_table) {
    auto& table = sync_tebako_memfs_table::get_tebako_memfs_table();
    table.insert(2, new memfs("BBB", 3));

    table.clear(); // Clear the table

    EXPECT_FALSE(table.check(1)); // Ensure the entry is removed
}

static void add_entries(sync_tebako_memfs_table& table, int start, int end) {
    for (int i = start; i < end; ++i) {
      table.insert(100+i, new memfs("III", 3));
    }
}

static void check_entries(sync_tebako_memfs_table& table, int start, int end, std::atomic<int>& success_count) {
    for (int i = start; i < end; ++i) {
        if (table.check(i)) {
            success_count++;
        }
    }
}

TEST_F(SyncTebakoMemfsTableTest, concurrent_add_and_check) {
    auto& table = sync_tebako_memfs_table::get_tebako_memfs_table();
    std::atomic<int> success_count(0);

    std::thread t1(add_entries, std::ref(table), 0, 500);
    std::thread t2(add_entries, std::ref(table), 500, 1000);
    std::thread t3(check_entries, std::ref(table), 0, 1000, std::ref(success_count));
    t1.join();
    t2.join();

    std::thread t4(check_entries, std::ref(table), 0, 1000, std::ref(success_count));

    t3.join();
    t4.join();

    EXPECT_GT(success_count, 1000);
}

TEST_F(SyncTebakoMemfsTableTest, concurrent_clear) {
    auto& table = sync_tebako_memfs_table::get_tebako_memfs_table();
    std::atomic<int> success_count(0);

    // Add entries before starting the test
    add_entries(table, 0, 1000);

    std::thread t1(check_entries, std::ref(table), 0, 1000, std::ref(success_count));
    std::thread t2(&sync_tebako_memfs_table::clear, &table);
    t1.join();
    std::thread t3(&sync_tebako_memfs_table::clear, &table);

    t2.join();
    t3.join();
    EXPECT_LE(success_count, 1000); // Ensure all entries were checked before clear
    EXPECT_FALSE(table.check(345)); // Ensure the table is cleared
}

} // namespace tebako

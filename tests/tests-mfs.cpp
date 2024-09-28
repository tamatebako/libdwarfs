#include "tests.h"
#include <tebako-mfs.h>

namespace tebako {

class MfsTests : public ::testing::Test {
 protected:
  const size_t page_size = sysconf(_SC_PAGESIZE);
  std::vector<uint8_t> memory;
  mfs* mfs_instance;

  void SetUp() override
  {
    memory.resize(page_size * 2);  // Allocate memory for two pages
    mfs_instance = new mfs(memory.data(), page_size);
  }

  void TearDown() override { delete mfs_instance; }
};

TEST_F(MfsTests, lock_and_release)
{
  auto ec = mfs_instance->lock(0, page_size);
  EXPECT_FALSE(ec) << "Expected no error, but got: " << ec.message();

  ec = mfs_instance->release(0, page_size);
  EXPECT_FALSE(ec) << "Expected no error, but got: " << ec.message();
}

}  // namespace tebako
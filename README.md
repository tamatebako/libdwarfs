#  libdwarfs wrapper 

This is libdwarfs-wr  - a wrapper for https://github.com/mhx/dwarfs core library that provides the following features: 
* C interface (as opposed to dwarfs C++ API)
* fd (file descriptor) addressing above dwarfs inode implementation


### CMake project options

* **WITH_TESTS**, default:ON      -- If this option is ON, build script looks for Google test,  installs INCBIN, and build google tests and static test application.  
* **WITH_LINK_TEST**, default:ON  -- If this option is ON,  symbolik/ hard link tests are enabled.
* **USE_TEMP_FS**, default:OFF    -- If this option is ON, the data for test file system is created under /tmp.  Otherwise in-source location is used.
  
  






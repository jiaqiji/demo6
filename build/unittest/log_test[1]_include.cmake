if(EXISTS "/home/dgk/MiniOB/miniob/build/unittest/log_test[1]_tests.cmake")
  include("/home/dgk/MiniOB/miniob/build/unittest/log_test[1]_tests.cmake")
else()
  add_test(log_test_NOT_BUILT log_test_NOT_BUILT)
endif()

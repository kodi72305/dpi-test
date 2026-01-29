if(CONFIG_ZMK_BEHAVIOR_TRACKBALL_DPI)
  add_subdirectory(src/behaviors)
endif()

zephyr_include_directories(include)

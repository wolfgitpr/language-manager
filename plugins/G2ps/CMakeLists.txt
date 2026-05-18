get_filename_component(_dir_name ${CMAKE_CURRENT_LIST_DIR} NAME)
set(CURRENT_PLUGIN_CATEGORY ${_dir_name})

add_subdirectory(Common)
add_subdirectory(LstmG2p)
add_subdirectory(ChainG2p)
add_subdirectory(MandarinG2p)
add_subdirectory(CantoneseG2p)
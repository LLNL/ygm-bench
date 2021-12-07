cmake_minimum_required(VERSION 3.14) 
project(ygm-bench         
	VERSION 0.1         
	DESCRIPTION "YGM Benchmarks"         
	LANGUAGES CXX) 

set(CMAKE_CXX_STANDARD 17) 

include(FetchContent)

find_package(ygm CONFIG)
if(NOT ygm_FOUND)
	FetchContent_Declare(
	ygm
	GIT_REPOSITORY https://github.com/LLNL/ygm
	GIT_TAG        develop         
	)         
	set(JUST_INSTALL_YGM ON)
	set(YGM_INSTALL ON)
	FetchContent_MakeAvailable(ygm)
	message(STATUS "Cloned ygm dependency " ${ygm_SOURCE_DIR})
else()
	message(STATUS "Found ygm dependency " ${ygm_DIR})
endif()

function(setup_ygm_target exe_name)
	target_link_libraries(${exe_name} PRIVATE ygm::ygm)
endfunction()

add_subdirectory(src)

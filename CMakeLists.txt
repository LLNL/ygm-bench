# Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
# Project Developers. See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.14) 
project(ygm-bench         
	VERSION 0.1         
	DESCRIPTION "YGM Benchmarks"         
	LANGUAGES CXX) 

set(CMAKE_CXX_STANDARD 17) 

include(FetchContent)

#
#  MPI
find_package(MPI)

# 
#  cereal 
find_package(cereal QUIET) 
if (NOT cereal_FOUND)     
	set(JUST_INSTALL_CEREAL TRUE)
	FetchContent_Declare(cereal             
		GIT_REPOSITORY https://github.com/USCiLab/cereal.git             
		GIT_TAG master             
		)     
	FetchContent_MakeAvailable(cereal) 
endif ()

#
#  ygm
find_package(ygm CONFIG)
if(NOT ygm_FOUND)
	if(DEFINED ENV{YGM_BENCH_YGM_REPO})
		set(YGM_REPO $ENV{YGM_BENCH_YGM_REPO})
	else()
		set(YGM_REPO "https://github.com/steiltre/ygm.git")
	endif()
	if(DEFINED ENV{YGM_BENCH_YGM_TAG})
		set(YGM_TAG $ENV{YGM_BENCH_YGM_TAG})
	else()
		set(YGM_TAG "feature/multi_listener_routing")
	endif()

	FetchContent_Declare(
	ygm
	GIT_REPOSITORY 	${YGM_REPO}
	GIT_TAG 		${YGM_TAG}
	)         
	set(JUST_INSTALL_YGM ON)
	FetchContent_MakeAvailable(ygm)
	message(STATUS "Cloned ygm dependency " ${ygm_SOURCE_DIR})
else()
	message(STATUS "Found ygm dependency " ${ygm_DIR})
endif()

function(setup_ygm_target exe_name)
	add_executable(${exe_name} ${exe_name}.cpp)
	target_link_libraries(${exe_name} PRIVATE ygm::ygm)
	target_include_directories(${exe_name} PRIVATE "${PROJECT_SOURCE_DIR}/include")
endfunction()

function(setup_mpi_target exe_name)
	add_executable(${exe_name} ${exe_name}.cpp)
	target_link_libraries(${exe_name} PRIVATE MPI::MPI_CXX)
	target_include_directories(${exe_name} PRIVATE "${PROJECT_SOURCE_DIR}/include")
endfunction()

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
	message(STATUS "CMAKE_BUILD_TYPE is set as Release")
endif ()

add_subdirectory(src)

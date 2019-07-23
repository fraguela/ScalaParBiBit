cmake_minimum_required( VERSION 2.8.12.2 )

project( parbibit )

if(NOT WIN32)
  string(ASCII 27 Esc)
  set(ColourReset "${Esc}[m")
  set(Red         "${Esc}[31m")
  set(Blue        "${Esc}[34m")
endif()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel"
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )

find_package( MPI )

if ( MPI_CXX_FOUND )
	set( CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER} )
	add_compile_options( ${MPI_CXX_COMPILE_FLAGS} )
	include_directories( ${MPI_CXX_INCLUDE_PATH} )
	set( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${MPI_CXX_LINK_FLAGS}" )
	link_libraries( ${MPI_CXX_LIBRARIES} )
else ( MPI_CXX_FOUND )
	message(STATUS "==============================================================")
	message(STATUS )
	message(STATUS "No MPI compiler found. Perhaps you want to set one explicitly?")
	message(STATUS "To override the default compiler, set the environment variable")
	message(STATUS "     export CXX=/full/path/to/mpic++")
	message(STATUS "in bash or ksh or sh.  In csh or tcsh, use:")
	message(STATUS "     setenv CXX /full/path/to/mpic++")
	message(STATUS "before running the cmake command.")
	message(STATUS )
	message(STATUS "==============================================================")
endif( MPI_CXX_FOUND )

set( PARBIBIT_DEP_SOURCES Utils.cpp Options.cpp FileWorker.cpp InputMatrix.cpp Bicluster.cpp ServerState.cpp ThreadHandler.cpp allocator.cpp  murmurhash.cpp arff/arff_attr.cpp	arff/arff_data.cpp	arff/arff_instance.cpp	arff/arff_lexer.cpp	arff/arff_parser.cpp arff/arff_scanner.cpp arff/arff_token.cpp arff/arff_utils.cpp arff/arff_value.cpp )

add_library( parbibit_deps STATIC ${PARBIBIT_DEP_SOURCES} )
target_include_directories( parbibit_deps PRIVATE arff )

add_executable( ScalaParBiBit main.cpp )

target_include_directories( ScalaParBiBit PRIVATE arff )
target_link_libraries( ScalaParBiBit PRIVATE parbibit_deps z pthread )

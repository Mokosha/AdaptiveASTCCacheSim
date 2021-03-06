CMAKE_MINIMUM_REQUIRED(VERSION 3.2 FATAL_ERROR)
PROJECT(AdaptiveASTCCacheSim CXX C)

SET(AdaptiveASTCCacheSim_MAJOR_VERSION 0)
SET(AdaptiveASTCCacheSim_MINOR_VERSION 0)
SET(AdaptiveASTCCacheSim_PATCH_VERSION 1)
SET(AdaptiveASTCCacheSim_VERSION
  ${AdaptiveASTCCacheSim_MAJOR_VERSION}.${AdaptiveASTCCacheSim_MINOR_VERSION}.${AdaptiveASTCCacheSim_PATCH_VERSION})
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

OPTION(TREAT_WARNINGS_AS_ERRORS "Treat compiler warnings as errors. We use the highest warnings levels for compilers." OFF)

IF(MSVC)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

ELSEIF(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)

  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fms-extensions")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fms-extensions")

ENDIF(MSVC)

IF(TREAT_WARNINGS_AS_ERRORS)
  IF(MSVC)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /WX")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
  ELSEIF(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
  ENDIF(MSVC)
ENDIF(TREAT_WARNINGS_AS_ERRORS)
SET(CMAKE_CXX_STANDARD 11)

SET(SOURCES
  main.cpp
  texture.cpp
  access_pattern.cpp
)

SET(HEADERS
  cache.h
  access_pattern.h
  texture.h
)

ADD_EXECUTABLE(cache-sim ${HEADERS} ${SOURCES})
ADD_EXECUTABLE(split split.cpp)
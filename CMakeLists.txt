cmake_minimum_required(VERSION 3.0.0)

project(ActWebSocketOverlay C CXX)

ADD_DEFINITIONS(
  -DBOOST_ALL_NO_LIB
  -D_CRT_SECURE_NO_WARNINGS
  -D_WIN32_WINNT=0x0601
  -DWIN32_LEAN_AND_MEAN
  -DUNICODE
  -D_UNICODE
  -DNOMINMAX
)
set(Boost_ADDITIONAL_VERSIONS
	"1.63" "1.63.0" 
	"1.64" "1.64.0" 
	${BOOST_ADDITIONAL_VERSION}
)
set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
##########################################################################################
# boost build step
##########################################################################################
#
# 1. download https://dl.bintray.com/boostorg/release/1.64.0/source/:boost_1_64_0.7z
# 2. unzip
# 3. bootstrap.bat
# 4. b2 --stagedir=stage   variant=debug,release address-model=32 threading=multi link=static runtime-link=shared --prefix=c:/boost32 install
# 5. b2 --stagedir=stage64 variant=debug,release address-model=64 threading=multi link=static runtime-link=shared --prefix=c:/boost64 install

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(address_model "64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(address_model "32")
endif()

set(BOOST_ROOT "c:/boost${address_model}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib/${address_model})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib/${address_model})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin/${address_model})

find_package(Boost REQUIRED COMPONENTS system filesystem )

ADD_SUBDIRECTORY(external)
ADD_SUBDIRECTORY(src)

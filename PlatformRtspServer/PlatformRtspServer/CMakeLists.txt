#cmake file for project association #±íÊ¾×¢ÊÍ 
#author:	chenjx
#created:	2014-09-10
PROJECT(RtspServer)

MESSAGE(STATUS "Project: RtspServer")
MESSAGE(STATUS "Project Directory: ${RtspServer_SOURCE_DIR}")

INCLUDE_DIRECTORIES(${poeU5802_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${RtspServer_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${Onvif_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${Hicommon_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${Audio2_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${Video2_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${Transport_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${HiGpioReg_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${Sysctrl_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${BroadcasDiscover_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${GVAPDevice_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${Iniparser_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${Record_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${Interface_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${GS_DEVICE_SDK_SOURCE_DIR}/include)

AUX_SOURCE_DIRECTORY(./src DIR_SRCS)

ADD_LIBRARY(RtspServer_s STATIC ${DIR_SRCS})
#ADD_LIBRARY(RtspServer SHARED ${DIR_SRCS})

SET_TARGET_PROPERTIES(RtspServer_s PROPERTIES OUTPUT_NAME "RtspServer")

#SET_TARGET_PROPERTIES(RtspServer PROPERTIES LIBRARY_OUTPUT_DIRECTORY  "${poeU5802_SOURCE_DIR}/__install/lib")
SET_TARGET_PROPERTIES(RtspServer_s PROPERTIES ARCHIVE_OUTPUT_DIRECTORY  "${poeU5802_SOURCE_DIR}/__install/lib")

#INSTALL(FILES 
#		${deviceSDk_SOURCE_DIR}/include/device_info.h  
#		${deviceSDk_SOURCE_DIR}/include/device_sdk.h  
#		${deviceSDk_SOURCE_DIR}/include/SDK_Defs.h 
#		DESTINATION "${my51c_platform_sdk_SOURCE_DIR}/__install/deviceSDk/include")

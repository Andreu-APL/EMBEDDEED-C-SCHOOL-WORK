INCLUDE(CMakeForceCompiler)
SET(CMAKE_SYSTEM_NAME Generic)
CMAKE_MINIMUM_REQUIRED (VERSION 3.13)
project(robotsito C CXX ASM)

# Absolute Path to your SDK
SET(SDK_DIR "/Users/andre_apl/Coding/KL25Z/SDK_2_2_0_FRDM-KL25Z")

ENABLE_LANGUAGE(ASM)
SET(ProjDirPath ${CMAKE_CURRENT_SOURCE_DIR})

# --- COMPILER FLAGS ---
SET(CMAKE_ASM_FLAGS_DEBUG "${CMAKE_ASM_FLAGS_DEBUG} -DDEBUG -D__STARTUP_CLEAR_BSS -g -flto -mcpu=cortex-m0plus -Wall -mfloat-abi=soft -mthumb -fno-common -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -mapcs -std=gnu99")
SET(CMAKE_ASM_FLAGS_RELEASE "${CMAKE_ASM_FLAGS_RELEASE} -D__STARTUP_CLEAR_BSS -flto -mcpu=cortex-m0plus -Wall -mfloat-abi=soft -mthumb -fno-common -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -mapcs -std=gnu99")

SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -DCPU_MKL25Z128VLK4 -DPRINTF_FLOAT_ENABLE=0 -DSCANF_FLOAT_ENABLE=0 -DPRINTF_ADVANCED_ENABLE=0 -DSCANF_ADVANCED_ENABLE=0 -DFRDM_KL25Z -DFREEDOM -g -O0 -flto -mcpu=cortex-m0plus -Wall -mfloat-abi=soft -mthumb -MMD -MP -fno-common -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -mapcs -std=gnu99")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG -DCPU_MKL25Z128VLK4 -DPRINTF_FLOAT_ENABLE=0 -DSCANF_FLOAT_ENABLE=0 -DPRINTF_ADVANCED_ENABLE=0 -DSCANF_ADVANCED_ENABLE=0 -DFRDM_KL25Z -DFREEDOM -Os -flto -mcpu=cortex-m0plus -Wall -mfloat-abi=soft -mthumb -MMD -MP -fno-common -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -mapcs -std=gnu99")

SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -DCPU_MKL25Z128VLK4 -g -O0 -mcpu=cortex-m0plus -Wall -mfloat-abi=soft -mthumb -MMD -MP -fno-common -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -mapcs -fno-rtti -fno-exceptions")
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DCPU_MKL25Z128VLK4 -Os -mcpu=cortex-m0plus -Wall -mfloat-abi=soft -mthumb -MMD -MP -fno-common -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -mapcs -fno-rtti -fno-exceptions")

SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto -mcpu=cortex-m0plus -Wall -mfloat-abi=soft --specs=nano.specs --specs=nosys.specs -fno-common -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -mthumb -mapcs -Xlinker --gc-sections -Xlinker -static -Xlinker -z -Xlinker muldefs")
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -g -flto -mcpu=cortex-m0plus -Wall -mfloat-abi=soft --specs=nano.specs --specs=nosys.specs -fno-common -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -mthumb -mapcs -Xlinker --gc-sections -Xlinker -static -Xlinker -z -Xlinker muldefs")

# --- INCLUDE DIRECTORIES ---
include_directories(${SDK_DIR}/CMSIS/Include)
include_directories(${SDK_DIR}/devices)
include_directories(${ProjDirPath}/..)
include_directories(${SDK_DIR}/devices/MKL25Z4/drivers)
include_directories(${SDK_DIR}/devices/MKL25Z4/utilities)
include_directories(${SDK_DIR}/devices/MKL25Z4)

# --- EXECUTABLE TARGET ---
add_executable(robotsito.elf 
"${SDK_DIR}/devices/MKL25Z4/gcc/startup_MKL25Z4.S"
"${SDK_DIR}/devices/MKL25Z4/drivers/fsl_lpsci.h"
"${SDK_DIR}/devices/MKL25Z4/drivers/fsl_lpsci.c"
"${SDK_DIR}/devices/MKL25Z4/drivers/fsl_uart.h"
"${SDK_DIR}/devices/MKL25Z4/drivers/fsl_uart.c"
"${SDK_DIR}/devices/MKL25Z4/drivers/fsl_port.h"
"${ProjDirPath}/../robotsito.c" 
"${ProjDirPath}/../pin_mux.c"
"${ProjDirPath}/../pin_mux.h"
"${ProjDirPath}/../board.c"
"${ProjDirPath}/../board.h"
"${SDK_DIR}/devices/MKL25Z4/drivers/fsl_common.h"
"${SDK_DIR}/devices/MKL25Z4/drivers/fsl_common.c"
"${SDK_DIR}/devices/MKL25Z4/utilities/fsl_debug_console.c"
"${SDK_DIR}/devices/MKL25Z4/utilities/fsl_debug_console.h"
"${SDK_DIR}/devices/MKL25Z4/system_MKL25Z4.c"
"${SDK_DIR}/devices/MKL25Z4/system_MKL25Z4.h"
"${SDK_DIR}/devices/MKL25Z4/drivers/fsl_gpio.h"
"${SDK_DIR}/devices/MKL25Z4/drivers/fsl_gpio.c"
"${ProjDirPath}/../clock_config.c"
"${ProjDirPath}/../clock_config.h"
"${SDK_DIR}/devices/MKL25Z4/drivers/fsl_smc.h"
"${SDK_DIR}/devices/MKL25Z4/drivers/fsl_smc.c"
"${SDK_DIR}/devices/MKL25Z4/drivers/fsl_flash.c"
"${SDK_DIR}/devices/MKL25Z4/drivers/fsl_flash.h"
"${SDK_DIR}/devices/MKL25Z4/drivers/fsl_clock.h"
"${SDK_DIR}/devices/MKL25Z4/drivers/fsl_clock.c"
)

# FIXED LINKER SCRIPT PATH (Points to SDK instead of local folder)
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -T${SDK_DIR}/devices/MKL25Z4/gcc/MKL25Z128xxx4_flash.ld -static")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -T${SDK_DIR}/devices/MKL25Z4/gcc/MKL25Z128xxx4_flash.ld -static")

# --- LINK LIBRARIES ---
TARGET_LINK_LIBRARIES(robotsito.elf -Wl,--start-group)
target_link_libraries(robotsito.elf debug m)
target_link_libraries(robotsito.elf debug c)
target_link_libraries(robotsito.elf debug gcc)
target_link_libraries(robotsito.elf debug nosys)
target_link_libraries(robotsito.elf optimized m)
target_link_libraries(robotsito.elf optimized c)
target_link_libraries(robotsito.elf optimized gcc)
target_link_libraries(robotsito.elf optimized nosys)
TARGET_LINK_LIBRARIES(robotsito.elf -Wl,--end-group)

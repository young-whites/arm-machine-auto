################################################################################
# 自动生成的文件。不要编辑！
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../rt-thread/libcpu/arm/cortex-m3/cpuport.c 

S_UPPER_SRCS += \
../rt-thread/libcpu/arm/cortex-m3/context_gcc.S 

OBJS += \
./rt-thread/libcpu/arm/cortex-m3/context_gcc.o \
./rt-thread/libcpu/arm/cortex-m3/cpuport.o 

S_UPPER_DEPS += \
./rt-thread/libcpu/arm/cortex-m3/context_gcc.d 

C_DEPS += \
./rt-thread/libcpu/arm/cortex-m3/cpuport.d 


# Each subdirectory must supply rules for building sources it contributes
rt-thread/libcpu/arm/cortex-m3/%.o: ../rt-thread/libcpu/arm/cortex-m3/%.S
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -O0 -ffunction-sections -fdata-sections -Wall  -g -gdwarf-2 -x assembler-with-cpp -I"D:\RT-ThreadStudio\workspace\Project2026331" -Xassembler -mimplicit-it=thumb -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
rt-thread/libcpu/arm/cortex-m3/%.o: ../rt-thread/libcpu/arm/cortex-m3/%.c
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -O0 -ffunction-sections -fdata-sections -Wall  -g -gdwarf-2 -DSOC_FAMILY_STM32 -DSOC_SERIES_STM32F1 -DUSE_HAL_DRIVER -DSTM32F103xE -I"D:\RT-ThreadStudio\workspace\Project2026331\drivers" -I"D:\RT-ThreadStudio\workspace\Project2026331\drivers\include" -I"D:\RT-ThreadStudio\workspace\Project2026331\drivers\include\config" -I"D:\RT-ThreadStudio\workspace\Project2026331\libraries\CMSIS\Device\ST\STM32F1xx\Include" -I"D:\RT-ThreadStudio\workspace\Project2026331\libraries\CMSIS\Include" -I"D:\RT-ThreadStudio\workspace\Project2026331\libraries\CMSIS\RTOS\Template" -I"D:\RT-ThreadStudio\workspace\Project2026331\libraries\STM32F1xx_HAL_Driver\Inc" -I"D:\RT-ThreadStudio\workspace\Project2026331\libraries\STM32F1xx_HAL_Driver\Inc\Legacy" -I"D:\RT-ThreadStudio\workspace\Project2026331" -I"D:\RT-ThreadStudio\workspace\Project2026331\applications" -I"D:\RT-ThreadStudio\workspace\Project2026331" -I"D:\RT-ThreadStudio\workspace\Project2026331\cubemx\Inc" -I"D:\RT-ThreadStudio\workspace\Project2026331\cubemx" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\components\drivers\include" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\components\finsh" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\components\libc\compilers\common\include" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\components\libc\compilers\newlib" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\components\libc\posix\io\epoll" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\components\libc\posix\io\eventfd" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\components\libc\posix\io\poll" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\components\libc\posix\ipc" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\include" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\libcpu\arm\common" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\libcpu\arm\cortex-m3" -include"D:\RT-ThreadStudio\workspace\Project2026331\rtconfig_preinc.h" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"


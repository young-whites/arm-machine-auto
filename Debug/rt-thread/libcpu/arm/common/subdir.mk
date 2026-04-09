################################################################################
# 自动生成的文件。不要编辑！
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../rt-thread/libcpu/arm/common/atomic_arm.c \
../rt-thread/libcpu/arm/common/div0.c \
../rt-thread/libcpu/arm/common/showmem.c 

OBJS += \
./rt-thread/libcpu/arm/common/atomic_arm.o \
./rt-thread/libcpu/arm/common/div0.o \
./rt-thread/libcpu/arm/common/showmem.o 

C_DEPS += \
./rt-thread/libcpu/arm/common/atomic_arm.d \
./rt-thread/libcpu/arm/common/div0.d \
./rt-thread/libcpu/arm/common/showmem.d 


# Each subdirectory must supply rules for building sources it contributes
rt-thread/libcpu/arm/common/%.o: ../rt-thread/libcpu/arm/common/%.c
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -O0 -ffunction-sections -fdata-sections -Wall  -g -gdwarf-2 -DSOC_FAMILY_STM32 -DSOC_SERIES_STM32F1 -DUSE_HAL_DRIVER -DSTM32F103xE -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\drivers" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\drivers\include" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\drivers\include\config" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\libraries\CMSIS\Device\ST\STM32F1xx\Include" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\libraries\CMSIS\Include" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\libraries\CMSIS\RTOS\Template" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\libraries\STM32F1xx_HAL_Driver\Inc" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\libraries\STM32F1xx_HAL_Driver\Inc\Legacy" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\applications" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\cubemx\Inc" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\cubemx" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\components\drivers\include" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\components\finsh" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\components\libc\compilers\common\include" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\components\libc\compilers\newlib" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\components\libc\posix\io\epoll" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\components\libc\posix\io\eventfd" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\components\libc\posix\io\poll" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\components\libc\posix\ipc" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\include" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\libcpu\arm\common" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\libcpu\arm\cortex-m3" -include"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rtconfig_preinc.h" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"


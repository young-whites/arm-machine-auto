################################################################################
# 自动生成的文件。不要编辑！
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../rt-thread/components/finsh/cmd.c \
../rt-thread/components/finsh/msh.c \
../rt-thread/components/finsh/msh_parse.c \
../rt-thread/components/finsh/shell.c 

OBJS += \
./rt-thread/components/finsh/cmd.o \
./rt-thread/components/finsh/msh.o \
./rt-thread/components/finsh/msh_parse.o \
./rt-thread/components/finsh/shell.o 

C_DEPS += \
./rt-thread/components/finsh/cmd.d \
./rt-thread/components/finsh/msh.d \
./rt-thread/components/finsh/msh_parse.d \
./rt-thread/components/finsh/shell.d 


# Each subdirectory must supply rules for building sources it contributes
rt-thread/components/finsh/%.o: ../rt-thread/components/finsh/%.c
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -O0 -ffunction-sections -fdata-sections -Wall  -g -gdwarf-2 -DSOC_FAMILY_STM32 -DSOC_SERIES_STM32F1 -DUSE_HAL_DRIVER -DSTM32F103xE -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\drivers" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\drivers\include" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\drivers\include\config" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\libraries\CMSIS\Device\ST\STM32F1xx\Include" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\libraries\CMSIS\Include" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\libraries\CMSIS\RTOS\Template" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\libraries\STM32F1xx_HAL_Driver\Inc" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\libraries\STM32F1xx_HAL_Driver\Inc\Legacy" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\applications" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\cubemx\Inc" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\cubemx" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\components\drivers\include" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\components\finsh" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\components\libc\compilers\common\include" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\components\libc\compilers\newlib" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\components\libc\posix\io\epoll" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\components\libc\posix\io\eventfd" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\components\libc\posix\io\poll" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\components\libc\posix\ipc" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\include" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\libcpu\arm\common" -I"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rt-thread\libcpu\arm\cortex-m3" -include"C:\Users\18452\Documents\GitHub-young-whites\arm-machine-auto\rtconfig_preinc.h" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"


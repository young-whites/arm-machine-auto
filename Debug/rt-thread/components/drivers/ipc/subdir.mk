################################################################################
# 自动生成的文件。不要编辑！
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../rt-thread/components/drivers/ipc/completion.c \
../rt-thread/components/drivers/ipc/condvar.c \
../rt-thread/components/drivers/ipc/dataqueue.c \
../rt-thread/components/drivers/ipc/pipe.c \
../rt-thread/components/drivers/ipc/ringblk_buf.c \
../rt-thread/components/drivers/ipc/ringbuffer.c \
../rt-thread/components/drivers/ipc/waitqueue.c \
../rt-thread/components/drivers/ipc/workqueue.c 

OBJS += \
./rt-thread/components/drivers/ipc/completion.o \
./rt-thread/components/drivers/ipc/condvar.o \
./rt-thread/components/drivers/ipc/dataqueue.o \
./rt-thread/components/drivers/ipc/pipe.o \
./rt-thread/components/drivers/ipc/ringblk_buf.o \
./rt-thread/components/drivers/ipc/ringbuffer.o \
./rt-thread/components/drivers/ipc/waitqueue.o \
./rt-thread/components/drivers/ipc/workqueue.o 

C_DEPS += \
./rt-thread/components/drivers/ipc/completion.d \
./rt-thread/components/drivers/ipc/condvar.d \
./rt-thread/components/drivers/ipc/dataqueue.d \
./rt-thread/components/drivers/ipc/pipe.d \
./rt-thread/components/drivers/ipc/ringblk_buf.d \
./rt-thread/components/drivers/ipc/ringbuffer.d \
./rt-thread/components/drivers/ipc/waitqueue.d \
./rt-thread/components/drivers/ipc/workqueue.d 


# Each subdirectory must supply rules for building sources it contributes
rt-thread/components/drivers/ipc/%.o: ../rt-thread/components/drivers/ipc/%.c
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -O0 -ffunction-sections -fdata-sections -Wall  -g -gdwarf-2 -DSOC_FAMILY_STM32 -DSOC_SERIES_STM32F1 -DUSE_HAL_DRIVER -DSTM32F103xE -I"D:\RT-ThreadStudio\workspace\Project2026331\drivers" -I"D:\RT-ThreadStudio\workspace\Project2026331\drivers\include" -I"D:\RT-ThreadStudio\workspace\Project2026331\drivers\include\config" -I"D:\RT-ThreadStudio\workspace\Project2026331\libraries\CMSIS\Device\ST\STM32F1xx\Include" -I"D:\RT-ThreadStudio\workspace\Project2026331\libraries\CMSIS\Include" -I"D:\RT-ThreadStudio\workspace\Project2026331\libraries\CMSIS\RTOS\Template" -I"D:\RT-ThreadStudio\workspace\Project2026331\libraries\STM32F1xx_HAL_Driver\Inc" -I"D:\RT-ThreadStudio\workspace\Project2026331\libraries\STM32F1xx_HAL_Driver\Inc\Legacy" -I"D:\RT-ThreadStudio\workspace\Project2026331" -I"D:\RT-ThreadStudio\workspace\Project2026331\applications" -I"D:\RT-ThreadStudio\workspace\Project2026331" -I"D:\RT-ThreadStudio\workspace\Project2026331\cubemx\Inc" -I"D:\RT-ThreadStudio\workspace\Project2026331\cubemx" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\components\drivers\include" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\components\finsh" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\components\libc\compilers\common\include" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\components\libc\compilers\newlib" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\components\libc\posix\io\epoll" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\components\libc\posix\io\eventfd" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\components\libc\posix\io\poll" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\components\libc\posix\ipc" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\include" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\libcpu\arm\common" -I"D:\RT-ThreadStudio\workspace\Project2026331\rt-thread\libcpu\arm\cortex-m3" -include"D:\RT-ThreadStudio\workspace\Project2026331\rtconfig_preinc.h" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"


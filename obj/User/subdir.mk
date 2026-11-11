################################################################################
# MRS Version: 3.0.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/Main.c \
../User/Work_uart.c \
../User/central.c \
../User/iap.c \
../User/pairKey.c \
../User/protocol.c \
../User/rgbled.c \
../User/sys_time.c \
../User/usb.c 

C_DEPS += \
./User/Main.d \
./User/Work_uart.d \
./User/central.d \
./User/iap.d \
./User/pairKey.d \
./User/protocol.d \
./User/rgbled.d \
./User/sys_time.d \
./User/usb.d 

OBJS += \
./User/Main.o \
./User/Work_uart.o \
./User/central.o \
./User/iap.o \
./User/pairKey.o \
./User/protocol.o \
./User/rgbled.o \
./User/sys_time.o \
./User/usb.o 

DIR_OBJS += \
./User/*.o \

DIR_DEPS += \
./User/*.d \

DIR_EXPANDS += \
./User/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
User/%.o: ../User/%.c $(FLAGS_C_FILE)
	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -DDEBUG=1 -I"d:/work/kit codit/CH591D_master/StdPeriphDriver/inc" -I"d:/work/kit codit/CH591D_master/RVMSIS" -I"d:/work/kit codit/CH591D_master/Startup" -I"d:/work/kit codit/CH591D_master/Profile/include" -I"d:/work/kit codit/CH591D_master/HAL/include" -I"d:/work/kit codit/CH591D_master/Ld" -I"d:/work/kit codit/CH591D_master/LIB" -I"d:/work/kit codit/CH591D_master/User/include" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"


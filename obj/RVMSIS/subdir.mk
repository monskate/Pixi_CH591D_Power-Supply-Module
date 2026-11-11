################################################################################
# MRS Version: 3.0.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../RVMSIS/core_riscv.c 

C_DEPS += \
./RVMSIS/core_riscv.d 

OBJS += \
./RVMSIS/core_riscv.o 

DIR_OBJS += \
./RVMSIS/*.o \

DIR_DEPS += \
./RVMSIS/*.d \

DIR_EXPANDS += \
./RVMSIS/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
RVMSIS/%.o: ../RVMSIS/%.c $(FLAGS_C_FILE)
	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -DDEBUG=1 -I"d:/work/kit codit/CH591D_master/StdPeriphDriver/inc" -I"d:/work/kit codit/CH591D_master/RVMSIS" -I"d:/work/kit codit/CH591D_master/Startup" -I"d:/work/kit codit/CH591D_master/Profile/include" -I"d:/work/kit codit/CH591D_master/HAL/include" -I"d:/work/kit codit/CH591D_master/Ld" -I"d:/work/kit codit/CH591D_master/LIB" -I"d:/work/kit codit/CH591D_master/User/include" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"


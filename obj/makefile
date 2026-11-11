################################################################################
# MRS Version: 3.0.0
# Automatically-generated file. Do not edit!
################################################################################
-include ../makefile.init

RM := rm -rf
FLAGS_C :=  -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -DDEBUG=1 -I"d:/work/kit codit/CH591D_master/StdPeriphDriver/inc" -I"d:/work/kit codit/CH591D_master/RVMSIS" -I"d:/work/kit codit/CH591D_master/Startup" -I"d:/work/kit codit/CH591D_master/Profile/include" -I"d:/work/kit codit/CH591D_master/HAL/include" -I"d:/work/kit codit/CH591D_master/Ld" -I"d:/work/kit codit/CH591D_master/LIB" -I"d:/work/kit codit/CH591D_master/User/include" -std=gnu99
FLAGS_C_FILE := .compile_c_flags

FLAGS_CPP :=  -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -std=gnu++11 -fabi-version=0
FLAGS_CPP_FILE := .compile_cpp_flags

FLAGS_ASM :=  -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -x assembler-with-cpp
FLAGS_ASM_FILE := .compile_asm_flags

FLAGS_LD:=  -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -T "d:\work\kit codit\CH591D_master\Ld\Link.ld" -nostartfiles -Xlinker --gc-sections -L"d:\work\kit codit\CH591D_master" -L"d:\work\kit codit\CH591D_master\StdPeriphDriver" -L"d:\work\kit codit\CH591D_master\LIB" -Xlinker --print-memory-usage -Wl,-Map,"CH591D_test.map" --specs=nano.specs --specs=nosys.specs
FLAGS_LD_FILE := .ld_flags

$(FLAGS_C_FILE): FORCE
	@echo '$(FLAGS_C)' | cmp -s - $@ || echo '$(FLAGS_C)' > $@
$(FLAGS_CPP_FILE): FORCE
	@echo '$(FLAGS_CPP)' | cmp -s - $@ || echo '$(FLAGS_CPP)' > $@
$(FLAGS_ASM_FILE): FORCE
	@echo '$(FLAGS_ASM)' | cmp -s - $@ || echo '$(FLAGS_ASM)' > $@
$(FLAGS_LD_FILE): FORCE
	@echo '$(FLAGS_LD)' | cmp -s - $@ || echo '$(FLAGS_LD)' > $@
.PHONY: FORCE
FORCE:

# All of the sources participating in the build are defined here
-include sources.mk
-include User/subdir.mk
-include StdPeriphDriver/subdir.mk
-include Startup/subdir.mk
-include RVMSIS/subdir.mk
-include LIB/subdir.mk
-include HAL/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

SOURCES_LIST := .sources_list
$(SOURCES_LIST): FORCE
	@echo '$(C_SRCS) $(CPP_SRCS) $(S_UPPER_SRCS)' | tr ' ' '\n' | sort > $@.tmp
	@cmp -s $@.tmp $@ || mv $@.tmp $@
	@rm -f $@.tmp

LD_SCRIPTS := d:/work/kit\ codit/CH591D_master/Ld/Link.ld

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
CH591D_test.hex \

SECONDARY_LIST += \
CH591D_test.lst \

SECONDARY_SIZE += \
CH591D_test.siz \


# All Target
all: 
	$(MAKE) --no-print-directory main-build 

main-build: CH591D_test.elf secondary-outputs

# Tool invocations
CH591D_test.elf: $(OBJS) $(USER_OBJS_ESCAPE) $(SOURCES_LIST) $(FLAGS_LD_FILE) $(LD_SCRIPTS)
	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -T "d:\work\kit codit\CH591D_master\Ld\Link.ld" -nostartfiles -Xlinker --gc-sections -L"d:\work\kit codit\CH591D_master" -L"d:\work\kit codit\CH591D_master\StdPeriphDriver" -L"d:\work\kit codit\CH591D_master\LIB" -Xlinker --print-memory-usage -Wl,-Map,"CH591D_test.map" --specs=nano.specs --specs=nosys.specs -o "CH591D_test.elf" $(OBJS) $(LIBS)
CH591D_test.hex: CH591D_test.elf
	@	riscv-none-embed-objcopy -O ihex "CH591D_test.elf" "CH591D_test.hex"
CH591D_test.lst: CH591D_test.elf
	@	riscv-none-embed-objdump --source --all-headers --demangle --line-numbers --wide "CH591D_test.elf" > "CH591D_test.lst"
CH591D_test.siz: CH591D_test.elf
	riscv-none-embed-size --format=berkeley "CH591D_test.elf"

# Other Targets
clean:
	-$(RM) $(DIR_OBJS) $(SECONDARY_FLASH)$(SECONDARY_LIST)$(SECONDARY_SIZE) CH591D_test.elf
	-$(RM) $(DIR_EXPANDS) $(CALLGRAPH_DOT)
	-$(RM) $(DIR_DEPS)
	-$(RM) $(FLAGS_C_FILE) $(FLAGS_CPP_FILE) $(FLAGS_ASM_FILE) $(FLAGS_LD_FILE) $(SOURCES_LIST)
secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents

-include ../makefile.targets
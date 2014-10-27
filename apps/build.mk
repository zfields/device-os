

# Linker flags
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map
ifeq ("$(ARCH)","arm")
LDFLAGS += -lnosys --specs=nosys.specs -lc
LDFLAGS += -L$(BUILD_PATH_BASE)/main/$(BUILD_PATH_EXT)
LDFLAGS += -T$(ELF_LINKER_SCRIPT)

ELF_LINKER_SCRIPT = linker.ld
ELF_STARTUP_SCRIPT = startup.S

ASRC += $(ELF_STARTUP_SCRIPT)
ASFLAGS += -I$(APPS_MODULE_PATH)
#ASFLAGS +=  -Wa,--defsym -Wa,SPARK_INIT_STARTUP=1

endif



MODULE=apps
APPS_MODULE_PATH=.

BUILD_PATH_EXT = prod-$(SPARK_PRODUCT_ID)

DEPENDENCIES = main wiring services hal communication
MAKE_DEPENDENCIES := 
LIBS := 

include ../build/product-id.mk

# Target this makefile is building.
ifeq ("$(ARCH)","gcc")
TARGET=none
else
TARGET=elf bin hex size lst sym
endif


include ../build/arm-tlm.mk



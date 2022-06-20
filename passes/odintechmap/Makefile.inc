
ifeq ($(ENABLE_ODIN_TECHMAP),1)

OBJS += passes/odintechmap/arch_fpga.o
OBJS += passes/odintechmap/odin_techmap.o

endif

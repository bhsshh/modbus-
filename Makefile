##################################################################
# Copy right:     Coffee Tech.
# Author:         jiaoyue
##################################################################
#生成可执行程序
OJB = modbus_test
OBJS = modbus_test.o myfunc.o 

OJB_OUT = $(OJB).out

all:$(OJB_OUT)

$(OJB_OUT):$(OBJS)
	$(CC) -o $@ $^ $(ULDFLAGS)

dep_files := $(foreach f,$(OBJS),.$(f).d)
dep_files := $(wildcard $(dep_files))

ifneq ($(dep_files),)
  include $(dep_files)
endif

$(OBJS):%.o:%.c
	$(CC) -Wp,-MD,.$@.d $(UCFLAGS) $< -o $@

install:
	cp $(OJB_OUT) $(TESTBINS_DIR)/$(OJB)

clean:
	rm -rf .*.o.d *.o $(OJB_OUT)
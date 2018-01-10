# Makefile, by Liu Mingyang


TARGET = cvio
LIB = lib$(TARGET).so
EXE = $(TARGET)


SVN_REVISION = $(shell svn info |grep Revision|awk '{print $$2}')
$(info SVN Rev = $(SVN_REVISION))

CROSS = 

INCLUDE := -I../include -I../../dscv/include/ -I/usr/local/include
LIB_DIR := 
LIBS     =
COMMON_FLAGS += -g -Wall -Wextra -pipe
COMMON_FLAGS += -DOSA_MODULE_NAME='"$(TARGET)"' -DSVN_REVISION=$(SVN_REVISION)

SRC     = $(wildcard ../src/*.c)
CXXSRC += $(wildcard ../src/*.cpp)
EXESRC  = $(wildcard ../tests/*.c)
EXESRC += $(wildcard ../tests/*.cpp)

# OBJDIR = ./objs
OBJDIR = ./
OBJ += $(SRC:%.c=$(OBJDIR)/%.o) $(CXXSRC:%.cpp=$(OBJDIR)/%.o)
EXEOBJ += $(EXESRC:%.cpp=$(OBJDIR)/%.o)

COMMON_FLAGS += $(INCLUDE) $(LIB_DIR)
COMMON_FLAGS += -fPIC

CC        = $(CROSS)g++
CXX       = $(CROSS)g++
LD        = $(CROSS)g++
CFLAGS   +=
SHARED_LIB_FLAGS += -fPIC -shared

# Debug Options
ifdef RELEASE
  CPPFLAGS += -Os -mtune=native
else
  CPPFLAGS += -g -O0
endif

CFLAGS   += $(COMMON_FLAGS) $(SHARED_LIB_FLAGS)
CPPFLAGS += $(COMMON_FLAGS) $(SHARED_LIB_FLAGS)
# CPPFLAGS += -std=c++11 -D_GLIBCXX_USE_CXX11_ABI=0
LDFLAGS  += $(LIBS)
LDFLAGS_EXE += $(LDFLAGS)
LDFLAGS_EXE += -L/usr/local/lib -lopencv_core -lopencv_highgui


all:lib
exe:$(EXE)
lib:$(LIB)

$(EXE): $(EXEOBJ) $(LIB)
	$(LD) -o $@ $(EXEOBJ) $(COMMON_FLAGS) $(LIB) $(LDFLAGS_EXE)
	@echo "Running programm:"
$(LIB):$(OBJ)
	$(LD) -o $@ $(SHARED_LIB_FLAGS) $(OBJ) $(LDFLAGS)

$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR)/%.o : %.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@
	
clean:
	-rm -f $(OBJ)
	-rm -f $(EXE)
	-rm -f $(LIB)
#   -rm -r $(OBJDIR)

.PHONY:clean all exe lib


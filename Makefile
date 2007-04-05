# Things you might need to change in this Makefile:
#
# If you don't have the SensAble libraries installed, take out -lHD and -lHDU from the LIBS line

OBJECTS = dimple.o \
	CODEPrimitive.o \
	CODEMesh.o \
	CODEPrism.o \
	CODESphere.o \
	CODEPotentialProxy.o \
	OscObject.o

DEBUG = -g -ggdb
SOURCES = $(OBJECTS:.o=.cpp)
INCLUDE = -Ichai3d/include -Iode-0.7/include -I. -Iliblo-0.23
DARWIN_INCLUDE = -Ichai3d/darwin/GL -Ichai3d/darwin/GLUT
LOCALOBJS = $(notdir $(OBJECTS) )
DEFS = -D_POSIX -D_MAX_PATH=260 -D_LINUX -DLINUX -D__LINUX__ -D_POSIX # -DUSE_FREEGLUT
CC   = g++ -c $(DEFS)
CFLAGS = -O3 $(INCLUDE) $(DEBUG)
LD   = g++ -o 

DARWIN_LIBS = -framework OpenGL -framework GLUT
DARWIN_CHAI_LIBS = -Lchai3d/lib/darwin -lchai3d_darwin

LINUX_LIBS = -lGL -lglut -lGLU
LINUX_CHAI_LIBS = -Lchai3d/lib/linux -lchai3d_linux -ldhd -lpciscan -lusb

LIBS = -Lliblo-0.23/src/.libs -lpthread -llo -Lode-0.7/ode/src -lode

ifeq ($(shell uname),Darwin)
INCLUDE += $(DARWIN_INCLUDE)
LIBS +=  $(DARWIN_LIBS) $(DARWIN_CHAI_LIBS)
else
ifeq ($(shell uname),Linux)
LIBS +=  $(LINUX_LIBS) $(LINUX_CHAI_LIBS)
endif
endif

all: dimple

%.o: %.cpp  $(wildcard *.h)
	$(CC) $(CFLAGS) $<

# Actual target and dependencies
dimple: $(OBJECTS)
	$(LD) dimple $(LOCALOBJS) $(LIBS)
	@echo "compilation done"

# Target deleting unwanted files
clean:
	\rm -f *.o *~ dimple core 

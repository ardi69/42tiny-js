

CXXFLAGS  += -g -Wall 
CXXFLAGS  += $(CXXEXTRA) 

LDFLAGS += $(LDEXTRA)
LIBS += $(LIBSEXTRA)

ifneq ($(MAKECMDGOALS),clean)

HAVE_INCLUDE = $(shell $(CXX) $(2) -include $(1) -xc++ -E - < /dev/null &> /dev/null && echo "yes" || echo "no")

define PREBUILD_TEST 
$(shell 
	[ -f ./prebuild.test.$(1)$(2).ok ] && echo -n "yes"
	|| (
		[ -f ./prebuild.test.$(1)$(2).failed ] && echo -n "no"
		|| (
			echo $(CXX) $(CXXFLAGS) -DTEST_$(1) prebuild.tests.cpp $(LIBS) $(2) -o prebuild.test.$(1)$(2) 1>&2 &&
			$(CXX) $(CXXFLAGS) -DTEST_$(1) prebuild.tests.cpp $(LIBS) $(2) -o ./prebuild.test.$(1)$(2) 1>&2 && sleep 3.0 && [ -f ./prebuild.test.$(1)$(2) ] && ./prebuild.test.$(1)$(2)
			&& 
			(echo -n "yes" && touch ./prebuild.test.$(1)$(2).ok 1>&2 && rm -f ./prebuild.test.$(1)$(2) 1>&2)
			||
			(echo -n "no" && touch ./prebuild.test.$(1)$(2).failed 1>&2 && rm -f ./prebuild.test.$(1)$(2) 1>&2)
		)
	)
)
endef

ifeq ($(call PREBUILD_TEST,"CPP23","-std=c++23"),yes)
    CXX_STANDARD := c++23
else ifeq ($(call PREBUILD_TEST,"CPP20","-std=c++20"),yes)
    CXX_STANDARD := c++20
else ifeq ($(call PREBUILD_TEST,"CPP17","-std=c++17"),yes)
    CXX_STANDARD := c++17
else
$(error ❌ Dein Compiler unterstützt kein C++17! Bitte verwende GCC ≥ 7.1 oder Clang ≥ 5.0.)
endif

# Debug-Infos ausgeben
$(info ✅ Höchster unterstützter C++-Standard: $(CXX_STANDARD))

CXXFLAGS += -std=$(CXX_STANDARD)

ifeq ($(DEBUG),1)
	CXXFLAGS += -D_DEBUG
endif


ifeq ($(NO_POOL_ALLOCATOR),1)
	CXXFLAGS += -DNO_POOL_ALLOCATOR
else 
	ifeq ($(NO_SPINLOCK_IN_POOL_ALLOCATOR),1)
		CXXFLAGS += -DNO_SPINLOCK_IN_POOL_ALLOCATOR
	endif
endif


ifeq ($(NO_THREADING),1)
	CXXFLAGS += -DNO_THREADING
else 
	ifeq ($(shell $(CXX) $(CXXFLAGS) -xc++ -pthread -E - < /dev/null &> /dev/null && echo -n "yes"),yes)
		CXXFLAGS += -pthread
	endif

	ifeq ($(NO_CXX_THREADS),1)
		CXXFLAGS += -DNO_CXX_THREADS
	else ifneq ($(NO_CXX_THREADS),0)
		ifneq ($(call PREBUILD_TEST,"CXX11_THREAD"),yes)
			ifneq ($(call PREBUILD_TEST,"CXX11_THREAD","-lpthread"),yes)
				override NO_CXX_THREADS = 1
				CXXFLAGS += -DNO_CXX_THREADS
			else
				LIBS += -lpthread
			endif
		endif
	endif
endif

ifneq ($(call PREBUILD_TEST,"CXX11_FILESYSTEM"),yes)
	ifeq ($(call PREBUILD_TEST,"CXX11_FILESYSTEM","-lstdc++fs"),yes)
		LIBS += -lstdc++fs
	endif
endif


endif # ifneq ($(MAKECMDGOALS),clean)

LIBS    += -L '$(CURDIR)' -l42tiny-js

SOURCES=  \
	TinyJS.cpp \
	TinyJS_PoolAllocator.cpp \
	TinyJS_Functions.cpp \
	TinyJS_MathFunctions.cpp \
	TinyJS_StringFunctions.cpp \
	TinyJS_DateFunctions.cpp \
	TinyJS_Threading.cpp

OBJECTS=$(SOURCES:.cpp=.o)


all:  run_tests Script
#	@echo $(CXXFLAGS)
	
run_tests: lib42tiny-js.a run_tests.o
	@echo link $@
	$(CXX) $(LDFLAGS) run_tests.o $(LIBS) -o $@

Script: lib42tiny-js.a Script.o
	@echo link $@
	$(CXX) $(LDFLAGS) Script.o $(LIBS) -o $@

lib42tiny-js.a: $(OBJECTS)

help:
	@echo "using make [Options] [target]"
	@echo "Options:"
	@echo "	DEBUG=1:             activate debug-mode"
	@echo "	NO_POOL_ALLOCATOR=1: deactivate the using of the pool-allocator"

#-------------- rules --------------

.cpp.o:
	@echo compile $<
	$(CXX) -c -MMD -MP -MF $*.dep $(CXXFLAGS) $< -o $@

-include *.dep

clean:
	@echo "clean"
	@rm -fv prebuild.test.* lib42tiny-js.a run_tests run_tests.exe run_tests.o run_tests.dep Script Script.exe Script.o Script.dep $(OBJECTS) $(OBJECTS:.o=.dep)

%.a:
	@echo link $(notdir $@)
	@rm -f $@
	$(AR) -rcs $@ $^


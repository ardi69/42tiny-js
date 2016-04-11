

CXXFLAGS  += -g -Wall 
CXXFLAGS  += $(CXXEXTRA) 

LDFLAGS += $(LDEXTRA)

ifneq ($(MAKECMDGOALS),clean)

HAVE_INCLUDE = $(shell $(CXX) $(2) -include $(1) -xc++ -E - < /dev/null &> /dev/null && echo "yes" || echo "no")

GCC_GTEQ_430 := $(shell expr `$(CXX) -dumpversion | sed -e 's/\.\([0-9][0-9]\)/\1/g' -e 's/\.\([0-9]\)/0\1/g' -e 's/^[0-9]\{3,4\}$$/&00/'` \>= 40300)
GCC_GTEQ_470 := $(shell expr `$(CXX) -dumpversion | sed -e 's/\.\([0-9][0-9]\)/\1/g' -e 's/\.\([0-9]\)/0\1/g' -e 's/^[0-9]\{3,4\}$$/&00/'` \>= 40700)


ifeq "$(GCC_GTEQ_470)" "1"
	CXXFLAGS += -std=c++11
else ifeq "$(GCC_GTEQ_430)" "1"
	CXXFLAGS += -std=c++0x
endif

define PREBUILD_TEST 
$(shell 
	[ -f ./prebuild.test.$(1)$(2).ok ] && echo -n "yes"
	|| (
		[ -f ./prebuild.test.$(1)$(2).failed ] && echo -n "no"
		|| (
			$(CXX) $(CXXFLAGS) $(LIBS) $(2) -DTEST_$(1) -o prebuild.test.$(1)$(2) prebuild.tests.cpp &> /dev/null && ./prebuild.test.$(1)$(2)
			&& 
			(echo -n "yes" && touch ./prebuild.test.$(1)$(2).ok > /dev/null && rm -f ./prebuild.test.$(1)$(2) > /dev/null)
			||
			(echo -n "no" && touch ./prebuild.test.$(1)$(2).failed > /dev/null && rm -f ./prebuild.test.$(1)$(2) > /dev/null)
		)
	)
)
endef


ifeq ($(DEBUG),1)
	CXXFLAGS += -D_DEBUG
endif


ifeq ($(NO_POOL_ALLOCATOR),1)
	CXXFLAGS += -DNO_POOL_ALLOCATOR
else 
	ifeq ($(SPINLOCK_IN_POOL_ALLOCATOR),1)
		CXXFLAGS += -DSPINLOCK_IN_POOL_ALLOCATOR
	else ifneq ($(SPINLOCK_IN_POOL_ALLOCATOR),0)
		ifeq ($(call PREBUILD_TEST,"SPINLOCK"),yes)
			CXXFLAGS += -DSPINLOCK_IN_POOL_ALLOCATOR
		endif
	endif
endif

ifeq ($(NO_REGEXP),1)
	CXXFLAGS += -DNO_REGEXP
else ifeq ($(HAVE_CXX_REGEX),1)
	CXXFLAGS += -DHAVE_CXX_REGEX
else ifeq ($(HAVE_TR1_REGEX),1)
	CXXFLAGS += -DHAVE_TR1_REGEX
else ifeq ($(HAVE_BOOST_REGEX),1)
	CXXFLAGS += -DHAVE_BOOST_REGEX
	LIBS += -lboost_regex
else 
	ifneq ($(HAVE_CXX_REGEX),0) 
		ifeq ($(call PREBUILD_TEST,"CXX11_REGEX"),yes)
			CXXFLAGS += -DHAVE_CXX_REGEX
			override HAVE_CXX_REGEX = 1
		endif
	endif
	ifneq ($(HAVE_CXX_REGEX),1)
		ifneq ($(HAVE_TR1_REGEX),0)
			ifeq ($(call PREBUILD_TEST,"TR1_REGEX"),yes)
			CXXFLAGS += -DHAVE_TR1_REGEX
			override HAVE_TR1_REGEX = 1
		endif
		ifneq ($(HAVE_TR1_REGEX),1)
			ifneq ($(HAVE_BOOST_REGEX),0)
				ifeq ($(call PREBUILD_TEST,"BOOST_REGEX", "-lboost_regex"),yes)
					CXXFLAGS += -DHAVE_BOOST_REGEX
					LIBS += -lboost_regex
					override HAVE_BOOST_REGEX = 1
				endif
			endif
			ifneq ($(HAVE_BOOST_REGEX),1)
				override NO_REGEXP = 1
			endif
		endif
	endif
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
	
	ifeq ($(HAVE_PTHREAD),1)
		CXXFLAGS += -DHAVE_PTHREAD
		LIBS += -lpthread
	else ifneq ($(HAVE_PTHREAD),0)
		ifneq ($(call PREBUILD_TEST,"PTHREAD"),yes)
			override HAVE_PTHREAD = 1
			CXXFLAGS += -DHAVE_PTHREAD
		else ifeq ($(call PREBUILD_TEST,"PTHREAD","-lpthread"),yes)
			override HAVE_PTHREAD = 1
			CXXFLAGS += -DHAVE_PTHREAD
			LIBS += -lpthread
		endif
	endif
endif

ifeq ($(NO_GETTIMEOFDAY),1)
	CXXFLAGS += -DNO_GETTIMEOFDAY
else ifneq ($(call PREBUILD_TEST,"GETTIMEOFDAY"),yes)
	CXXFLAGS += -DNO_GETTIMEOFDAY
endif

ifeq ($(WITH_TIME_LOGGER),1)
	CXXFLAGS += -DWITH_TIME_LOGGER
endif


endif # ifneq ($(MAKECMDGOALS),clean)

LIBS    += -L '$(CURDIR)' -l42tiny-js

SOURCES=  \
	TinyJS.cpp \
	pool_allocator.cpp \
	TinyJS_Functions.cpp \
	TinyJS_MathFunctions.cpp \
	TinyJS_StringFunctions.cpp \
	TinyJS_DateFunctions.cpp \
	TinyJS_Threading.cpp

OBJECTS=$(SOURCES:.cpp=.o)


all:  run_tests Script
	@echo $(CXXFLAGS)
	
run_tests: lib42tiny-js.a run_tests.o
	@echo link $@
	echo $(LIBS)
	$(CXX) $(LDFLAGS) run_tests.o $(LIBS) -o $@

Script: lib42tiny-js.a Script.o
	@echo link $@
	$(CXX) $(LDFLAGS) Script.o $(LIBS) -o $@

lib42tiny-js.a: $(OBJECTS)

help:
	@echo "using make [Options] [target]
	@echo "Options:"
	@echo "	DEBUG=1:				activate debug-mode"
	@echo "	NO_POOL_ALLOCATOR=1:	deactivate the using of the pool-allocator"

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


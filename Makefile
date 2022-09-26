ifneq ($(ECHOVARS), )
BUILTINS := $(.VARIABLES)
endif

.PHONY: all run-verbit test test-unit test-client run-test-server run-test-example doc doc-server install install-doc debvars package clean soname

TARGET := /usr/local
OWNFLAGS := -o root -g root
UBUNTU := bionic

SRCNS := verbit/streaming
SRCDIR := src/$(SRCNS)
SRCS := $(wildcard $(SRCDIR)/*.cpp)
INCS := $(filter-out ${SRCDIR}/version.h, $(wildcard $(SRCDIR)/*.h)) $(SRCDIR)/version.h
OBJDIR := obj
OBJS := $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
BINDIR := bin
LIBNAME := verbit_streaming
SWVER := 1.0.1
LIBSO := 1
LIBVER := $(LIBSO).0
ALIB := $(OBJDIR)/lib$(LIBNAME).a
SOLIBV := $(OBJDIR)/lib$(LIBNAME).so.$(LIBVER)
SOLIB := lib$(LIBNAME).so.$(LIBSO)
SOLINK := lib$(LIBNAME).so

TESTDIR := test
TEST_SRCS := $(wildcard $(TESTDIR)/*.cpp)
TEST_INCS := $(wildcard $(TESTDIR)/*.h)
TEST_OBJS := $(TEST_SRCS:$(TESTDIR)/%.cpp=$(OBJDIR)/%.o)
TEST_BINDIR := test-bin
TEST_BINSRCS := $(wildcard $(TESTDIR)/*_test.cpp)
TEST_BINS := $(TEST_BINSRCS:$(TESTDIR)/%.cpp=$(TEST_BINDIR)/%)
TEST_CBINSRCS := $(wildcard $(TESTDIR)/*_test_c.cpp)
TEST_CBINS := $(TEST_CBINSRCS:$(TESTDIR)/%.cpp=$(TEST_BINDIR)/%)
TEST_SRVBIN := $(TEST_BINDIR)/test_server

EXAMDIR := examples
EXAM_SRCS := $(wildcard $(EXAMDIR)/*.cpp)
EXAM_INCS := $(wildcard $(EXAMDIR)/*.h)
EXAM_OBJS := $(EXAM_SRCS:$(EXAMDIR)/%.cpp=$(OBJDIR)/%.o)
EXAM_BINSRCS := $(wildcard $(EXAMDIR)/example_*.cpp)
EXAM_BINS := $(EXAM_BINSRCS:$(EXAMDIR)/%.cpp=$(BINDIR)/%)

ifneq ($(VERBIT_WS_URL), )
SVC_URL := -u $(VERBIT_WS_URL)
endif

DOCDIR := doc/html
DOCIDX := $(DOCDIR)/index.html

DEBUGFLAGS :=  # -DDEBUG
SRCFLAGS := $(DEBUGFLAGS) -D_WEBSOCKETPP_CPP11_STL_ -DBOOST_ERROR_CODE_HEADER_ONLY -DBOOST_SYSTEM_NO_DEPRECATED=1 -Isrc
CXXFLAGS := -std=c++11 -Wall -Werror -fPIC -pthread
TLSLIBS := -lssl -lcrypto

ifneq ($(ECHOVARS), )
$(foreach v, \
	$(filter-out $(BUILTINS) BUILTINS,$(.VARIABLES)), \
	$(info $(v) = $($(v))))
endif

all: $(ALIB) $(SOLIBV)

run-verbit: $(BINDIR)/example_client
	$(BINDIR)/example_client $(SVC_URL) test-files/fox-and-grapes.wav

test: test-unit test-client

test-unit: $(TEST_BINS) $(TEST_SRVBIN)
	scripts/run_unit_tests

test-client: $(TEST_CBINS) $(TEST_SRVBIN)
	scripts/run_client_tests

run-test-server: $(TEST_SRVBIN)
	$(TEST_SRVBIN)

run-test-example: $(BINDIR)/example_client
	$(BINDIR)/example_client -u wss://localhost:9002 test-files/thats-good.wav

doc: $(DOCIDX)

doc-server: $(DOCIDX) Doxyfile
	( cd $(DOCDIR) && python3 -m http.server 2080 )

$(DOCIDX): $(SRCS) $(INCS)
	doxygen

install: $(ALIB) $(SOLIBV)
	install $(OWNFLAGS) -m 644 $(ALIB) $(TARGET)/lib/$(notdir $(ALIB))
	install $(OWNFLAGS) -m 644 $(SOLIBV) $(TARGET)/lib/$(notdir $(SOLIBV))
	ldconfig -n $(TARGET)/lib
	( cd $(TARGET)/lib && ln -sf $(SOLIB) $(SOLINK) )
	mkdir -p $(TARGET)/include/$(SRCNS)
	install $(OWNFLAGS) -m 644 $(INCS) $(TARGET)/include/$(SRCNS)

install-doc: $(DOCIDX)
	mkdir -p $(TARGET)/share/doc/$(LIBNAME)/html/search
	install $(OWNFLAGS) -m 644 $(DOCDIR)/*.* $(TARGET)/share/doc/$(LIBNAME)/html
	install $(OWNFLAGS) -m 644 $(DOCDIR)/search/*.* $(TARGET)/share/doc/$(LIBNAME)/html/search

debvars:
	@echo LIBNAME=\"$(LIBNAME)\"
	@echo SWVER=\"$(SWVER)\"
	@echo LIBS=\"$(ALIB) $(SOLIBV)\"
	@echo SONAME=\"$(SOLIB)\"
	@echo SOLINK=\"$(SOLINK)\"
	@echo SRCNS=\"$(SRCNS)\"
	@echo INCS=\"$(INCS)\"
	@echo DOCDIR=\"$(DOCDIR)\"

package: $(ALIB) $(SOLIBV) $(DOCIDX)
	scripts/build_package $(UBUNTU)

clean:
	rm -f $(BINDIR)/* $(OBJDIR)/* $(SRCDIR)/version.h $(TEST_BINDIR)/*
	rm -f $(DOCDIR)/*.* $(DOCDIR)/search/*.*
	if [ -d "$(DOCDIR)/search" ]; then rmdir $(DOCDIR)/search; fi
	if [ -d "$(DOCDIR)" ]; then rmdir $(DOCDIR); fi

$(SRCDIR)/version.h: $(SRCDIR)/version.h.in Makefile
	sed "s/__WSSC__VERSION__/$(SWVER)/" $< >$@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(INCS)
	g++ $(CXXFLAGS) -o $@ -g -c $(SRCFLAGS) $<

$(OBJDIR)/%.o: $(TESTDIR)/%.cpp $(INCS) $(TEST_INCS)
	g++ $(CXXFLAGS) -o $@ -g -c $(SRCFLAGS) $<

$(OBJDIR)/%.o: $(EXAMDIR)/%.cpp $(INCS) $(EXAM_INCS)
	g++ $(CXXFLAGS) -o $@ -g -c $(SRCFLAGS) $<

$(ALIB): $(OBJS)
	ar crs $(ALIB) $(OBJS)

$(SOLIBV): $(OBJS)
	g++ -shared -Wl,-soname,$(SOLIB) -o $@ $^ -lc

soname:
	objdump -p $(SOLIBV) | grep SONAME

$(BINDIR)/example_client: $(OBJDIR)/example_client.o $(OBJDIR)/wav_media_generator.o $(ALIB)
	g++ $(CXXFLAGS) -o $@ $^ $(TLSLIBS)

$(TEST_BINDIR)/media_config_test: obj/test_main.o obj/media_config_test.o obj/media_config.o
	g++ $(CXXFLAGS) -o $@ $^ -lcppunit

$(TEST_BINDIR)/response_type_test: obj/test_main.o obj/response_type_test.o obj/response_type.o
	g++ $(CXXFLAGS) -o $@ $^ -lcppunit

$(TEST_BINDIR)/service_state_test: obj/test_main.o obj/service_state_test.o obj/service_state.o
	g++ $(CXXFLAGS) -o $@ $^ -lcppunit

$(TEST_BINDIR)/ws_streaming_client_test: obj/test_main.o obj/ws_streaming_client_test.o obj/empty_media_generator.o $(OBJS)
	g++ $(CXXFLAGS) -o $@ $^ $(TLSLIBS) -lcppunit

$(TEST_BINDIR)/empty_media_test_c: $(OBJDIR)/empty_media_test_c.o $(OBJDIR)/empty_media_generator.o $(ALIB)
	g++ $(CXXFLAGS) -o $@ $^ $(TLSLIBS)

$(TEST_BINDIR)/short_media_test_c: $(OBJDIR)/short_media_test_c.o $(OBJDIR)/wav_media_generator.o $(ALIB)
	g++ $(CXXFLAGS) -o $@ $^ $(TLSLIBS)

$(TEST_SRVBIN): obj/test_server.o
	g++ $(CXXFLAGS) -o $(TEST_SRVBIN) $^ $(TLSLIBS) -luuid

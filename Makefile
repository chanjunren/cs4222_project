DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = trace_together
APPS+=powertrace
all: $(CONTIKI_PROJECT)

CONTIKI_WITH_RIME = 1

CONTIKI = ../..
include $(CONTIKI)/Makefile.include

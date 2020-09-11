PROJECT=MQTT_Test
EXECUTABLE = ${PROJECT}
CC=gcc
CFLAGS += -std=c99 -O2  -Wall -c -Werror
LIBS += -lmosquitto
SOURCES=main.c tiny-json.c
INCLUDE_DIRS=-I .
OBJECTS=$(SOURCES:.c=.o)
DEFINE=
#DEFINE += -DENABLE_LOG   # uncomment this define, if need write system log to /var/log/syslog


all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $(DEFINE) $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
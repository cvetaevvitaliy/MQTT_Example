PROJECT=MQTT_Test
EXECUTABLE = ${PROJECT}
CC=gcc
CFLAGS += -std=c99 -O2  -Wall -c -Werror
LIBS += -lmosquitto
SOURCES=main.c tiny-json.c
INCLUDE_DIRS=-I .
OBJECTS=$(SOURCES:.c=.o)


all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
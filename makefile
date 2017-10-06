HEADERS = helpers.h
SOURCE = sws.c helpers.c
EXECUTABLE = sws

$(EXECUTABLE): $(SOURCE) $(HEADERS)
	gcc -o $@ $(SOURCE)
	
.PHONY: clean

clean:
	rm -f sws
CXX = x86_64-apple-darwin16.7.0-c++-8

TARGET = test
CXXFLAGS = -std=c++14 \
	-Wno-shift-negative-value \

CPPFLAGS = -I .. \
	   -I ../src/ \
	   -I ../lib/aesig/ \

OBJS = test.o ../data.o
LIBS =

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

../data.hh: ../data/data_compiler.py ../data/data.py
	cd ..; python data/data.py

clean:
	rm -f $(OBJS) $(TARGET)

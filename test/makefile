
TARGET = test
CXXFLAGS = -std=c++11 \
	-Wno-shift-negative-value \

CPPFLAGS = -I ../src/ -I ..

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

test.o: test.cc ../src/parameters.hh ../src/grain_oscillator.hh ../src/dsp.hh \
	../data.hh

CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra $(shell pkg-config --cflags libpng zlib)
LDFLAGS = $(shell pkg-config --libs libpng zlib)

SRCDIR = steganography
SOURCES = $(SRCDIR)/handler.cpp $(SRCDIR)/encoder.cpp $(SRCDIR)/decoder.cpp $(SRCDIR)/demo.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = stegasaur

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean

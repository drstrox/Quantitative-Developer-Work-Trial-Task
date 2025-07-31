TARGET = reconstruction_aayush
SRC = src/reconstruction_aayush.cpp
OUT = $(TARGET)

all:
	g++ -std=c++17 -O2 -o $(OUT) $(SRC)

clean:
	rm -f $(OUT)


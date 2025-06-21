FLAGS = -Wall -g
TARGET = ./bin/main.exe
INCLUDE = -I./include
LIBS = -lSDL3
OBJECTS := $(patsubst ./src/%.cpp, ./bin/%.o, $(wildcard ./src/*.cpp))

all : $(TARGET)

$(TARGET) : $(OBJECTS)
	g++ $(FLAGS) -o $(TARGET) $(OBJECTS) $(INCLUDE) $(LIBS)

./bin/%.o : ./src/%.cpp
	g++ $(FLAGS) -c $^ -o $@ $(INCLUDE)

clean :
	rm -f $(OBJECTS) $(TARGET)

run : $(TARGET)

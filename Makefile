INCLUDE = -I./include
LIBS = -lSDL3
DEV_OBJECTS :=  $(patsubst ./src/%.cpp, ./bin/dev_%.o,  $(wildcard ./src/*.cpp))
PROD_OBJECTS := $(patsubst ./src/%.cpp, ./bin/prod_%.o, $(wildcard ./src/*.cpp))

DEV_FLAGS = -g # -Wall
DEV_BUILD = ./bin/dev_build.exe

PROD_FLAGS = -O2 -DNDEBUG
PROD_BUILD = ./bin/prod_build.exe


dev : $(DEV_BUILD)

prod : $(PROD_BUILD)

$(DEV_BUILD) : $(DEV_OBJECTS)
	g++ $(DEV_FLAGS) -o $(DEV_BUILD) $(DEV_OBJECTS) $(INCLUDE) $(LIBS)

$(PROD_BUILD) : $(PROD_OBJECTS)
	g++ $(PROD_FLAGS) -o $(PROD_BUILD) $(PROD_OBJECTS) $(INCLUDE) $(LIBS)

./bin/dev_%.o : ./src/%.cpp
	g++ $(DEV_FLAGS) -c $^ -o $@ $(INCLUDE)

./bin/prod_%.o : ./src/%.cpp
	g++ $(PROD_FLAGS) -c $^ -o $@ $(INCLUDE)

clean :
	rm -f bin/*.o bin/*.exe
SOURCE_FILES += $(wildcard ./*.cpp)

INCLUDE_DIR = -I ./

webbench: $(SOURCE_FILES)
	g++ -ggdb $(INCLUDE_DIR) $(SOURCE_FILES) -lpthread -o ./webbench-cpp.out
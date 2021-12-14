COMPILER = clang++
COMPILER_FLAGS = -std=c++17 -pthread
OPTIMISATION_FLAGS = -O3
DEBUG_OPTIONS = -ggdb -g3 -fsanitize=undefined -fsanitize=address -fno-omit-frame-pointer

CRASH_ON_INCORRECT_FREE =  -D CONCURENT_LIB_THROW_ON_FALIURE=true

SRC_FILES = $(wildcard src/*cpp)

default:
	cd build && \
	$(foreach PATH, $(SRC_FILES), \
		$(COMPILER) $(patsubst %.cpp, ../%.cpp, $(PATH)) -c $(COMPILER_FLAGS) $(OPTIMISATION_FLAGS) $(CRASH_ON_INCORRECT_FREE);)

quiet:
	cd build && \
	$(foreach PATH, $(SRC_FILES), \
		$(COMPILER) $(patsubst %.cpp, ../%.cpp, $(PATH)) -c $(COMPILER_FLAGS) $(OPTIMISATION_FLAGS);)

debug:
	cd build && \
	$(foreach PATH, $(SRC_FILES), \
		$(COMPILER) $(patsubst %.cpp, ../%.cpp, $(PATH)) -c $(COMPILER_FLAGS) $(DEBUG_OPTIONS) $(CRASH_ON_INCORRECT_FREE);)

channels: debug
	$(COMPILER) $(wildcard build/*.o) tests/chan_test.cpp -o ChannelTest $(COMPILER_FLAGS) $(DEBUG_OPTIONS)
	./ChannelTest

select: debug
	$(COMPILER) $(wildcard build/*.o) tests/select_test.cpp -o SelectTest $(COMPILER_FLAGS) $(DEBUG_OPTIONS)
	./SelectTest

semaphore: debug
	$(COMPILER) $(wildcard build/*.o) tests/sem_test.cpp -o SemaphoreTest $(COMPILER_FLAGS) $(DEBUG_OPTIONS)
	./SemaphoreTest

work_pool: debug
	$(COMPILER) $(wildcard build/*.o) tests/work_pool_test.cpp -o WorkPoolTest $(COMPILER_FLAGS) $(DEBUG_OPTIONS)
	./WorkPoolTest
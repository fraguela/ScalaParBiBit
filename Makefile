sources=allocator.cpp murmurhash.cpp ThreadHandler.cpp ServerState.cpp Utils.cpp Options.cpp FileWorker.cpp InputMatrix.cpp Bicluster.cpp main.cpp
arff_sources=arff_attr.cpp arff_data.cpp arff_instance.cpp arff_lexer.cpp arff_parser.cpp arff_scanner.cpp arff_token.cpp arff_utils.cpp arff_value.cpp

root_dir = .
core_dir = .
objs_dir = objs
arff_dir = objs/arff
objs += $(patsubst %.cpp, $(arff_dir)/%.cpp.o, $(arff_sources))
objs += $(patsubst %.cpp, $(objs_dir)/%.cpp.o, $(sources))
mylibs = -lpthread -lm -lz -lgomp
EXEC = ScalaParBibit

#compile optionsK
CXXFLAGS = -I . -I arff -pthread -std=c++11 -O3 -DNDEBUG
MYLIBS += -lz
CXX = mpicxx

all: dir $(objs)
	$(CXX) $(objs) -o $(EXEC) $(MYLIBS) $(CXXFLAGS)
	strip $(EXEC)

dir:
	mkdir -p $(objs_dir)
	mkdir -p $(arff_dir)

clean:
	-rm -rf $(objs_dir) $(arff_dir) $(EXEC)

$(objs_dir)/%.cpp.o: $(core_dir)/%.cpp
	$(CXX) $(MYLIBS) -o $@ -c $< $(CXXFLAGS)
	
$(arff_dir)/%.cpp.o: $(core_dir)/arff/%.cpp
	$(CXX) $(MYLIBS) -o $@ -c $< $(CXXFLAGS)

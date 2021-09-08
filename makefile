
ROOTFLAGS=`root-config --cflags --libs`  
OPT=-g # or -O3
CXXFLAGS=-Wall --pedantic  ${ROOTFLAGS} -std=c++17 -I./include 

all : run/example_analysis

run/example_analysis: examples/*.cpp
	mkdir -p run
	${CXX} ${CXXFLAGS} $^ fwksrc/*.cpp -o run/example_analysis

gentree:
	mkdir -p run
	cd run; root -q -b ../examples/generateTree.C
	echo ""

clean:
	rm -rf run/example_analysis
default:
	g++ -ggdb -Wl,--no-as-needed -std=c++11 `root-config --libs --ldflags` -lTreePlayer -I `root-config --incdir` benchmark.cpp -o benchmark


default: benchmark.cpp
	g++ -ggdb -Wl,--no-as-needed `root-config --libs --ldflags` -lTreePlayer -I `root-config --incdir` benchmark.cpp -o benchmark


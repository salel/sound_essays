main: test.cpp
	g++ -o main test.cpp RtMidi.cpp -lasound -g -Wall -D__LINUX_ALSA__ -pthread

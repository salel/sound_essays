main: test.cpp process_args.h process_args.cpp
	g++ -o main test.cpp RtMidi.cpp process_args.cpp -lasound -g -Wall -D__LINUX_ALSA__ -pthread

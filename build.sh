if [ ! -d "bin" ]; then
    mkdir bin
else
	rm bin/*
fi
g++ -g -O0 -I . -o bin/interrupts interrupts_101301971_101264747.cpp
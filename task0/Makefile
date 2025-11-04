CFLAGS=

all: task0 
	@echo "Done compiling all the tasks"

task0: main.c
	gcc  -o $@	$^ $(CFLAGS)

histogram: task0
	./task0 | python3 plotter.py
	
clean:
	rm -f task0

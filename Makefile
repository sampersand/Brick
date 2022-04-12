all: brick

.PHONY: clean
clean:
	-@rm *.o brick

brick: brick.o token.o ast.o value.o run.o env.o compile.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c | %.h
	$(CC) $(CFLAGS) -c $^ -o $@ 

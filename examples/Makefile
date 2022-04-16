BKC=../brick

main: main.ko knight.ko parse.ko value.ko function.ko env.ko
fizzbuzz: fizzbuzz.ko

.PHONY: clean
clean:
	rm *.ko *.c main fizzbuzz

%: %.ko
	$(CC) -fsanitize=address,undefined -g $^ -o $@

%.ko: %.bk | $(BKC)
	$(BKC) $^ $@ -c

../brick:
	make -C.. brick


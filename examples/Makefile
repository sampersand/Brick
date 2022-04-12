BKC=../brick

knight: knight.ko die.ko
fizzbuzz: fizzbuzz.ko

.PHONY: clean
clean:
	rm *.ko *.c knight fizzbuzz

%: %.ko
	$(CC) $^ -o $@
%.ko: %.bk | $(BKC)
	../brick $^ $@ -c

../brick:
	make -C.. brick


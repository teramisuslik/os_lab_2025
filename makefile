all:
	$(MAKE) -C lab3/src
	$(MAKE) -C lab4/src

clean:
	$(MAKE) -C lab3/src clean
	$(MAKE) -C lab4/src clean
all: lru mru arc min opt

lru: htable.c lru.c cachesim.c
	gcc -ggdb $^ -DALG=1 -o $@
mru: mru.c htable.c cachesim.c
	gcc -ggdb $^ -DALG=2 -o $@
arc: arc.c htable.c cachesim.c
	gcc -ggdb $^ -DALG=3 -o $@
min: min.c htable.c
	gcc -ggdb $^ -o $@
opt: opt.c pqueue.c htable.c
	gcc -ggdb $^ -o $@
clean:
	rm -rf lru mru arc min opt

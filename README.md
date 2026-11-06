# osi

#### 1.6


```bash
gcc -fPIC -c my_pthread_create.c -o my_pthread_create.o
gcc -shared -o libmythread.so my_pthread_create.o
gcc -o test test.c unity.c -L. -lmythread -Wl,-rpath,.
./test
```

#### 1.7

```bash
gcc -o test test.c unity.c uthread.c
```

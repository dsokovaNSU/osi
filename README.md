# osi

#### 1.6


```bash
gcc -fPIC -c 1.6/my_pthread_create.c -o 1.6/my_pthread_create.o
gcc -shared -o 1.6/libmythread.so 1.6/my_pthread_create.o
gcc -o 1.6/test 1.6/test.c common/unity.c -L 1.6/ -lmythread -Wl,-rpath,.
./1.6/test
```

#### 1.7

# Изменения

Освободившиеся слоты массива потоков используются повторно внутри одного uthread_run

При завершении поток передает управление планировщику через yield вместо жесткого возврата в main-контекст.


```bash
gcc -o 1.7/test 1.7/test.c 1.7/uthread.c common/unity.c
./1.7/test
```

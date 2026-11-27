# osi

#### 1.6


```bash
gcc -fPIC -c 1.6/my_pthread_create.c -o 1.6/my_pthread_create.o
gcc -shared -o 1.6/libmythread.so 1.6/my_pthread_create.o
gcc -o 1.6/test 1.6/test.c common/unity.c -L 1.6/ -lmythread -Wl,-rpath,.
./1.6/test
```

#### 1.7

##### Изменения

Освободившиеся слоты массива потоков используются повторно внутри одного uthread_run

При завершении поток передает управление планировщику через yield вместо жесткого возврата в main-контекст.


```bash
gcc -o 1.7/test 1.7/test.c 1.7/uthread.c common/unity.c
./1.7/test
```

#### 2.2

добавила мьютекс lock_2 вместо бинарного семафора и другие семафоры items,spaces никогда не выйдет за границв так как:  в любой момент времени сумма значений семафоров items и spaces строго равна max_count.

queue_add: sem_wait(&spaces) (spaces--), затем sem_post(&items) (items++) - сумма не меняется 

queue_get: sem_wait(&items) (items--), затем sem_post(&spaces) (spaces++) - сумма не меняется

В граничных случаях: 
- Очередь пуста (items == 0). Поток-читатель блокируется на sem_wait(&items) и не может извлечь элемент или вызвать sem_post(&spaces), пока писатель не добавит новый элемент. Счётчик items не уходит в отрицательные значения.

- Очередь заполнена (items == max_count, spaces == 0). Поток-писатель блокируется на sem_wait(&spaces) и не может добавить новый элемент или вызвать sem_post(&items), пока читатель не освободит место. Счётчик items не превышает max_count.

```bash
gcc -Wall -Wextra -g sema_queue.c  queue-threads.c -o queue -pthread
```

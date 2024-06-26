# ИДЗ 4. Сетевые протоколы. Использование UDP.

**Дельцов Даниил Витальевич БПИ226** 

**Вариант 6**  

Условие: Базу данных, представленную массивом целых положительных чисел, разделяют два типа процессов: N читателей и K писателей.   
Читатели периодически просматривают случайные записи базы данных и выводя номер свой номер (например, PID), индекс записи, ее значение, а также вычисленное значение факториала.  
Писатели изменяют случайные записи на случайное число и также выводят информацию о своем номере, индексе записи, старом значении и новом значении.   
Предполагается, что в начале БД находится в непротиворечивом состоянии (все числа отсортированы).   
Каждая отдельная новая запись переводит БД из одного непротиворечивого состояния в другое (то есть, новая сортировка может поменять индексы записей или переставить числа).  
Транзакции выполняются в режиме «грязного чтения». То есть, процесс–писатель не может получить доступ к БД только в том случае, если ее уже занял другой процесс-писатель, а процессы-читатели ему не мешают обратиться к БД.  
Поэтому он может изменять базу данных, когда в ней находятся читатели.  
Создать многопроцессное приложение с потоками-писателями и потоками-читателями.  
Каждый читатель и писатель моделируется отдельным процессом.  

Сценарий использования:
Предположим, что в университете или компании необходимо постоянно обновлять и анализировать большие объемы данных. Писатели могут быть процессами, обновляющими записи о последних событиях или транзакциях, в то время как читатели могут анализировать эти данные для генерации отчетов или вывода статистики.

# 4-5 баллов - ihw4_v1

Серверная программа представляет собой многопроцессное приложение, которое моделирует базу данных и взаимодействует с клиентами, предоставляя операции чтения и записи.

# Реализация клиент-серверного приложения
- [x] Используется `shm_open`, `ftruncate` и `mmap` для создания и отображения разделяемой памяти.
- [x] Инициализируется мьютекс для обеспечения синхронизации доступа к разделяемой памяти между процессами. Используется `pthread_mutex_init` с атрибутом `PTHREAD_PROCESS_SHARED`.
- [x] Создается сокет для прослушивания входящих соединений.  Cокет создается с использованием `socket(AF_INET, SOCK_DGRAM, 0)`
- [x] IP адреса и порты задаются в командной строке, для обеспечения гибкой подстройки к любой сети -> Сокет привязывается к указанному IP-адресу и порту -> начинает прослушивание входящих соединений. IP-адрес и порт задаются в командной строке, и сокет привязывается с использованием `bind`.
- [x] Дочерний процесс создается с помощью `fork()`, и `handle_client()` вызывается в дочернем процессе.
- [x] Используется обработчик сигналов `handle_signal` для закрытия сокета и удаления разделяемой памяти.

# Протокол UDP  

1. Сокет создается с использованием `SOCK_DGRAM`
```
server_fd = socket(AF_INET, SOCK_DGRAM, 0); // Сервер
sock = socket(AF_INET, SOCK_DGRAM, 0); // Клиент
```
2. Для отправки сообщений клиентам используется `sendto`
   Для получения сообщений от клиентов используется `recvfrom`
```
// Отправка сообщения клиенту
sendto(sock, response, strlen(response), 0, (struct sockaddr *)&client_addr, client_len);

// Получение сообщения от клиента
int n = recvfrom(server_fd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&client, &client_len);
```

# Важно
1. Мьютексы обеспечивают корректное взаимодействие процессов при чтении и записи данных, предотвращая гонки данных.
2. Многопроцессная архитектура позволяет обрабатывать несколько клиентов одновременно.
3. Сокеты используются для установления соединений между сервером и клиентами, где клиенты могут отправлять запросы на чтение или запись данных, а сервер отвечает на эти запросы, выполняя соответствующие операции с данными.  

<img width="1792" alt="Снимок экрана 2024-06-14 в 02 49 09" src="https://github.com/danikd1/OS_IHW4/assets/36849026/89874139-b6fa-4c64-8883-a30d8f942d49">

# 6-7 баллов - ihw4_v2  
Программа monitor.c работает как мульткаст-клиент, принимающий данные, отправляемые сервером. Монитор создаёт UDP сокет и настраивает его для прослушивания мульткаст-сообщений по указанному адресу и порту. Это включает присоединение к мульткаст с помощью `IP_ADD_MEMBERSHIP`.  
```
// Присоединение к мульткаст-группе
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_IP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("Setsockopt IP_ADD_MEMBERSHIP failed");
        close(sock);
        return 1;
    }
```
<img width="1791" alt="Снимок экрана 2024-06-14 в 04 31 46" src="https://github.com/danikd1/OS_IHW4/assets/36849026/92df84bf-901f-4cd4-877c-7caedecc0b72">

# 8 баллов - ihw4_v2  

Так как предыдущая версия `monitor.c` включает в себя механизм мульткаста, возможность подключения множества клиентов-наблюдателей к серверу уже реализована. Это позволяет наблюдателям динамически подключаться и отключаться от сервера без перебоев в его работе.  

# 9-10 баллов - ihw4_v3

`controller.c` отправляет команды start или stop на IP адрес, указанный в командной строке, и на фиксированный порт (9090), который прослушивают клиенты.  
```
// управляющие команды
int bytes_received = recvfrom(control_sock, buffer, BUFFER_SIZE - 1, MSG_DONTWAIT, (struct sockaddr *)&from, &from_len);
if (bytes_received > 0) {
    buffer[bytes_received] = '\0';  // Null-terminate the received string
    if (strcmp(buffer, "stop") == 0) {
        client_paused = 1;
        printf("Client paused\n");
    } else if (strcmp(buffer, "start") == 0) {
        client_paused = 0;
        printf("Client resumed\n");
    }
}
```
`client.c` прослушивает порт 9090 для получения управляющих команд. Когда приходит команда stop, клиент перестает отправлять запросы на сервер; команда start активирует клиента обратно.
`server.c` отправляет сообщения "shutdown" через мультикаст при завершении работы для корректного завершения работы всех клиентов.
```
bytes_received = recvfrom(multicast_sock, buffer, BUFFER_SIZE - 1, MSG_DONTWAIT, (struct sockaddr *)&from, &from_len);
if (bytes_received > 0) {
    buffer[bytes_received] = '\0';  // Null-terminate the received string
    if (strcmp(buffer, "shutdown") == 0) {
        client_running = 0;
        printf("Client received shutdown message\n");
    }
}
```
1. Проверка работы **Start-Stop**
<img width="1792" alt="Снимок экрана 2024-06-14 в 05 13 20" src="https://github.com/danikd1/OS_IHW4/assets/36849026/7a79855d-de12-4c3f-ab42-9e73da7a5f8c">

2. Провера работы **Shutdown**
<img width="1775" alt="Снимок экрана 2024-06-14 в 05 17 55" src="https://github.com/danikd1/OS_IHW4/assets/36849026/09b31383-7b42-4b01-8a06-5ef3953fd8eb">



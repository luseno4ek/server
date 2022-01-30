## Серверная часть пользовательского чата

Код можно посмотреть [здесь](https://github.com/luseno4ek/server/blob/bf90a0af6bb2c3a62fda7571f393ccbe0c0d97be/server/main.c).

### Описание работы

Чат представляет собой один серверный процесс, подключающий клиентов через сокет. Для подключения к серверу и отправки сообщений в чат используется команда **telnet**.

При подключении нового пользователя чат запрашивает у пользователя уникальное имя. Выйти из чата пользователь может с помощью команды **bye!**. В течение рабочей сессии сервер отправляет сообщения о состоянии чата (количество подключенных пользователей) и о новых событиях - подключении и отключении пользователей.

### Пример работы сервера

(base) luseno4ek@MacBook-Air-Olesya server % ./main 747
\# chat is running at port 747
\# chat server is running (0 users are connected...)
\# user_1 joined the chat
\# user_1 changed name to 'lusenochek'
[lusenochek]: hi!
\# chat server is running (1 users are connected...)
\# user_2 joined the chat
\# user_2 changed name to 'kotik'
[kotik]: hello!
\# chat server is running (2 users are connected...)
[lusenochek]: hi!
\# chat server is running (2 users are connected...)
[lusenochek]: I'm leaving
\# lusenochek left the chat
[kotik]: bye lusenochek!
\# kotik left the chat
\# chat server is running (0 users are connected...)


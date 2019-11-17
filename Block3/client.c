#include "client.h"

Connection *connection_create(int protocol, char *address, char *port)
{

    int sockfd = 0;
    struct addrinfo hints;
    struct addrinfo *results;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = (protocol == UDP) ? SOCK_DGRAM : SOCK_STREAM;

    int retval = 0;
    if ((retval = getaddrinfo(address, port, &hints, &results)) != 0)
    {
        fprintf(stderr, "[client]: Could not find address! ");
        freeaddrinfo(results);
        exit(1);
    }

    while (results != NULL)
    {
        if ((sockfd = socket(results->ai_family, results->ai_socktype, results->ai_protocol)) == -1)
        {
            results = results->ai_next;
            continue;
        }
        if (connect(sockfd, results->ai_addr, results->ai_addrlen) == -1)
        {
            results = results->ai_next;
            continue;
        }
        break;
    }
    if (results == NULL) return NULL;

    Connection *connection = calloc(1, sizeof(Connection));
    connection->sockfd = sockfd;
    connection->addrinfo = results;
    return connection;
}

int connection_close(Connection *connection)
{
    if (close(connection->sockfd) != 0)
        return 1;
    if (connection->addrinfo != NULL)
        freeaddrinfo(connection->addrinfo);
    free(connection);
    return 0;
}

char *connection_recv_tcp(Connection *connection)
{
    char *buffer = calloc(MAXDATASIZE, sizeof(char));
    ssize_t bytes_received = 0;
    if ((bytes_received = recv(connection->sockfd, buffer, MAXDATASIZE - 1, 0)) < 1)
    {
        free(buffer);
        fprintf(stderr, "[client]: Could not receive data!\n");
        return NULL;
    }
    return buffer;
}

int connection_send_tcp(Connection *connection, char *message, int msg_size)
{
    if (send(connection->sockfd, message, msg_size, 0) == -1)
        return 1;
    else
        return 0;
}

Message *unmarshall(char *buffer, Message *m)
{
    m->del = ((buffer[0] & DEL) > 0) ? 1 : 0;
    m->set = ((buffer[0] & SET) > 0) ? 1 : 0;
    m->get = ((buffer[0] & GET) > 0) ? 1 : 0;
    m->ack = ((buffer[0] & ACK) > 0) ? 1 : 0;
    m->transaction_id = buffer[1];

    // dummy way to get the host by order - in this case just fixed assumtpion that the host has LSB > MSB  order
    unsigned int int_key_length = (buffer[2] * 16) + (buffer[3]);
    unsigned int int_value_length = (buffer[4] * 16) + (buffer[5]);

    m->key_length = int_key_length;
    m->value_length = int_value_length;

    char *key = malloc(m->key_length * sizeof(char) + 1);
    char *value = malloc(m->value_length * sizeof(char) + 1);

    if (m->get)
    {
        key = m->key;
        value = m->value;
    }

    memcpy(key, buffer + 6, m->key_length);
    key[m->key_length] = '\0';
    memcpy(value, buffer + 6 + m->key_length, m->value_length);
    value[m->value_length] = '\0';
    return m;
}

char *marshall(Message *m)
{
    if (m->value == NULL)
    {
        m->value = malloc(1);
        m->value[0] = '\0';
    }

    char *buffer = NULL;
    if (m->set == 1)
    {
        buffer = calloc(1, 6 + strlen(m->key) + strlen(m->value));
        m->size = 6 + strlen(m->key) + strlen(m->value); // set the size of the buffer to appropiate value

        buffer[0] |= SET;
        buffer[1] = m->transaction_id;

        short net_key_length = htons(strlen(m->key));
        short net_value_length = htons(strlen(m->value));
        memcpy(buffer + 2, &net_key_length, 2);
        memcpy(buffer + 4, &net_value_length, 2);
        memcpy(buffer + 6, m->key, strlen(m->key));
        memcpy(buffer + 6 + strlen(m->key), m->value, strlen(m->value));
    }
    else // in case of key only
    {
        buffer = calloc(1, 6 + strlen(m->key) + 1);
        m->size = 6 + strlen(m->key) + 1; // set the size of the buffer to appropiate value
        if (m->del == 1)
            buffer[0] |= DEL;
        else
            buffer[0] |= GET;
        buffer[1] = m->transaction_id;

        char *zero_value = "\0";
        short net_key_length = htons(strlen(m->key));
        short net_value_length = 0;
        memcpy(buffer + 2, &net_key_length, 2);
        memcpy(buffer + 4, &net_value_length, 2);
        memcpy(buffer + 6, m->key, strlen(m->key));
        memcpy(buffer + 6 + strlen(m->key), zero_value, 1);
    }
    return buffer;
}

char *get(Connection *connection, char *key)
{
    Message *m = calloc(1, sizeof(Message));
    m->set = 0;
    m->get = 1;
    m->del = 0;
    m->ack = 0;
    m->transaction_id = 0;
    m->key_length = strlen(key);
    m->key = key;
    m->size = 0;

    char *buffer = marshall(m);
    if (connection_send_tcp(connection, buffer, m->size))
    {
        free(m);
        return NULL;
    }

    free(buffer); //reuses same variable
    buffer = connection_recv_tcp(connection);

    if (buffer == NULL)
    {
        free(m);
        return NULL;
    }

    m = unmarshall(buffer, m);
    if (m->ack != 1)
    {
        free(m);
        return NULL;
    }

    char *res = m->value;
    free(buffer);
    free(m);
    return res;
}

int set(Connection *connection, char *key, char *value)
{
    Message *m = calloc(1, sizeof(Message));
    m->set = 1;
    m->get = 0;
    m->del = 0;
    m->ack = 0;
    m->transaction_id = 0;
    m->key_length = strlen(key);
    m->key = key;
    m->value_length = strlen(value);
    m->value = value;

    char *buffer = marshall(m);
    if (connection_send_tcp(connection, buffer, m->size))
    {
        free(m);
        return 0;
    }

    free(buffer); //reuses the same variable
    buffer = connection_recv_tcp(connection);

    if (buffer == NULL)
    {
        free(m);
        return 0;
    }

    m = unmarshall(buffer, m);
    if (m->ack != 1)
    {
        free(m);
        return 0;
    }

    free(buffer);
    free(m);
    return 1;
}

int del(Connection *connection, char *key)
{
    Message *m = calloc(1, sizeof(Message));
    m->set = 0;
    m->get = 0;
    m->del = 1;
    m->ack = 0;
    m->transaction_id = 0;
    m->key_length = strlen(key);
    m->key = key;

    char *buffer = marshall(m);
    if (connection_send_tcp(connection, buffer, m->size))
    {
        free(m);
        return 0;
    }

    free(buffer); //reuses the same variable
    buffer = connection_recv_tcp(connection);

    if (buffer == NULL)
    {
        free(m);
        return 0;
    }

    m = unmarshall(buffer, m);
    if (m->ack != 1)
    {
        free(buffer);
        free(m);
        return 0;
    }

    free(buffer);
    free(m);
    return 1;
}

int main(int argc, char *argv[])
{

    if (argc != 6 && argc != 5)
    {
        fprintf(stderr, "[client]: wrong usage: ./client <ip_address> <port_number> <command> <key> <value>\n");
        return 1;
    }

    Connection *server = connection_create(TCP, argv[1], argv[2]);
    if (server == NULL)
    {
        fprintf(stderr, "[client]: Error while establishing connection\n");
        return 1;
    }

    if (strcmp(argv[3], "SET") == 0)
    {
        if (set(server, argv[4], argv[5]))
            fprintf(stderr, "[client]: SET successful!\nKey: %s\nValue: %s\n", argv[4], argv[5]);
        else
        {
            fprintf(stderr, "[client]: Something went wrong\n");
            connection_close(server);
            return 1;
        }
    }

    else if (strcmp(argv[3], "GET") == 0)
    {
        char *message = get(server, argv[4]);
        if (message != NULL)
        {
            fprintf(stderr, "[client]: GET successful!\nKey: %s\nValue: %s\n", argv[4], message);
            free(message);
        }
        else
        {
            fprintf(stderr, "[client]: Something went wrong\n");
            connection_close(server);
            return 1;
        }
    }

    else if (strcmp(argv[3], "DELETE") == 0)
    {
        if (del(server, argv[4]))
            fprintf(stderr, "[client]: DELETE successful!\nKey: %s\n", argv[4]);
        else
        {
            fprintf(stderr, "[client]: Something went wrong\n");
            connection_close(server);
            return 1;
        }
    }
    else
    {
        connection_close(server);
        fprintf(stderr, "[client]: Please enter a valid command: (SET, GET, DELETE)\n");
        return 1;
    }
    connection_close(server);
    return 0;
}
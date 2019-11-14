#include "client.h"

int main(int argc, char *argv[])
{

    if (argc != 6 && argc != 5)
    {
        fprintf(stderr, "[client]: wrong usage: ./client <ip_address> <port_number> <command> <key> <value>\n");
        return 1;
    }

    //Establishing a connection
    Connection *server = connection_create(TCP, argv[1], argv[2]);
    if (server == NULL)
    {
        printf("[client]: Error while establishing connection\n");
        return 1;
    }

    /* SET */
    if (strcmp(argv[3], "SET") == 0)
    {
        if (set(server, argv[4], argv[5]))
            printf("[client]: SET successful!\nKey: %s\nValue: %s\n", argv[4], argv[5]);
        else
        {
            printf("[client]: Something went wrong\n");
            connection_close(server);
            return 1;
        }
    }

        /* GET */
    else if (strcmp(argv[3], "GET") == 0)
    {
        char *message = get(server, argv[4]);
        if (message != NULL)
        {
            printf("[client]: GET successful!\nKey: %s\nValue: %s\n", argv[4], message);
            free(message);
        }
        else
        {
            printf("[client]: Something went wrong\n");
            connection_close(server);
            return 1;
        }
    }

        /* DELETE */
    else if (strcmp(argv[3], "DELETE") == 0)
    {
        if (del(server, argv[4]))
            printf("[client]: DELETE successful!\nKey: %s\n", argv[4]);
        else
        {
            printf("[client]: Something went wrong\n");
            connection_close(server);
            return 1;
        }
    }

    else
    {
        connection_close(server);
        printf("[client]: Please enter a valid command: (SET, GET, DELETE)\n");
        return 1;
    }

    connection_close(server);

    return 0;
}
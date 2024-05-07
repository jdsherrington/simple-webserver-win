#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>

int main(int argc, char *argv[]) {
    // Check if port number is provided, default to 8080 if not
    int PORT = (argc > 1) ? atoi(argv[1]) : 8080;

    // attempts to initialise Winsock version 2.2
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { 
        // prints error that caused failure
        printf("Initialization failed: %d", WSAGetLastError());
        // returns fail
        return 1;
    }


    // --- Create Socket ---

    SOCKET server_socket;
    // TCP/IP Socket: AF_INET = IPV4, SOCK_STREAM = stream socket, 0 = default for specified socket type
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // checks if socket creation failed
    if (server_socket == INVALID_SOCKET) {
        // print error that caused failure
        printf("Failed to create socket: %d", WSAGetLastError());
        // clean up Winsock
        WSACleanup();
        // returns fail
        return 1;
    }
    
    printf("Socket created successfully.\n");


    // --- Bind Socket ---

    // initialises socket address struct
    struct sockaddr_in server_address;
    // specify address family as IPV4
    server_address.sin_family = AF_INET;
    // specify that the server will listen on all available network interfaces
    server_address.sin_addr.s_addr = INADDR_ANY;
    // converts port number to network byte order and stores it as the port number for the server
    server_address.sin_port = htons(PORT);

    // binds socket to the address and port, and checks for failure
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
        // prints the error that caused failure
        printf("Could not bind socket: %d", WSAGetLastError());
        // clean up Winsock
        closesocket(server_socket);
        WSACleanup();
        // returns fail
        return 1;
    }

    printf("Bind successful.\n");


    // --- Listen on the Socket ---

    // attempts to listen on socket, specifying maximum backlog queue length
    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        // prints the error that caused failure
        printf("Listen failed: %d", WSAGetLastError());
        // clean up Winsock
        closesocket(server_socket);
        WSACleanup();
        // returns fail
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        // --- Block on Accept Until Connection Made ---

        // initialises variable that will hold the socket descriptor for the client connection
        SOCKET client_socket;
        // struct holds information about client's address and port
        struct sockaddr_in client_address;
        // holds the size of client's address structure
        int client_address_length = sizeof(client_address);
        // call accept() with server socket, pointer to the structure for client's address info, and length of client's address structure
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_length);
        // checks if accept failed
        if (client_socket == INVALID_SOCKET) {
            // prints the error that caused failure
            printf("Accept failed: %d", WSAGetLastError());
            // clean up Winsock
            closesocket(server_socket);
            WSACleanup();
            // returns fail
            return 1;
        }

        printf("Connection accepted.\n");


        // --- Read on Connected Socket ---

        // set buffer size of received data to 1024 bytes
        #define BUFFER_SIZE 1024
        char buffer[BUFFER_SIZE];
        
        // receive data from client
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);

        // if error occurs with bytes
        if (bytes_received == SOCKET_ERROR) {
            printf("Receive failed: %d", WSAGetLastError());
            // clean up Winsock
            closesocket(client_socket);
            closesocket(server_socket);
            WSACleanup();
            // returns fail
            return 1;
        // if no bytes received/client disconnects
        } else if (bytes_received == 0) {
            printf("Client disconnected.\n");
            printf("Server listening on port %d...\n", PORT);
            
        } else {
            // data received successfully
            printf("Received %d bytes from client:\n", bytes_received);
            printf("%.*s\n", bytes_received, buffer);


            // --- Write back on the Connected Socket ---

            // HTTP response message
            const char* response_header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
            // Send the HTTP response header to the client
            int header_length = strlen(response_header);
            int bytes_sent = send(client_socket, response_header, header_length, 0);

            // Check if sending failed
            if (bytes_sent == SOCKET_ERROR) {
                printf("Send failed: %d", WSAGetLastError());
                // clean up Winsock
                closesocket(client_socket);
                closesocket(server_socket);
                WSACleanup();
                // returns fail
                return 1;
            }
            
            // Data sent successfully
            printf("Sent %d bytes to client:\n%s\n", bytes_sent, response_header);


            // --- Open and read the contents of the HTML file ---

            FILE* html_file = fopen("index.html", "r");
            if (html_file == NULL) {
                printf("Error opening HTML file.\n");
                // clean up Winsock
                closesocket(client_socket);
                closesocket(server_socket);
                WSACleanup();
                // returns fail
                return 1;
            }


            // --- Send the contents of the HTML file as the response body ---

            char buffer[BUFFER_SIZE];
            // reads HTML file line by line
            while (fgets(buffer, BUFFER_SIZE, html_file) != NULL) {
                int bytes_sent = send(client_socket, buffer, strlen(buffer), 0);
                if (bytes_sent == SOCKET_ERROR) {
                    printf("Send failed: %d", WSAGetLastError());
                    // clean up Winsock
                    fclose(html_file);
                    closesocket(client_socket);
                    closesocket(server_socket);
                    WSACleanup();
                    // returns fail
                    return 1;
                }
            }

            // Close the HTML file
            fclose(html_file);

        }

        // Close the client socket
        closesocket(client_socket);
    }
    

    // clean up Winsock
    closesocket(server_socket);
    WSACleanup();
    return 0;
}
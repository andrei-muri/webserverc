#include <unistd.h>
#include <arpa/inet.h> //inet_addr, htons
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> //core socket functionalities
#include <netinet/in.h> //sockadr_in
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <pthread.h>
 
#define PORT 8080

const char* fileNotFoundResponse = 
    "HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: 53\r\n"
    "\r\n"
    "<html><body><h1>404 File not found</h1></body></html>";

const char* fileNotAccessibleResponse = 
    "HTTP/1.1 403 Forbidden\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: 58\r\n"
    "\r\n"
    "<html><body><h1>403 File not accessible</h1></body></html>";


typedef struct HTTPreq {
    char method[8];
    char route[128];
    char mime[32];
} httpreq;

void set_mime_type(httpreq *request) {
    const char *ext = strrchr(request->route, '.'); 

    if (!ext) {
        strncpy(request->mime, "text/plain", sizeof(request->mime) - 1);
    } else if (strcmp(ext, ".html") == 0) {
        strncpy(request->mime, "text/html", sizeof(request->mime) - 1);
    } else if (strcmp(ext, ".css") == 0) {
        strncpy(request->mime, "text/css", sizeof(request->mime) - 1);
    } else if (strcmp(ext, ".js") == 0) {
        strncpy(request->mime, "application/javascript", sizeof(request->mime) - 1);
    } else if (strcmp(ext, ".png") == 0) {
        strncpy(request->mime, "image/png", sizeof(request->mime) - 1);
    } else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) {
        strncpy(request->mime, "image/jpeg", sizeof(request->mime) - 1);
    } else if (strcmp(ext, ".gif") == 0) {
        strncpy(request->mime, "image/gif", sizeof(request->mime) - 1);
    } else {
        strncpy(request->mime, "application/octet-stream", sizeof(request->mime) - 1); 
    }

    request->mime[sizeof(request->mime) - 1] = '\0';
}

int is_supported_mime(const char *httpstring) {
    //if root path just return true because root path is converted to /index.html
    if (strcmp(httpstring, "/") == 0) {
        return 1;
    }
    const char *supported_extensions[] = {
        ".html", ".css", ".js", ".png", ".jpg", ".jpeg", ".gif", ".ico"
    };
    size_t num_extensions = sizeof(supported_extensions) / sizeof(supported_extensions[0]);

    for (size_t i = 0; i < num_extensions; i++) {
        if (strstr(httpstring, supported_extensions[i]) != NULL) {
            return 1; 
        }
    }
    return 0;  
}

httpreq* parseHTTPstring(const char* httpstring) {
    

    httpreq* request = (httpreq*) malloc(sizeof(httpreq));
    if (!request) return NULL;

    if (sscanf(httpstring, "%7s %127s", request->method, request->route) == 2) {
        if (!is_supported_mime(request->route)) {
            free(request);
            return NULL;
        }
        if (strcmp(request->method, "GET") == 0) {
            set_mime_type(request); 
            if (strcmp(request->route, "/") == 0) {
                strncpy(request->route, "/index.html", sizeof(request->route) - 1);
                request->route[sizeof(request->route) - 1] = '\0';
                strcpy(request->mime, "text/html");
            }
            
            
            printf("\n\nPath found: %s\nMIME Type: %s\n\n", request->route, request->mime);
            return request;
        }
    }

    
    return NULL;
}



char* getFileContent(const char* path) {
    struct stat file_stat;


    if (stat(path, &file_stat) < 0) {
        perror("Error getting file status");
        return NULL;
    }

    int file_fd = open(path, O_RDONLY);
    if (file_fd < 0) {
        perror("Error opening file");
        return NULL;
    }

    char* file_content = (char*) malloc(file_stat.st_size + 1);
    if (!file_content) {
        perror("Memory allocation failed");
        close(file_fd);
        return NULL;
    }

    ssize_t bytes_read = read(file_fd, file_content, file_stat.st_size);
    if (bytes_read < 0) {
        perror("Error reading file");
        free(file_content);
        close(file_fd);
        return NULL;
    }

    file_content[bytes_read] = '\0'; 
    close(file_fd);
    return file_content;
}

int isMedia(const char* mime) {
    return (strcmp(mime, "image/jpeg") == 0) ||
           (strcmp(mime, "image/png") == 0) ||
           (strcmp(mime, "image/gif") == 0) ||
           (strcmp(mime, "image/x-icon") == 0);
}

void sendResponse(httpreq* request, int client_fd ) {
    char response[4096];
    char fullpath[132] = "html";
    strcat(fullpath, request->route);

    if (access(fullpath, F_OK) == -1) {
        perror("Entered");
        strcpy(response, fileNotFoundResponse);
    } else if (access(fullpath, R_OK) == -1) { 
        strcpy(response, fileNotAccessibleResponse);
    } else if (!isMedia(request->mime)) {
        char* file_content = getFileContent(fullpath);
        if (file_content) {
            snprintf(response, 4096, 
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: %s\r\n"
                     "Content-Length: %ld\r\n"
                     "\r\n"
                     "%s",
                     request->mime, strlen(file_content), file_content);
            free(file_content);
            send(client_fd, response, strlen(response), 0);
        } else {
            strcpy(response, fileNotFoundResponse);
        }
    } else if (isMedia(request->mime)) { //if the file is a media one, send it with sendfile() syscall
        int media_fd = open(fullpath, O_RDONLY);
        struct stat mediaStat;
        if (media_fd < 0 || stat(fullpath, &mediaStat) < 0) {
            perror("Error opening file");
            send(client_fd, fileNotFoundResponse, strlen(fileNotFoundResponse), 0);
            return;
        }

        char header[512];
        snprintf(header, sizeof(header),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: %s\r\n"
                "Content-Length: %ld\r\n"
                "\r\n",
                request->mime, mediaStat.st_size);
        
        send(client_fd, header, strlen(header), 0);
        off_t offset = 0;
        sendfile(client_fd, media_fd, &offset, mediaStat.st_size);
        close(media_fd);
    }
}

void* handle_client(void* arg) {
    int client_fd = *(int*)arg;
    free(arg); 

    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));

    httpreq* request;

    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        request = parseHTTPstring(buffer);
    } else {
        perror("Error in receiving data");
        close(client_fd);
        return NULL;
    }

    if (request) {
        printf("\nReceived HTTP request: %s %s %s\n\n", request->method, request->route, request->mime);
        sendResponse(request, client_fd);
        free(request);
    } else {
        send(client_fd, fileNotFoundResponse, strlen(fileNotFoundResponse), 0);
    }

    close(client_fd);
    return NULL;
}

int main() {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0); //IP_v4, TCP connection, IP protocol

    if(socket_fd == -1) {
        perror("Error in socket initialization");
        exit(-1);
    }
    //socket settings
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT); //from local order (little endian) to network (big endian) (host to network short)
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); //host to network long

    if(bind(socket_fd, (struct sockaddr* ) &addr, sizeof(addr)) == -1) {
        perror("Error in binding");
        exit(-1); 
    } 

    int backlog = 5;
    if(listen(socket_fd, backlog) == -1) {
        perror("Error in listening");
        exit(-1);
    }

    while (1) {
        int *client_fd = malloc(sizeof(int)); 
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        memset(&client_addr, 0, sizeof(client_addr));

        *client_fd = accept(socket_fd, (struct sockaddr*) &client_addr, &client_len);
        if (*client_fd == -1) {
            perror("Error in accepting");
            free(client_fd);
            continue;
        }

        printf("New client connected from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Create a new thread for each client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, client_fd) != 0) {
            perror("Error creating thread");
            free(client_fd);
            close(*client_fd);
        } else {
            pthread_detach(thread_id); 
        }
    }

}

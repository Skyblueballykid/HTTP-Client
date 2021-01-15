#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "8888" //the connection port

#define MAXDATASIZE 1000 // max number of bytes we can get at once

FILE *fileptr;
char header_keys[][50] = {"Date: ", "Hostname: ", "Location: ", "Content-Type: "};
char status[4] = {0, 0, 0, 0};
char filepath[256];
char fileType[256];

int get_request(char *url);
int validIP(char *ip);
int parseHTTPheader(char *header);
char *split_header_keys(char *line, int index);
void openFile();

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char **argv)
{
    int sockfd, ret;  
    int get;
    int rv;
    char buffer[MAXDATASIZE];
    size_t received;
    char s[INET6_ADDRSTRLEN];
    char ok_status[] = "OK";
    char ok_http[] = "HTTP/1.0 200 OK";
    char *url, *http, *temp, *host;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in addr;
    socklen_t addr_size;

    if (argc < 2)
    {
        printf("wrong number of arguments \n");
        exit(1);
    }

    openFile();

    url = argv[1];

    get = get_request(url);

    memset(&buffer, 0, sizeof(buffer));
    ret = recv(get, buffer, MAXDATASIZE, 0);
    if (ret < 0)
    {
        printf("Error in HTTP response\n");
    }
    else
    {
        if ((temp = strstr(buffer, ok_http)) != NULL)
        {
            send(get, ok_status, strlen(ok_status), 0);
        }
        else
        {
            close(get);
            return 0;
        }
    }

    memset(&buffer, 0, sizeof(buffer));
    fileptr = fopen("output", "wb+" );
    ret = recv(get, buffer, MAXDATASIZE, 0);
    while(1) 
    {

        if (ret < 0)
        {
        printf("Error in connection\n");
        }

        else
        {
        printf("%s", buffer);
        fprintf(fileptr, "%s", buffer);
        printf("success");
        }
    }
    fclose(fileptr);
    close(get);

    return 0;
}

//checks that the IP address is valid (case for when when a hostname is not given)
int validIP(char *ip)
{
    struct sockaddr_in addr;
    int validip = inet_pton(AF_INET, ip, &(addr.sin_addr));
    return validip != 0;
}

// The HTTP GET request function
int get_request(char *url)
{

    int sockfd;
    char *ptr, *host, *filedir1, *filedir2, *file, *ch;
    char getrequest[MAXDATASIZE];
    struct sockaddr_in addr;

    if (validIP(url))
    { //IP address case
    // HTTP specifies that the end of a request should be marked by a blank line
    // Each request ends with two newlines- \n\n
        sprintf(getrequest, "GET / HTTP/1.0\n HOST: %s\n\n", url);
        printf(getrequest);
        //printf("test1");
    }
    else
    {
        if ((ptr = strstr(url, "/")) == NULL)
        { //Host name only with no file path
            sprintf(getrequest, "GET / HTTP/1.0\nHOST: %s\n\n", url);
            printf(getrequest);
            //printf("test2");
        }
        else
        {
            //Host name + file path
            strcpy(filepath, ptr);
            file = strrchr(url, '/');
            host = strtok(url, "/");
            host = strtok(NULL, "/");
            filedir1 = strtok(filepath, "//");
            filedir1 = strtok(NULL, "/");
            filedir2 = strtok(filepath, "/");
            sprintf(getrequest, "GET /%s%s HTTP/1.0\n\n", filedir1, file, url);
            //printf(getrequest);
            //printf("test3\n");
        }
    }
    
    //create a socket to the host
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("Error creating socket\n");
        exit(1);
    }
    //printf("Socket created successfully\n");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(PORT));

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("Connection Error\n");
        exit(1);
    }

    write(sockfd, getrequest, strlen(getrequest));

    return sockfd;
}

int parseHTTPheader(char *header)
{
    char *line, *key, *value;
    char temp_value[1000];
    int num = 0;
    line = strtok(header, "\n");
    while (line != NULL)
    {
        strcpy(temp_value, line);
        value = split_header_keys(line, num);
        if (num == 3)
        {
            strcpy(fileType, value);
        }
        line = strtok(NULL, "\n");
        num++;
    }
    for (num = 0; num < 4; num++)
    {
        if (status[num] == 0)
            return 1;
    }
    return 0;
}

char *split_header_keys(char *line, int index)
{
    char *value;
    if ((value = strstr(line, header_keys[index])) != NULL)
    {
        value = value + strlen(header_keys[index]);
        status[index] = 1;
    }
    return value;
}

void openFile()
{
    char *temp;
    char command[100];
    char fileName[1000];
    strcpy(fileName, filepath);

    system(command);
}

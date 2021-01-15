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
char keys[][25] = {"Date: ", "Hostname: ", "Location: ", "Content-Type: "};
char status[4] = {0, 0, 0, 0};
char filepath[256];
char contentFileType[100];

int get_request(char *url);
int validIP(char *ip);
int parseHeader(char *header);
char *splitKeyValue(char *line, int index);
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
    char s[INET6_ADDRSTRLEN];
    char status_ok[] = "OK";
    char http_ok[] = "HTTP/1.0 200 OK";
    char *url, *temp;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in addr;
    socklen_t addr_size;

    if (argc < 2)
    {
        printf("wrong number of arguments \n");
        exit(1);
    }

    url = argv[1];

    get = get_request(url);

    memset(&buffer, 0, sizeof(buffer));
    ret = recv(get, buffer, MAXDATASIZE, 0);
    if (ret < 0)
    {
        printf("Error receiving HTTP status!\n");
    }
    else
    {
        //printf("%s\n", buffer);
        if ((temp = strstr(buffer, http_ok)) != NULL)
        {
            send(get, status_ok, strlen(status_ok), 0);
        }
        else
        {
            close(get);
            return 0;
        }
    }

    memset(&buffer, 0, sizeof(buffer));
    ret = recv(get, buffer, MAXDATASIZE, 0);
    if (ret < 0)
    {
        printf("Error receiving HTTP header!\n");
    }
    else
    {
        //printf("%s\n", buffer);
        if (parseHeader(buffer) == 0)
        {
            send(get, status_ok, strlen(status_ok), 0);

            //prints the file
            //fprintf(fileptr, "%s", buffer);
            fileptr = fopen("output.txt", "w" );
            fwrite(&buffer, sizeof(buffer), 1, fileptr);
            fclose(fileptr);
        }
        else
        {
            //prints the file
            fileptr = fopen("output.txt", "w" );
            fwrite(&buffer, sizeof(buffer), 1, fileptr);
            fclose(fileptr);
            printf("Error in HTTP header!\n");
            close(sockfd);
            return 0;
        }
    }

    //printf("file: [%s]\n", fileName);
    fileptr = fopen("output.txt", "w");
    if (fileptr == NULL)
    {
        printf("Error opening file!\n");
        close(sockfd);
        return 0;
    }

    memset(&buffer, 0, sizeof(buffer));
    while (recv(get, buffer, MAXDATASIZE, 0) > 0)
    { //receives the file

            //fprintf(fileptr, "%s", buffer);
            fileptr = fopen("output.txt", "w" );
            fwrite(&buffer, sizeof(buffer), 1, fileptr);
            fclose(fileptr);

        memset(&buffer, 0, sizeof(buffer));
    }

    fclose(fileptr);
    close(sockfd);

    openFile();

    return 0;
}

//checks that the IP address is valid (case for when when a hostname is not given)
int validIP(char *ip)
{
    struct sockaddr_in addr;
    int valid = inet_pton(AF_INET, ip, &(addr.sin_addr));
    return valid != 0;
}

// The HTTP GET request function
int get_request(char *url)
{

    int sockfd;
    char *ptr, *host;
    char getrequest[MAXDATASIZE];
    struct sockaddr_in addr;

    if (validIP(url))
    { //IP address case
        sprintf(getrequest, "GET / HTTP/1.0\n HOST: %s\n\n", url);
    }
    else
    {
        if ((ptr = strstr(url, "/")) == NULL)
        { //Host name only with no file path
            sprintf(getrequest, "GET / HTTP/1.0\nHOST: %s\n\n", url);
        }
        else
        {
            //Host name + file path
            strcpy(filepath, ptr);
            host = strtok(url, "/");
            sprintf(getrequest, "GET %s HTTP/1.0\nHOST: %s\n\n", filepath, url);
        }
    }
    
    //create a socket to the host
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("Error creating socket\n");
        exit(1);
    }
    printf("Socket created successfully\n");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(PORT));

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("Connection Error!\n");
        exit(1);
    }
    printf("Connection successful \n\n\n");
    ptr = strtok(filepath, "/");
    strcpy(filepath, ptr);

    write(sockfd, getrequest, strlen(getrequest));

    return sockfd;
}

int parseHeader(char *header)
{
    char *line, *key, *value;
    char temp[100];
    int i = 0;
    line = strtok(header, "\n");
    while (line != NULL)
    {
        //printf("%s\n", line);
        strcpy(temp, line);
        value = splitKeyValue(line, i);
        if (i == 3)
        {
            strcpy(contentFileType, value);
        }
        //printf("value=%s\n", value);
        line = strtok(NULL, "\n");
        i++;
    }
    for (i = 0; i < 4; i++)
    {
        if (status[i] == 0)
            return 1;
        //printf("status[%d]=%d\n", i, status[i]);
    }
    return 0;
}

char *splitKeyValue(char *line, int index)
{
    char *temp;
    if ((temp = strstr(line, keys[index])) != NULL)
    {
        temp = temp + strlen(keys[index]);
        status[index] = 1;
    }
    return temp;
}

void openFile()
{
    char *temp;
    char command[100];
    char fileName[1000];
    strcpy(fileName, filepath);
    printf("File Name: %s\n", fileName);
    printf("Content Type: %s\n", contentFileType);
    if ((temp = strstr(contentFileType, "text/html")) != NULL)
    {
        if ((temp = strstr(fileName, ".txt")) != NULL)
        {
            sprintf(command, "gedit %s", fileName);
        }
        else
        {
            sprintf(command, "firefox %s", fileName);
        }
        system(command);
    }
    else if ((temp = strstr(contentFileType, "application/pdf")) != NULL)
    {
        sprintf(command, "acroread %s", fileName);
        system(command);
    }
    else
    {
        printf("The filetype %s is not supported. Failed to open %s!\n", contentFileType, fileName);
    }
}

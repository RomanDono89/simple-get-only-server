#include<stdio.h> 
#include<fcntl.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<signal.h>
#include<arpa/inet.h>

int grep();

void cleanup() {
    fputs("Cleaning up before exiting...\n", stderr);
    exit(-1);
}

	char buffer[512];
	char fbuffer[512];

int main(argc, argv)
int argc;
char **argv;
{
	if (signal(SIGINT, cleanup) == SIG_ERR) {
        perror("Failed to register signal handler");
        exit(1);
    }
	int i = 0; //utility integer

	//get the port
	unsigned int port = 0x901f; //default port 8080
	
	while(i < (argc-1)){
	if(*argv[i] == '-' && *((argv[i])+1) == 'p'){
	if(sscanf(argv[i+1], "%u",&port) != 1 || port > 65535){
	fputs("bad option, usage: server -p (port)\n port has to be smaller than 65535 and a positive number\n",stderr);
	exit(-1);
}	else {
	port = htons(port);
}}
	i++;
}
	i = 0;

	int nfd = socket(AF_INET, SOCK_STREAM, 0);
	if(nfd == -1){
	fputs("couln't create socket\n", stderr);
	exit(-1);
}
	int fd = 0;
	int client_nfd;
	char *cut, *check;

	struct sockaddr_in addr = {
	.sin_family = AF_INET,
	.sin_port = port,
	.sin_addr.s_addr = 0
};

	if(bind(nfd,  (struct sockaddr *)&addr, sizeof(addr)) != 0){
	fputs("couln't bind the socket\n", stderr);
	exit(-1);
}
	if(listen(nfd, 25) != 0){
	fputs("couln't listen on the socket\n", stderr);
	exit(-1);
}

	char OK[900];

	while(1){
	if((client_nfd = accept(nfd, 0, 0)) == -1){
	fputs("error accepting connection\n", stderr);
	continue;
}

	//get the request
	if((i = recv(client_nfd, buffer, 512, 0)) < 1){ //either the message wasn't sent or it failed
	close(client_nfd);
	continue;
}
	//print it, this can be deleted or commented if you don't want to see the request
	write(1, buffer, i);

	i = 0;
	
	//get the filename
	if(strlen(buffer) > 5)
	cut = buffer +5;

	if( (check = strchr(cut, ' ')) == NULL){
	fputs("someone is probably nmapping the server", stderr);
	continue;
}
	*check = '\0';

	//security checking
	if(*cut == '/'){ //try to acces root
	send(client_nfd, "HTTP/1.1 200 OK\r\n\r\n <!DOCTYPE html>\n<html><head></head><body><p>DO NOT TRY TO ACCES MY ROOT YOU NIGGER MONKEY</p></body></html>", 126, MSG_NOSIGNAL);
	close(client_nfd);
	continue;
}	//try directory traversal
	if( grep(cut,"..") >= 0){
	send(client_nfd, "HTTP/1.1 200 OK\r\n\r\n <!DOCTYPE html>\n<html><head></head><body><p>Stop the directory traversals you NIGGER MONKEY</p></body></html>", 129, MSG_NOSIGNAL);
	close(client_nfd);
	continue;
}
	if(strchr(cut, '\\') != NULL){
	send(client_nfd, "HTTP/1.1 200 OK\r\n\r\n <!DOCTYPE html>\n<html><head></head><body><p>Stop the directory traversals you NIGGER MONKEY</p></body></html>", 129, MSG_NOSIGNAL);
	close(client_nfd);
	continue;

}

		//open the file
		//if the file doesn't exist send index.html, else send the file asked for
	if(*cut == 0){ //btw this 2 if cannot merge, or when it tries to open index and doesn't return -1 which is an error, it will try to open the file again
	if((fd = open("index.html" , 0)) == -1){
	send(client_nfd, "HTTP/1.1 404 Not Found\n\n Error index not found", 45, MSG_NOSIGNAL);
	close(client_nfd);
	continue;
}}	else if((fd = open(cut, 0)) == -1){  
	send(client_nfd, "HTTP/1.1 404 Not Found\r\n\r\n Error file not found", 47, MSG_NOSIGNAL);
	close(client_nfd);
	continue;
}
	//calculate the OK header
	i = lseek(fd, 0L, SEEK_END); //calculate file length
	if(i != -1){
	if(ishtml(cut) == 0 || *cut == '\0')
	snprintf(OK, 900,"HTTP/1.1 200 OK\nContent-Type: text/html; charset=UTF-8\nContent-Length: %d\r\n\r\n",i );
	else	snprintf(OK, 900,"HTTP/1.1 200 OK\nContent-Length: %d\r\n\r\n",i );

	if(lseek(fd, 0L, SEEK_SET) == -1){
	fputs("couldn't reset the file", stderr);
	close(client_nfd);
	close(fd);

}}	else { //error catched
	fputs("couldn't calculate file length", stderr);
	close(client_nfd);
	close(fd);
}
	i = 0;

	if(send(client_nfd, OK , strlen(OK), MSG_NOSIGNAL) != strlen(OK)){
	fputs("couldn't transmit the full http header", stderr);
	close(client_nfd);
	close(fd);
	continue;
}


	while((i = read(fd, fbuffer, 512)) > 0)
	if(send(client_nfd, fbuffer, i, MSG_NOSIGNAL) < i){
	fputs("error sending the file", stderr);
	break;
}
	i = 0;	
	
	close(client_nfd);
	close(fd);
}
	exit(0);
}

int grep(line, compare) //return the position of compare in line
char line[], compare[];
{
	int i,q,k;
	for(i = 0; line[i];i++)
		for(q = 0, k = i; line[k++] == compare[q++];)
			if(compare[q] == 0)
				return((k-q));
	return -1;
}

int ishtml(string) //return 0 if .html
char *string;
{
	int length = strlen(string);
	if(length <= 5) // ".html" has 5 characters so any filename shorter than that is not html
	return -1;

	string += (length - 5);
	return strcmp(string, ".html");
}

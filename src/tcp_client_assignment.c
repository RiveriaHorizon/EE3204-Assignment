/*******************************
tcp_client.c: the source file of the client in tcp transmission
********************************/

#include "headsock.h"

// transmission function
float str_cli(FILE *fp, int sockfd, long *len);
// calculate the time interval between out and in
void tv_sub(struct  timeval *out, struct timeval *in);

int main(int argc, char **argv)
{
	int sockfd, ret;
	float ti, rt;
	long len;
	struct sockaddr_in ser_addr;
	char ** pptr;
	struct hostent *sh;
	struct in_addr **addrs;
	FILE *fp;

	if (argc != 2) {
		printf("parameters not match");
	}

	// get host's information
	sh = gethostbyname(argv[1]);
	if (sh == NULL) {
		printf("error when gethostby name");
		exit(0);
	}

  // print the remote host's information
	printf("canonical name: %s\n", sh->h_name);
	for (pptr=sh->h_aliases; *pptr != NULL; pptr++)
		printf("the aliases name is: %s\n", *pptr);

	switch(sh->h_addrtype)
	{
		case AF_INET:
			printf("AF_INET\n");
		break;
		default:
			printf("unknown addrtype\n");
		break;
	}

	addrs = (struct in_addr **)sh->h_addr_list;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);           //create the socket
	if (sockfd <0)
	{
		printf("error in socket");
		exit(1);
	}
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(MYTCP_PORT);
	memcpy(&(ser_addr.sin_addr.s_addr), *addrs, sizeof(struct in_addr));
	bzero(&(ser_addr.sin_zero), 8);

	// connect the socket with the host
	ret = connect(sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr));
	if (ret != 0) {
		printf ("connection failed\n");
		close(sockfd);
		exit(1);
	}

	if((fp = fopen ("myfile.txt","r+t")) == NULL)
	{
		printf("File doesn't exit\n");
		exit(0);
	}

	ti = str_cli(fp, sockfd, &len);      // perform the transmission and receiving
	rt = (len/(float)ti);                // caculate the average transmission rate
	printf("Time(ms) : %.3f, Data sent(byte): %d\nData rate: %f (Kbytes/s)\n", ti, (int)len, rt);

	close(sockfd);
	fclose(fp);

	exit(0);
}

float str_cli(FILE *fp, int sockfd, long *len)
{
	char *buf;
	long lsize, ci;
	char sends[DATALEN];          // DATALEN is 500 as defined in headsock.h
	struct ack_so ack;            // Found in headsock.h, consists of
				                        // unsigned 8 bit num and len
	int n, slen;                  // n is used for transmission (recv and send),
	int trackpacket = 0;          // slen for packet size
																// trackpacket for alternating data size
	float time_inv = 0.0;
	struct timeval sendt, recvt;
	ci = 0;

	fseek(fp, 0, SEEK_END);      // sets the position of the stream
	lsize = ftell(fp);           // records the size of the file
	rewind(fp);                  // resets the position of the stream
	printf("The file length is %d bytes\n", (int)lsize);
	printf("the packet length is %d bytes\n",DATALEN);

  // allocate memory to contain the whole file.
	buf = (char *) malloc (lsize);
	if (buf == NULL) {
		exit (2);
	}

  // copy the file into the buffer.
	fread(buf,1,lsize,fp);

  /*** the whole file is loaded in the buffer. ***/
	buf[lsize] ='\0';            // append the end byte
	gettimeofday(&sendt, NULL);  // get the current time
	while(ci<= lsize) {          // while file has not reached the end
		// TODO: File is to be transmitted in alternate batches of
		//			 DATALEN and 2*DATALEN
		trackpacket++;
		if ((lsize+1-ci) <= DATALEN) {  // check for near end of file
			slen = lsize+1-ci;
		} else if (trackpacket%2 != 0) {
			slen = DATALEN;								// file packet size is 500
		} else if (trackpacket == 0) {
			slen = DATALEN*2;             // file packet size is 1000
		}

		memcpy(sends, (buf+ci), slen);   // Copies slen characters from memory area
															       // (buf+ci) to sends

		// TODO: When packet sent, wait for ACK,
		// 	     then send 2 data packets, wait for ack
		//       repeat procedure
		// 		   Currently, the send does not wait for ACK
		//       before continuing transmission
		if (trackpacket == 1) { // check again whether need to check for first packet
			n = send(sockfd, &sends, slen, 0);  // n determines number of bytes sent
			ci += slen;
		} else {
			if ((n = recv(sockfd, &ack, 2, 0)) != -1) {
				n = send(sockfd, &sends, slen, 0);
			}
			ci += slen;
		}
		if(n == -1) {                       // if result is -1, throw an exception
			printf("send error!");
			exit(1);
		}
	}

	if ((n = recv(sockfd, &ack, 2, 0)) == -1) {
		printf("error when receiving\n");
		exit(1);
	}
	if (ack.num != 1 || ack.len != 0) {
		printf("error in transmission\n");
	}

	gettimeofday(&recvt, NULL);                 // get current time
	*len = ci;
	tv_sub(&recvt, &sendt);                     // get the whole trans time
	time_inv += (recvt.tv_sec)*1000.0 + (recvt.tv_usec)/1000.0;
	return(time_inv);
}

void tv_sub(struct  timeval *out, struct timeval *in)
{
	if ((out->tv_usec -= in->tv_usec) <0)
	{
		--out ->tv_sec;
		out ->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

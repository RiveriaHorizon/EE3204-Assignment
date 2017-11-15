/*******************************
 tcp_client.c: the source file of the client in tcp transmission
 ********************************/

#include "headsock.h"
#include "xlsxwriter.h"

// transmission function
float str_cli(FILE *fp, int sockfd, long *len);
// calculate the time interval between out and in
void tv_sub(struct timeval *out, struct timeval *in);

int main(int argc, char **argv) {
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
  for (pptr = sh->h_aliases; *pptr != NULL; pptr++)
    printf("the aliases name is: %s\n", *pptr);

  switch (sh->h_addrtype) {
    case AF_INET:
      printf("AF_INET\n");
      break;
    default:
      printf("unknown addrtype\n");
      break;
  }

  addrs = (struct in_addr **) sh->h_addr_list;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);           //create the socket
  if (sockfd < 0) {
    printf("error in socket");
    exit(1);
  }
  ser_addr.sin_family = AF_INET;
  ser_addr.sin_port = htons(MYTCP_PORT);
  memcpy(&(ser_addr.sin_addr.s_addr), *addrs, sizeof(struct in_addr));
  bzero(&(ser_addr.sin_zero), 8);

  // connect the socket with the host
  ret = connect(sockfd, (struct sockaddr *) &ser_addr, sizeof(struct sockaddr));
  if (ret != 0) {
    printf("connection failed\n");
    close(sockfd);
    exit(1);
  }

  if ((fp = fopen("myfile.txt", "r+t")) == NULL) {
    printf("File doesn't exit\n");
    exit(0);
  }

  ti = str_cli(fp, sockfd, &len);      // perform the transmission and receiving
  rt = (len / (float) ti);             // caculate the average transmission rate
  printf("Time(ms) : %.3f, Data sent(byte): %d\nData rate: %f (Kbytes/s)\n", ti,
         (int) len, rt);

  FILE *resultsfp = fopen("results.txt", "a");
  if (resultsfp == NULL) {
    printf("Error opening file!\n");
    exit(1);
  }
  fprintf(resultsfp,
          "Time(ms) : %.3f, Data sent(byte): %d\nData rate: %f (Kbytes/s)\n",
          ti, (int) len, rt);
  fclose(resultsfp);

  close(sockfd);
  fclose(fp);

  exit(0);
}

float str_cli(FILE *fp, int sockfd, long *len) {
  char *buf;
  long lsize, ci;
  char sends[DATALEN];          // DATALEN is 500 as defined in headsock.h
  struct ack_so ack;            // Found in headsock.h, consists of
                                // unsigned 8 bit num and len
  int n, slen = 0;              // n is used for transmission (recv and send),
  int trackstatus = 1;          // slen for packet size
                                // trackpacket for alternating data size
  int ackpacket = 0;
  float time_inv = 0.0;
  struct timeval sendt, recvt;
  ci = 0;

  fseek(fp, 0, SEEK_END);      // sets the position of the stream
  lsize = ftell(fp);           // records the size of the file
  rewind(fp);                  // resets the position of the stream
  printf("The file length is %d bytes\n", (int) lsize);
  printf("the packet length is %d bytes\n", DATALEN);

  // allocate memory to contain the whole file.
  buf = (char *) malloc(lsize);
  if (buf == NULL) {
    exit(2);
  }

  // copy the file into the buffer.
  fread(buf, 1, lsize, fp);

  /*** the whole file is loaded in the buffer. ***/
  buf[lsize] = '\0';            // append the end byte
  gettimeofday(&sendt, NULL);  // get the current time

  // First packet to send
  memcpy(sends, (buf + ci), DATALEN);
  n = send(sockfd, &sends, DATALEN, 0);
  if (n == -1) {
    printf("send error!");  // Throw exception and exit when not able to send
    exit(1);
  }
  ci += DATALEN;

  ackpacket = recv(sockfd, &ack, 2, 0);
  if (ackpacket == -1) {                  //receive the ack
    printf("error when receiving ack\n");
    exit(1);
  }
  if (ack.num != 1 || ack.len != 0) {
    printf("error in transmission\n");
  }

  // printf("[DEBUG] First packet sent\n");

  // Subsequent packets to send
  while (ci <= lsize) {  // while file has not reached the end
    trackstatus++;

    if (trackstatus % 2 == 0) {   // Even interval
      if ((lsize + 1 - ci) <= DATALEN / 2) {  // check for near end of file
        slen = lsize + 1 - ci;
      } else {
        slen = DATALEN / 2;
      }

      memcpy(sends, (buf + ci), slen);  // Copies slen characters from memory area
                                        // (buf+ci) to sends

      n = send(sockfd, &sends, slen, 0);
      if (n == -1) {
        printf("send error!");  // Throw exception and exit when not able to send
        exit(1);
      }
      ci += slen;

      // printf("[DEBUG] Even interval\n");
    } else {                      // Odd interval
      if ((lsize + 1 - ci) <= DATALEN) {  // check for near end of file
        slen = lsize + 1 - ci;
      } else {
        slen = DATALEN;
      }

      memcpy(sends, (buf + ci), slen);  // Copies slen characters from memory area
                                        // (buf+ci) to sends

      n = send(sockfd, &sends, slen, 0);
      if (n == -1) {
        printf("send error!");  // Throw exception and exit when not able to send
        exit(1);
      }
      ci += slen;
      // printf("[DEBUG] Odd interval\n");
    }

    ackpacket = recv(sockfd, &ack, 2, 0);
    if (ackpacket == -1) {                  //receive the ack
      printf("error when receiving ack\n");
      exit(1);
    }
    if (ack.num != 1 || ack.len != 0) {
      printf("error in transmission\n");
    }
    // printf("[DEBUG] Packet set acknowledged: %d\n", trackstatus);
  }

  gettimeofday(&recvt, NULL);                 // get current time
  *len = ci;
  tv_sub(&recvt, &sendt);                     // get the whole trans time
  time_inv += (recvt.tv_sec) * 1000.0 + (recvt.tv_usec) / 1000.0;
  return (time_inv);
}

void tv_sub(struct timeval *out, struct timeval *in) {
  if ((out->tv_usec -= in->tv_usec) < 0) {
    --out->tv_sec;
    out->tv_usec += 1000000;
  }
  out->tv_sec -= in->tv_sec;
}

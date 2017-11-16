# EE3204-Assignment

## Programming Task
Develop a TCP-based client-server socket program for transferring a large message. Here, the message transmitted from the client to server is read from a large file. The message is split into short packets which are sent and acknowledged in alternating batches of size x and 2x. The sender sends a packet of size x, waits for an acknowledgment (ACK); sends a packet of size 2x, waits for an acknowledgement; and repeats the above procedure until the entire file is sent. The receiver sends an ACK after receiving every packet. Note that the size of the last packet may not be x or 2x.

## Result Tabulation
Verify if the file has been sent completely and correctly by comparing the received file with the original file. Measure the message transfer time and throughput for various values of x and compare it with the stop-and-wait protocol where the size of every packet is the same x. Choose appropriate values for “x” and measure the performance. Repeat the experiment several times and plot the average values in a report with a brief description of results, assumptions made, etc.
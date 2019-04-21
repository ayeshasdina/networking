
Intoduction:
There are three files in this program. main program is coded in modified_server_client.c which is a modified version of http://cs.ecs.baylor.edu/~donahoo/practical/CSockets/code/UDPEchoClient-Timeout.c to implement the Distance vector routing protocol.

There are three files:
1.modified_server_client.c
2. confA.txt  # there are more configuration files saved in the /user/adi252/ directory in each node
3. Makefile   # this helps to build the executable for the program.

Instructions:
from our linux termina use this command 
  ssh adi252@pc3.lan.sdn.uky.edu -p 28811 -i ~/.ssh/id_geni_ssh_rsa
Feed the password when asked.
in the home directorycheck if the modified_server_client exists. other wise copy this form my local directory.
Step 1:
We can build it using the makefie
  >>make
or 
  >>make msc
  
We can use the gcc compiler for building the executable as well

>>gcc modified_server_client.c -o msc

Step 2:
Run the program.
>>./msc confA.txt


Description:
This program runs a distance vector routing algorithm on GENI experimental nodes.In each node following task are executed.
1. The program finds its own hostname and reads the confuiguration file to detect its neighbors and their corresponding distance and ip address.
2. Initializes the routing table based on the information the neighbor table. At this point these are the only information that the node has.
file:///home/mir/Pictures/Screenshot_20190421_175010.png
